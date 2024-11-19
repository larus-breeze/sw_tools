#!/user/bin/env python3
import os
import plotly
from tkinter import filedialog

file_path = filedialog.askopenfilename(title="Select a File", filetypes=[("Larus Binary files", "*.f37")])
eeprom_file_path = str(file_path).replace('.f37', '.EEPROM')

if not os.path.isfile(eeprom_file_path):
    raise Exception("There must be a {} file".format(eeprom_file_path))

os.system("{}/data_analyzer_commit_6598331_linux {}".format(os.getcwd(), file_path))
result_file = str(file_path) + '.f114'
print('Conversion done.')
print('Colums: {}'.format())

from plot_mag import plot_mag


print('Plotting...')
plot_mag(result_file)


print('Exiting')

#TODO: create dataframe once and plot two or thee diagrams using plotly.

