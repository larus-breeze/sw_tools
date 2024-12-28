#!/user/bin/env python3
import numpy as np
import pandas as pd
import os
import sys
import subprocess
from pathlib import Path

# Format with datatypes
# dtypes  int32_t  = i4,  int8_t = i1, uint16_t = u2,  f4 = float, f8 = double

# Raw data sensor files
data_f37 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
            ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("pitot", "f4"), ("static p", "f4"), ("temp", "f4"),
            ("ubatt", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"), ("vel E", "f4"),
            ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"), ("track GNSS", "f4"),
            ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"), ("rel HDG", "f4"),
            ("speed acc", "f4"),
            ("Lat", "f8"), ("Long", "f8"),
            ("year", "u1"),
            ("month", "u1"),
            ("day", "u1"),
            ("hour", "u1"),
            ("minute", "u1"),
            ("second", "u1"),
            ("sat number", "u1"),
            ("sat fix type", "u1"),
            ("nanoseconds", "u4"),
            ("geo separation dm", "i2"),
            ("dummy", "u2")]

data_f50 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
            ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("Lowcost acc x", "f4"), ("Lowcost acc y", "f4"),
            ("Lowcost acc z", "f4"), ("Lowcost gyro x", "f4"), ("Lowcost gyro y", "f4"), ("Lowcost gyro z", "f4"),
            ("Lowcost mag x", "f4"), ("Lowcost mag y", "f4"), ("Lowcost mag z", "f4"), ("pitot", "f4"),
            ("static p", "f4"), ("abs p", "f4"), ("temp", "f4"), ("abs sens t", "f4"), ("ubatt", "f4"),
            ("", "f4"), ("", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"),
            ("vel E", "f4"), ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"),
            ("track GNSS", "f4"), ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"),
            ("rel HDG", "f4"), ("speed acc", "f4"),
            ("Lat", "f8"), ("Long", "f8"),
            ("year", "u1"),
            ("month", "u1"),
            ("day", "u1"),
            ("hour", "u1"),
            ("minute", "u1"),
            ("second", "u1"),
            ("sat number", "u1"),
            ("sat fix type", "u1"),
            ("xx", "f4"),
            ("yy", "f4")]


# Processed data with vario, wind and ahrs values, ...
data_f110 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
             ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("pitot", "f4"), ("static p", "f4"), ("temp", "f4"),
             ("ubatt", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"), ("vel E", "f4"),
             ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"), ("track GNSS", "f4"),
             ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"), ("rel HDG", "f4"),
             ("speed acc", "f4"),
             ("Lat", "f8"), ("Long", "f8"),
             ("year", "u1"),
             ("month", "u1"),
             ("day", "u1"),
             ("hour", "u1"),
             ("minute", "u1"),
             ("second", "u1"),
             ("sat number", "u1"),
             ("sat fix type", "u1"),
             ("nanoseconds", "u4"),
             ("geo separation dm", "i2"),
             ("dummy", "u2"),
             ("IAS", "f4"), ("TAS", "f4"), ("vario uncomp", "f4"), ("vario", "f4"), ("vario pressure", "f4"),
             ("speed comp TAS", "f4"), ("speed comp INS", "f4"), ("vario integrator", "f4"), ("wind N", "f4"),
             ("wind E", "f4"), ("wind D", "f4"), ("wind avg N", "f4"), ("wind avg E", "f4"), ("wind avg D", "f4"),
             ("circle mode", "u4"),
             ("q0", "f4"), ("q1", "f4"), ("q2", "f4"), ("q3", "f4"),
             ("roll", "f4"), ("pitch", "f4"), ("yaw", "f4"),
             ("acc vertical", "f4"), ("turn rate", "f4"), ("slip angle", "f4"), ("pitch angle", "f4"),
             ("G_load", "f4"), ("Pressure-altitude", "f4"), ("Air Density", "f4"), ("Magnetic Disturbance", "f4"),
             ("nav acc N", "f4"), ("nav acc E", "f4"), ("nav acc D", "f4"), ("nav ind N", "f4"),
             ("nav ind E", "f4"), ("nav ind D", "f4"), ("nav corr N", "f4"), ("nav corr E", "f4"),
             ("nav corr D", "f4"), ("gyro corr F", "f4"), ("gyro corr R", "f4"), ("gyro corr D", "f4"),
             ("nav acc mag N", "f4"), ("nav acc mag E", "f4"), ("nav acc mag D", "f4"), ("nav ind mag N", "f4"),
             ("nav ind mag E", "f4"), ("nav ind mag D", "f4"),
             ("roll mag", "f4"), ("pitch mag", "f4"), ("yaw mag", "f4"),
             ("q1mag", "f4"), ("q2mag", "f4"), ("q3mag", "f4"), ("q4mag", "f4"),
             ("acc_F", "f4"), ("acc_R", "f4"), ("acc_D", "f4"),
             ("gyro_F", "f4"), ("gyro_R", "f4"), ("gyro_D", "f4"),
             ("SAT-AHRS-Heading", "f4"), ("QFF", "f4"), ("sat fix type f", "f4"),
             ("avg. Headwind", "f4"), ("avg. crosswind", "f4"),
             ("wind_N", "f4"), ("wind_E", "f4"),
             ("inst wind N", "f4"), ("inst wind E", "f4"),
             ("speed_comp_1", "f4"),
             ("speed_comp_2", "f4"),
             ("speed_comp_3", "f4")]

data_f114 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
             ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("pitot", "f4"), ("static p", "f4"), ("temp", "f4"),
             ("ubatt", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"), ("vel E", "f4"),
             ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"), ("track GNSS", "f4"),
             ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"), ("rel HDG", "f4"),
             ("speed acc", "f4"),
             ("Lat", "f8"), ("Long", "f8"),
             ("year", "u1"),
             ("month", "u1"),
             ("day", "u1"),
             ("hour", "u1"),
             ("minute", "u1"),
             ("second", "u1"),
             ("sat number", "u1"),
             ("sat fix type", "u1"),
             ("nanoseconds", "u4"),
             ("geo separation dm", "i2"),
             ("dummy", "u2"),
             ("IAS", "f4"), ("TAS", "f4"), ("vario uncomp", "f4"), ("vario", "f4"), ("vario pressure", "f4"),
             ("speed comp TAS", "f4"), ("speed comp INS", "f4"), ("vario integrator", "f4"), ("wind N", "f4"),
             ("wind E", "f4"), ("wind D", "f4"), ("wind avg N", "f4"), ("wind avg E", "f4"), ("wind avg D", "f4"),
             ("circle mode", "u4"),
             ("q0", "f4"), ("q1", "f4"), ("q2", "f4"), ("q3", "f4"),
             ("roll", "f4"), ("pitch", "f4"), ("yaw", "f4"),
             ("acc vertical", "f4"), ("turn rate", "f4"), ("slip angle", "f4"), ("pitch angle", "f4"),
             ("G_load", "f4"), ("Pressure-altitude", "f4"), ("Air Density", "f4"), ("Magnetic Disturbance", "f4"),
             ("nav acc N", "f4"), ("nav acc E", "f4"), ("nav acc D", "f4"), ("nav ind N", "f4"),
             ("nav ind E", "f4"), ("nav ind D", "f4"), ("nav corr N", "f4"), ("nav corr E", "f4"),
             ("nav corr D", "f4"), ("gyro corr F", "f4"), ("gyro corr R", "f4"), ("gyro corr D", "f4"),
             ("nav acc mag N", "f4"), ("nav acc mag E", "f4"), ("nav acc mag D", "f4"), ("nav ind mag N", "f4"),
             ("nav ind mag E", "f4"), ("nav ind mag D", "f4"),
             ("roll mag", "f4"), ("pitch mag", "f4"), ("yaw mag", "f4"),
             ("q1mag", "f4"), ("q2mag", "f4"), ("q3mag", "f4"), ("q4mag", "f4"),
             ("acc_F", "f4"), ("acc_R", "f4"), ("acc_D", "f4"),
             ("gyro_F", "f4"), ("gyro_R", "f4"), ("gyro_D", "f4"),
             ("SAT-AHRS-Heading", "f4"), ("QFF", "f4"), ("sat fix type f", "f4"),
             ("inst wind N", "f4"), ("inst wind E", "f4"),
             ("avg. Headwind", "f4"), ("avg. crosswind", "f4"),
             ("inst wind corrected N", "f4"), ("inst wind corrected E", "f4"),
             ("speed_comp_1", "f4"),
             ("speed_comp_2", "f4"),
             ("speed_comp_3", "f4"),
             ("speed_comp_4", "f4"),
             ("cross_acc_correction", "f4"),
             ("vario_wind_N", "f4"),
             ("vario_wind_E", "f4")
             ]

data_f119 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
             ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("pitot", "f4"), ("static p", "f4"), ("temp", "f4"),
             ("ubatt", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"), ("vel E", "f4"),
             ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"), ("track GNSS", "f4"),
             ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"), ("rel HDG", "f4"),
             ("speed acc", "f4"),
             ("Lat", "f8"), ("Long", "f8"),
             ("year", "u1"),
             ("month", "u1"),
             ("day", "u1"),
             ("hour", "u1"),
             ("minute", "u1"),
             ("second", "u1"),
             ("sat number", "u1"),
             ("sat fix type", "u1"),
             ("nanoseconds", "u4"),
             ("geo separation dm", "i2"),
             ("dummy", "u2"),
             ("IAS", "f4"), ("TAS", "f4"), ("vario uncomp", "f4"), ("vario", "f4"), ("vario pressure", "f4"),
             ("speed comp TAS", "f4"), ("speed comp INS", "f4"), ("vario integrator", "f4"), ("wind N", "f4"),
             ("wind E", "f4"), ("wind D", "f4"), ("wind avg N", "f4"), ("wind avg E", "f4"), ("wind avg D", "f4"),
             ("circle mode", "u4"),
             ("q0", "f4"), ("q1", "f4"), ("q2", "f4"), ("q3", "f4"),
             ("roll", "f4"), ("pitch", "f4"), ("yaw", "f4"),
             ("acc vertical", "f4"), ("turn rate", "f4"), ("slip angle", "f4"), ("pitch angle", "f4"),
             ("G_load", "f4"), ("Pressure-altitude", "f4"), ("Air Density", "f4"), ("Magnetic Disturbance", "f4"),
             ("nav acc N", "f4"), ("nav acc E", "f4"), ("nav acc D", "f4"), ("nav ind N", "f4"),
             ("nav ind E", "f4"), ("nav ind D", "f4"), ("nav corr N", "f4"), ("nav corr E", "f4"),
             ("nav corr D", "f4"), ("gyro corr F", "f4"), ("gyro corr R", "f4"), ("gyro corr D", "f4"),
             ("nav acc mag N", "f4"), ("nav acc mag E", "f4"), ("nav acc mag D", "f4"), ("nav ind mag N", "f4"),
             ("nav ind mag E", "f4"), ("nav ind mag D", "f4"),
             ("roll mag", "f4"), ("pitch mag", "f4"), ("yaw mag", "f4"),
             ("q1mag", "f4"), ("q2mag", "f4"), ("q3mag", "f4"), ("q4mag", "f4"),
             ("acc_F", "f4"), ("acc_R", "f4"), ("acc_D", "f4"),
             ("gyro_F", "f4"), ("gyro_R", "f4"), ("gyro_D", "f4"),
             ("SAT-AHRS-Heading", "f4"), ("QFF", "f4"), ("sat fix type f", "f4"),
             ("inst wind N", "f4"), ("inst wind E", "f4"),
             ("avg. Headwind", "f4"), ("avg. crosswind", "f4"),
             ("inst wind corrected N", "f4"), ("inst wind corrected E", "f4"),
             ("speed_comp_1", "f4"),
             ("speed_comp_2", "f4"),
             ("speed_comp_3", "f4"),
             ("cross_acc_correction", "f4"),
             ("vario_wind_N", "f4"),
             ("vario_wind_E", "f4"),
             ("body induction x", "f4"), ("body induction y", "f4"), ("body induction z", "f4"),
             ("body induction error x", "f4"), ("body induction error y", "f4"), ("body induction error z", "f4")
             ]

data_f120 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
             ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("Lowcost acc x", "f4"), ("Lowcost acc y", "f4"),
             ("Lowcost acc z", "f4"), ("Lowcost gyro x", "f4"), ("Lowcost gyro y", "f4"), ("Lowcost gyro z", "f4"),
             ("Lowcost mag x", "f4"), ("Lowcost mag y", "f4"), ("Lowcost mag z", "f4"), ("pitot", "f4"),
             ("static p", "f4"), ("abs p", "f4"), ("temp", "f4"), ("abs sens t", "f4"), ("ubatt", "f4"),
             ("OAT", "f4"), ("Humidity", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"),
             ("vel E", "f4"), ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"),
             ("track GNSS", "f4"), ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"),
             ("rel HDG", "f4"), ("speed acc", "f4"),
             ("Lat", "f8"), ("Long", "f8"),
             ("year", "u1"),
             ("month", "u1"),
             ("day", "u1"),
             ("hour", "u1"),
             ("minute", "u1"),
             ("second", "u1"),
             ("sat number", "u1"),
             ("sat fix type", "u1"),
             ("nanoseconds", "u4"),
             ("geo separation dm", "i2"),
             ("dummy", "u2"),
             ("IAS", "f4"), ("TAS", "f4"), ("vario uncomp", "f4"), ("vario", "f4"), ("vario pressure", "f4"),
             ("speed comp TAS", "f4"), ("speed comp INS", "f4"), ("vario integrator", "f4"), ("wind N", "f4"),
             ("wind E", "f4"), ("wind D", "f4"), ("wind avg N", "f4"), ("wind avg E", "f4"), ("wind avg D", "f4"),
             ("circle mode", "u4"), ("q0", "f4"), ("q1", "f4"), ("q2", "f4"), ("q3", "f4"), ("roll", "f4"),
             ("pitch ", "f4"), ("yaw", "f4"), ("acc vertical", "f4"), ("turn rate", "f4"), ("slip angle", "f4"),
             ("pitch angle", "f4"), ("G_load", "f4"), ("Pressure-altitude", "f4"), ("Air Density", "f4"),
             ("Magnetic Disturbance", "f4"), ("nav acc N", "f4"), ("nav acc E", "f4"), ("nav acc D", "f4"),
             ("nav ind N", "f4"), ("nav ind E", "f4"), ("nav ind D", "f4"), ("nav corr N", "f4"), ("nav corr E", "f4"),
             ("nav corr D", "f4"), ("gyro corr F", "f4"), ("gyro corr R", "f4"), ("gyro corr D", "f4"),
             ("nav acc mag N", "f4"), ("nav acc mag E", "f4"), ("nav acc mag D", "f4"), ("nav ind mag N", "f4"),
             ("nav ind mag E", "f4"), ("nav ind mag D", "f4"), ("roll mag", "f4"), ("pitch mag", "f4"),
             ("yaw mag", "f4"), ("q1mag", "f4"), ("q2mag", "f4"), ("q3mag", "f4"), ("q4mag", "f4"), ("acc_F", "f4"),
             ("acc_R", "f4"), ("acc_D", "f4"), ("gyro_F", "f4"), ("gyro_R", "f4"), ("gyro_D", "f4"),
             ("SAT-AHRS-Heading", "f4"), ("QFF", "f4"), ("sat fix type f", "f4"), ("avg. Headwind", "f4"),
             ("avg. crosswind", "f4"), ("wind_N", "f4"), ("wind_E", "f4"), ("inst wind N", "f4"),
             ("inst wind E", "f4") ]

data_f123 = [("acc x", "f4"), ("acc y", "f4"), ("acc z", "f4"), ("gyro x", "f4"), ("gyro y", "f4"), ("gyro z", "f4"),
             ("mag x", "f4"), ("mag y", "f4"), ("mag z", "f4"), ("Lowcost acc x", "f4"), ("Lowcost acc y", "f4"),
             ("Lowcost acc z", "f4"), ("Lowcost gyro x", "f4"), ("Lowcost gyro y", "f4"), ("Lowcost gyro z", "f4"),
             ("Lowcost mag x", "f4"), ("Lowcost mag y", "f4"), ("Lowcost mag z", "f4"), ("pitot", "f4"),
             ("static p", "f4"), ("abs p", "f4"), ("temp", "f4"), ("abs sens t", "f4"), ("ubatt", "f4"),
             ("OAT", "f4"), ("Humidity", "f4"), ("pos N", "f4"), ("pos E", "f4"), ("pos DWN", "f4"), ("vel N", "f4"),
             ("vel E", "f4"), ("vel DWN", "f4"), ("acc N", "f4"), ("acc E", "f4"), ("acc DWN", "f4"),
             ("track GNSS", "f4"), ("speed GNSS", "f4"), ("relpos N", "f4"), ("relpos E", "f4"), ("relpos D", "f4"),
             ("rel HDG", "f4"), ("speed acc", "f4"),
             ("Lat", "f8"), ("Long", "f8"),
             ("year", "u1"),
             ("month", "u1"),
             ("day", "u1"),
             ("hour", "u1"),
             ("minute", "u1"),
             ("second", "u1"),
             ("sat number", "u1"),
             ("sat fix type", "u1"),
             ("nanoseconds", "u4"),
             ("geo separation dm", "i2"),
             ("dummy", "u2"),
             ("IAS", "f4"), ("TAS", "f4"), ("vario uncomp", "f4"), ("vario", "f4"), ("vario pressure", "f4"),
             ("speed comp TAS", "f4"), ("speed comp INS", "f4"), ("vario integrator", "f4"), ("wind N", "f4"),
             ("wind E", "f4"), ("wind D", "f4"), ("wind avg N", "f4"), ("wind avg E", "f4"), ("wind avg D", "f4"),
             ("circle mode", "u4"), ("q0", "f4"), ("q1", "f4"), ("q2", "f4"), ("q3", "f4"), ("roll", "f4"),
             ("pitch", "f4"), ("yaw", "f4"), ("acc vertical", "f4"), ("turn rate", "f4"), ("slip angle", "f4"),
             ("pitch angle", "f4"), ("G_load", "f4"), ("Pressure-altitude", "f4"), ("Air Density", "f4"),
             ("Magnetic Disturbance", "f4"), ("nav acc N", "f4"), ("nav acc E", "f4"), ("nav acc D", "f4"),
             ("nav ind N", "f4"), ("nav ind E", "f4"), ("nav ind D", "f4"),
             ("nav corr N", "f4"), ("nav corr E", "f4"), ("nav corr D", "f4"),
             ("gyro corr F", "f4"), ("gyro corr R", "f4"), ("gyro corr D", "f4"), ("nav acc mag N", "f4"),
             ("nav acc mag E", "f4"), ("nav acc mag D", "f4"), ("nav ind mag N", "f4"), ("nav ind mag E", "f4"),
             ("nav ind mag D", "f4"), ("roll mag", "f4"), ("pitch mag", "f4"), ("yaw mag", "f4"),
             ("q1mag", "f4"), ("q2mag", "f4"), ("q3mag", "f4"), ("q4mag", "f4"),
             ("acc_F", "f4"), ("acc_R", "f4"), ("acc_D", "f4"),
             ("gyro_F", "f4"), ("gyro_R", "f4"), ("gyro_D", "f4"), ("SAT-AHRS-Heading", "f4"),
             ("QFF", "f4"), ("sat fix type f", "f4"), ("avg. Headwind", "f4"), ("avg. crosswind", "f4"),
             ("wind_N", "f4"), ("wind_E", "f4"), ("inst wind N", "f4"), ("inst wind E", "f4"), ("speed_comp_1", "f4"),
             ("speed_comp_2", "f4"), ("speed_comp_3", "f4")]

raw_data_formats = {'.f37':data_f37, '.f50':data_f50}
processed_data_formats = {'.f110':data_f110, '.f114':data_f114, '.f119':data_f119, '.f120':data_f120, '.f123':data_f123}

# Supported Lib Versions and corresponding output formats for lib
lib_versions = {'v0.1.0':'.f114', 'v0.1.1':'.f114', 'v0.1.2':'.f119'}

# Check if files has larus format file ending
def check_if_larus_file(file):
    for file_format in processed_data_formats.keys():
        if file.endswith(file_format):
            return True

    for file_format in raw_data_formats.keys():
        if file.endswith(file_format):
            return True
    return False # This is not a known Larus File format

# Class to load a binary larus logfile into a pandas dataframe.  Converts raw data into processed data.
class Larus2Df:
    df = None
    file = None
    datatype = None
    dataformat = None
    def __init__(self, file, recalc=True, version='v0.1.2'):

        if version in lib_versions:
            lib_output_format = lib_versions[version]
        else:
            raise Exception("Lib version not supported")

        for file_format in processed_data_formats.keys():
            if file.endswith(file_format):
                self.datatype = 'PROCESSED_DATA'
                self.dataformat = processed_data_formats[file_format]

        for file_format in raw_data_formats.keys():
            if file.endswith(file_format):
                self.datatype = 'RAW_DATA'
                self.dataformat = raw_data_formats[file_format]

        if self.datatype == 'RAW_DATA':
            # Process raw data into processed data
            if file.endswith('.f37'):
                dataformat = data_f37
                eeprom_file_path = str(file).replace('.f37', '.EEPROM')
                result_file = str(file) + lib_output_format

                if not os.path.isfile(file):
                    raise Exception("File {} does not exist".format(file))

                if not os.path.isfile(eeprom_file_path):
                    raise Exception("There must be a {} file".format(eeprom_file_path))

                if recalc is False and os.path.isfile(result_file):
                    pass # Just use the existing result file and read it into the dataframe

                else:
                    if "linux" in sys.platform:
                        print("Executing: {}".format(version))
                        subprocess.call(["{}/_internal/data_analyzer_lib_linux_tag_{}".format(Path(__file__).parent.absolute(), version), file])
                    elif "win" in sys.platform:
                        subprocess.call(["{}/_internal/data_analyzer_lib_windows_tag_{}.exe".format(Path(__file__).parent.absolute(), version), file])
                    else:
                        raise Exception("Platform not supported: {}".format(sys.platform))

                file = result_file
                self.dataformat = processed_data_formats[lib_versions[version]]
                self.datatype = 'PROCESSED_DATA'

            else:
                raise Exception("Raw data format {} not implemented yet.".format(self.dataformat))

        # Convert larus dataformat into a pandas dataframe
        dt = np.dtype(self.dataformat)
        data = np.fromfile(file, dtype=dt, sep="")
        self.df = pd.DataFrame(data)

    def get_df(self):
        return self.df


if __name__ == "__main__":
    df = Larus2Df('240520_091630.f37')
    print(df.get_df().columns)