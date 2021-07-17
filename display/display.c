#include "display.h"

const uint32_t screen_w = (DISPLAY_W % 8 == 0)
  ? (DISPLAY_W / 8)
  : (DISPLAY_W / 8 + 1);
const uint32_t screen_h = DISPLAY_H;

static uint8_t display_buffer_black[DISPLAY_W * DISPLAY_H];
static uint8_t display_buffer_red[DISPLAY_W * DISPLAY_H];

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

  spi_init(SPI_PORT, SPI_BD); // 4000 * 1000??
  gpio_set_function(SPI_PIN_CLK, GPIO_FUNC_SPI);
  gpio_set_function(SPI_PIN_DIN, GPIO_FUNC_SPI);

  printf("SPI Device initialised\n");

  display_reset();

  display_turn_on();

  display_send_command(EPD_PANEL_SETTING);
  // LUT from OTP, 128 x 296
  display_send_data(0x0f);
  // Temp sensor, boost, timing settings
  display_send_data(0x89);

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
  switch(colour.pigment) {
    case Black:
      display_buffer_black[x + (screen_w * y)] = colour.depth;
      break;
    case Red:
    case Yellow:
      display_buffer_red[x + (screen_w * y)] = colour.depth;
      break;
    default:
      printf("No pigment selected for pixel (%d, %d)\n", x, y);
  }
}

char display_draw_char(uint16_t* x, uint16_t* y, char ch, FontDef_t* font, colour_t colour) {
  uint32_t i, b, j;

  for(i = 0; i < font->font_height; i++) {
    b = font->data[(ch-32) * font->font_height + i];
    for(j = 0; j < font->font_width; j++) {
      if((b<<j & 0x8000)) {
        display_draw_pixel(*x + j, *y + i, colour);
      }
    }
  }

  *x += font->font_width;

  return ch;
}

inline char display_draw_font(char* text, FontDef_t* font, uint16_t x, uint16_t y, colour_t colour) {
  printf("Drawing: ");

  while(*text) {
    if(display_draw_char(&x, &y, *text, font, colour) != *text) {
      return *text;
    }
    printf("%c", *text);

    text++;
  }

  printf("\n");
}

char display_draw_text(char* text, uint16_t x, uint16_t y, colour_t colour) {
  display_draw_font(text, font_normal, x, y, colour);
}
char display_draw_title(char* text, uint16_t x, uint16_t y, colour_t colour) {
  display_draw_font(text, font_title, x, y, colour);
}

inline void display_fill(const uint8_t command, const uint8_t value) {
  display_send_command(command);
  for(uint32_t y = 0; y < screen_h; y++) {
    for(uint32_t x = 0; x < screen_w; x++) {
      display_send_data(value);
    }
  }
}

void display_clear() {
  for(uint32_t y = 0; y < screen_h; y++) {
    for(uint32_t x = 0; x < screen_w; x++) {
      display_buffer_black[x + (y * screen_w)] = 0xff;
      display_buffer_red[x + (y * screen_w)] = 0xff;
    }
  }

  display_fill(EPD_DATA_START_TRANSMISSION_1, 0xff);
  display_fill(EPD_DATA_START_TRANSMISSION_2, 0xff);

  display_send_command(EPD_DISPLAY_REFRESH);
  display_read_busy();
}

void display_update() {
  // refresh black
  display_send_command(EPD_DATA_START_TRANSMISSION_1);
  for(uint32_t y = 0; y < screen_h; y++) {
    for(uint32_t x = 0; x < screen_w; x++) {
      display_send_data(display_buffer_black[x + (y * screen_w)]);
    }
  }
  display_send_command(EPD_PARTIAL_OUT);

  // refresh red
  display_send_command(EPD_DATA_START_TRANSMISSION_2);
  for(uint32_t y = 0; y < screen_h; y++) {
    for(uint32_t x = 0; x < screen_w; x++) {
      display_send_data(display_buffer_red[x + (y * screen_w)]);
    }
  }
  display_send_command(EPD_PARTIAL_OUT);

  display_send_command(EPD_DISPLAY_REFRESH);
  display_read_busy();
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

