# Analysis
Python scripts to convert and plot larus raw or processed data. 

## How to start
You need one of the supported binary file such as:
- Raw measurement data record file from a Larus sensor e.g. (*.f37) which contains the non-processed raw sensor 
measurement values (ACC, GYRO, MAG, (D-)GNSS, Static, Pitot). 
- A with https://github.com/larus-breeze/SIL_flight_sensor_emulator/ generated flight log file such as *.f110 
which contains the raw measurement values and the calculated values (AHRS, WIND, VARIO, TAS, IAS, Earth Induction,
...)
- Supported files are described in dataformats.py

## Scripts
- binary_to_csv.py: Converts binary data to a human-readable csv data. 
- load_to_df.py: Python class to import one of the in dataformats.py described files into a pandas dataframe.
- plot_*.py: Files to generate plots from generated flight log files. 

### Example plots

    python3 binary_to_csv.py ../flightdata/230430_090724.f37
    python3 plot_wind.py ../flightdata/230414_085440.f37.f110
    python3 plot_mag.py ../flightdata/230414_085440.f37.f110

![Induction in earth system plot](example_plots/induction_earth.svg)
![Induction in earth system plot](example_plots/induction_earth_zoom.svg)
![Induction in earth system plot](example_plots/wind.svg)


### Create binaries for Linux and Windows
- Cd into analysis directory
- Create virtual python environment via ```python3 -m virtualenv myvenv```
- Activate myvenv with 
  - Linux: ```source myvenv/bin/activate``` 
  - Windows: ```.\myvenv/Scripts/activate.ps1```
- Install dependencies with ```pip install -r requirements.txt```
- Create binary with pyinstaller analysis_plots.py

- TODO Distribution:  
  - Linux appimage  https://python-appimage.readthedocs.io/en/latest/   https://docs.appimage.org/packaging-guide/manual.html
  - Windows installer: https://nsis.sourceforge.io/Main_Page
- Test on Ubuntu 24.10, 22.04,  Windows 11


## Wishlist / TODOs

- Add GNSS sat status and velocity error to ahrs plot. Plot for GNSS Quality and Signal Losses  D-GNSS and GNSS
- Provide Functionality to only export a timeframe via csv
- Float csv precision. 

