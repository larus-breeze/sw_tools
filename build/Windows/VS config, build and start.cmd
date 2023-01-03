cd /D %~dp0../../..

:: Start in the parent directory of the project source,
:: for example 'C:\Projects\Larus
:: Here you can clone the Source (C:\Projects\Larus\SIL_flight_sensor_emulator),
:: will be built the BUILD_DIR (C:\Projects\Larus\larus_emulator\msvc) and
:: can be stored the data files (f.e. D:\Projects\Larus\Data\Wind_30km_h)

:: call init for MSVC environment:
call "D:\Programs\Microsoft Visual Studio\2022\Preview\VC\Auxiliary\Build\vcvars64.bat"

set BUILD_DIR=larus_emulator\msvc
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

:: Build the project:
cmake -S SIL_flight_sensor_emulator -B %BUILD_DIR% -G "Visual Studio 17 2022"
if errorlevel 1 goto ERROR

:: Compile it:
cmake --build %BUILD_DIR% --config Release --
if errorlevel 1 goto ERROR

:: Install it on the computer (not needed yet):
cmake --install %BUILD_DIR%
if errorlevel 1 goto ERROR

:: and run:
:: location of executable, example data and time offset:
%BUILD_DIR%\sensor_data_analyzer\Release\larus_emulator.exe Data/Wind_30km_h/20220601_Wind_30.f50 1

pause
REM pause
exit /B 0

: ERROR
echo %CD%
pause

exit /B 1000
