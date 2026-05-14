#!/user/bin/env python3
import numpy as np
import pandas as pd
import os
import sys
import subprocess
from datetime import date
from pathlib import Path

# Format with datatypes in tuple format
# (identifier, data type, description)
# dtypes  int32_t  = i4,  int8_t = i1, uint16_t = u2,  f4 = float, f8 = double

# Raw data sensor files without low-cost ins sensor
data_f37 = [
    ('acceleration_x', 'f4', 'acceleration body frame front'),
    ('acceleration_y', 'f4', 'acceleration body frame right'),
    ('acceleration_z', 'f4', 'acceleration body frame down'),
    ('rotation_x', 'f4', 'rotation body frame roll'),
    ('rotation_y', 'f4', 'rotation body frame pitch'),
    ('rotation_z', 'f4', 'rotation body frame yaw'),
    ('magnetic_x', 'f4', 'magnetic body frame front'),
    ('magnetic_y', 'f4', 'magnetic body frame right'),
    ('magnetic_z', 'f4', 'magnetic body frame down'),
    ('pitot', 'f4', 'pitot difference pressure'),
    ('static_pressure', 'f4', 'static absolute pressure'),
    ('temperature_pcb', 'f4', 'pcb sensor temperature'),
    ('voltage', 'f4', 'sensor supply voltage'),
    ('position_north', 'f4', 'position north'),
    ('position_east', 'f4', 'position east'),
    ('position_down', 'f4', 'position down'),
    ('velocity_north', 'f4', 'velocity world frame north'),
    ('velocity_east', 'f4', 'velocity world frame east'),
    ('velocity_down', 'f4', 'velocity world frame down'),
    ('acceleration_gnss_north', 'f4', 'gnss acceleration world frame north'),
    ('acceleration_gnss_east', 'f4', 'gnss acceleration world frame east'),
    ('acceleration_gnss_down', 'f4', 'not available always set to 0'),
    ('track_ground', 'f4', 'gnss based ground track'),
    ('speed_ground', 'f4', 'gnss based ground speed'),
    ('relpos_north', 'f4', 'relative position of 2nd GNSS antenna north'),
    ('relpos_east', 'f4', 'relative position of 2nd GNSS antenna east'),
    ('relpos_down', 'f4', 'relative position of 2nd GNSS antenna down'),
    ('rel_heading', 'f4', 'relative heading between GNSS antennas'),
    ('speed_accuracy', 'f4', 'GNSS speed accuracy'),
    ('latitude', 'f8', 'GNSS latitude'),
    ('longitude', 'f8', 'GNSS longitude'),
    ('year', 'u1', 'GNSS timestamp'),
    ('month', 'u1', 'GNSS timestamp'),
    ('day', 'u1', 'GNSS timestamp'),
    ('hour', 'u1', 'GNSS timestamp'),
    ('minute', 'u1', 'GNSS timestamp'),
    ('second', 'u1', 'GNSS timestamp'),
    ('sat_count', 'u1', 'Number of currently used GNSS satellites'),
    ('sat_fix_type', 'u1', 'Type of GNSS satfix (None, 2D, 3D, RTK-Heading)'),
    ('nanoseconds', 'i4', 'GNSS timestamp'),
    ('geo_separation', 'i2', 'GNSS geoid separation'),
    ('dummy', 'u2', 'reserved'),
  ]

processed_data_common = [
    ('ias', 'f4', 'indicated airspeed'),
    ('tas', 'f4', 'true airspeed'),
    ('vario_uncompensated', 'f4', 'uncompensated variometer signal'),
    ('vario', 'f4', 'speed compensated total energy variometer signal'),
    ('vario_pressure', 'f4', 'pneumatic variometer value'),
    ('speed_compensation_tas', 'f4', 'speed compensation true airspeed signal'),
    ('speed_compensation_ins', 'f4', 'speed compensation inertial navigation system signal'),
    ('vario_average', 'f4', 'averaged mean vario signal'),
    ('wind_north', 'f4', 'instant wind in north direction'),
    ('wind_east', 'f4', 'instant wind in east direction'),
    ('wind_down', 'f4', 'instant wind in down direction'),
    ('wind_average_north', 'f4', 'average wind in north direction'),
    ('wind_average_east', 'f4', 'average wind in east direction'),
    ('wind_average_down', 'f4', 'average wind in down direction'),
    ('circle_mode', 'u4', 'sensors operating mode: circling, straight or in between'),
    ('q0', 'f4', 'quaternion component 0'),
    ('q1', 'f4', 'quaternion component 1'),
    ('q2', 'f4', 'quaternion component 2'),
    ('q3', 'f4', 'quaternion component 3'),
    ('roll_angle', 'f4', 'roll angle'),
    ('pitch_angle', 'f4', 'pitch angle'),
    ('yaw_angle', 'f4', 'heading angle'),
    ('effective_acceleration_vertical', 'f4', 'vertical effective acceleration (gravitation removed)'),
    ('turn_rate', 'f4', 'turn yaw rotation rate'),
    ('slip_angle', 'f4', 'slip angle'),
    ('apparent_pitch_angle', 'f4', 'apparent pitch angle'),
    ('g_load', 'f4', 'total g load'),
    ('pressure_altitude', 'f4', 'pressure altitude above standard atmosphere'),
    ('air_density', 'f4', 'estimated air density'),
    ('magnetic_disturbance', 'f4', 'magnetic disturbance after applied calibration factors'),
    ('acceleration_north', 'f4', 'inertial acceleration world frame north'),
    ('acceleration_east', 'f4', 'inertial acceleration world frame east'),
    ('acceleration_down', 'f4', 'inertial acceleration world frame down'),
    ('magnetic_north', 'f4', 'magnetic induction in world frame north'),
    ('magnetic_east', 'f4', 'magnetic induction in world frame east'),
    ('magnetic_down', 'f4', 'magnetic induction in world frame down'),
    ('dev_nav_correction_north', 'f4', 'development debug'),
    ('dev_nav_correction_east', 'f4', 'development debug'),
    ('dev_nav_correction_down', 'f4', 'development debug'),
    ('dev_gyro_correction_front', 'f4', 'development debug'),
    ('dev_gyro_correction_right', 'f4', 'development debug'),
    ('dev_gyro_correction_down', 'f4', 'development debug'),
    ('dev_nav_acc_mag_north', 'f4', 'development debug'),
    ('dev_nav_acc_mag_east', 'f4', 'development debug'),
    ('dev_nav_acc_mag_down', 'f4', 'development debug'),
    ('dev_nav_ind_mag_north', 'f4', 'development debug'),
    ('dev_nav_ind_mag_east', 'f4', 'development debug'),
    ('dev_nav_ind_mag_down', 'f4', 'development debug'),
    ('dev_roll_mag', 'f4', 'development debug'),
    ('dev_pitch_mag', 'f4', 'development debug'),
    ('dev_yaw_mag', 'f4', 'development debug'),
    ('dev_q1_mag', 'f4', 'development debug'),
    ('dev_q2_mag', 'f4', 'development debug'),
    ('dev_q3_mag', 'f4', 'development debug'),
    ('dev_q4_mag', 'f4', 'development debug'),
    ('dev_acc_front', 'f4', 'development debug'),
    ('dev_acc_right', 'f4', 'development debug'),
    ('dev_acc_down', 'f4', 'development debug'),
    ('dev_gyro_front', 'f4', 'development debug'),
    ('dev_gyro_right', 'f4', 'development debug'),
    ('dev_gyro_down', 'f4', 'development debug'),
    ('dev_sat_ahrs_heading', 'f4', 'development debug'),
    ('dev_qff', 'f4', 'development debug'),
    ('dev_sat_fix_type_f', 'f4', 'development debug'),
    ('dev_inst_wind_north', 'f4', 'development debug'),
    ('dev_inst_wind_east', 'f4', 'development debug'),
    ('dev_average_headwind', 'f4', 'development debug'),
    ('dev_average_crosswind', 'f4', 'development debug'),
    ('dev_instance_wind_corrected_north', 'f4', 'development debug'),
    ('dev_instance_wind_corrected_east', 'f4', 'development debug'),
    ('dev_speed_comp_1', 'f4', 'development debug'),
    ('dev_speed_comp_2', 'f4', 'development debug'),
    ('dev_speed_comp_3', 'f4', 'development debug')]

# Processed data with vario, wind and ahrs values, ...
data_f114 = data_f37 + processed_data_common + [
    ("speed_comp_4", "f4", "development debug"),
    ("dev_cross_acc_correction", "f4"),
    ("dev_vario_wind_north", "f4"),
    ("dev_vario_wind_east", "f4")]

data_f119 = data_f37 + processed_data_common + [
    ("dev_cross_acc_correction", "f4", "development debug"),
    ("dev_vario_wind_north", "f4", "development debug"),
    ("dev_vario_wind_east", "f4", "development debug"),
    ("dev_body_induction_x", "f4", "development debug"),
    ("dev_body_induction_y", "f4", "development debug"),
    ("dev_body_induction_z", "f4", "development debug"),
    ("dev_body_induction_error_x", "f4", "development debug"),
    ("dev_body_induction_error_y", "f4", "development debug"),
    ("dev_body_induction_error_z", "f4", "development debug")]

#f120 files exist already in legacy format versions.
data_f120_no_low_cost_imu = data_f37 + processed_data_common +[
    ("dev_cross_acc_correction", "f4", "development debug"),
    ("dev_vario_wind_north", "f4", "development debug"),
    ("dev_vario_wind_east", "f4", "development debug"),
    ("dev_body_induction_x", "f4", "development debug"),
    ("dev_body_induction_y", "f4", "development debug"),
    ("dev_body_induction_z", "f4", "development debug"),
    ("dev_body_induction_error_x", "f4", "development debug"),
    ("dev_body_induction_error_y", "f4", "development debug"),
    ("dev_body_induction_error_z", "f4", "development debug"),
    ("dev_gyro_correction_power" ,"f4", "development debug")]

# Raw data sensor files with low-cost ins sensor. Only required for the first legacy larus sensor hardware version.
data_f50 = [
    ('acceleration_x', 'f4', 'acceleration body frame front'),
    ('acceleration_y', 'f4', 'acceleration body frame right'),
    ('acceleration_z', 'f4', 'acceleration body frame down'),
    ('rotation_x', 'f4', 'rotation body frame roll'),
    ('rotation_y', 'f4', 'rotation body frame pitch'),
    ('rotation_z', 'f4', 'rotation body frame yaw'),
    ('magnetic_x', 'f4', 'magnetic body frame front'),
    ('magnetic_y', 'f4', 'magnetic body frame right'),
    ('magnetic_z', 'f4', 'magnetic body frame down'),
    ('low_cost_acceleration_x', 'f4', 'not known'),
    ('low_cost_acceleration_y', 'f4', 'not known'),
    ('low_cost_acceleration_z', 'f4', 'not known'),
    ('low_cost_gyro_x', 'f4', 'not known'),
    ('low_cost_gyro_y', 'f4', 'not known'),
    ('low_cost_gyro_z', 'f4', 'not known'),
    ('low_cost_mag_x', 'f4', 'not known'),
    ('low_cost_mag_y', 'f4', 'not known'),
    ('low_cost_mag_z', 'f4', 'not known'),
    ('pitot', 'f4', 'pitot difference pressure'),
    ('static_pressure', 'f4', 'static absolute pressure'),
    ('absolute_pressure', 'f4', 'not known'),
    ('temperature_pcb', 'f4', 'pcb sensor temperature'),
    ('absolute_pressure_temperature', 'f4', 'not known'),
    ('voltage', 'f4', 'sensor supply voltage'),
    ('outside_air_temperature', 'f4', 'not known'),
    ('humidity', 'f4', 'not known'),
    ('position_north', 'f4', 'position north'),
    ('position_east', 'f4', 'position east'),
    ('position_down', 'f4', 'position down'),
    ('velocity_north', 'f4', 'velocity world frame north'),
    ('velocity_east', 'f4', 'velocity world frame east'),
    ('velocity_down', 'f4', 'velocity world frame down'),
    ('acceleration_gnss_north', 'f4', 'gnss acceleration world frame north'),
    ('acceleration_gnss_east', 'f4', 'gnss acceleration world frame east'),
    ('acceleration_gnss_down', 'f4', 'not available always set to 0'),
    ('track_ground', 'f4', 'gnss based ground track'),
    ('speed_ground', 'f4', 'gnss based ground speed'),
    ('relpos_north', 'f4', 'relative position of 2nd GNSS antenna north'),
    ('relpos_east', 'f4', 'relative position of 2nd GNSS antenna east'),
    ('relpos_down', 'f4', 'relative position of 2nd GNSS antenna down'),
    ('rel_heading', 'f4', 'relative heading between GNSS antennas'),
    ('speed_accuracy', 'f4', 'GNSS speed accuracy'),
    ('latitude', 'f8', 'GNSS latitude'),
    ('longitude', 'f8', 'GNSS longitude'),
    ('year', 'u1', 'GNSS timestamp'),
    ('month', 'u1', 'GNSS timestamp'),
    ('day', 'u1', 'GNSS timestamp'),
    ('hour', 'u1', 'GNSS timestamp'),
    ('minute', 'u1', 'GNSS timestamp'),
    ('second', 'u1', 'GNSS timestamp'),
    ('sat_count', 'u1', 'Number of currently used GNSS satellites'),
    ('sat_fix_type', 'u1', 'Type of GNSS satfix (None, 2D, 3D, RTK-Heading)'),
    ('nanoseconds', 'f4', 'GNSS timestamp'),
    ('geo_separation', 'f4', 'GNSS geoid separation')]

data_f120 = data_f50 + [
    ('dummy', 'u2', 'reserved'),
    ('ias', 'f4', 'indicated airspeed'),
    ('tas', 'f4', 'true airspeed'),
    ('vario_uncompensated', 'f4', 'uncompensated variometer signal'),
    ('vario', 'f4', 'speed compensated total energy variometer signal'),
    ('vario_pressure', 'f4', 'pneumatic variometer value'),
    ('speed_compensation_tas', 'f4', 'speed compensation true airspeed signal'),
    ('speed_compensation_ins', 'f4', 'speed compensation inertial navigation system signal'),
    ('vario_average', 'f4', 'averaged mean vario signal'),
    ('wind_north', 'f4', 'instant wind in north direction'),
    ('wind_east', 'f4', 'instant wind in east direction'),
    ('wind_down', 'f4', 'instant wind in down direction'),
    ('wind_average_north', 'f4', 'average wind in north direction'),
    ('wind_average_east', 'f4', 'average wind in east direction'),
    ('wind_average_down', 'f4', 'average wind in down direction'),
    ('circle_mode', 'u4', 'sensors operating mode: circling, straight or in between'),
    ('q0', 'f4', 'quaternion component 0'),
    ('q1', 'f4', 'quaternion component 1'),
    ('q2', 'f4', 'quaternion component 2'),
    ('q3', 'f4', 'quaternion component 3'),
    ('roll_angle', 'f4', 'roll angle'),
    ('pitch ', 'f4', 'not known'),
    ('yaw_angle', 'f4', 'heading angle'),
    ('effective_acceleration_vertical', 'f4', 'vertical effective acceleration (gravitation removed)'),
    ('turn_rate', 'f4', 'turn yaw rotation rate'),
    ('slip_angle', 'f4', 'slip angle'),
    ('apparent_pitch_angle', 'f4', 'apparent pitch angle'),
    ('g_load', 'f4', 'total g load'),
    ('pressure_altitude', 'f4', 'pressure altitude above standard atmosphere'),
    ('air_density', 'f4', 'estimated air density'),
    ('magnetic_disturbance', 'f4', 'magnetic disturbance after applied calibration factors'),
    ('acceleration_north', 'f4', 'inertial acceleration world frame north'),
    ('acceleration_east', 'f4', 'inertial acceleration world frame east'),
    ('acceleration_down', 'f4', 'inertial acceleration world frame down'),
    ('magnetic_north', 'f4', 'magnetic induction in world frame north'),
    ('magnetic_east', 'f4', 'magnetic induction in world frame east'),
    ('magnetic_down', 'f4', 'magnetic induction in world frame down'),
    ('dev_nav_correction_north', 'f4', 'development debug'),
    ('dev_nav_correction_east', 'f4', 'development debug'),
    ('dev_nav_correction_down', 'f4', 'development debug'),
    ('dev_gyro_correction_front', 'f4', 'development debug'),
    ('dev_gyro_correction_right', 'f4', 'development debug'),
    ('dev_gyro_correction_down', 'f4', 'development debug'),
    ('dev_nav_acc_mag_north', 'f4', 'development debug'),
    ('dev_nav_acc_mag_east', 'f4', 'development debug'),
    ('dev_nav_acc_mag_down', 'f4', 'development debug'),
    ('dev_nav_ind_mag_north', 'f4', 'development debug'),
    ('dev_nav_ind_mag_east', 'f4', 'development debug'),
    ('dev_nav_ind_mag_down', 'f4', 'development debug'),
    ('dev_roll_mag', 'f4', 'development debug'),
    ('dev_pitch_mag', 'f4', 'development debug'),
    ('dev_yaw_mag', 'f4', 'development debug'),
    ('dev_q1_mag', 'f4', 'development debug'),
    ('dev_q2_mag', 'f4', 'development debug'),
    ('dev_q3_mag', 'f4', 'development debug'),
    ('dev_q4_mag', 'f4', 'development debug'),
    ('dev_acc_front', 'f4', 'development debug'),
    ('dev_acc_right', 'f4', 'development debug'),
    ('dev_acc_down', 'f4', 'development debug'),
    ('dev_gyro_front', 'f4', 'development debug'),
    ('dev_gyro_right', 'f4', 'development debug'),
    ('dev_gyro_down', 'f4', 'development debug'),
    ('dev_sat_ahrs_heading', 'f4', 'development debug'),
    ('dev_qff', 'f4', 'development debug'),
    ('dev_sat_fix_type_f', 'f4', 'development debug'),
    ('dev_average_headwind', 'f4', 'development debug'),
    ('dev_average_crosswind', 'f4', 'development debug'),
    ('dev_vario_wind_north', 'f4', 'development debug'),
    ('dev_vario_wind_east', 'f4', 'development debug'),
    ('dev_inst_wind_north', 'f4', 'development debug'),
    ('dev_inst_wind_east', 'f4', 'development debug')]

data_f123 = data_f50 + [
    ('dummy', 'u2', 'reserved'),
    ('ias', 'f4', 'indicated airspeed'),
    ('tas', 'f4', 'true airspeed'),
    ('vario_uncompensated', 'f4', 'uncompensated variometer signal'),
    ('vario', 'f4', 'speed compensated total energy variometer signal'),
    ('vario_pressure', 'f4', 'pneumatic variometer value'),
    ('speed_compensation_tas', 'f4', 'speed compensation true airspeed signal'),
    ('speed_compensation_ins', 'f4', 'speed compensation inertial navigation system signal'),
    ('vario_average', 'f4', 'averaged mean vario signal'),
    ('wind_north', 'f4', 'instant wind in north direction'),
    ('wind_east', 'f4', 'instant wind in east direction'),
    ('wind_down', 'f4', 'instant wind in down direction'),
    ('wind_average_north', 'f4', 'average wind in north direction'),
    ('wind_average_east', 'f4', 'average wind in east direction'),
    ('wind_average_down', 'f4', 'average wind in down direction'),
    ('circle_mode', 'u4', 'sensors operating mode: circling, straight or in between'),
    ('q0', 'f4', 'quaternion component 0'),
    ('q1', 'f4', 'quaternion component 1'),
    ('q2', 'f4', 'quaternion component 2'),
    ('q3', 'f4', 'quaternion component 3'),
    ('roll_angle', 'f4', 'roll angle'),
    ('pitch_angle', 'f4', 'pitch angle'),
    ('yaw_angle', 'f4', 'heading angle'),
    ('effective_acceleration_vertical', 'f4', 'vertical effective acceleration (gravitation removed)'),
    ('turn_rate', 'f4', 'turn yaw rotation rate'),
    ('slip_angle', 'f4', 'slip angle'),
    ('apparent_pitch_angle', 'f4', 'apparent pitch angle'),
    ('g_load', 'f4', 'total g load'),
    ('pressure_altitude', 'f4', 'pressure altitude above standard atmosphere'),
    ('air_density', 'f4', 'estimated air density'),
    ('magnetic_disturbance', 'f4', 'magnetic disturbance after applied calibration factors'),
    ('acceleration_north', 'f4', 'inertial acceleration world frame north'),
    ('acceleration_east', 'f4', 'inertial acceleration world frame east'),
    ('acceleration_down', 'f4', 'inertial acceleration world frame down'),
    ('magnetic_north', 'f4', 'magnetic induction in world frame north'),
    ('magnetic_east', 'f4', 'magnetic induction in world frame east'),
    ('magnetic_down', 'f4', 'magnetic induction in world frame down'),
    ('dev_nav_correction_north', 'f4', 'development debug'),
    ('dev_nav_correction_east', 'f4', 'development debug'),
    ('dev_nav_correction_down', 'f4', 'development debug'),
    ('dev_gyro_correction_front', 'f4', 'development debug'),
    ('dev_gyro_correction_right', 'f4', 'development debug'),
    ('dev_gyro_correction_down', 'f4', 'development debug'),
    ('dev_nav_acc_mag_north', 'f4', 'development debug'),
    ('dev_nav_acc_mag_east', 'f4', 'development debug'),
    ('dev_nav_acc_mag_down', 'f4', 'development debug'),
    ('dev_nav_ind_mag_north', 'f4', 'development debug'),
    ('dev_nav_ind_mag_east', 'f4', 'development debug'),
    ('dev_nav_ind_mag_down', 'f4', 'development debug'),
    ('dev_roll_mag', 'f4', 'development debug'),
    ('dev_pitch_mag', 'f4', 'development debug'),
    ('dev_yaw_mag', 'f4', 'development debug'),
    ('dev_q1_mag', 'f4', 'development debug'),
    ('dev_q2_mag', 'f4', 'development debug'),
    ('dev_q3_mag', 'f4', 'development debug'),
    ('dev_q4_mag', 'f4', 'development debug'),
    ('dev_acc_front', 'f4', 'development debug'),
    ('dev_acc_right', 'f4', 'development debug'),
    ('dev_acc_down', 'f4', 'development debug'),
    ('dev_gyro_front', 'f4', 'development debug'),
    ('dev_gyro_right', 'f4', 'development debug'),
    ('dev_gyro_down', 'f4', 'development debug'),
    ('dev_sat_ahrs_heading', 'f4', 'development debug'),
    ('dev_qff', 'f4', 'development debug'),
    ('dev_sat_fix_type_f', 'f4', 'development debug'),
    ('dev_average_headwind', 'f4', 'development debug'),
    ('dev_average_crosswind', 'f4', 'development debug'),
    ('dev_vario_wind_north', 'f4', 'development debug'),
    ('dev_vario_wind_east', 'f4', 'development debug'),
    ('dev_inst_wind_north', 'f4', 'development debug'),
    ('dev_inst_wind_east', 'f4', 'development debug'),
    ('dev_speed_comp_1', 'f4', 'development debug'),
    ('dev_speed_comp_2', 'f4', 'development debug'),
    ('dev_speed_comp_3', 'f4', 'development debug')]

raw_data_formats = {'.f37':data_f37, '.f50':data_f50}
processed_data_formats = {'.f114':data_f114, '.f119':data_f119, '.f120':data_f120_no_low_cost_imu, '.f123':data_f123}

# Supported Lib Versions and corresponding output formats for lib
lib_versions = {'v0.1.0':'.f114',
                'v0.1.1':'.f114',
                'v0.1.2':'.f119',
                'v0.2.1':'.f120',
                'v0.2.2':'.f120',
                'v0.2.3':'.f120',
                'v0.3.0':'.f120',
                'xyz':'.f120'
                }

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
    def __init__(self, file, recalc=True, version='v0.3.0'):

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
        numpy_dataformat = []
        for entry in self.dataformat:
            numpy_dataformat.append((entry[0], entry[1]))

        dt = np.dtype(numpy_dataformat)
        data = np.fromfile(file, dtype=dt, sep="")
        self.df = pd.DataFrame(data)

        try:
            # Simplified file validation which checks if the first and last row contains a valid date value.
            count_row = self.df.shape[0]  # Gives number of rows
            date(self.df['year'][0], self.df['month'][0], self.df['day'][0])
            date(self.df['year'][count_row-1], self.df['month'][count_row-1], self.df['day'][count_row-1])

        except Exception as e:
            print(f'This is not a supported file format {e}')
            raise Exception(f'This is not a supported file format{e}')

    def get_df(self):
        return self.df


if __name__ == "__main__":
    df = Larus2Df('240520_091630.f37')
    print(df.get_df().columns)


