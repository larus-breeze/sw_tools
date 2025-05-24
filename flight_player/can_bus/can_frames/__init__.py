import  struct
from math import atan2, pi, sqrt
import numpy as np
from logger import Logger


class CanFrames():
    def __init__(self):
        """Initialize datagram class"""
        self._can_frames = []
    
    def add(self, id: int, data: bytes):
        """Add a datagram to list"""
        self._can_frames.append(CanFrame(id, data))

    @property
    def can_frames(self):
        """Return list of datagrams"""
        return self._can_frames
   

# Below are some functions that generate defined binary data

def to_u32(val: int|float) -> bytes:
    val = np.clip(val, 0, 4294967295)
    return round(val).to_bytes(4, byteorder='little', signed=False)

def to_u16(val: int|float) -> bytes:
    val = np.clip(val, 0, 65535 )
    return round(val).to_bytes(2, byteorder='little', signed=False)

def to_u8(val: int|float) -> bytes:
    val = np.clip(val, 0, 255)
    return round(val).to_bytes(1, byteorder='little', signed=False)

def to_i32(val: int|float) -> bytes:
    val = np.clip(val, -2147483648, 2147483647)
    return round(val).to_bytes(4, byteorder='little', signed=True)

def to_i16(val: int|float) -> bytes:
    val = np.clip(val, -32768, 32767)
    return round(val).to_bytes(2, byteorder='little', signed=True)

def to_i8(val: int|float) -> bytes:
    val = np.clip(val, -128, 127)
    return round(val).to_bytes(1, byteorder='little', signed=True)

def to_f32(val: float) -> bytes:
    return struct.pack('<f', val)

def to_f64(val: float) -> bytes:
    return struct.pack('<d', val)


class CanFrame():
    def __init__(self, id, data):
        self._id = id
        self._data = data

    @property
    def id(self):
        return self._id
    
    @property
    def data(self):
        return self._data
    
    @property
    def is_generic(self):
        return self._id >= 0x400
    
    @property
    def is_setting(self):
        return self.is_generic and self._id &0x00f==0x002

    def __repr__(self):
        hex_data = ''.join('{:02x}'.format(x) for x in self._data)
        return f"<CanFrame id {self._id}, data {hex_data}>"


class CanDataParser():

    # overwrite parse
    def parse(can_frame: CanFrame, can_frames: CanFrames, _log: Logger):
        raise NotImplementedError
