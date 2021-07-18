add_library(display 
  font.c
  display.c
)

target_link_libraries(display
  pico_stdlib
  pico_mem_ops
  hardware_spi
)
