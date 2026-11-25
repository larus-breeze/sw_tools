//! Non-blocking receiver for the firmware update protocol.
//! Hardware operations are requested as actions and completed explicitly.
use crate::crc::{stm32_crc_bytes, Stm32Crc};
use crate::protocol::{CanFrame, ReceivingResponse, TransmittingCommand};
use crate::{BLOCK_SIZE, FLASH_ERASE_TIMEOUT_MS, PROTOCOL_TIMEOUT_MS};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Work requested from the application by the receiver state machine.
///
/// The application must execute actions in order. Flash operations are
/// asynchronous from the protocol perspective and require a later
/// [`ReceivingDevice::flash_completed`] call.
pub enum ReceivingAction {
    SendFrame(CanFrame),
    EraseFlash {
        image_size: u32,
    },
    /// Read the bytes using `block_data()` before calling `flash_completed`.
    WriteFlash {
        offset: u32,
        length: usize,
    },
    /// Application must verify the stored image, including the expected CRC.
    VerifyImage {
        expected_crc: u32,
    },
    RestartDevice,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Failures reported by the receiver state machine.
pub enum ReceivingError {
    Busy,
    InvalidFrame,
    InvalidImageSize,
    UnexpectedCommand,
    Timeout,
    FlashFailure,
    InvalidBlock,
    InternalBusy,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Observable phases of the receiver firmware update state machine.
pub enum State {
    Idle,
    Erasing,
    WaitingBlock,
    ReceivingBlock,
    WritingBlock,
    Verifying,
    WaitingRestart,
    Finished,
    Failed,
}

/// Non-blocking firmware receiver, independent of flash and CAN drivers.
///
/// Incoming frames are handled by [`Self::on_frame`]. The host executes
/// actions returned by [`Self::next_action`] and acknowledges completed
/// flash operations with [`Self::flash_completed`].
pub struct ReceivingDevice {
    uuid: u32,
    transmitting_id: u32,
    receiving_id: u32,
    state: State,
    deadline: u32,
    image_size: u32,
    written: u32,
    block_number: u32,
    expected_block_crc: u32,
    last_block_crc: Option<u32>,
    replaying_last_block: bool,
    block_len: usize,
    received: usize,
    buffer: [u8; BLOCK_SIZE],
    pending: Option<ReceivingAction>,
    image_crc: Stm32Crc,
}

impl ReceivingDevice {
    /// Creates an idle receiver for a device UUID and two CAN identifiers.
    pub const fn new(uuid: u32, transmitting_id: u32, receiving_id: u32) -> Self {
        Self {
            uuid,
            transmitting_id,
            receiving_id,
            state: State::Idle,
            deadline: 0,
            image_size: 0,
            written: 0,
            block_number: 0,
            expected_block_crc: 0,
            last_block_crc: None,
            replaying_last_block: false,
            block_len: 0,
            received: 0,
            buffer: [0xFF; BLOCK_SIZE],
            pending: None,
            image_crc: Stm32Crc::new(),
        }
    }

    /// Returns the current receiver state.
    pub const fn state(&self) -> State {
        self.state
    }
    /// Returns the valid bytes of the current block for a flash write.
    ///
    /// Padding bytes are excluded; flash alignment is handled by the host.
    pub fn block_data(&self) -> &[u8] {
        &self.buffer[..self.block_len]
    }
    /// Takes the next pending action, if any.
    ///
    /// Each action is returned only once; the caller must handle it before
    /// progressing operations that depend on its completion.
    pub fn next_action(&mut self) -> Option<ReceivingAction> {
        self.pending.take()
    }
    // Only one outstanding action is allowed.
    fn emit(&mut self, action: ReceivingAction) -> Result<(), ReceivingError> {
        if self.pending.is_some() {
            return Err(ReceivingError::InternalBusy);
        }
        self.pending = Some(action);
        Ok(())
    }
    fn response(&mut self, response: ReceivingResponse) -> Result<(), ReceivingError> {
        self.emit(ReceivingAction::SendFrame(
            response.encode(self.receiving_id, self.uuid),
        ))
    }
    fn arm(&mut self, now: u32) {
        self.deadline = now.wrapping_add(PROTOCOL_TIMEOUT_MS);
    }
    fn arm_erase(&mut self, now: u32) {
        self.deadline = now.wrapping_add(FLASH_ERASE_TIMEOUT_MS);
    }
    fn fail(&mut self, error: ReceivingError) -> ReceivingError {
        self.state = State::Failed;
        self.pending = None;
        error
    }
    /// Timeout only applies while waiting for incoming CAN traffic. Flash operations
    /// are application-owned and must eventually be completed or failed explicitly.
    /// Checks protocol and flash-erase timeouts.
    pub fn tick(&mut self, now: u32) -> Result<(), ReceivingError> {
        if matches!(self.state, State::WaitingBlock | State::ReceivingBlock)
            && (now.wrapping_sub(self.deadline) as i32) >= 0
        {
            return Err(self.fail(ReceivingError::Timeout));
        }
        Ok(())
    }
    /// Consumes one incoming command and requests any resulting action.
    ///
    /// The command CAN ID and addressed UUID are checked by the protocol.
    pub fn on_frame(&mut self, frame: CanFrame, now: u32) -> Result<(), ReceivingError> {
        if frame.id != self.transmitting_id {
            return Ok(());
        }
        if self.pending.is_some() {
            return Err(ReceivingError::Busy);
        }
        let cmd = TransmittingCommand::decode(&frame).map_err(|_| ReceivingError::InvalidFrame)?;
        match (self.state, cmd) {
            (
                State::Idle | State::Failed | State::Finished,
                TransmittingCommand::UpdateInit { image_len, uuid },
            ) if uuid == self.uuid => {
                if image_len == 0 {
                    return Err(ReceivingError::InvalidImageSize);
                }
                self.image_size = image_len;
                self.written = 0;
                self.block_number = 0;
                self.last_block_crc = None;
                self.replaying_last_block = false;
                self.image_crc.reset();
                self.state = State::Erasing;
                self.emit(ReceivingAction::EraseFlash {
                    image_size: image_len,
                })?;
            }
            (State::WaitingBlock, TransmittingCommand::BlockInit { block_number, crc }) => {
                // Only the next block or the last committed block is valid.
                // A replay is received and CRC-checked but never written again.
                let replay = self.block_number > 0 && block_number == self.block_number - 1;
                if replay {
                    if self.last_block_crc != Some(crc) {
                        return Err(ReceivingError::InvalidBlock);
                    }
                    let previous_offset = (block_number as u32) * BLOCK_SIZE as u32;
                    self.block_len =
                        core::cmp::min(BLOCK_SIZE as u32, self.image_size - previous_offset)
                            as usize;
                } else if block_number == self.block_number && self.written < self.image_size {
                    self.block_len =
                        core::cmp::min(BLOCK_SIZE as u32, self.image_size - self.written) as usize;
                } else {
                    return Err(ReceivingError::InvalidBlock);
                }
                self.replaying_last_block = replay;
                self.expected_block_crc = crc;
                self.received = 0;
                self.buffer.fill(0xFF);
                self.state = State::ReceivingBlock;
                self.arm(now);
                self.response(ReceivingResponse::BlockReady)?;
            }
            (State::ReceivingBlock, TransmittingCommand::BlockData { data, len }) => {
                let count = len as usize;
                if count != 7 || self.received + count >= self.block_len {
                    return Err(ReceivingError::InvalidBlock);
                }
                self.buffer[self.received..self.received + count].copy_from_slice(&data[..count]);
                self.received += count;
                self.arm(now);
            }
            (State::ReceivingBlock, TransmittingCommand::BlockDataLast { data, len }) => {
                let count = len as usize;
                if self.received + count != self.block_len {
                    return Err(ReceivingError::InvalidBlock);
                }
                self.buffer[self.received..self.received + count].copy_from_slice(&data[..count]);
                self.received += count;
                if stm32_crc_bytes(&self.buffer[..self.block_len.next_multiple_of(4)])
                    .map_err(|_| ReceivingError::InvalidBlock)?
                    != self.expected_block_crc
                {
                    self.state = State::WaitingBlock;
                    self.arm(now);
                    self.response(ReceivingResponse::BlockNack)?;
                } else if self.replaying_last_block {
                    self.state = State::WaitingBlock;
                    self.arm(now);
                    self.response(ReceivingResponse::BlockAck)?;
                } else {
                    self.state = State::WritingBlock;
                    self.emit(ReceivingAction::WriteFlash {
                        offset: self.written,
                        length: self.block_len,
                    })?;
                }
            }
            (State::WaitingBlock, TransmittingCommand::UploadFinished { crc })
                if self.written == self.image_size =>
            {
                // This checks the stream; the application additionally verifies stored flash.
                if self.image_crc.finalize() != crc {
                    self.state = State::Failed;
                    self.response(ReceivingResponse::UploadNack)?;
                } else {
                    self.state = State::Verifying;
                    self.emit(ReceivingAction::VerifyImage { expected_crc: crc })?;
                }
            }
            _ => return Err(ReceivingError::UnexpectedCommand),
        }
        Ok(())
    }
    /// Report completion of EraseFlash, WriteFlash or VerifyImage.
    /// On failure, the device sends NACK when possible and returns to Failed.
    /// Reports completion of a previously requested flash or verification action.
    ///
    /// `success` must reflect the actual hardware operation outcome. A
    /// successful block write advances the running image CRC using `0xFF`
    /// padding for an incomplete final word.
    pub fn flash_completed(&mut self, success: bool, now: u32) -> Result<(), ReceivingError> {
        if self.pending.is_some() {
            return Err(ReceivingError::Busy);
        }
        match self.state {
            State::Erasing => {
                if !success {
                    return Err(self.fail(ReceivingError::FlashFailure));
                }
                self.state = State::WaitingBlock;
                self.arm_erase(now);
                self.response(ReceivingResponse::UpdateReady)
            }
            State::WritingBlock => {
                if !success {
                    self.state = State::Failed;
                    return self.response(ReceivingResponse::BlockNack);
                }
                self.image_crc
                    .update_bytes(&self.buffer[..self.block_len.next_multiple_of(4)])
                    .map_err(|_| ReceivingError::InvalidBlock)?;
                self.written += self.block_len as u32;
                self.last_block_crc = Some(self.expected_block_crc);
                self.block_number += 1;
                self.state = State::WaitingBlock;
                self.arm(now);
                self.response(ReceivingResponse::BlockAck)
            }
            State::Verifying => {
                self.state = if success {
                    State::WaitingRestart
                } else {
                    State::Failed
                };
                self.response(if success {
                    ReceivingResponse::UploadAck
                } else {
                    ReceivingResponse::UploadNack
                })
            }
            _ => Err(ReceivingError::UnexpectedCommand),
        }
    }
    /// Called after the final ACK has been sent by the application.
    /// Requests the final restart action after a verified upload.
    pub fn request_restart(&mut self) -> Result<(), ReceivingError> {
        if self.state != State::WaitingRestart || self.pending.is_some() {
            return Err(ReceivingError::Busy);
        }
        self.state = State::Finished;
        self.emit(ReceivingAction::RestartDevice)
    }
    /// Returns the CRC accumulated from successfully written firmware blocks.
    pub fn image_crc(&self) -> u32 {
        self.image_crc.finalize()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn command(u: &mut ReceivingDevice, c: TransmittingCommand, now: u32) {
        u.on_frame(c.encode(0x100).unwrap(), now).unwrap();
    }
    fn reply(u: &mut ReceivingDevice, r: ReceivingResponse) {
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::SendFrame(r.encode(0x101, 42)))
        );
    }
    #[test]
    fn initialization_order_and_timeout() {
        let mut u = ReceivingDevice::new(42, 0x100, 0x101);
        command(
            &mut u,
            TransmittingCommand::UpdateInit {
                image_len: 4,
                uuid: 42,
            },
            100,
        );
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::EraseFlash { image_size: 4 })
        );
        assert_eq!(u.state(), State::Erasing);
        u.flash_completed(true, 300).unwrap();
        reply(&mut u, ReceivingResponse::UpdateReady);
        assert_eq!(u.tick(300 + FLASH_ERASE_TIMEOUT_MS - 1), Ok(()));
        assert_eq!(
            u.tick(300 + FLASH_ERASE_TIMEOUT_MS),
            Err(ReceivingError::Timeout)
        );
        assert_eq!(u.state(), State::Failed);
    }
    #[test]
    fn complete_short_image() {
        let mut u = ReceivingDevice::new(42, 0x100, 0x101);
        let bytes = [1u8, 2, 3, 4];
        command(
            &mut u,
            TransmittingCommand::UpdateInit {
                image_len: 4,
                uuid: 42,
            },
            0,
        );
        assert!(matches!(
            u.next_action(),
            Some(ReceivingAction::EraseFlash { .. })
        ));
        u.flash_completed(true, 0).unwrap();
        reply(&mut u, ReceivingResponse::UpdateReady);
        command(
            &mut u,
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc: stm32_crc_bytes(&bytes).unwrap(),
            },
            1,
        );
        reply(&mut u, ReceivingResponse::BlockReady);
        command(
            &mut u,
            TransmittingCommand::BlockDataLast {
                data: [1, 2, 3, 4, 0, 0, 0],
                len: 4,
            },
            2,
        );
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::WriteFlash {
                offset: 0,
                length: 4
            })
        );
        assert_eq!(u.block_data(), &bytes);
        u.flash_completed(true, 3).unwrap();
        reply(&mut u, ReceivingResponse::BlockAck);
        command(
            &mut u,
            TransmittingCommand::UploadFinished {
                crc: stm32_crc_bytes(&bytes).unwrap(),
            },
            4,
        );
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::VerifyImage {
                expected_crc: stm32_crc_bytes(&bytes).unwrap()
            })
        );
        u.flash_completed(true, 5).unwrap();
        reply(&mut u, ReceivingResponse::UploadAck);
        u.request_restart().unwrap();
        assert_eq!(u.next_action(), Some(ReceivingAction::RestartDevice));
    }
    #[test]
    fn unaligned_image_length_uses_ff_padding_for_crc() {
        let mut u = ReceivingDevice::new(42, 0x100, 0x101);
        let bytes = [1u8, 2, 3];
        let padded = [1u8, 2, 3, 0xFF];
        let crc = stm32_crc_bytes(&padded).unwrap();
        command(
            &mut u,
            TransmittingCommand::UpdateInit {
                image_len: 3,
                uuid: 42,
            },
            0,
        );
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::EraseFlash { image_size: 3 })
        );
        u.flash_completed(true, 0).unwrap();
        reply(&mut u, ReceivingResponse::UpdateReady);
        command(
            &mut u,
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc,
            },
            1,
        );
        reply(&mut u, ReceivingResponse::BlockReady);
        command(
            &mut u,
            TransmittingCommand::BlockDataLast {
                data: [1, 2, 3, 0, 0, 0, 0],
                len: 3,
            },
            2,
        );
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::WriteFlash {
                offset: 0,
                length: 3
            })
        );
        assert_eq!(u.block_data(), &bytes);
        u.flash_completed(true, 3).unwrap();
        reply(&mut u, ReceivingResponse::BlockAck);
        command(&mut u, TransmittingCommand::UploadFinished { crc }, 4);
        assert_eq!(
            u.next_action(),
            Some(ReceivingAction::VerifyImage { expected_crc: crc })
        );
    }

    #[test]
    fn crc_failure_allows_retry() {
        let mut u = ReceivingDevice::new(42, 0x100, 0x101);
        command(
            &mut u,
            TransmittingCommand::UpdateInit {
                image_len: 4,
                uuid: 42,
            },
            0,
        );
        u.next_action();
        u.flash_completed(true, 0).unwrap();
        u.next_action();
        command(
            &mut u,
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc: 0,
            },
            1,
        );
        u.next_action();
        command(
            &mut u,
            TransmittingCommand::BlockDataLast {
                data: [1, 2, 3, 4, 0, 0, 0],
                len: 4,
            },
            2,
        );
        reply(&mut u, ReceivingResponse::BlockNack);
        assert_eq!(u.state(), State::WaitingBlock);
        command(
            &mut u,
            TransmittingCommand::BlockInit {
                block_number: 0,
                crc: stm32_crc_bytes(&[1, 2, 3, 4]).unwrap(),
            },
            3,
        );
        reply(&mut u, ReceivingResponse::BlockReady);
    }
}
