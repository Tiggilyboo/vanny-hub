#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>
#include <hardware/spi.h>

#include "font.h"
#include "display-ws-eink.h"

typedef enum {
  White,
  Black,
  Red,
  Yellow,
} colour_t;

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t w;
  uint8_t h;
} coord_t;

int display_init();
void display_draw_pixel(uint16_t x, uint16_t y, colour_t colour);
char display_draw_text(char* text, uint16_t x, uint16_t y, colour_t colour);
char display_draw_title(char* title, uint16_t x, uint16_t y, colour_t colour);
void display_clear();
void display_update();
void display_partial(const uint8_t* buffer, coord_t region, int dtm);
void display_fill_buffers(colour_t colour);
void display_turn_off();
void display_turn_on();

