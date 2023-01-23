cmake_minimum_required(VERSION 3.00)
project(larus)

# usually LARUS_PARENT_DIR is the Parent dir of this project
get_filename_component(LARUS_PARENT_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/..  ABSOLUTE)

if(MSVC)
  add_compile_definitions(__MSVC__)
endif()

add_subdirectory(sensor_data_analyzer)

