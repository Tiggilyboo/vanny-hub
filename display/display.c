#include "display.h"

#define I2C_PORT    i2c1
#define I2C_SLAVE   0x3c
#define I2C_PIN_SDA 14
#define I2C_PIN_SCL 15
#define SSD1306_W   128
#define SSD1306_H   64

uint16_t currentX, currentY;
static uint8_t display_buffer[(SSD1306_H * SSD1306_W) >> 3];
static FontDef_t* font_normal = &FontNormal;
static FontDef_t* font_title = &FontTitle;

void display_turn_off() {
  const uint8_t reg[2] = { 0x00, 0xAE };
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);
}
void display_turn_on() {
  const uint8_t reg[2] = { 0x00, 0xAF };
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);
}

void ssd1306_init() {
  static uint8_t reg[2];

  printf("Initialising ssd1306...\n");
  sleep_ms(200);

  display_turn_off();

  reg[0] = 0x00; reg[1] = 0x20; //Set Memory Addressing Mode 
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0x10; //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);
  reg[0] = 0x00; reg[1] = 0xB0; //Set Page Start Address for Page Addressing Mode,0-7
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xC8; //Set COM Output Scan Direction
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0x00; //---set low column address
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0x10; //---set high column address
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x40;  //--set start line address
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0x81; //--set contrast control register
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0xFF;
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0xA1; //--set segment re-map 0 to 127
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xA6; //--set normal display
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xA8; //--set multiplex ratio(1 to 64)
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x3F; 
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xA4; //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xD3; //-set display offset
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x00; //-not offset
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0xD5; //--set display clock divide ratio/oscillator frequency
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xF0; //--set divide ratio 
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xD9; //--set pre-charge period
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xDA; //--set com pins hardware configuration
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x12; 
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0xDB; //--set vcomh
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x20; //0x20,0.77xVcc
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false); 
  reg[0] = 0x00; reg[1] = 0x8D; //--set DC-DC enable
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  
  reg[0] = 0x00; reg[1] = 0x14;
  i2c_write_blocking(I2C_PORT, I2C_SLAVE, reg, 2, false);  

  display_turn_on();

  printf(" Done.\n");
}

inline void display_draw_pixel(uint16_t x, uint16_t y, bool colour) {
  if((x >= SSD1306_W) || y >= SSD1306_H) {
    return;
  }
  if(colour) {
    display_buffer[x+(y / 8) * SSD1306_W] |= 1 << (y % 8);
  } else {
    display_buffer[x+(y / 8) * SSD1306_W] &= ~(1 << (y % 8));
  }
}

void display_clear() {
  memset(display_buffer, 0x00, sizeof(display_buffer));
}

inline char display_draw_char(char ch, FontDef_t* font) {
  uint32_t i, b, j;

  if((SSD1306_W <= (currentX + font->font_width))
      || (SSD1306_H <= (currentY + font->font_height))) {
    return 0;
  }

  for(i = 0; i < font->font_height; i++) {
    b = font->data[(ch-32) * font->font_height + i];
    for(j = 0; j < font->font_width; j++) {
      if((b<<j & 0x8000)) {
        display_draw_pixel(currentX + j, currentY + i, 1);
      }
    }
  }

  currentX += font->font_width;

  return ch;
}

inline char display_draw_font(char* text, FontDef_t* font, uint16_t x, uint16_t y) {
  currentX = x;
  currentY = y;

  while(*text) {
    if(display_draw_char(*text, font) != *text) {
      return *text;
    }

    text++;
  }
}

char display_draw_text(char* text, uint16_t x, uint16_t y) {
  display_draw_font(text, font_normal, x, y);
}
char display_draw_title(char* title, uint16_t x, uint16_t y) {
  display_draw_font(title, font_title, x, y);
}

void display_update() {
  static uint8_t buff[2];
  static uint8_t buffer[150];

  for(uint8_t m=0; m<8; m++) {
    buff[0] = 0x00; 
    buff[1] = 0xB0+m;
    i2c_write_blocking(I2C_PORT, I2C_SLAVE, buff, 2, false); 
    buff[1] = 0x00;
    i2c_write_blocking(I2C_PORT, I2C_SLAVE, buff, 2, false); 
    buff[1] = 0x10;
    i2c_write_blocking(I2C_PORT, I2C_SLAVE, buff, 2, false); 
    
    buffer[0] = 0x40;
    for(uint8_t i = 0; i < SSD1306_W; i++) {
        buffer[i+1] = (uint8_t) *(display_buffer+(SSD1306_W * m) + i);
    }
    i2c_write_blocking(I2C_PORT, I2C_SLAVE, buffer, SSD1306_W + 1, false);
  }
}

int display_init() {
  printf("Initialising i2c...");
  i2c_init(I2C_PORT, 100000);

  gpio_set_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_PIN_SDA);
  gpio_pull_up(I2C_PIN_SCL);

  currentX = currentY = 0;

  ssd1306_init(I2C_SLAVE);
}
