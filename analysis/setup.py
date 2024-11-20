# TODO: remove file if really not required and move      pyinstaller  documentation to readme.md
#import sys
#from cx_Freeze import setup, Executable
#import matplotlib
# Cd into analysis directory
# Create virtual python environment via python3 -m virtualenv myvenv
# Activate myvenv  with source myvenv/bin/activate
# Install only required depencendies with pip install -r requirements.txt
# pip install cx_Freeze
# Build Binary with: python setup.py build

# pip install pyinstaller
# instead of cx_Freeze just use pyinstaller analysis_plots.py --hidden-import='PIL._tkinter_finder'

#sys.setrecursionlimit(3000)   # Might be required if to many dependencies
#base = None
#if sys.platform == "win32":
#    base = "Win32GUI"


# Dependencies are automatically detected, but may need to be adjusted.
#build_options = {
#    'include_files': ['data_analyzer_commit_6598331_linux', 'data_analyzer_commit_6598331_windows.exe',  matplotlib.get_data_path()]
#}

#setup(
#name= "Analysis Plots",
#version= "0.1",
#description="Plot *.f37 Data",
#options={"build_exe": build_options},
#executables=[Executable("analysis_plots.py", base=base)],
#)