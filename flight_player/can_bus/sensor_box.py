 
import os, struct
from can_bus.can_frames import *


class Setting():
    def __init__(self, name, value):
        self._name = name
        self._value = value
        
    @property
    def value(self):
        return self._value
    
    @value.setter
    def value(self, value: str):
        self._value = float(value)

    @property
    def data(self):
        return struct.pack('<f', self._value)
    
    @data.setter
    def data(self, data):
        self._value = struct.unpack("<f", data[-4:])[0]

    @property
    def line(self):
        return f"{self._name} = {self._value}\n"
    
    def __repr__(self):
        return f"<Sensorbox Setting name '{self._name}', value '{self._value:.3f}'>"


SETTINGS_LIST = (
    ("SensTilt_Roll", 0.0, 0x000),
    ("SensTilt_Pitch", 0.0, 0x001),
    ("SensTilt_Yaw", 0.0, 0x002),

    ("Pitot_Offset", 0.0, 0x003),
    ("Pitot_Span", 1.0, 0x004),
    ("QNH-delta", 0.0, 0x005),

    ("Mag_Auto_Calib", 1.0, 0x006),
    ("Vario_TC", 2.0, 0x007),
    ("Vario_Int_TC", 30.0, 0x008),
    ("Wind_TC", 5.0, 0x009),
    ("Mean_Wind_TC", 30.0, 0x00a),

    ("GNSS_CONFIG", 1.0, 0x00b),

    ("ANT_BASELEN", 1.0, 0x00c),
    ("ANT_SLAVE_DOWN", 0.0, 0x00d),
    ("ANT_SLAVE_RIGHT", 0.0, 0x00e),
)

INI_DEFS = (
    ("sensor-orientation", ("SensTilt_Roll", "SensTilt_Pitch", "SensTilt_Yaw")),
    ("pressure-calibration", ("Pitot_Offset", "Pitot_Span", "QNH-delta")),
    ("preferences", ("Mag_Auto_Calib", "Vario_TC", "Vario_Int_TC", "Wind_TC", "Mean_Wind_TC")),
    ("GNSS-type", ("GNSS_CONFIG", )), 
    ("D-GNSS-configuration", ("ANT_BASELEN", "ANT_SLAVE_DOWN", "ANT_SLAVE_RIGHT")),
)

SETTINGS_FILE_NAME = "sensor_config.ini"

class Settings():
    def __init__(self, settings_list):
        self._settings_by_id = {}
        self._settings_by_name = {}
        for name, value, id in settings_list:
            setting = Setting(name, value)
            self._settings_by_id[id] = setting
            self._settings_by_name[name] = setting

        self.load()

    def load(self):
        if os.path.isfile(SETTINGS_FILE_NAME):
            with open(SETTINGS_FILE_NAME, 'r') as f:
                self.string = f.read()

    def dump(self):
        with open(SETTINGS_FILE_NAME, 'w') as f:
            f.write(self.string)            

    @property
    def string(self):
        r = ""
        for ini_def in INI_DEFS:
            r += f"[{ini_def[0]}]\n"
            for name in ini_def[1]:
                r += self._settings_by_name[name].line
            r += "\n"
        return r
    
    @string.setter
    def string(self, s: str):
        for line in s.splitlines():
            if "=" in line:
                name, value = line.replace(" ", "").split("=")
                if name in self._settings_by_name:
                    setting = self._settings_by_name[name]
                    setting.value = value
                else:
                    raise ValueError
                
    def parse(self, config_id: int, config_data: bytes, can_frames: CanFrames, log: Logger):
        if config_id in self._settings_by_id:
            setting = self._settings_by_id[config_id]
            if config_data[0] == 0: # get
                data = to_u32(config_id) + setting.data
                log.info(f"Get {setting}")
                can_frames.add(0x12f, data) # Send value back
            elif config_data[0] == 1: #set
                setting.data = config_data
                log.info(f"Set {setting}")
                self.dump()
            else:
                raise ValueError

COMMANDS = (
    "cmd_measure_pos_1",
    "cmd_measure_pos_2",
    "cmd_measure_pos_3", 
    "cmd_calc_sensor_orientation", 
    "cmd_fine_tune_calibration",
)

class SensorBox(CanDataParser):
    def __init__(self):
        self._settings = Settings(SETTINGS_LIST)

    def parse(self, can_frame: CanFrame, can_frames: CanFrames, log: Logger):
        if len(can_frame.data) != 8:
            return
        
        if can_frame.is_setting:
            config_id = struct.unpack('<H', can_frame.data[:2])[0]
            config_data = can_frame.data[2:]
            idx = config_id & 0x0f 
    
            if config_id >= 0x2000 and config_id <= 0x2fff: # Specific Config Data
                self._settings.parse(idx, config_data, can_frames, log)
            elif config_id >= 0x3000 and config_id <= 0x3fff: # Specific Command
                if idx < 5:
                    log.info(f"Sensor Command '{COMMANDS[idx]}'")


