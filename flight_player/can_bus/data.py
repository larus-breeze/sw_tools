import  struct
from math import atan2, pi, sqrt
import numpy as np

class CanData():
    def __init__(self):
        """Initialize datagram class"""
        self._datagrams = []
    
    def add(self, id: int, data: bytes):
        """Add a datagram to list"""
        self._datagrams.append((id, data))

    @property
    def datagrams(self):
        """Return list of datagrams"""
        return self._datagrams
   

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

