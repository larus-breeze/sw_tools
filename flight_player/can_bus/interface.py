import socket
import can

from flight_data import FlightData
from can_bus.data import CanData, to_i16
from can_bus.legacy import can_legacy_protocol
from can_bus.new import can_new_protocol

class CanInterface():
    """This class represents the interface between the Gui, the CAN bus interfaces and the protocol 
    definitions"""

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
        self._protocol = 'legacy' # or new

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
        else:
            raise ValueError("Unknown Interface '%s'" % interface)

    def set_protocol(self, protocol: str):
        "Set the CAN protocol variant ('legacy' or 'new')"
        if protocol in ("legacy", "new"):
            self._protocol = protocol
        else:
            raise ValueError("Unknown CAN Protocol")

    def can_send_frames(self, data: FlightData):
        """Send the Larus canframes over the given physical interface"""
        can_data = CanData()
        if self._protocol == 'legacy':
            can_legacy_protocol(data, can_data)
        else:
            can_new_protocol(data, can_data)
        
        for id, frame in can_data.datagrams:
            if self._interface == 'UDP':
                payload = to_i16(id) + frame
                self._socket.sendto(payload, (self._ip, self._port))
            elif self._interface == 'CAN':
                msg = can.Message(arbitration_id=id, data=frame, is_extended_id=False)
                self._canbus.send(msg)
            else:
                raise ValueError("Unknown Interface")
