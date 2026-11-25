//! Non-blocking sender. One frame is offered at a time; the caller must confirm
//! successful CAN transmission via `frame_sent` before asking for another.
use crate::crc::{stm32_crc_bytes, Stm32Crc};
use crate::protocol::{CanFrame, ReceivingResponse, TransmittingCommand};
use crate::{BLOCK_SIZE, FLASH_ERASE_TIMEOUT_MS, PROTOCOL_TIMEOUT_MS};

/// Maximum number of retransmissions after a block is rejected.
pub const MAX_BLOCK_RETRIES: u8 = 3;

/// Random-access firmware storage. `read` must fill the entire supplied slice.
pub trait FirmwareSource {
    /// Storage-specific error returned by [`FirmwareSource::read`].
    type Error;
    /// Reads exactly `output.len()` bytes starting at `offset`.
    ///
    /// Short reads must be reported as errors rather than silently accepted.
    fn read(&mut self, offset: u32, output: &mut [u8]) -> Result<(), Self::Error>;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Observable phases of the non-blocking firmware sender.
pub enum TransmittingState {
    Idle,
    SendingInit,
    WaitingReady,
    SendingBlockInit,
    WaitingBlockReady,
    SendingData,
    WaitingBlockAck,
    SendingFinish,
    WaitingUploadAck,
    Completed,
    Failed,
}
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Failures reported by the sender state machine.
pub enum TransmittingError {
    InvalidSize,
    Source,
    Protocol,
    Timeout,
    BlockRejected,
    UploadRejected,
    Busy,
    Encode,
}

/// Drives an upload without owning the CAN driver or firmware storage.
///
/// The caller repeatedly obtains frames with [`Self::next_frame`], confirms
/// successful transmission with [`Self::frame_sent`], passes received replies
/// to [`Self::on_frame`], and calls [`Self::tick`] for timeout handling.
pub struct TransmittingDevice {
    transmitting_id: u32,
    receiving_id: u32,
    state: TransmittingState,
    deadline: u32,
    uuid: u32,
    image_len: u32,
    image_crc: u32,
    offset: u32,
    block_number: u32,
    block_len: usize,
    block: [u8; BLOCK_SIZE],
    cursor: usize,
    retries: u8,
    pending: Option<CanFrame>,
}
impl TransmittingDevice {
    /// Creates an idle sender using the supplied command and response CAN IDs.
    pub const fn new(transmitting_id: u32, receiving_id: u32) -> Self {
        Self {
            transmitting_id,
            receiving_id,
            state: TransmittingState::Idle,
            deadline: 0,
            uuid: 0,
            image_len: 0,
            image_crc: 0,
            offset: 0,
            block_number: 0,
            block_len: 0,
            block: [0xFF; BLOCK_SIZE],
            cursor: 0,
            retries: 0,
            pending: None,
        }
    }
    /// Returns the current sender state.
    pub const fn state(&self) -> TransmittingState {
        self.state
    }
    /// Returns the number of retries for the current firmware block.
    pub const fn retries(&self) -> u8 {
        self.retries
    }
    // Erasing flash may take longer than an ordinary protocol exchange.
    fn arm(&mut self, now: u32) {
        let timeout = if self.state == TransmittingState::WaitingReady {
            FLASH_ERASE_TIMEOUT_MS
        } else {
            PROTOCOL_TIMEOUT_MS
        };
        self.deadline = now.wrapping_add(timeout);
    }
    // Clear an unsent frame so it cannot be transmitted after failure.
    fn fail(&mut self, error: TransmittingError) -> TransmittingError {
        self.state = TransmittingState::Failed;
        self.pending = None;
        error
    }
    /// Compute image CRC with fixed stack/RAM usage. Image length is a u24 field.
    pub fn start<S: FirmwareSource>(
        &mut self,
        source: &mut S,
        uuid: u32,
        image_len: u32,
    ) -> Result<(), TransmittingError> {
        if !matches!(
            self.state,
            TransmittingState::Idle | TransmittingState::Completed | TransmittingState::Failed
        ) {
            return Err(TransmittingError::Busy);
        }
        if image_len == 0 || image_len > 0x00ff_ffff {
            return Err(TransmittingError::InvalidSize);
        }
        let mut crc = Stm32Crc::new();
        // Reuse a fixed-size buffer rather than allocating the whole image.
        let mut temp = [0xFFu8; BLOCK_SIZE];
        let mut offset = 0;
        while offset < image_len {
            let count = core::cmp::min(BLOCK_SIZE as u32, image_len - offset) as usize;
            // Essential: old block data must not become padding in the last
            // incomplete CRC word (for example, a 1025-byte image).
            temp.fill(0xFF);
            source
                .read(offset, &mut temp[..count])
                .map_err(|_| TransmittingError::Source)?;
            crc.update_bytes(&temp[..count.next_multiple_of(4)])
                .map_err(|_| TransmittingError::InvalidSize)?;
            offset += count as u32;
        }
        self.uuid = uuid;
        self.image_len = image_len;
        self.image_crc = crc.finalize();
        self.offset = 0;
        self.block_number = 0;
        self.retries = 0;
        self.cursor = 0;
        self.pending = None;
        self.state = TransmittingState::SendingInit;
        Ok(())
    }
    // Reload the current block; 0xFF padding is used for its CRC only.
    fn load_block<S: FirmwareSource>(&mut self, source: &mut S) -> Result<(), TransmittingError> {
        self.block_len = core::cmp::min(BLOCK_SIZE as u32, self.image_len - self.offset) as usize;
        self.block.fill(0xFF);
        source
            .read(self.offset, &mut self.block[..self.block_len])
            .map_err(|_| self.fail(TransmittingError::Source))?;
        self.cursor = 0;
        self.state = TransmittingState::SendingBlockInit;
        Ok(())
    }
    /// Obtain a CAN frame to transmit. Repeated calls return the same frame until
    /// `frame_sent` is called. The caller must not report success before enqueueing it.
    pub fn next_frame(&mut self) -> Result<Option<CanFrame>, TransmittingError> {
        if let Some(frame) = self.pending {
            return Ok(Some(frame));
        }
        let cmd = match self.state {
            TransmittingState::SendingInit => TransmittingCommand::UpdateInit {
                image_len: self.image_len,
                uuid: self.uuid,
            },
            TransmittingState::SendingBlockInit => TransmittingCommand::BlockInit {
                block_number: self.block_number,
                crc: stm32_crc_bytes(&self.block[..self.block_len.next_multiple_of(4)])
                    .map_err(|_| self.fail(TransmittingError::Protocol))?,
            },
            TransmittingState::SendingData => {
                let remaining = self.block_len - self.cursor;
                if remaining == 0 {
                    return Err(self.fail(TransmittingError::Protocol));
                }
                let count = core::cmp::min(7, remaining);
                // One payload byte identifies the command, leaving seven for data.
                let mut data = [0; 7];
                data[..count].copy_from_slice(&self.block[self.cursor..self.cursor + count]);
                if count == remaining {
                    TransmittingCommand::BlockDataLast {
                        data,
                        len: count as u8,
                    }
                } else {
                    TransmittingCommand::BlockData {
                        data,
                        len: count as u8,
                    }
                }
            }
            TransmittingState::SendingFinish => TransmittingCommand::UploadFinished {
                crc: self.image_crc,
            },
            _ => return Ok(None),
        };
        let frame = cmd
            .encode(self.transmitting_id)
            .map_err(|_| self.fail(TransmittingError::Encode))?;
        self.pending = Some(frame);
        Ok(Some(frame))
    }
    /// Called after the CAN driver accepted the frame for transmission.
    pub fn frame_sent(&mut self, now: u32) -> Result<(), TransmittingError> {
        let frame = self.pending.take().ok_or(TransmittingError::Busy)?;
        match self.state {
            TransmittingState::SendingInit => {
                self.state = TransmittingState::WaitingReady;
                self.arm(now);
            }
            TransmittingState::SendingBlockInit => {
                self.state = TransmittingState::WaitingBlockReady;
                self.arm(now);
            }
            TransmittingState::SendingData => {
                let count = (frame.len - 1) as usize;
                self.cursor += count;
                if self.cursor == self.block_len {
                    self.state = TransmittingState::WaitingBlockAck;
                }
                self.arm(now);
            }
            TransmittingState::SendingFinish => {
                self.state = TransmittingState::WaitingUploadAck;
                self.arm(now);
            }
            _ => return Err(self.fail(TransmittingError::Protocol)),
        }
        Ok(())
    }
    /// Processes one receiver response and advances the upload state.
    ///
    /// Responses with another CAN ID or device UUID are ignored. A block NACK
    /// retries the same block up to [`MAX_BLOCK_RETRIES`].
    pub fn on_frame<S: FirmwareSource>(
        &mut self,
        frame: CanFrame,
        now: u32,
        source: &mut S,
    ) -> Result<(), TransmittingError> {
        if frame.id != self.receiving_id {
            return Ok(());
        }
        if self.pending.is_some() {
            return Err(TransmittingError::Busy);
        }
        let (response, uuid) =
            ReceivingResponse::decode(&frame).map_err(|_| TransmittingError::Protocol)?;
        // Replies from other devices on the shared CAN ID are not for this upload.
        if uuid != self.uuid {
            return Ok(());
        }
        match (self.state, response) {
            (TransmittingState::WaitingReady, ReceivingResponse::UpdateReady) => {
                self.load_block(source)?
            }
            (TransmittingState::WaitingBlockReady, ReceivingResponse::BlockReady) => {
                self.state = TransmittingState::SendingData;
                self.cursor = 0;
            }
            (TransmittingState::WaitingBlockAck, ReceivingResponse::BlockAck) => {
                self.offset += self.block_len as u32;
                self.block_number += 1;
                self.retries = 0;
                if self.offset == self.image_len {
                    self.state = TransmittingState::SendingFinish;
                } else {
                    self.load_block(source)?;
                }
            }
            (TransmittingState::WaitingBlockAck, ReceivingResponse::BlockNack) => {
                if self.retries >= MAX_BLOCK_RETRIES {
                    return Err(self.fail(TransmittingError::BlockRejected));
                }
                self.retries += 1;
                self.cursor = 0;
                self.state = TransmittingState::SendingBlockInit;
            }
            (TransmittingState::WaitingUploadAck, ReceivingResponse::UploadAck) => {
                self.state = TransmittingState::Completed
            }
            (TransmittingState::WaitingUploadAck, ReceivingResponse::UploadNack) => {
                return Err(self.fail(TransmittingError::UploadRejected))
            }
            _ => return Err(self.fail(TransmittingError::Protocol)),
        }
        // Keep a deadline for each response stage; waiting stages are armed after transmission.
        let _ = now;
        Ok(())
    }
    /// Checks acknowledgement deadlines using wrapping millisecond timestamps.
    ///
    /// Call periodically while an upload is active, including while waiting
    /// for erase, block or upload acknowledgements.
    pub fn tick(&mut self, now: u32) -> Result<(), TransmittingError> {
        if matches!(
            self.state,
            TransmittingState::WaitingReady
                | TransmittingState::WaitingBlockReady
                | TransmittingState::WaitingBlockAck
                | TransmittingState::WaitingUploadAck
        ) && (now.wrapping_sub(self.deadline) as i32) >= 0
        {
            return Err(self.fail(TransmittingError::Timeout));
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Bytes<'a>(&'a [u8]);
    impl FirmwareSource for Bytes<'_> {
        type Error = ();
        fn read(&mut self, offset: u32, output: &mut [u8]) -> Result<(), ()> {
            output.copy_from_slice(
                self.0
                    .get(offset as usize..offset as usize + output.len())
                    .ok_or(())?,
            );
            Ok(())
        }
    }
    fn sent(c: &mut TransmittingDevice, now: u32) -> TransmittingCommand {
        let frame = c.next_frame().unwrap().unwrap();
        assert_eq!(c.next_frame().unwrap(), Some(frame));
        c.frame_sent(now).unwrap();
        TransmittingCommand::decode(&frame).unwrap()
    }
    fn reply(
        c: &mut TransmittingDevice,
        s: &mut Bytes<'_>,
        response: ReceivingResponse,
        now: u32,
    ) -> Result<(), TransmittingError> {
        c.on_frame(response.encode(0x101, c.uuid), now, s)
    }
    #[test]
    fn full_transfer_short_block() {
        let image = [1, 2, 3, 4, 5, 6, 7, 8];
        let mut src = Bytes(&image);
        let mut c = TransmittingDevice::new(0x100, 0x101);
        c.start(&mut src, 42, 8).unwrap();
        assert_eq!(
            sent(&mut c, 0),
            TransmittingCommand::UpdateInit {
                image_len: 8,
                uuid: 42
            }
        );
        reply(&mut c, &mut src, ReceivingResponse::UpdateReady, 10).unwrap();
        assert!(matches!(
            sent(&mut c, 11),
            TransmittingCommand::BlockInit {
                block_number: 0,
                ..
            }
        ));
        reply(&mut c, &mut src, ReceivingResponse::BlockReady, 12).unwrap();
        assert!(matches!(
            sent(&mut c, 13),
            TransmittingCommand::BlockData { len: 7, .. }
        ));
        assert!(matches!(
            sent(&mut c, 14),
            TransmittingCommand::BlockDataLast { len: 1, .. }
        ));
        reply(&mut c, &mut src, ReceivingResponse::BlockAck, 15).unwrap();
        assert_eq!(
            sent(&mut c, 16),
            TransmittingCommand::UploadFinished {
                crc: stm32_crc_bytes(&image).unwrap()
            }
        );
        reply(&mut c, &mut src, ReceivingResponse::UploadAck, 17).unwrap();
        assert_eq!(c.state(), TransmittingState::Completed);
    }
    #[test]
    fn three_retries_then_abort() {
        let image = [1, 2, 3, 4];
        let mut src = Bytes(&image);
        let mut c = TransmittingDevice::new(0x100, 0x101);
        c.start(&mut src, 42, 4).unwrap();
        sent(&mut c, 0);
        reply(&mut c, &mut src, ReceivingResponse::UpdateReady, 1).unwrap();
        for attempt in 0..4 {
            sent(&mut c, 2);
            reply(&mut c, &mut src, ReceivingResponse::BlockReady, 3).unwrap();
            sent(&mut c, 4);
            let result = reply(&mut c, &mut src, ReceivingResponse::BlockNack, 5);
            if attempt < 3 {
                assert!(result.is_ok());
                assert_eq!(c.retries(), attempt + 1);
            } else {
                assert_eq!(result, Err(TransmittingError::BlockRejected));
            }
        }
        assert_eq!(c.state(), TransmittingState::Failed);
    }
    #[test]
    fn timeout_aborts_without_retry() {
        let image = [1, 2, 3, 4];
        let mut src = Bytes(&image);
        let mut c = TransmittingDevice::new(0x100, 0x101);
        c.start(&mut src, 42, 4).unwrap();
        sent(&mut c, 100);
        assert_eq!(c.tick(100 + FLASH_ERASE_TIMEOUT_MS - 1), Ok(()));
        assert_eq!(
            c.tick(100 + FLASH_ERASE_TIMEOUT_MS),
            Err(TransmittingError::Timeout)
        );
        assert_eq!(c.retries(), 0);
    }
}
