//! End-to-end simulation: no CAN controller or flash hardware required.
use can_update::{
    CanFrame, FirmwareSource, ReceivingAction, ReceivingDevice, ReceivingError, ReceivingResponse,
    ReceivingState, TransmittingCommand, TransmittingDevice, TransmittingError, TransmittingState,
};

const TD: u32 = 0x100;
const RD: u32 = 0x101;
const UUID: u32 = 0x1234_5678;
const TIMEOUT: u32 = 1500;

struct Source(Vec<u8>);
impl FirmwareSource for Source {
    type Error = ();
    fn read(&mut self, offset: u32, output: &mut [u8]) -> Result<(), ()> {
        let from = offset as usize;
        output.copy_from_slice(self.0.get(from..from + output.len()).ok_or(())?);
        Ok(())
    }
}

struct Rig {
    transmitting: TransmittingDevice,
    receiving: ReceivingDevice,
    source: Source,
    flash: Vec<u8>,
    now: u32,
    writes: Vec<(u32, usize)>,
    erase_count: usize,
    restarts: usize,
    corrupt_first_block: bool,
    corrupted: bool,
    block_nacks: usize,
}
impl Rig {
    fn new(size: usize) -> Self {
        assert!(size > 0 && size % 4 == 0);
        let image: Vec<u8> = (0..size)
            .map(|i| (i as u8).wrapping_mul(37).wrapping_add(11))
            .collect();
        let mut transmitting = TransmittingDevice::new(TD, RD);
        let mut source = Source(image);
        transmitting.start(&mut source, UUID, size as u32).unwrap();
        Self {
            transmitting,
            receiving: ReceivingDevice::new(UUID, TD, RD),
            source,
            flash: vec![0xff; size],
            now: 0,
            writes: Vec::new(),
            erase_count: 0,
            restarts: 0,
            corrupt_first_block: false,
            corrupted: false,
            block_nacks: 0,
        }
    }

    // Deliver one sender frame, then process all receiver actions synchronously.
    // Flash operations are modeled as immediate completions for this test.
    fn step(&mut self) -> bool {
        let Some(mut frame) = self.transmitting.next_frame().unwrap() else {
            return false;
        };
        self.now += 1;
        self.transmitting.frame_sent(self.now).unwrap();
        if self.corrupt_first_block && !self.corrupted && frame.data[0] == 3 {
            frame.data[1] ^= 0x80;
            self.corrupted = true;
        }
        self.receiving.on_frame(frame, self.now).unwrap();
        self.drain_actions();
        true
    }

    fn drain_actions(&mut self) {
        while let Some(action) = self.receiving.next_action() {
            self.now += 1;
            match action {
                ReceivingAction::EraseFlash { image_size } => {
                    assert_eq!(image_size as usize, self.flash.len());
                    self.flash.fill(0xff);
                    self.erase_count += 1;
                    self.receiving.flash_completed(true, self.now).unwrap();
                }
                ReceivingAction::WriteFlash { offset, length } => {
                    let data = self.receiving.block_data();
                    assert_eq!(data.len(), length);
                    self.flash[offset as usize..offset as usize + length].copy_from_slice(data);
                    self.writes.push((offset, length));
                    self.receiving.flash_completed(true, self.now).unwrap();
                }
                ReceivingAction::VerifyImage { expected_crc } => {
                    let actual = can_update::stm32_crc_bytes(&self.flash).unwrap();
                    self.receiving
                        .flash_completed(actual == expected_crc, self.now)
                        .unwrap();
                }
                ReceivingAction::SendFrame(frame) => {
                    if ReceivingResponse::decode(&frame) == Ok((ReceivingResponse::BlockNack, UUID))
                    {
                        self.block_nacks += 1;
                    }
                    self.transmitting
                        .on_frame(frame, self.now, &mut self.source)
                        .unwrap();
                    if self.receiving.state() == ReceivingState::WaitingRestart {
                        self.receiving.request_restart().unwrap();
                    }
                }
                ReceivingAction::RestartDevice => self.restarts += 1,
            }
        }
    }

    fn finish(&mut self) {
        for _ in 0..100_000 {
            if self.transmitting.state() == TransmittingState::Completed {
                self.drain_actions();
                return;
            }
            assert!(
                self.step(),
                "sender stalled in {:?}",
                self.transmitting.state()
            );
        }
        panic!("too many frames");
    }
}

#[test]
fn end_to_end_single_short_block() {
    let mut rig = Rig::new(12);
    rig.finish();
    assert_eq!(rig.flash, rig.source.0);
    assert_eq!(rig.writes, vec![(0, 12)]);
    assert_eq!(rig.erase_count, 1);
    assert_eq!(rig.restarts, 1);
}

#[test]
fn end_to_end_multiple_blocks_and_partial_last_block() {
    let mut rig = Rig::new(1028);
    rig.finish();
    assert_eq!(rig.flash, rig.source.0);
    assert_eq!(rig.writes, vec![(0, 512), (512, 512), (1024, 4)]);
    assert_eq!(rig.restarts, 1);
}

#[test]
fn corrupted_data_is_nacked_and_retried() {
    let mut rig = Rig::new(520);
    rig.corrupt_first_block = true;
    rig.finish();
    assert_eq!(rig.block_nacks, 1);
    assert_eq!(rig.flash, rig.source.0);
    assert_eq!(rig.writes, vec![(0, 512), (512, 8)]);
    assert_eq!(rig.restarts, 1);
}

#[test]
fn missing_ready_response_causes_sender_timeout() {
    let mut rig = Rig::new(4);
    let frame = rig.transmitting.next_frame().unwrap().unwrap();
    rig.transmitting.frame_sent(100).unwrap();
    // The frame was lost: the Receiving Device never receives it.
    assert_eq!(frame.id, TD);
    assert_eq!(rig.transmitting.tick(100 + 10_000 - 1), Ok(()));
    assert_eq!(
        rig.transmitting.tick(100 + 10_000),
        Err(TransmittingError::Timeout)
    );
    assert_eq!(rig.transmitting.state(), TransmittingState::Failed);
}

#[test]
fn receiver_timeout_returns_error() {
    let mut rig = Rig::new(4);
    assert!(rig.step()); // TD.1 -> erase -> RD.1, then waits for TD.2
    let t = rig.now - 1;
    assert!(rig.receiving.tick(t + 10_000 - 1).is_ok());
    assert!(rig.receiving.tick(t + 10_000).is_err());
    assert_eq!(rig.receiving.state(), ReceivingState::Failed);
}

// Simulate a replay after the original BlockAck was lost. The sender's
// timeout policy is intentionally unchanged; inject the retransmission here.
#[test]
fn replay_of_committed_block_does_not_write_flash_twice() {
    let mut rig = Rig::new(520);
    // Advance until the first 512-byte block has been committed.
    for _ in 0..1000 {
        assert!(rig.step());
        if rig.writes.len() == 1 {
            break;
        }
    }
    assert_eq!(rig.writes, vec![(0, 512)]);
    let original = rig.flash[..512].to_vec();
    let crc = can_update::stm32_crc_bytes(&original).unwrap();
    let mut now = rig.now + 1;
    rig.receiving
        .on_frame(
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc,
            }
            .encode(TD)
            .unwrap(),
            now,
        )
        .unwrap();
    assert_eq!(
        rig.receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockReady.encode(RD, UUID)
        ))
    );
    for (i, chunk) in original.chunks(7).enumerate() {
        now += 1;
        let mut data = [0; 7];
        data[..chunk.len()].copy_from_slice(chunk);
        let cmd = if i == (512 + 6) / 7 - 1 {
            TransmittingCommand::BlockDataLast {
                data,
                len: chunk.len() as u8,
            }
        } else {
            TransmittingCommand::BlockData {
                data,
                len: chunk.len() as u8,
            }
        };
        rig.receiving
            .on_frame(cmd.encode(TD).unwrap(), now)
            .unwrap();
    }
    assert_eq!(
        rig.receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockAck.encode(RD, UUID)
        ))
    );
    assert_eq!(rig.writes, vec![(0, 512)]);
    assert_eq!(&rig.flash[..512], &original);
    // Skipping the next block is rejected.
    assert_eq!(
        rig.receiving.on_frame(
            TransmittingCommand::BlockInit {
                block_number: 2,
                crc
            }
            .encode(TD)
            .unwrap(),
            now + 1,
        ),
        Err(ReceivingError::InvalidBlock)
    );
}

// An out-of-sequence block must be rejected without changing receiver state
// or requesting a flash write. The correct next block must still be accepted.
#[test]
fn out_of_sequence_block_number_is_rejected() {
    let mut rig = Rig::new(1028);
    for _ in 0..1000 {
        assert!(rig.step());
        if rig.writes.len() == 1 {
            break;
        }
    }
    assert_eq!(rig.writes, vec![(0, 512)]);
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);

    let next_block = &rig.source.0[512..1024];
    let crc = can_update::stm32_crc_bytes(next_block).unwrap();
    let invalid = TransmittingCommand::BlockInit {
        block_number: 2,
        crc,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(
        rig.receiving.on_frame(invalid, rig.now + 1),
        Err(ReceivingError::InvalidBlock)
    );
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);
    assert_eq!(rig.receiving.next_action(), None);
    assert_eq!(rig.writes, vec![(0, 512)]);

    let valid = TransmittingCommand::BlockInit {
        block_number: 1,
        crc,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(rig.receiving.on_frame(valid, rig.now + 2), Ok(()));
    assert_eq!(rig.receiving.state(), ReceivingState::ReceivingBlock);
    assert_eq!(
        rig.receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockReady.encode(RD, UUID)
        ))
    );
}

// An unexpected but well-formed command must not advance the receiver,
// alter flash, or prevent a subsequent valid BlockInit.
#[test]
fn unexpected_can_command_preserves_receiver_state() {
    let mut rig = Rig::new(12);
    assert!(rig.step()); // TD.1 -> erase -> RD.1
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);
    let writes_before = rig.writes.clone();
    let erase_before = rig.erase_count;

    let unexpected = TransmittingCommand::BlockData {
        data: [0x55; 7],
        len: 7,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(
        rig.receiving.on_frame(unexpected, rig.now + 1),
        Err(ReceivingError::UnexpectedCommand)
    );
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);
    assert_eq!(rig.receiving.next_action(), None);
    assert_eq!(rig.writes, writes_before);
    assert_eq!(rig.erase_count, erase_before);

    let crc = can_update::stm32_crc_bytes(&rig.source.0).unwrap();
    let valid = TransmittingCommand::BlockInit {
        block_number: 0,
        crc,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(rig.receiving.on_frame(valid, rig.now + 2), Ok(()));
    assert_eq!(rig.receiving.state(), ReceivingState::ReceivingBlock);
    assert_eq!(
        rig.receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockReady.encode(RD, UUID)
        ))
    );
}

// Unrelated CAN identifiers must be ignored, even if their payload resembles
// a protocol command. Malformed frames on our own identifier are rejected.
#[test]
fn foreign_and_malformed_can_frames_do_not_change_state() {
    let mut rig = Rig::new(12);
    assert!(rig.step());
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);

    let foreign = TransmittingCommand::UpdateInit {
        image_len: 12,
        uuid: UUID,
    }
    .encode(TD + 10)
    .unwrap();
    assert_eq!(rig.receiving.on_frame(foreign, rig.now + 1), Ok(()));

    let malformed = CanFrame {
        id: TD,
        len: 1,
        data: [2, 0, 0, 0, 0, 0, 0, 0],
    };
    assert_eq!(
        rig.receiving.on_frame(malformed, rig.now + 2),
        Err(ReceivingError::InvalidFrame)
    );
    assert_eq!(rig.receiving.state(), ReceivingState::WaitingBlock);
    assert_eq!(rig.receiving.next_action(), None);
    assert!(rig.writes.is_empty());
    assert_eq!(rig.erase_count, 1);
}

// Flash work is asynchronous: no incoming protocol command may advance the
// receiver while erase/write is still in progress.
#[test]
fn incoming_frames_are_rejected_during_flash_erase() {
    let mut receiving = ReceivingDevice::new(UUID, TD, RD);
    let init = TransmittingCommand::UpdateInit {
        image_len: 4,
        uuid: UUID,
    }
    .encode(TD)
    .unwrap();
    receiving.on_frame(init, 10).unwrap();
    assert_eq!(receiving.state(), ReceivingState::Erasing);
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::EraseFlash { image_size: 4 })
    );

    let block = TransmittingCommand::BlockInit {
        block_number: 0,
        crc: 0,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(
        receiving.on_frame(block, 11),
        Err(ReceivingError::UnexpectedCommand)
    );
    assert_eq!(
        receiving.on_frame(init, 12),
        Err(ReceivingError::UnexpectedCommand)
    );
    assert_eq!(receiving.state(), ReceivingState::Erasing);
    assert_eq!(receiving.next_action(), None);
    // No receive timeout while flash hardware is working.
    assert_eq!(receiving.tick(10 + TIMEOUT + 100), Ok(()));

    receiving.flash_completed(true, 2000).unwrap();
    assert_eq!(receiving.state(), ReceivingState::WaitingBlock);
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::UpdateReady.encode(RD, UUID)
        ))
    );
}

#[test]
fn incoming_frames_are_rejected_during_flash_write() {
    let mut receiving = ReceivingDevice::new(UUID, TD, RD);
    receiving
        .on_frame(
            TransmittingCommand::UpdateInit {
                image_len: 4,
                uuid: UUID,
            }
            .encode(TD)
            .unwrap(),
            0,
        )
        .unwrap();
    assert!(matches!(
        receiving.next_action(),
        Some(ReceivingAction::EraseFlash { .. })
    ));
    receiving.flash_completed(true, 1).unwrap();
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::UpdateReady.encode(RD, UUID)
        ))
    );

    let bytes = [1u8, 2, 3, 4];
    let crc = can_update::stm32_crc_bytes(&bytes).unwrap();
    receiving
        .on_frame(
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc,
            }
            .encode(TD)
            .unwrap(),
            2,
        )
        .unwrap();
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockReady.encode(RD, UUID)
        ))
    );
    receiving
        .on_frame(
            TransmittingCommand::BlockDataLast {
                data: [1, 2, 3, 4, 0, 0, 0],
                len: 4,
            }
            .encode(TD)
            .unwrap(),
            3,
        )
        .unwrap();
    assert_eq!(receiving.state(), ReceivingState::WritingBlock);
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::WriteFlash {
            offset: 0,
            length: 4
        })
    );
    assert_eq!(receiving.block_data(), &bytes);

    let unexpected = TransmittingCommand::BlockInit {
        block_number: 1,
        crc,
    }
    .encode(TD)
    .unwrap();
    assert_eq!(
        receiving.on_frame(unexpected, 4),
        Err(ReceivingError::UnexpectedCommand)
    );
    assert_eq!(receiving.state(), ReceivingState::WritingBlock);
    assert_eq!(receiving.block_data(), &bytes);
    assert_eq!(receiving.next_action(), None);
    assert_eq!(receiving.tick(3 + TIMEOUT + 100), Ok(()));

    receiving.flash_completed(true, 2000).unwrap();
    assert_eq!(receiving.state(), ReceivingState::WaitingBlock);
    assert_eq!(
        receiving.next_action(),
        Some(ReceivingAction::SendFrame(
            ReceivingResponse::BlockAck.encode(RD, UUID)
        ))
    );
}
