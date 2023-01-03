cd /D %~dp0../../..

:: Start in the parent directory of the project source,
:: for example 'C:\Projects\Larus
:: Here you can clone the Source (C:\Projects\Larus\SIL_flight_sensor_emulator),
:: will be built the BUILD_DIR (C:\Projects\Larus\larus_emulator\mgw and
:: can be stored the data files (f.e. D:\Projects\Larus\Data\Wind_30km_h)

:: Add MinGW bin dir to PATH variable (if not set)
PATH=D:\Programs\MinGW\mgw112\bin;%PATH%

set BUILD_DIR=larus_emulator\mgw
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

:: Build the project:
cmake -S SIL_flight_sensor_emulator -B %BUILD_DIR% -G "MinGW Makefiles"
if errorlevel 1 goto ERROR

:: Compile it:
cmake --build %BUILD_DIR% --config Release -- -j 8
if errorlevel 1 goto ERROR

:: Install it on the computer (not needed yet):
cmake --install %BUILD_DIR%
if errorlevel 1 goto ERROR

:: and run:
:: location of executable, example data and time offset:
%BUILD_DIR%\sensor_data_analyzer\larus_emulator.exe Data/Wind_30km_h/20220601_Wind_30.f50 1

REM pause
exit /B 0

: ERROR
echo %CD%
pause

exit /B 1000
