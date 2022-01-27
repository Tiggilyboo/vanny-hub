cmake_minimum_required(VERSION 3.12)

# SDK
include(pico-sdk/pico_sdk_init.cmake)
project(vanny-hub)
pico_sdk_init()

# Source
add_executable(vanny_hub
  vanny-hub.c
  devices-modbus.c
)

# Libraries
set(ENDIANNESS little)
set(LIGHTMODBUS_BIG_ENDIAN)
add_subdirectory(liblightmodbus)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/liblightmodbus/include liblightmodbus/src)

add_subdirectory(display)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/display)

# linking
target_link_libraries(vanny_hub
  pico_stdlib
  pico_mem_ops
  hardware_uart
  lightmodbus
  display
)

# usb serial cdc
pico_enable_stdio_usb(vanny_hub 1)
pico_enable_stdio_uart(vanny_hub 0)

pico_add_extra_outputs(vanny_hub)

