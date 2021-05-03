#include <pico/stdlib.h>
#include <hardware/i2c.h>

#include "font.h"

int display_init();
void draw_pixel(uint16_t x, uint16_t y, bool colour);
char draw_char(char ch, uint16_t x, uint16_t y);
char draw_text(char* text, uint16_t x, uint16_t y);
void display_update();
