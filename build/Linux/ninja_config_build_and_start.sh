#!/bin/sh

# current directory should be the parent dir above the cloned dir 'SIL_flight_sensor_emulator'


BUILD_DIR=larus_emulator/ninja
SOURCE_DIR=$(pwd)/SIL_flight_sensor_emulator

echo  BUILD_DIR = ${BUILD_DIR}
echo  SOURCE_DIR = ${SOURCE_DIR}

# Build the project:
cmake -S ${SOURCE_DIR} -B ${BUILD_DIR} -G "Ninja"

# Compile it:
cmake --build ${BUILD_DIR} --config Release -- -j 8

# Install it on the computer (not needed yet):
cmake --install ${BUILD_DIR}


# and run:
# location of executable, example data and time offset:
echo start larus_emulator with 'Data/Wind_30km_h/20220601_Wind_30'
${BUILD_DIR}/sensor_data_analyzer/larus_emulator Data/Wind_30km_h/20220601_Wind_30.f50 1
