# Create analysis_plots.py binaries for Linux and Windows
- Cd into analysis directory
- Create virtual python environment via ```python3 -m virtualenv myvenv```
- Activate myvenv with 
  - Linux: ```source myvenv/bin/activate``` 
  - Windows: ```.\myvenv/Scripts/activate.ps1```
- Install dependencies with ```pip install -r requirements.txt```
- Create binary with pyinstaller larus_analyzer.py

# Create updates sensor_data_analyzer binaries
The files data_analyzer_lib_[linux|windos.exe]_tag_vx.y.z are binaries from the 
sensor_data_analyzer for windows and linux for the corresponding tag. This binary calculates 
the AHRS, Vario and Wind, ... values from the raw larus measurement data *.f37 files. It is the library:
https://github.com/larus-breeze/sw_algorithms_lib/ in a software loop.

## Wishlist / TODOs
- Provide Functionality to only export a timeframe via csv
- Sign application so that less false positive virus warnings occur within windows
- Analysis plots for iOS/MAC
- TODO Distribution:  
  - Linux appimage  https://python-appimage.readthedocs.io/en/latest/   https://docs.appimage.org/packaging-guide/manual.html
  - Windows installer: https://nsis.sourceforge.io/Main_Page
- Test on Ubuntu 24.10, 22.04,  Windows 11, 
