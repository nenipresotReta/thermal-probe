cmake_minimum_required(VERSION 3.20.0)
# Validate on every configure, including direct west/CMake builds.
find_program(PROBE_CONFIG_PYTHON NAMES python3 REQUIRED)
execute_process(COMMAND ${PROBE_CONFIG_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/tools/configure.py
  RESULT_VARIABLE PROBE_CONFIG_RESULT)
if(NOT PROBE_CONFIG_RESULT EQUAL 0)
  message(FATAL_ERROR "Fill config/device.json before building or flashing (see README.md).")
endif()
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(thermal_probe)
target_sources(app PRIVATE src/main.c src/wireless.c src/modbus_codec.c src/http_request.c src/filter.c src/format.c)
target_sources_ifdef(CONFIG_THERMAL_DISPLAY app PRIVATE src/display.c src/tdisplay_lcd.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/local)
