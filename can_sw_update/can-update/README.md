# larus-can-update

First implementation step: hardware-independent, `no_std` CAN datagram codec.

- Caller supplies the two actual CAN identifiers (Transmitting Device and Receiving Device).
- The first payload byte selects the command/response.
- u24 and u32 fields use little-endian byte order.
- `TransmittingCommand::encode/decode` and `ReceivingResponse::encode/decode` handle frames.
- No allocation, CAN driver, timer, or flash driver dependency.

Run `cargo test` and `cargo check --lib` in this directory.

## Open protocol decisions

- CRC polynomial, initial value, reflection, final XOR.
- Handling a final block shorter than 512 bytes (and padding policy).
- Whether the final data telegram always carries 7 bytes or may be shorter.
- Device-ID filtering belongs in the later state machine, not in this codec.

## CRC module (v0.2)

`crc::Stm32Crc` implements the STM32 non-reflected 32-bit CRC (polynomial `0x04C11DB7`, init `0xFFFFFFFF`, no final XOR). `update_bytes` groups bytes as little-endian 32-bit words and rejects lengths not divisible by four. Call `update_bytes` incrementally across word-aligned chunks to compute the full-image CRC. The final firmware block may be shorter than 512 bytes but must be word-aligned.

Run `cargo test` and `cargo check --lib`.

## Receiving Device state machine (experimental v0.3)

`ReceivingDevice::on_frame(frame, now_ms)` accepts Transmitting Device frames. `next_action()`
returns one hardware action; the application must consume it before delivering another
frame. `EraseFlash`, `WriteFlash` and `VerifyImage` must be acknowledged via
`flash_completed(success, now_ms)`. For `WriteFlash`, call `block_data()` before
acknowledging completion. After sending the final ACK, call `request_restart()`.

Assumptions for this initial implementation:
- Block numbering starts at **zero**, with no skipped blocks.
- Block payloads use seven bytes in every intermediate frame and 0..7 bytes
  in the final frame; last block may be shorter than 512 bytes.
- Incoming image length is nonzero and divisible by four.
- The final `VerifyImage` action asks the application to verify stored flash.
- `tick` checks CAN receive timeouts in waiting/receiving states; application
  flash operations have their own completion/error mechanism.
- `on_frame` returns `Busy` if an action is still pending.
- A failed erase returns `FlashFailure`; failed block writes emit BlockNack,
  failed image verification emits UploadNack.

This is a first draft, **not yet hardware-validated**. In particular, sending
of acknowledgments, error recovery, timing during flash operations, and the
precise CAN ID/command semantics should be integration-tested before use.

## Transmitting Device (v0.4)

`TransmittingDevice` sends firmware using a caller-supplied `FirmwareSource` and two
configurable CAN IDs. `start` reads the source to compute the complete image CRC.
Call `next_frame()` to get one outgoing frame; call `frame_sent(now_ms)` only
once the CAN driver has accepted it. Forward received frames via
`on_frame(frame, now_ms, &mut source)` and periodically call `tick(now_ms)`.
A BlockNack causes up to three **additional** attempts; a timeout aborts
immediately. The image must be word-aligned and fit in the protocol's u24 length.

Note: the sending state machine has no wall-clock timeout while the application
is unable to enqueue a frame. This remains the application's responsibility.
The current Receiving Device implementation does not yet accept retransmission of
an already committed block after a lost ACK; this needs protocol-level handling
before production use.


## Integration tests (v0.5)

Run `cargo test`. The `tests/integration.rs` harness simulates both state
machines, flash storage, CAN frame delivery, a corrupted data frame, and
timeouts without external dependencies.

**Known limitation:** Lost ACK recovery and duplicate block handling are not
yet implemented. The simulator uses immediate flash completions; it does not
model flash latency, CAN queue saturation, or reset/bootloader behavior.

### Additional integration checks (v0.9)

`tests/integration.rs` checks that incoming CAN protocol commands cannot
advance the Receiving Device during asynchronous flash erase or write operations.
The 1500 ms receive timeout does not apply while flash hardware is busy.

## Linux command-line uploader (SocketCAN)

Build the executable on Linux with `cargo build --release --bin larus-can-upload`.
The existing `no_std` library is unchanged; the binary uses Linux raw SocketCAN.

Example:

```sh
./target/release/larus-can-upload --interface can0 --transmitting-id 0x100 --receiving-id 0x101 --uuid 0x12345678 firmware.bin
```

The CAN interface must already be configured and UP. Firmware must be 4-byte
aligned and at most 16,777,215 bytes. This executable requires Linux and
classic CAN frames. The device must implement the matching LARUS protocol.
The sender aborts on missing responses (no automatic retry on timeout).
