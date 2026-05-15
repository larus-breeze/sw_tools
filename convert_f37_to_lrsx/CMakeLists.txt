
add_subdirectory(lib)
# add_subdirectory(Communication)

set(SOURCE_FILES
    src/EEPROM_emulation.cpp
    src/sensor_data_analyzer.cpp
    src/TCP_server.cpp

    Communication/CAN_USB_gateway.cpp
    Communication/USB_serial.cpp
)

set(HEADER_FILES
    src/EEPROM_emulation.h
    src/embedded_math.h
    src/embedded_memory.h
    src/mainpage.h
    src/system_configuration.h
    src/TCP_server.h

    Communication/USB_serial.h
)

include_directories(
    src
    Communication

    lib/Generic_Algorithms
    lib/NAV_Algorithms
    lib/Output_Formatter
 
    Communication
)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  string(APPEND CMAKE_CXX_FLAGS " -std=c++14")
  add_compile_definitions(_USE_MATH_DEFINES)   # necessary under C++17!
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  add_compile_definitions(_WITH_USB2CAN)
endif()

if (WIN32)
    get_filename_component(LARUS_DATA_DIR  ${LARUS_PARENT_DIR}/Data  ABSOLUTE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/larus_emulator.vcxproj.user.in
        ${CMAKE_CURRENT_BINARY_DIR}/larus_emulator.vcxproj.user
    )
endif()

add_executable(larus_emulator
  ${SOURCE_FILES}
  ${HEADER_FILES}
)

target_link_libraries(larus_emulator PRIVATE
  larus_lib
)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(larus_emulator PRIVATE  ws2_32.lib)

    if(NOT MSVC)
      target_link_libraries(larus_emulator PRIVATE pthread.lib)
    endif()
endif()


