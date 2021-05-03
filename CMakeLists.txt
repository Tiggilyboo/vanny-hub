cmake_minimum_required(VERSION 3.16)

# SDK
include(pico-sdk/pico_sdk_init.cmake)
project(vanny-hub)
pico_sdk_init()

# Libraries
set(ENDIANNESS little)
add_subdirectory(liblightmodbus)
include_directories(liblightmodbus/include liblightmodbus/src)

# Source
add_executable(vanny_hub
  vanny-hub.c
)
target_link_libraries(vanny_hub pico_stdlib lightmodbus)

# usb serial cdc
pico_enable_stdio_usb(vanny_hub 1)
pico_enable_stdio_uart(vanny_hub 0)

pico_add_extra_outputs(vanny_hub)

