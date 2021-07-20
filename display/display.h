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
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
} coord_t;

int display_init();
void display_set_buffer(const uint8_t* buffer);
void display_send_buffer(const uint8_t* buffer, int w, int h, int dtm);
void display_draw_partial(const uint8_t* black, const uint8_t* red, const coord_t region);
void display_draw_pixel(uint16_t x, uint16_t y, colour_t colour);
void display_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void display_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void display_draw_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
char display_draw_text(char* text, uint16_t x, uint16_t y, colour_t colour);
char display_draw_title(char* title, uint16_t x, uint16_t y, colour_t colour);
void display_fill_colour(colour_t colour);
void display_clear();
void display_refresh(bool wait_busy);
void display_sleep();
void display_wake();

