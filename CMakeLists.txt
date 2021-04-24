if (PICO_ON_DEVICE)
  message(WARNING "not building because TinyUSB submodule is not initialized in the SDK")
endif()

cmake_minimum_required(VERSION 3.13)

# SDK
include(pico-sdk/pico_sdk_init.cmake)
project(vanny-hub)
pico_sdk_init()

# Source
add_executable(vanny_hub
  vanny-hub.c
  modbus.c
)
target_link_libraries(vanny_hub pico_stdlib)

# usb serial cdc
pico_enable_stdio_usb(vanny_hub 1)
pico_enable_stdio_uart(vanny_hub 0)

pico_add_extra_outputs(vanny_hub)

