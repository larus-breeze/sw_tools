#!/bin/sh

# current directory should be the parent dir above the cloned dir 'SIL_flight_sensor_emulator'

BUILD_DIR=larus_emulator/gcc
SOURCE_DIR=SIL_flight_sensor_emulator

echo  BUILD_DIR = ${BUILD_DIR}
echo  SOURCE_DIR = ${SOURCE_DIR}

cmake -S ${SOURCE_DIR} -B ${BUILD_DIR} -G "Unix Makefiles"

cmake --build ${BUILD_DIR} --config Release -- -j 8

cmake --install ${BUILD_DIR}


${BUILD_DIR}/sensor_data_analyzer/larus_emulator Data/Hangflug_Bergstrasse/20220918120730.f50 1
