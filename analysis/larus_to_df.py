#!/user/bin/env python3
from dataformats import *
import numpy as np
import pandas as pd
import os
import sys
import subprocess

# Check if files has larus format file ending
def check_if_larus_file(file):
    for file_format in processed_data_formats:
        if file.endswith(file_format[0]):
            return True

    for file_format in raw_data_formats:
        if file.endswith(file_format[0]):
            return True
    return False # This is not a known Larus File format

# Class to load a binary larus logfile into a pandas dataframe.  Converts raw data into processed data.
class Larus2Df:
    df = None
    file = None
    datatype = None
    dataformat = None
    def __init__(self, file):
        for file_format in processed_data_formats:
            if file.endswith(file_format[0]):
                self.datatype = 'PROCESSED_DATA'
                self.dataformat = file_format[1]

        for file_format in raw_data_formats:
            if file.endswith(file_format[0]):
                self.datatype = 'RAW_DATA'
                self.dataformat = file_format[1]

        if self.datatype == 'RAW_DATA':
            # Process raw data into processed data
            if file.endswith('.f37'):
                dataformat = data_f37
                eeprom_file_path = str(file).replace('.f37', '.EEPROM')
                result_file = str(file) + '.f114'

                if not os.path.isfile(eeprom_file_path):
                    raise Exception("There must be a {} file".format(eeprom_file_path))

                if not os.path.isfile(result_file):
                    if "linux" in sys.platform:
                        subprocess.call(["{}/data_analyzer_commit_6598331_linux".format(os.getcwd()), file])
                    elif "win" in sys.platform:
                        subprocess.call(["{}/data_analyzer_commit_6598331_windows.exe".format(os.getcwd()), file])

                file = result_file
                self.dataformat = data_f114
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