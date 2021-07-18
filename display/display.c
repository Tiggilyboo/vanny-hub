#include "display.h"

#define SCREEN_W (DISPLAY_W / 8)
#define SCREEN_H DISPLAY_H

static uint8_t display_buffer_black[(DISPLAY_W / 8) * DISPLAY_H];
static uint8_t display_buffer_red[(DISPLAY_W / 8) * DISPLAY_H];

static FontDef_t* font_normal = &FontNormal;
static FontDef_t* font_title = &FontTitle;

void display_reset() {
  gpio_put(SPI_PIN_RST, 1);
  sleep_ms(200);
  gpio_put(SPI_PIN_RST, 0);
  sleep_ms(2);
  gpio_put(SPI_PIN_RST, 1);
  sleep_ms(200);
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

  sleep_ms(200);
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

  display_reset();

  display_turn_on();

  display_send_command(EPD_PANEL_SETTING);
  // LUT from OTP, 128 x 296
  display_send_data(0x0f); // B/W = 0x1f
  // LUT from REG
  //display_send_data(0x2f); // B/W = 0x3f

  // Temp sensor, boost, timing settings
  display_send_data(0x89);

  // 3A = 100hz, 29 = 150hz, 39 = 200hz, 31 = 171Hz, 3C = 50Hz, 0B = 10Hz
  //display_send_command(EPD_PLL_CONTROL);
  //display_send_data(0x3C);

  display_send_command(EPD_TCON_RESOLUTION);
  display_send_data(0x80);
  display_send_data(0x01);
  display_send_data(0x28);

  display_send_command(EPD_VCOM_AND_DATA_INTERVAL_SETTING);
  // WB mode: VBDF 17 D7 VBDW 97 VBDB 57
  // WBR mode: VBDF F7 VBDW 77 VBDB 37 VBDR B7
  display_send_data(0x77);

  printf("EPD initialised.\n");

  return 0;
}

void display_draw_pixel(uint16_t x, uint16_t y, colour_t colour) {
  if(x > DISPLAY_W || y > DISPLAY_H) {
    printf("Pixel outside bounds (%d, %d)\n", x, y);
    return;
  }
  
  uint32_t addr = x / 8 + y * SCREEN_W;
  uint8_t data; 

  switch(colour) {
    case Black:
      data = display_buffer_black[addr];
      display_buffer_black[addr] = data & ~(0x80 >> (x % 8));
      break;
    case Red:
    case Yellow:
      data = display_buffer_red[addr];
      display_buffer_red[addr] = data & ~(0x80 >> (x % 8));
      break;
    case White:
      data = display_buffer_red[addr];
      display_buffer_red[addr] = data | (0x80 >> (x % 8));

      data = display_buffer_black[addr];
      display_buffer_black[addr] = data | (0x80 >> (x % 8));
      break;
    default:
      printf("No colour selected for pixel (%d, %d)\n", x, y);
      break;
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

void display_send_fill(const uint8_t command, const uint8_t value) {
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
  display_send_command(EPD_DISPLAY_REFRESH);
  display_read_busy();
}

void display_fill_buffers(uint8_t colour_value) {
  for(uint32_t y = 0; y < SCREEN_H; y++) {
    for(uint32_t x = 0; x < SCREEN_W; x++) {
      uint32_t addr = x + y * SCREEN_W;
      display_buffer_black[addr] = colour_value;
      display_buffer_red[addr] = colour_value;
    }
  }
}

void display_update() {
  // refresh black
  display_send_command(EPD_DATA_START_TRANSMISSION_1);
  for(uint32_t y = 0; y < SCREEN_H; y++) {
    for(uint32_t x = 0; x < SCREEN_W; x++) {
      display_send_data(display_buffer_black[x + y * SCREEN_W]);
    }
  }
  display_send_command(EPD_PARTIAL_OUT);

  // refresh red
  display_send_command(EPD_DATA_START_TRANSMISSION_2);
  for(uint32_t y = 0; y < SCREEN_H; y++) {
    for(uint32_t x = 0; x < SCREEN_W; x++) {
      display_send_data(display_buffer_red[x + y * SCREEN_W]);
    }
  }
  display_send_command(EPD_PARTIAL_OUT);

  display_send_command(EPD_DISPLAY_REFRESH);
  display_read_busy();
}


void display_partial(const uint8_t* buffer, coord_t region, int dtm) {
  display_send_command(EPD_PARTIAL_IN);
  display_send_command(EPD_PARTIAL_WINDOW);
  display_send_data(region.x >> 8);
  display_send_data(region.x & 0xf8); // x multiple of 8, last 3 bit ignored
  display_send_data(((region.x & 0xf8) + region.w - 1) >> 8);
  display_send_data(((region.x & 0xf8) + region.w - 1) | 0x07);
  display_send_data(region.y >> 8);
  display_send_data(region.y & 0xff);
  display_send_data((region.y + region.h - 1) >> 8);
  display_send_data((region.y + region.h - 1) & 0xff);
  display_send_data(0x01);  // gate scan inside and outside partial window
  sleep_ms(2);

  display_send_command((dtm == 1) ? EPD_DATA_START_TRANSMISSION_1 : EPD_DATA_START_TRANSMISSION_2);
  if(buffer != NULL) {
    for(int i = 0; i < region.w / 8 * region.h; i++) {
      display_send_data(buffer[i]);
    }
  } else {
    for(int i = 0; i < region.w / 8 * region.h; i++) {
      display_send_data(0x00);
    }
  }

  display_send_command(EPD_PARTIAL_OUT);

  display_send_command(EPD_DISPLAY_REFRESH);
}

void display_turn_off() {
  display_send_command(EPD_POWER_OFF);
  display_read_busy();
  display_send_command(EPD_DEEP_SLEEP);
  display_send_data(EPD_CHECK_CODE);
  sleep_ms(2000);
  printf("Display off and sleeping!\n");
}

void display_turn_on() {
  display_send_command(EPD_POWER_ON);
  display_read_busy();
  printf("Display on!\n");
}

