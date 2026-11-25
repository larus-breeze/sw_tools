//! STM32 CRC-32 (word-fed, non-reflected), polynomial 0x04C11DB7.
//!
//! Input bytes are grouped into little-endian `u32` words, matching a
//! little-endian STM32 reading 32-bit words from memory. Incomplete words
//! are rejected rather than silently padded.

/// Initial value of the STM32 CRC peripheral.
pub const CRC_INIT: u32 = 0xffff_ffff;
/// Non-reflected CRC-32 polynomial used by the STM32 peripheral.
pub const CRC_POLY: u32 = 0x04c1_1db7;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Input validation errors for byte-oriented CRC updates.
pub enum CrcError {
    UnalignedLength,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Incremental STM32-compatible CRC-32 calculator.
///
/// Each 32-bit word is processed most-significant-bit first. Byte slices
/// are interpreted as little-endian words and must have a multiple-of-four
/// length. No final XOR or implicit padding is applied.
pub struct Stm32Crc {
    value: u32,
}

impl Stm32Crc {
    /// Creates a CRC accumulator with [`CRC_INIT`].
    pub const fn new() -> Self {
        Self { value: CRC_INIT }
    }
    /// Resets the accumulator to its initial value.
    pub fn reset(&mut self) {
        self.value = CRC_INIT;
    }
    /// Feeds one 32-bit word into the accumulator.
    pub fn update_word(&mut self, word: u32) {
        self.value ^= word;
        for _ in 0..32 {
            self.value = if self.value & 0x8000_0000 != 0 {
                (self.value << 1) ^ CRC_POLY
            } else {
                self.value << 1
            };
        }
    }
    /// Feeds a sequence of 32-bit words.
    pub fn update_words(&mut self, words: &[u32]) {
        for &word in words {
            self.update_word(word);
        }
    }
    /// Processes complete 32-bit words only; never pads trailing bytes.
    /// Feeds a slice of complete little-endian 32-bit words.
    ///
    /// Returns [`CrcError::UnalignedLength`] without modifying the CRC if
    /// the byte length is not divisible by four. The caller must explicitly
    /// pad incomplete firmware words with `0xFF` where required.
    pub fn update_bytes(&mut self, bytes: &[u8]) -> Result<(), CrcError> {
        if bytes.len() % 4 != 0 {
            return Err(CrcError::UnalignedLength);
        }
        for chunk in bytes.chunks_exact(4) {
            self.update_word(u32::from_le_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]));
        }
        Ok(())
    }
    /// Returns the current CRC without consuming or resetting the accumulator.
    pub const fn finalize(&self) -> u32 {
        self.value
    }
}

impl Default for Stm32Crc {
    fn default() -> Self {
        Self::new()
    }
}

/// Computes the STM32 CRC-32 of a complete word slice.
pub fn stm32_crc(words: &[u32]) -> u32 {
    let mut crc = Stm32Crc::new();
    crc.update_words(words);
    crc.finalize()
}

/// Computes the STM32 CRC-32 of complete little-endian words in a byte slice.
pub fn stm32_crc_bytes(bytes: &[u8]) -> Result<u32, CrcError> {
    let mut crc = Stm32Crc::new();
    crc.update_bytes(bytes)?;
    Ok(crc.finalize())
}

#[cfg(test)]
mod tests {
    use super::*;

    // Original implementation supplied by the protocol author.
    fn reference(data: &[u32]) -> u32 {
        let mut crc: u32 = 0xffff_ffff;
        for w in data {
            for val in w.to_be_bytes() {
                crc ^= (val as u32) << 24;
                for _ in 0..8 {
                    if (crc & 0x8000_0000) == 0 {
                        crc <<= 1;
                    } else {
                        crc = crc.wrapping_shl(1) ^ 0x04c1_1db7;
                    }
                }
            }
        }
        crc
    }

    #[test]
    fn reference_equivalence() {
        for words in [
            &[][..],
            &[0u32][..],
            &[0x1234_5678][..],
            &[0xdead_beef, 0, 0xffff_ffff, 0x1020_3040][..],
        ] {
            assert_eq!(stm32_crc(words), reference(words));
        }
    }

    #[test]
    fn byte_order_and_incremental_update() {
        let words = [0x1234_5678, 0x90ab_cdef, 0x0000_0001];
        let bytes = [0x78, 0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x90, 1, 0, 0, 0];
        assert_eq!(stm32_crc_bytes(&bytes), Ok(stm32_crc(&words)));
        let mut crc = Stm32Crc::new();
        crc.update_bytes(&bytes[..4]).unwrap();
        crc.update_bytes(&bytes[4..]).unwrap();
        assert_eq!(crc.finalize(), stm32_crc(&words));
        crc.reset();
        assert_eq!(crc.finalize(), CRC_INIT);
    }

    #[test]
    fn rejects_partial_words_without_changing_state() {
        let mut crc = Stm32Crc::new();
        assert_eq!(crc.update_bytes(&[1, 2, 3]), Err(CrcError::UnalignedLength));
        assert_eq!(crc.finalize(), CRC_INIT);
    }

    #[test]
    fn full_and_short_block() {
        let block = [0x5au8; 512];
        let mut crc = Stm32Crc::new();
        crc.update_bytes(&block[..256]).unwrap();
        crc.update_bytes(&block[256..]).unwrap();
        assert_eq!(crc.finalize(), stm32_crc_bytes(&block).unwrap());
        assert!(stm32_crc_bytes(&block[..508]).is_ok());
    }
}
