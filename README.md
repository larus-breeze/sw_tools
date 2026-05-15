# Software tools for larus devices for development and verification

## Analysis
Python scripts to plot the from the sensor recorded and via the Sensor Data Analyzer converted raw data logfiles

## Vonvert f37 to lrsx
Tool to convert the old logfile format (*.f37 and *.EEPROM) into the new flexible file format. [LRSX](https://github.com/larus-breeze/doc_larus/blob/master/documentation/LRSX_log_file_format.md)

## Flight Player
A Python QT based software to replay the via the Sensor Data Analyzer generated logfiles. This tool can generate CAN and NMEA Traffic to test the HMI and Frontend Solutions.

## Larus Data
A base class to load Larus Logfiles of Type *.f37 or *.lrsx into a pandas dataframe using a precompiled sensor data analyzer binary. (Supports Windows and Linux based systems)

## Sensor Data Analyzer
Software-In-The-Loop Simulator for the Larus Glider Flight Sensor. Details here: [README](sensor_data_analyzer/README.md)
Supported Dataformats can be found here: [Formats](analysis/dataformats.py)

## SW Update
A solution for firmware updates of the larus devices. 
