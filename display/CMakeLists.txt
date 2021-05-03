add_library(display 
  font.c
  display.c
)

target_link_libraries(display
  pico_stdlib
  hardware_i2c
)
