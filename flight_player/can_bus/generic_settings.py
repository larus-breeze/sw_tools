 
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

class GenericSettings(CanDataParser):
    def parse(self, can_frame: CanFrame, can_frames: CanFrames, log: Logger):
        # print("generic_seetings", can_frame)
        if len(can_frame.data) != 8:
            return
        
        if can_frame.is_setting:
            config_id = struct.unpack('<H', can_frame.data[0:2])[0]

            if config_id < len(SETTINGS_LIST):
                name = SETTINGS_LIST[config_id]
                if config_id in (0, 6):
                    value = can_frame.data[2]
                    log.info(f"Set <GenericSetting name '{name}', value '{value}'>")
                else:
                    value = struct.unpack("<f", can_frame.data[-4:])[0]
                    log.info(f"Set <GenericSetting name '{name}', value '{value:.3f}'>")


