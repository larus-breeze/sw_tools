import socket
import can
from PyQt5 import QtCore

from flight_data import FlightData
from can_bus.can_frames import CanFrame, CanFrames, to_i16
from can_bus.legacy import can_legacy_protocol
from can_bus.new import can_new_protocol
from can_bus.generic_settings import GenericSettings
from can_bus.sensor_box import SensorBox

class CanInterface():
    """This class represents the interface between the Gui, the CAN bus interfaces and the protocol 
    definitions"""

    def __init__(self, udp_port: int, logger, can_frames: CanFrames):
        """Initialize the class with given UDP port"""
        self._ip = '127.0.0.1'
        self._port = udp_port
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP
        try:
            self._canbus = can.interface.Bus(interface='socketcan', channel='can0', bitrate=1_000_000)
        except:
            self._canbus = None
        self._interface = 'UDP' # or CAN
        self._protocol = 'legacy' # or new
        self._generic_settings = GenericSettings()
        self._sensor_box = SensorBox()
        self._logger = logger
        self._can_frames = can_frames

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
        if interface in ("UDP", "CAN"):
            self._interface = interface
            settings = QtCore.QSettings()
            settings.setValue('interface', interface)
        else:
            raise ValueError("Unknown Interface '%s'" % interface)

    def set_protocol(self, protocol: str):
        "Set the CAN protocol variant ('legacy' or 'new')"
        if protocol in ("legacy", "new"):
            self._protocol = protocol
            settings = QtCore.QSettings()
            settings.setValue('protocol', protocol)
        else:
            raise ValueError("Unknown CAN Protocol")

    def can_send_frames(self, data: FlightData):
        """Send the Larus canframes over the given physical interface"""
        if self._protocol == 'legacy':
            can_legacy_protocol(data, self._can_frames)
        else:
            can_new_protocol(data, self._can_frames)

        if self._interface == 'CAN':
            while True:
                msg = self._canbus.recv(0.000_1)
                if msg == None:
                    break
                can_frame = CanFrame(msg.arbitration_id, msg.data)
                self._generic_settings.parse(can_frame, self._can_frames, self._logger)
                self._sensor_box.parse(can_frame, self._can_frames, self._logger)
       
        while True:
            frame = self._can_frames.can_frame
            if frame is None:
                break
            else:
                if self._interface == 'UDP':
                    payload = to_i16(frame.id) + frame.data
                    self._socket.sendto(payload, (self._ip, self._port))
                elif self._interface == 'CAN':
                    msg = can.Message(arbitration_id=frame.id, data=frame.data, is_extended_id=False)
                    self._canbus.send(msg)
                else:
                    raise ValueError("Unknown Interface")
