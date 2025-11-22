cmake_minimum_required(VERSION 3.10.0)
project(sensor_simulator VERSION 0.1.0 LANGUAGES C CXX)

add_subdirectory(lib)
add_executable(sensor_simulator 
    src/main.cpp
    src/USB_serial.cpp
    src/TCP_server.cpp 
    src/EEPROM_emulation.cpp 
    src/CAN_socket_driver.cpp
    src/CAN_USB_gateway.cpp
    src/helper.cpp
)

target_link_libraries(sensor_simulator PRIVATE larus_lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)