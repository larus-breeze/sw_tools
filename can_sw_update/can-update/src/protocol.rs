//! CAN firmware update wire format. CAN identifiers are configured by the caller.
//! The first payload byte is the command discriminator; multibyte integers are LE.

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// One classic CAN frame with up to eight payload bytes.
///
/// `len` identifies the number of valid bytes in `data`.
pub struct CanFrame {
    /// CAN identifier selected by the application.
    pub id: u32,
    /// Number of valid payload bytes (0..=8).
    pub len: u8,
    /// Payload storage; bytes after `len` are not part of the frame.
    pub data: [u8; 8],
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Errors encountered while encoding a protocol command.
pub enum EncodeError {
    ValueOutOfRange,
}
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Errors encountered while decoding an incoming CAN payload.
pub enum DecodeError {
    InvalidLength,
    UnknownCommand,
    InvalidField,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Commands sent by the transmitting device to the receiving device.
///
/// The first CAN data byte selects the command. Multi-byte values use
/// little-endian encoding; some counters and lengths occupy three bytes.
pub enum TransmittingCommand {
    UpdateInit { image_len: u32, uuid: u32 },
    BlockInit { block_number: u32, crc: u32 },
    BlockData { data: [u8; 7], len: u8 },
    BlockDataLast { data: [u8; 7], len: u8 },
    UploadFinished { crc: u32 },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
/// Acknowledgements and rejections sent by the receiving device.
pub enum ReceivingResponse {
    UpdateReady,
    BlockReady,
    BlockAck,
    BlockNack,
    UploadAck,
    UploadNack,
}

// Store a bounded 24-bit integer in little-endian wire format.
fn put_u24(dst: &mut [u8], value: u32) -> Result<(), EncodeError> {
    if value > 0x00ff_ffff {
        return Err(EncodeError::ValueOutOfRange);
    }
    dst.copy_from_slice(&value.to_le_bytes()[..3]);
    Ok(())
}
// Recover a 24-bit little-endian integer without sign extension.
fn get_u24(src: &[u8]) -> u32 {
    u32::from_le_bytes([src[0], src[1], src[2], 0])
}
fn frame(id: u32, bytes: &[u8]) -> CanFrame {
    let mut data = [0u8; 8];
    data[..bytes.len()].copy_from_slice(bytes);
    CanFrame {
        id,
        len: bytes.len() as u8,
        data,
    }
}
// Reject malformed DLC values before indexing the payload.
fn payload(frame: &CanFrame) -> Result<&[u8], DecodeError> {
    if frame.len == 0 || frame.len > 8 {
        return Err(DecodeError::InvalidLength);
    }
    Ok(&frame.data[..frame.len as usize])
}
fn exact(bytes: &[u8], len: usize) -> Result<(), DecodeError> {
    if bytes.len() != len {
        Err(DecodeError::InvalidLength)
    } else {
        Ok(())
    }
}

impl TransmittingCommand {
    /// Encodes a sender command into a CAN frame with identifier `id`.
    pub fn encode(self, id: u32) -> Result<CanFrame, EncodeError> {
        let mut bytes = [0u8; 8];
        let len = match self {
            Self::UpdateInit { image_len, uuid } => {
                bytes[0] = 1;
                put_u24(&mut bytes[1..4], image_len)?;
                bytes[4..8].copy_from_slice(&uuid.to_le_bytes());
                8
            }
            Self::BlockInit { block_number, crc } => {
                bytes[0] = 2;
                put_u24(&mut bytes[1..4], block_number)?;
                bytes[4..8].copy_from_slice(&crc.to_le_bytes());
                8
            }
            Self::BlockData { data, len } | Self::BlockDataLast { data, len } => {
                if len > 7 {
                    return Err(EncodeError::ValueOutOfRange);
                }
                bytes[0] = if matches!(self, Self::BlockDataLast { .. }) {
                    4
                } else {
                    3
                };
                bytes[1..1 + len as usize].copy_from_slice(&data[..len as usize]);
                1 + len as usize
            }
            Self::UploadFinished { crc } => {
                bytes[0] = 5;
                bytes[4..8].copy_from_slice(&crc.to_le_bytes());
                8
            }
        };
        Ok(frame(id, &bytes[..len]))
    }

    /// Decodes and validates a sender command from a CAN frame.
    pub fn decode(frame: &CanFrame) -> Result<Self, DecodeError> {
        let b = payload(frame)?;
        match b[0] {
            1 => {
                exact(b, 8)?;
                Ok(Self::UpdateInit {
                    image_len: get_u24(&b[1..4]),
                    uuid: u32::from_le_bytes(b[4..8].try_into().unwrap()),
                })
            }
            2 => {
                exact(b, 8)?;
                Ok(Self::BlockInit {
                    block_number: get_u24(&b[1..4]),
                    crc: u32::from_le_bytes(b[4..8].try_into().unwrap()),
                })
            }
            3 | 4 => {
                let mut data = [0u8; 7];
                data[..b.len() - 1].copy_from_slice(&b[1..]);
                let len = (b.len() - 1) as u8;
                if b[0] == 3 {
                    Ok(Self::BlockData { data, len })
                } else {
                    Ok(Self::BlockDataLast { data, len })
                }
            }
            5 => {
                exact(b, 8)?;
                Ok(Self::UploadFinished {
                    crc: u32::from_le_bytes(b[4..8].try_into().unwrap()),
                })
            }
            _ => Err(DecodeError::UnknownCommand),
        }
    }
}

impl ReceivingResponse {
    /// All RD replies contain command, three zero padding bytes, and device UUID.
    /// Encodes a receiver response tagged with the target device UUID.
    pub fn encode(self, id: u32, uuid: u32) -> CanFrame {
        let code = match self {
            Self::UpdateReady => 1,
            Self::BlockReady => 2,
            Self::BlockAck => 3,
            Self::BlockNack => 4,
            Self::UploadAck => 5,
            Self::UploadNack => 6,
        };
        let mut bytes = [0u8; 8];
        bytes[0] = code;
        bytes[4..8].copy_from_slice(&uuid.to_le_bytes());
        frame(id, &bytes)
    }

    /// Returns the response type and the UUID of the responding device.
    /// Decodes a receiver response and returns its associated device UUID.
    pub fn decode(frame: &CanFrame) -> Result<(Self, u32), DecodeError> {
        let b = payload(frame)?;
        exact(b, 8)?;
        if b[1..4] != [0, 0, 0] {
            return Err(DecodeError::InvalidField);
        }
        let response = match b[0] {
            1 => Self::UpdateReady,
            2 => Self::BlockReady,
            3 => Self::BlockAck,
            4 => Self::BlockNack,
            5 => Self::UploadAck,
            6 => Self::UploadNack,
            _ => return Err(DecodeError::UnknownCommand),
        };
        let uuid = u32::from_le_bytes(b[4..8].try_into().unwrap());
        Ok((response, uuid))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn transmitting_roundtrip() {
        let commands = [
            TransmittingCommand::UpdateInit {
                image_len: 0x123456,
                uuid: 0x12345678,
            },
            TransmittingCommand::BlockInit {
                block_number: 0x234567,
                crc: 0xabcdef12,
            },
            TransmittingCommand::BlockData {
                data: [1, 2, 3, 4, 5, 6, 7],
                len: 7,
            },
            TransmittingCommand::BlockDataLast {
                data: [9, 8, 7, 0, 0, 0, 0],
                len: 3,
            },
            TransmittingCommand::UploadFinished { crc: 0x76543210 },
        ];
        for command in commands {
            let encoded = command.encode(0x100).unwrap();
            assert_eq!(encoded.id, 0x100);
            assert_eq!(TransmittingCommand::decode(&encoded), Ok(command));
        }
    }
    #[test]
    fn receiving_roundtrip() {
        for response in [
            ReceivingResponse::UpdateReady,
            ReceivingResponse::BlockReady,
            ReceivingResponse::BlockAck,
            ReceivingResponse::BlockNack,
            ReceivingResponse::UploadAck,
            ReceivingResponse::UploadNack,
        ] {
            let encoded = response.encode(0x101, 0x12345678);
            assert_eq!(encoded.len, 8);
            assert_eq!(encoded.data[1..4], [0, 0, 0]);
            assert_eq!(encoded.data[4..8], [0x78, 0x56, 0x34, 0x12]);
            assert_eq!(
                ReceivingResponse::decode(&encoded),
                Ok((response, 0x12345678))
            );
        }
    }
    #[test]
    fn little_endian_layout() {
        let f = TransmittingCommand::UpdateInit {
            image_len: 0x123456,
            uuid: 0x12345678,
        }
        .encode(1)
        .unwrap();
        assert_eq!(f.data, [1, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12]);
    }
    #[test]
    fn reject_invalid_receiving_response() {
        let valid = ReceivingResponse::BlockAck.encode(0x101, 0x12345678);
        let mut invalid_padding = valid;
        invalid_padding.data[2] = 1;
        assert_eq!(
            ReceivingResponse::decode(&invalid_padding),
            Err(DecodeError::InvalidField)
        );
        let mut invalid_length = valid;
        invalid_length.len = 1;
        assert_eq!(
            ReceivingResponse::decode(&invalid_length),
            Err(DecodeError::InvalidLength)
        );
        let mut unknown_command = valid;
        unknown_command.data[0] = 99;
        assert_eq!(
            ReceivingResponse::decode(&unknown_command),
            Err(DecodeError::UnknownCommand)
        );
    }
    #[test]
    fn reject_invalid_values() {
        assert_eq!(
            TransmittingCommand::UpdateInit {
                image_len: 0x1000000,
                uuid: 0
            }
            .encode(1),
            Err(EncodeError::ValueOutOfRange)
        );
        assert_eq!(
            TransmittingCommand::BlockData {
                data: [0; 7],
                len: 8
            }
            .encode(1),
            Err(EncodeError::ValueOutOfRange)
        );
        assert_eq!(
            TransmittingCommand::decode(&CanFrame {
                id: 1,
                len: 0,
                data: [0; 8]
            }),
            Err(DecodeError::InvalidLength)
        );
    }
}
