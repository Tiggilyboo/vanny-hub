#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/spi.h>

#include "font.h"
#include "display-ws-eink.h"

typedef enum {
  Black,
  Red,
  Yellow,
} pigment_t;

typedef struct {
  pigment_t pigment;
  uint8_t depth;
} colour_t;

int display_init();
void display_draw_pixel(uint16_t x, uint16_t y, colour_t colour);
char display_draw_text(char* text, uint16_t x, uint16_t y, colour_t colour);
char display_draw_title(char* title, uint16_t x, uint16_t y, colour_t colour);
void display_clear();
void display_update();
void display_turn_off();
void display_turn_on();

