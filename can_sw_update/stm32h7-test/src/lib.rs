#![no_std]

pub mod flash_bank2;

use can_update::{CanFrame, ReceivingAction, ReceivingDevice, ReceivingState};
use stm32h7xx_hal::flash::LockedFlashBank;

/// CAN transport used by the firmware-update protocol.
pub trait CanPort {
    type Error;
    fn receive(&mut self) -> Result<Option<CanFrame>, Self::Error>;
    fn send(&mut self, frame: CanFrame) -> Result<(), Self::Error>;
}

/// Firmware-update receiver. Stores the image in the first 512 KiB of flash bank 2.
/// Does not restart the MCU or activate the new firmware.
pub struct TestReceiver {
    device: ReceivingDevice,
    previous: ReceivingState,
    writes: u32,
    image_size: usize,
}

impl TestReceiver {
    pub fn new(uuid: u32, transmitting_id: u32, receiving_id: u32) -> Self {
        let device = ReceivingDevice::new(uuid, transmitting_id, receiving_id);
        Self {
            previous: device.state(),
            device,
            writes: 0,
            image_size: 0,
        }
    }

    pub fn state(&self) -> ReceivingState {
        self.device.state()
    }
    pub fn writes(&self) -> u32 {
        self.writes
    }

    /// Call regularly using a monotonically increasing millisecond counter.
    /// Flash erase/program/verify operations are synchronous and may block.
    pub fn poll<C: CanPort>(
        &mut self,
        can: &mut C,
        bank2: &mut LockedFlashBank,
        now_ms: u32,
    ) -> Result<(), C::Error> {
        if let Err(e) = self.device.tick(now_ms) {
            defmt::error!("Update timeout/error: {:?}", defmt::Debug2Format(&e));
        }
        if let Some(frame) = can.receive()? {
            defmt::debug!(
                "CAN RX id={=u32:#x}, cmd={=u8}, len={=u8}",
                frame.id,
                frame.data[0],
                frame.len
            );
            if let Err(e) = self.device.on_frame(frame, now_ms) {
                defmt::warn!("Update protocol error: {:?}", defmt::Debug2Format(&e));
            }
        }
        self.drain(can, bank2, now_ms)?;
        let current = self.device.state();
        if current != self.previous {
            defmt::info!(
                "Update state: {:?} -> {:?}",
                defmt::Debug2Format(&self.previous),
                defmt::Debug2Format(&current)
            );
            self.previous = current;
        }
        Ok(())
    }

    fn drain<C: CanPort>(
        &mut self,
        can: &mut C,
        bank2: &mut LockedFlashBank,
        now_ms: u32,
    ) -> Result<(), C::Error> {
        while let Some(action) = self.device.next_action() {
            match action {
                ReceivingAction::SendFrame(frame) => {
                    defmt::debug!("CAN TX id={=u32:#x}, cmd={=u8}", frame.id, frame.data[0]);
                    can.send(frame)?;
                }
                ReceivingAction::EraseFlash { image_size } => {
                    defmt::info!("Prepare flash: erasing...");
                    let size = image_size as usize;
                    let result = flash_bank2::erase_image(bank2, size);
                    let valid = result.is_ok();
                    if valid {
                        self.image_size = size;
                        self.writes = 0;
                        defmt::info!("Bank 2 erased for image size={=u32}", image_size);
                    } else {
                        self.image_size = 0;
                        defmt::error!(
                            "Bank 2 erase failed: {:?}",
                            defmt::Debug2Format(&result.err())
                        );
                    }
                    if let Err(e) = self.device.flash_completed(valid, now_ms) {
                        defmt::error!("Erase completion error: {:?}", defmt::Debug2Format(&e));
                    }
                }
                ReceivingAction::WriteFlash { offset, length } => {
                    let start = offset as usize;
                    let data = self.device.block_data();
                    let in_bounds = data.len() == length
                        && start
                            .checked_add(length)
                            .is_some_and(|end| end <= self.image_size);
                    let valid = if in_bounds {
                        match flash_bank2::program_block(bank2, start, data) {
                            Ok(()) => true,
                            Err(e) => {
                                defmt::error!("Bank 2 write failed: {:?}", defmt::Debug2Format(&e));
                                false
                            }
                        }
                    } else {
                        defmt::error!("Bank 2 write out of image bounds or wrong length");
                        false
                    };
                    if valid {
                        self.writes += 1;
                        defmt::info!(
                            "Bank 2 write offset={=u32}, length={=usize}",
                            offset,
                            length
                        );
                    }
                    if let Err(e) = self.device.flash_completed(valid, now_ms) {
                        defmt::error!("Write completion error: {:?}", defmt::Debug2Format(&e));
                    }
                }
                ReceivingAction::VerifyImage { expected_crc } => {
                    let result = flash_bank2::verify_image(bank2, self.image_size, expected_crc);
                    let valid = result.is_ok();
                    if valid {
                        defmt::info!("Bank 2 CRC verified: {=u32:#x}", expected_crc);
                    } else {
                        defmt::error!(
                            "Bank 2 CRC verification failed: {:?}",
                            defmt::Debug2Format(&result.err())
                        );
                    }
                    if let Err(e) = self.device.flash_completed(valid, now_ms) {
                        defmt::error!("Verify completion error: {:?}", defmt::Debug2Format(&e));
                    }
                }
                ReceivingAction::RestartDevice => {
                    defmt::warn!("Restart suppressed in test mode");
                }
            }
        }
        Ok(())
    }
}
