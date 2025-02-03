 
import struct
from can_bus.can_frames import *


SETTINGS_LIST = (
    "volume_vario",
    "mac_cready",
    "water_ballast",
    "bugs",
    "qnh",
    "pilot_weight",
    "vario_mode_control",
    "tc_climb_rate",
    "tc_speed_to_fly",
)

class GenericSetting():
    def __init__(self, can_frame: CanFrame):
        self._config_id = struct.unpack('<H', can_frame.data[0:2])[0]
        if self._config_id in (0, 6):
            self._value = can_frame.data[2]
        else:
            self._value = struct.unpack("<f", can_frame.data[-4:])[0]
        self._name = SETTINGS_LIST[self._config_id]

    def __repr__(self):
        if type(self._value) == int:
            return f"<GenericSetting name '{self._name}', value '{self._value}'>"
        else:
            return f"<GenericSetting name '{self._name}', value '{self._value:.3f}'>"



class GenericSettings(CanDataParser):
    def parse(self, can_frame: CanFrame, can_frames: CanFrames, log: Logger):
        if len(can_frame.data) != 8:
            return
        
        if can_frame.is_setting:
            setting = GenericSetting(can_frame)
            log.info(f"Set {setting}")


