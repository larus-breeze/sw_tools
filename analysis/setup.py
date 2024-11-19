import sys
from cx_Freeze import setup, Executable

# Create virtual python environment via python -m virtualenv myvenv
# Activate myvenv  with source myvenv/bin/activate
# Install only required depencendies with pip install -r requirements.txt
# pip install cx_Freeze
# Build Binary with: python setup.py build

#sys.setrecursionlimit(3000)   # Might be required if to many dependencies

# Dependencies are automatically detected, but may need to be adjusted.
build_options = {
    'include_files': ['linux_analyzer_commit6598331']
}

setup(
name= "Analysis Plots",
version= "0.1",
description="Plot *.f37 Data",
options={"build_exe": build_options},
executables=[Executable("analysis_plots.py", base=None)],
)