//! Firmware update storage in the first 512 KiB of STM32H743 flash bank 2.
//! Uses stm32h7xx-hal 0.16 flash API; no direct register access.
//!
//! Obtain bank 2 once during startup:
//! `use stm32h7xx_hal::flash::FlashExt;`
//! `let (_bank1, mut bank2) = dp.FLASH.split();`
//! Pass `&mut bank2` to the functions below.

use can_update::stm32_crc_bytes;
use stm32h7xx_hal::flash::LockedFlashBank;

pub const BASE: usize = 0x0810_0000;
pub const CAPACITY: usize = 512 * 1024;
pub const SECTOR_SIZE: usize = 128 * 1024;
pub const FLASH_WORD_SIZE: usize = 32;
pub const SECTOR_COUNT: usize = CAPACITY / SECTOR_SIZE;

#[derive(Debug)]
pub enum FlashError {
    Empty,
    OutOfRange,
    Unaligned,
    WrongBank,
    Hal(stm32h7xx_hal::flash::Error),
    Crc,
    CrcMismatch { expected: u32, actual: u32 },
}

fn check_bank(bank: &mut LockedFlashBank) -> Result<(), FlashError> {
    let unlocked = bank.unlocked();
    let valid = unlocked.address() == BASE && unlocked.len() >= CAPACITY;
    drop(unlocked); // Automatically relocks the bank.
    if valid {
        Ok(())
    } else {
        Err(FlashError::WrongBank)
    }
}

pub fn sectors_for_image(image_len: usize) -> Result<usize, FlashError> {
    if image_len == 0 {
        return Err(FlashError::Empty);
    }
    if image_len > CAPACITY {
        return Err(FlashError::OutOfRange);
    }
    Ok(image_len.div_ceil(SECTOR_SIZE))
}

/// Erases only the sectors covering `image_len` (bank-2 sectors 0..=3).
pub fn erase_image(bank: &mut LockedFlashBank, image_len: usize) -> Result<(), FlashError> {
    let count = sectors_for_image(image_len)?;
    check_bank(bank)?;
    let mut flash = bank.unlocked();
    for sector in 0..count {
        flash.erase_sector(sector as u8).map_err(FlashError::Hal)?;
    }
    Ok(()) // Drop relocks bank even on errors.
}

/// Writes one update block. Offsets must be 32-byte aligned; the HAL pads
/// a final partial flash word with 0xFF. Never rewrite an already programmed word.
pub fn program_block(
    bank: &mut LockedFlashBank,
    offset: usize,
    data: &[u8],
) -> Result<(), FlashError> {
    if data.is_empty() {
        return Err(FlashError::Empty);
    }
    if offset % FLASH_WORD_SIZE != 0 {
        return Err(FlashError::Unaligned);
    }
    if offset
        .checked_add(data.len())
        .filter(|&end| end <= CAPACITY)
        .is_none()
    {
        return Err(FlashError::OutOfRange);
    }
    check_bank(bank)?;
    bank.unlocked()
        .program(offset, data.iter())
        .map_err(FlashError::Hal)
}

/// Reads the image back and compares its STM32 CRC32 with the expected value.
/// CRC includes 0xFF flash padding up to the next 4-byte boundary.
/// The original image length remains unchanged.
pub fn verify_image(
    bank: &mut LockedFlashBank,
    image_len: usize,
    expected: u32,
) -> Result<(), FlashError> {
    sectors_for_image(image_len)?;
    check_bank(bank)?;
    let flash = bank.unlocked();
    let crc_len = image_len.next_multiple_of(4);
    let bytes = &flash.read_all()[..crc_len];
    let actual = stm32_crc_bytes(bytes).map_err(|_| FlashError::Crc)?;
    if actual == expected {
        Ok(())
    } else {
        Err(FlashError::CrcMismatch { expected, actual })
    }
}
