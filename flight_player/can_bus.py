import socket, struct
from math import atan2, pi, sqrt

import can
import numpy as np

from flight_data import FlightData


# This class generates Larus Can packets and sends them either over a UDP port or over a USB canbus 
# adapter. If the canbus adapter is to be used, it must be plugged in and enabled.

class Can():
    def __init__(self, udp_port: int):
        """Initialize the class with given UDP port"""
        self._ip = '127.0.0.1'
        self._port = udp_port
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP
        try:
            self._canbus = can.interface.Bus(interface='socketcan', channel='can0', bitrate=1_000_000)
        except:
            self._canbus = None
        self._interface = 'UDP' # or CAN

    def __del__(self):
        """Close the canbus interface correctly if it was open, when the class is destroyed"""
        if self._canbus is not None:
            self._canbus.shutdown()

    def canbus_available(self):
        """Tell if canbus is available"""
        if self._canbus is None:
            return False
        else:
            return True

    def set_interface(self, interface: str):
        """Set the interface ('CAN' or 'UDP')"""
        self._interface = interface

    def can_send_frames(self, data: FlightData):
        """Send the Larus canframes over the given physical interface"""

        # EULER ANGLES i16, i16, i16 roll nick yaw / 1/1000 rad
        self.can_send(0x0101,
                      to_i16(data['roll'] * 1000.0) +
                      to_i16(data['pitch'] * 1000.0) +
                      to_i16(data['yaw'] * 1000.0))

        # AIRSPEED tas, ias in km/h
        self.can_send(0x0102,
                      to_i16(data['TAS'] * 3.6) +
                      to_i16(data['IAS'] * 3.6))

        # VARIO climb_rate, average_climb_rate in mm/s
        self.can_send(0x103,
                      to_i16(data['vario'] * 1000.0) +
                      to_i16(data['vario integrator'] * 1000.0))

        # GPS date and time
        self.can_send(0x104,
                      to_u8(data['year']) + 
                      to_u8(data['month']) + 
                      to_u8(data['day']) + 
                      to_u8(data['hour']) + 
                      to_u8(data['minute']) + 
                      to_u8(data['second']))

        # GPS lat, lon
        self.can_send(0x105,
                      to_i32(data['Lat'] * 10_000_000.0) + 
                      to_i32(data['Long'] * 10_000_000.0))

        # GPS altitude and geo separation
        self.can_send(0x106,
                      to_i32(data['pos DWN'] * -1000.0) +
                      to_i32(data["geo separation dm"]))

        # GPS track and groundspeed
        self.can_send(0x107,
                      to_i16((data['track GNSS'] * pi / 180)*1000.0) +
                      to_u16(data['speed GNSS'] * 3.6))
        
        # GPS number of sats & gps state
        self.can_send(0x10a,
                      to_u8(data['sat number']) + 
                      to_u8(data['sat fix type']))

        # WIND wind 0.001 rad i16 km/h i16 avg wind 0.001 rad i16 km/h i16
        wind_direction = atan2(- data['wind E'], - data['wind N'])
        if wind_direction < 0.0:
            wind_direction += 2 * pi
        av_wind_direction = atan2(- data['wind avg E'], - data['wind avg N'])
        if av_wind_direction < 0.0:
            av_wind_direction += 2 * pi

        self.can_send(0x108,
                      to_i16(wind_direction * 1000.0) +
                      to_i16(sqrt(data['wind E'] ** 2 + data['wind N'] ** 2) * 3.6) + \
                      to_i16(av_wind_direction * 1000.0) +
                      to_i16(sqrt(data['wind avg E'] ** 2 + data['wind avg N'] ** 2) * 3.6))

        # ATMOSPHERE static pressure in Pa, air_density in g/m³
        self.can_send(0x109,
                      to_u32(data['Pressure-altitude']) +
                      to_u32(data['Air Density'] * 1000.0))

        # ACCELERATION g_load in mm/s², eff_vert_acc mm/s², vario_uncomp in mm/s,
        #              circle_mode (0 straight, 1 transition, 2 circling)
        self.can_send(0x10b,
                      to_i16(data['G_load'] * 1000.0) +
                      to_i16(data['acc vertical'] * -1000.0) +
                      to_i16(data['vario uncomp'] * -1000.0) +
                      to_u8(data['circle mode']))

        # TURN_COORD slip_angle in rad, turn_rate in rad/s, nick_angle in rad
        self.can_send(0x10c,
                      to_i16(data['slip angle'] * 1000.0) +
                      to_i16(data['turn rate'] * 1000.0) +
                      to_i16(data['pitch angle'] * 1000.0))
        

    def can_send(self, id: int, frame: bytes):
        """Send a single canbus frame"""
        if self._interface == 'UDP':
            payload = to_i16(id) + frame
            self._socket.sendto(payload, (self._ip, self._port))
        elif self._interface == 'CAN':
            msg = can.Message(arbitration_id=id, data=frame, is_extended_id=False)
            self._canbus.send(msg)
        else:
            raise ValueError("Unknown Interface")


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

