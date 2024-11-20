#!/user/bin/env python3
from dataformats import *
import numpy as np
import pandas as pd
import os
import sys

# Class to import a Larus data logfile into a pandas dataframe.
class ProcessLarus2Df:
    df = None
    file = None
    def __init__(self, file):
        self.file = file
        if file.endswith('.f37'):
            dataformat = data_f37
            eeprom_file_path = str(file).replace('.f37', '.EEPROM')
            result_file = str(file) + '.f114'

            if not os.path.isfile(eeprom_file_path):
                raise Exception("There must be a {} file".format(eeprom_file_path))

            if not os.path.isfile(result_file):
                if "linux" in sys.platform:
                    os.system("{}/data_analyzer_commit_6598331_linux {}".format(os.getcwd(), file))
                elif "win" in sys.platform:
                    os.system("{}/data_analyzer_commit_6598331_windows.exe {}".format(os.getcwd(), file))
            file = result_file
            dataformat = data_f114

        elif file.endswith('.f50'):
            dataformat = data_f50
        elif file.endswith('.f123'):
            dataformat = data_f123
        elif file.endswith('.f120'):
            dataformat = data_f120
        elif file.endswith('.f110'):
            dataformat = data_f110
        elif file.endswith('.f114'):
            dataformat = data_f114
        else:
            print('Format, not supported')
            exit()

        # Create a pandas dataframe
        dt = np.dtype(dataformat)
        data = np.fromfile(file, dtype=dt, sep="")
        self.df = pd.DataFrame(data)

    def get_df(self):
        return self.df


if __name__ == "__main__":
    df = ProcessLarus2Df('240520_091630.f37')
    print(df.get_df().columns)