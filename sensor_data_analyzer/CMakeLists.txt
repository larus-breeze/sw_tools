cmake_minimum_required(VERSION 3.16)
project(sensor_data_analyzer C CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The algorithm library (submodule "lib") is compiled directly into the
# executable, using the same source directories as the Eclipse project.
# Globbing keeps this list in sync with the library.
file(GLOB LIB_SOURCE_FILES CONFIGURE_DEPENDS
    lib/ARM_math/*.c
    lib/Generic_Algorithms/*.cpp
    lib/NAV_Algorithms/*.cpp
    lib/Output_Formatter/*.cpp
    lib/Persistent_Data/*.cpp
)

set(SOURCE_FILES
    src/sensor_data_analyzer.cpp

    Interfaces/CAN_socket_driver.cpp
    Interfaces/EEPROM_emulation.cpp
    Interfaces/flexible_log_file_implementation.cpp
    Interfaces/persistent_data_file_emulation.cpp
    Interfaces/TCP_server.cpp

    Communication/CAN_USB_gateway.cpp
    Communication/USB_serial.cpp
)

set(HEADER_FILES
    src/embedded_math.h
    src/embedded_memory.h
    src/mainpage.h
    src/system_configuration.h

    Interfaces/CAN_socket_driver.h
    Interfaces/EEPROM_emulation.h
    Interfaces/flexible_log_file_implementation.h
    Interfaces/mutex_implementation.h
    Interfaces/persistent_data_file_emulation.h
    Interfaces/TCP_server.h

    Communication/USB_serial.h
)

add_executable(larus_emulator
  ${SOURCE_FILES}
  ${HEADER_FILES}
  ${LIB_SOURCE_FILES}
)

target_include_directories(larus_emulator PRIVATE
    src
    Interfaces
    Communication

    lib/ARM_math
    lib/Generic_Algorithms
    lib/NAV_Algorithms
    lib/Output_Formatter
    lib/Persistent_Data
)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  set_target_properties(larus_emulator PROPERTIES CXX_STANDARD 14)
  target_compile_definitions(larus_emulator PRIVATE _USE_MATH_DEFINES)   # necessary under C++17!
else()
  set_target_properties(larus_emulator PROPERTIES CXX_STANDARD 17)
endif()

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  target_compile_definitions(larus_emulator PRIVATE _WITH_USB2CAN)
endif()

if (WIN32)
    get_filename_component(LARUS_DATA_DIR  ${LARUS_PARENT_DIR}/Data  ABSOLUTE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/larus_emulator.vcxproj.user.in
        ${CMAKE_CURRENT_BINARY_DIR}/larus_emulator.vcxproj.user
    )
endif()

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(larus_emulator PRIVATE  ws2_32.lib)

    if(NOT MSVC)
      target_link_libraries(larus_emulator PRIVATE pthread.lib)
    endif()
else()
    find_package(Threads REQUIRED)
    target_link_libraries(larus_emulator PRIVATE Threads::Threads)
endif()
