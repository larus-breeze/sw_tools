//! Hardware-independent, allocation-free CAN firmware update protocol.
//!
//! The transmitting device sends firmware in 512-byte blocks over classic CAN; the
//! receiving device receives the blocks and requests flash operations from its host.
//! Both sides are non-blocking state machines and use caller-provided time in ms.
//! The application is responsible for CAN I/O, persistent storage and flash access.
//! CRC calculations use STM32 word-fed CRC-32 and pad incomplete words with 0xFF
//! at the call sites; the transmitted image length remains the original length.
#![no_std]

/// Maximum wait for flash erase acknowledgement (RD UpdateReady).
pub const FLASH_ERASE_TIMEOUT_MS: u32 = 10_000;
/// Maximum wait for every other protocol acknowledgement.
pub const PROTOCOL_TIMEOUT_MS: u32 = 1_500;
/// Firmware block capacity in bytes; the final block may be shorter.
const BLOCK_SIZE: usize = 512;

/// STM32-compatible CRC-32 implementation.
pub mod crc;
/// CAN message encoding and decoding.
pub mod protocol;
pub use crc::{stm32_crc, stm32_crc_bytes, CrcError, Stm32Crc};
pub use protocol::{CanFrame, DecodeError, EncodeError, ReceivingResponse, TransmittingCommand};
/// Receiver state machine and flash-operation requests.
pub mod receiving_device;
pub use receiving_device::{
    ReceivingAction, ReceivingDevice, ReceivingError, State as ReceivingState,
};
/// Sender state machine and firmware-source abstraction.
pub mod transmitting_device;
pub use transmitting_device::{
    FirmwareSource, TransmittingDevice, TransmittingError, TransmittingState, MAX_BLOCK_RETRIES,
};
