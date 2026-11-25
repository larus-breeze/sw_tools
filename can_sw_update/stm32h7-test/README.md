# LARUS STM32H743 FDCAN1 Hardwaretest

Version 0.3: korrigierte API für `fdcan = "=0.2.1"` (kein `fdcan 0.1.x`).

Eigenständiges `no_std`-Testprogramm mit FDCAN1 (PB8 RX, PB9 TX, AF9), Classic CAN 1 Mbit/s und `defmt`/RTT. Verwendet das vorhandene `ReceivingDevice` über `TestReceiver`.

**Flash bleibt unangetastet.** Erase und Write werden nur protokolliert; die finale Image-Verifikation liefert absichtlich NACK. Ein Upload des Linux-Tools kann daher nicht als erfolgreich abgeschlossen werden.

## Build

```sh
rustup target add thumbv7em-none-eabihf
cargo build --release --manifest-path larus-h743-test/Cargo.toml
```

Aus dem Archiv-Hauptverzeichnis aufrufen. Die `memory.x` beschreibt **beispielhaft** 2 MiB Flash und 512 KiB AXI-SRAM; vor dem Flashen an den konkreten H743-Typ anpassen. Auch `probe-rs`-Chipnamen prüfen.

Die Firmware erwartet HSE 16 MHz. FDCAN-Kernelclock wird explizit aus PLL1Q 50 MHz gewählt. Nominal Bit Timing 5 × (1+7+2) = 50 TQ / Bit => 1 Mbit/s, 80 % Samplepoint.

## Konfiguration

In `src/main.rs`:

```rust
const TRANSMITTING_ID: u32 = 0x100;
const RECEIVING_ID: u32 = 0x101;
const UUID: u32 = 0x1234_5678;
```

Die IDs sind **11-Bit-Standard-IDs**. Auf dem Bus werden nur klassische CAN-Frames bis 8 Bytes verwendet. Externen CAN-Transceiver, 120-Ohm-Abschluss und gemeinsame Masse vorsehen.

## Wichtige Einschränkungen

- Noch **nicht für das STM32-Target kompiliert oder ausgeführt**. In der Erstellungsumgebung ist kein Cargo installiert; HAL-API-Abweichungen müssen ggf. anhand der Compilerdiagnose korrigiert werden.
- Polling ohne Interrupts: hohe Buslast kann zu FIFO-Überläufen führen. Für den Test möglichst nur Update-Verkehr auf dem Bus.
- Flash-Operationen werden simuliert; Verifikation wird bewusst abgelehnt.
- Für die spätere Firmwareintegration sind echte Flash-Operationen, Fehlerpfade, Interrupt-/FIFO-Handling und ein sicherer Speicherbereich gesondert zu implementieren.
- DWT-CYCCNT-Zeitbasis setzt 400 MHz CPU-Takt voraus; bei Änderung anpassen.

## fdcan 0.2.1

Empfang: FIFO0 zuerst auf leer prüfen, danach `ReceiveOverrun::{NoOverrun, Overrun}` auswerten. Senden: `Ok(None)` bedeutet erfolgreich eingereiht ohne Verdrängung; `Ok(Some(()))` bedeutet, dass ein ausstehendes Telegramm verdrängt wurde und wird hier als Fehler behandelt.

## Neuaufbau mit funktionierender RTT-Konfiguration (2026-11-21)

- `DEFMT_LOG = "trace"` aktiviert die `defmt`-Meldungen beim Build.
- Runner verwendet `STM32H743VGTx` und `--log-format "{L} {s}"`.
- `debug = 2` in Dev- und Release-Profil.
- `memory.x` auf 1 MiB Flash für den **VGTx** (G-Dichtegruppe) eingestellt; Chip-Aufdruck kontrollieren.
- Die HAL-Firmware wurde aus Hardwaretest v2 übernommen; CAN-Logik unverändert.
- Build aus `larus-h743-test/`: `cargo build --release`; Start: `cargo run --release`.
- **Achtung:** `cargo run` löscht/programmiert Flash. Die simulierten Update-Flashaktionen dagegen schreiben nichts.
- **Hardwarevoraussetzung:** HSE 16 MHz und externer CAN-Transceiver an PB8/PB9.
- Dieses neue Archiv konnte hier nicht gegen den Embedded-Target kompiliert werden. Ausgangspunkt ist die vom Benutzer bereits kompilierte v2.
