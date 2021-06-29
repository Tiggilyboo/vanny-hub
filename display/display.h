#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/i2c.h>

#include "font.h"

int display_init();
void display_draw_pixel(uint16_t x, uint16_t y, bool colour);
char display_draw_text(char* text, uint16_t x, uint16_t y);
char display_draw_title(char* title, uint16_t x, uint16_t y);
void display_clear();
void display_update();
void display_turn_off();
void display_turn_on();
