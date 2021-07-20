#include "display.h"

#define SCREEN_W (DISPLAY_W / 8)
#define SCREEN_H DISPLAY_H

static uint8_t* display_buffer;

static FontDef_t* font_normal = &FontNormal;
static FontDef_t* font_title = &FontTitle;

void display_reset() {
  gpio_put(SPI_PIN_RST, 0);
  busy_wait_ms(10);
  gpio_put(SPI_PIN_RST, 1);
  busy_wait_ms(10);
}

void display_send_command(uint8_t reg) {
  gpio_put(SPI_PIN_DC, 0);
  gpio_put(SPI_PIN_CS, 0);
  spi_write_blocking(SPI_PORT, &reg, 1);
  gpio_put(SPI_PIN_CS, 1);
}

void display_send_data(uint8_t data) {
  gpio_put(SPI_PIN_DC, 1);
  gpio_put(SPI_PIN_CS, 0);
  spi_write_blocking(SPI_PORT, &data, 1);
  gpio_put(SPI_PIN_CS, 1);
}

void display_read_busy() {
  uint8_t busy;

  printf("EPD busy...");
  do {
    display_send_command(EPD_GET_STATUS);
    busy = gpio_get(SPI_PIN_BSY);
    busy = !(busy & 0x01);
  } 
  while(busy);
  printf("Done.\n");

  busy_wait_ms(200);
}

void display_gpio_init() {
  gpio_init(SPI_PIN_RST);
  gpio_set_dir(SPI_PIN_RST, GPIO_OUT);

  gpio_init(SPI_PIN_DC);
  gpio_set_dir(SPI_PIN_DC, GPIO_OUT);

  gpio_init(SPI_PIN_CS);
  gpio_set_dir(SPI_PIN_CS, GPIO_OUT);

  gpio_init(SPI_PIN_BSY);
  gpio_set_dir(SPI_PIN_BSY, GPIO_IN);

  gpio_put(SPI_PIN_CS, 1);
}

int display_init() {
  display_gpio_init();

  spi_init(SPI_PORT, SPI_BD);
  gpio_set_function(SPI_PIN_CLK, GPIO_FUNC_SPI);
  gpio_set_function(SPI_PIN_DIN, GPIO_FUNC_SPI);

  printf("SPI Device initialised\n");

  display_wake();

  printf("EPD initialised.\n");

  return 0;
}

void display_set_buffer(const uint8_t* buffer) {
  display_buffer = buffer;  
}

void display_draw_pixel(uint16_t x_start, uint16_t y_start, colour_t colour) {
  // translate 270 degrees
  const uint16_t x = y_start;
  const uint16_t y = DISPLAY_H - x_start - 1;

  if(x > DISPLAY_W || y > DISPLAY_H) {
    printf("Pixel outside bounds (%d, %d)\n", x, y);
    return;
  }
  
  uint32_t addr = x / 8 + y * SCREEN_W;
  uint8_t data; 

  switch(colour) {
    case Black:
    case Red:
    case Yellow:
      data = display_buffer[addr];
      display_buffer[addr] = data & ~(0x80 >> (x % 8));
      break;
    case White:
      data = display_buffer[addr];
      display_buffer[addr] = data | (0x80 >> (x % 8));
      break;
    default:
      printf("No colour selected for pixel (%d, %d)\n", x, y);
      break;
  }
}
void display_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  const int16_t steep = y1 - y0 > x1 - x0;
  if(steep) {
    uint16_t t = x0;
    x0 = y0;
    y0 = t;
    
    t = x1;
    x1 = y1;
    y1 = t;
    
    if (x0 > x1) {
      t = x0;
      x0 = x1;
      x1 = t;
      
      t = y0;
      y0 = y1;
      y1 = t;
    }
  }

  const int dx = x1 - x0;
  const int dy = y1 - y0;
  const int16_t ystep = (y0 < y1) ? 1 : -1;

  int16_t err = dx / 2;
  
  printf("Drawing line: (%d, %d, %d, %d) ", x0, y0, x1, y1);
  printf("[%d, %d]\n", dx, dy);
  for(; x0 <= x1; x0++) {
    if(steep) {
      display_draw_pixel(y0, x0, Black);
    } else {
      display_draw_pixel(x0, y0, Black);
    }
    err -= dy;
    if(err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void display_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  // Top left -> Top Right
  display_draw_line(x1, y1, x2, y1);
  // Bottom left -> Bottom Right
  display_draw_line(x1, y2, x2, y2);
  // Top left -> Bottom left
  display_draw_line(x1, y1, x1, y2);
  // Top right -> Bottom right
  display_draw_line(x2, y1, x2, y2);
}

void display_draw_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  for(uint16_t x = x1; x < x2; x++) {
    for(uint16_t y = y1; y < y2; y++) {
      display_draw_pixel(x, y, Black);
    }
  }
}

char display_draw_char(uint16_t x, uint16_t y, char ch, FontDef_t* font, colour_t colour) {
  uint16_t page, col;

  if(x > DISPLAY_W || y > DISPLAY_H) {
    printf("Character trying to be drawn outside screen (%d, %d): '%c'\n", x, y, ch);
    return NULL;
  }

  uint32_t char_offset = (ch - ' ') * font->font_height * (font->font_width / 8 + (font->font_width % 8 ? 1 : 0));
  const unsigned char *ptr = &font->data[char_offset];

  for(page = 0; page < font->font_height; page++) {
    for(col = 0; col < font->font_width; col++) {
      if(*ptr & (0x80 >> (col % 8))) {
        display_draw_pixel(x + col, y + page, colour);
      } else {
        display_draw_pixel(x + col, y + page, White);
      }
      if(col % 8 == 7)
        ptr++;
    }
    if(font->font_width % 8 != 0)
      ptr++;
  }

  return ch;
}

char display_draw_font(char* text, FontDef_t* font, uint16_t x, uint16_t y, colour_t colour) {
  uint16_t current_x = x;

  printf("Drawing: ");

  while(*text) {
    if(display_draw_char(current_x, y, *text, font, colour) != *text) {
      return *text;
    }
    printf("%c", *text);

    // increment to next character, and associated character width
    text++;
    current_x += font->font_width;
  }

  printf("\n");
}

char display_draw_text(char* text, uint16_t x, uint16_t y, colour_t colour) {
  display_draw_font(text, font_normal, x, y, colour);
}
char display_draw_title(char* text, uint16_t x, uint16_t y, colour_t colour) {
  display_draw_font(text, font_title, x, y, colour);
}

inline void display_send_fill(const uint8_t command, const uint8_t value) {
  display_send_command(command);
  for(int j = 0; j < SCREEN_H; j++) {
    for(int i = 0; i < SCREEN_W; i++) {
      display_send_data(value);
    }
  }
}

void display_clear() {
  display_send_fill(EPD_DATA_START_TRANSMISSION_1, 0xff);
  display_send_fill(EPD_DATA_START_TRANSMISSION_2, 0xff);
  display_refresh(true);
}

void display_fill_colour(colour_t colour) {
  uint8_t colour_value = (colour == White) ? 0xff : 0x00;

  for(uint16_t i = 0; i < SCREEN_W * SCREEN_H; i++) {
    display_buffer[i] = colour_value;
  }
}

void display_send_buffer(const uint8_t* buffer, int w, int h, int dtm) {
  uint8_t cmd = (dtm == 1) ? EPD_DATA_START_TRANSMISSION_1 : EPD_DATA_START_TRANSMISSION_2;
  display_send_command(cmd);
  for(uint32_t y = 0; y < h; y++) {
    for(uint32_t x = 0; x < w; x++) {
      display_send_data(buffer[x + y * w]);
    }
  }
}

uint16_t display_set_partial_window(coord_t region) {
  region.x &= 0xfff8; // byte boundary
  region.w = (region.w - 1) | 0x0007; // byte boundary

  display_send_command(EPD_PARTIAL_WINDOW);
  display_send_data(region.x % 256);
  display_send_data(region.w % 256);
  display_send_data(region.y / 256);
  display_send_data(region.y % 256);
  display_send_data(region.h / 256);
  display_send_data(region.h % 256);
  display_send_data(0x01);

  // return number of bytes to transfer per line
  return (7 + region.w - region.x) / 8;
}

void display_draw_partial(const uint8_t* black_buffer, const uint8_t* red_buffer, const coord_t region) {
  display_send_command(EPD_PARTIAL_IN);
  display_set_partial_window(region);

  display_send_buffer(black_buffer, SCREEN_W, SCREEN_H, 1);
  display_send_buffer(red_buffer, SCREEN_W, SCREEN_H, 2);
  display_send_command(EPD_PARTIAL_OUT);

  display_refresh(true);
  busy_wait_ms(500);
}

void display_refresh(bool wait_busy) {
  display_send_command(EPD_DISPLAY_REFRESH);
  if(wait_busy)
    display_read_busy();
}

void display_sleep() {
  display_send_command(EPD_POWER_OFF);
  display_read_busy();
  display_send_command(EPD_DEEP_SLEEP);
  display_send_data(EPD_CHECK_CODE);
  printf("Display off and sleeping!\n");
}

void display_wake() {
  display_reset();
  display_send_command(EPD_BOOSTER_SOFT_START);
  display_send_data(0x17);
  display_send_data(0x17);
  display_send_data(0x17);
  display_send_command(EPD_POWER_ON);
  display_send_command(EPD_PANEL_SETTING);
  display_send_data(0x0f);
  display_send_command(EPD_PLL_CONTROL);
  display_send_data(0x3C);
  display_send_command(EPD_VCOM_AND_DATA_INTERVAL_SETTING);
  display_send_data(0x77);
  display_send_command(EPD_TCON_RESOLUTION);
  display_send_data(0x80);
  display_send_data(0x01);
  display_send_data(0x28);
  printf("Display awake!\n");
}

