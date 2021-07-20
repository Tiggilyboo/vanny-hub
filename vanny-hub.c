#include "devices-modbus.h"
#include "vanny-hub.h"

#define _OFFLINE_TESTING
#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

#define EPD_FULL_REFRESH_AFTER  5

#define RS485_DCC50S_ADDRESS    1
#define RS485_LFP12S_ADDRESS_A  2
#define RS485_LFP12S_ADDRESS_B  3

#define RS232_RVR40_ADDRESS     1


typedef enum {
  Overview,
  Altenator,
  Solar,
  PageContentsCount,
} PageContents_t;

// Interface State
static PageContents_t current_page;
static uint64_t btn_last_pressed;

// EPD State
static uint8_t display_buffer_black[SCREEN_W * SCREEN_H];
static uint8_t display_buffer_red[SCREEN_W * SCREEN_H];
static uint8_t display_refresh_count;
static bool display_state;
static bool display_partial_mode;

// Device State
static uint16_t dcc50s_registers[DCC50S_REG_END+1];
static uint16_t rvr40_registers[RVR40_REG_END+1];

char* append_buf(char* s1, char* s2) {
  if(s1 == NULL || s2 == NULL)
    return NULL;

  int n = sizeof(s1);
  char* dest = s1;
  while(*dest != '\0'){
    dest++;
  }
  while(n--){
    if(!(*dest++ = *s2++))
      return s1;
  }
  *dest = '\0';

  return s1;
}

void get_charge_status(char* buffer, uint16_t dcc50s_state, uint16_t rvr40_state) {
  bool charging = false;

  if ((dcc50s_state & DCC50S_CHARGE_STATE_SOLAR) > 0) {
    append_buf(buffer, "Sol ");
    charging = true;
  }
  if ((dcc50s_state & DCC50S_CHARGE_STATE_ALT) > 0) {
    append_buf(buffer, "Alt ");
    charging = true;
  }
  if ((rvr40_state & RVR40_CHARGE_ACTIVE) > 0) {
    append_buf(buffer, "Sol ");
    charging = true;
  }
  if(!charging){
    sprintf(buffer, "No charge");
    return;
  }

  if ((rvr40_state & DCC50S_CHARGE_STATE_EQ) > 0
      || (rvr40_state & RVR40_CHARGE_EQ) > 0) {
    append_buf(buffer, "=");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_BOOST) > 0
      || (rvr40_state & RVR40_CHARGE_BOOST) > 0) {
    append_buf(buffer, "++");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_FLOAT) > 0
      || (rvr40_state & RVR40_CHARGE_FLOAT) > 0) {
    append_buf(buffer, "~");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_LIMITED) > 0
      || (rvr40_state & RVR40_CHARGE_LIMITED) > 0) {
    append_buf(buffer, "+");
  }
}

void get_temperatures(char* buffer, uint16_t state) {
  uint16_t internal_temp = (state >> 8);
  uint16_t aux_temp = (state & 0xFF);

  sprintf(buffer, "C %d*C B %d*C", internal_temp, aux_temp);
}

void update_page_overview() {
  char line[32];

  uint16_t aux_soc = dcc50s_registers[DCC50S_REG_AUX_SOC];
  float aux_v = (float)dcc50s_registers[DCC50S_REG_AUX_V] / 10.f;   // 0.1v
  float aux_a = (float)dcc50s_registers[DCC50S_REG_AUX_A] / 100.f;  // 0.01a

  uint16_t dcc_charge_state = dcc50s_registers[DCC50S_REG_CHARGE_STATE];
  uint16_t temperature = dcc50s_registers[DCC50S_REG_TEMPERATURE];

  // Sum any charge from the RVR40 as well
  float sol_a = (float)rvr40_registers[RVR40_REG_SOLAR_A] / 100.f;
  uint16_t rvr_charge_state = rvr40_registers[RVR40_REG_CHARGE_STATE];

  get_charge_status((char*)&line, dcc_charge_state, rvr_charge_state);
  display_draw_title(line, 0, 0, Black);

  sprintf((char*)&line, "%d%%", aux_soc); 
  display_draw_title(line, 72, 28, Black);

  // Also add any solar amperage
  sprintf((char*)&line, "+%.*fA", 2, aux_a + sol_a);
  display_draw_text(line, 0, 24, Black);

  sprintf((char*)&line, "%.*fV", 1, aux_v);
  display_draw_text(line, 0, 36, Black);

  get_temperatures((char*)&line, temperature);
  display_draw_text(line, 0, 50, Black);
}

void update_page_solar() {
  char line[32];

  /* DCC50S Solar if so inclined...
  uint16_t sol_a = dcc50s_registers[DCC50S_REG_SOL_A];
  uint16_t sol_v = dcc50s_registers[DCC50S_REG_SOL_V];
  uint16_t sol_w = dcc50s_registers[DCC50S_REG_SOL_W];
  */
  float sol_v = (float)rvr40_registers[RVR40_REG_SOLAR_V] / 10.f;
  float sol_a = (float)rvr40_registers[RVR40_REG_SOLAR_A] / 100.f;
  uint16_t sol_w = rvr40_registers[RVR40_REG_SOLAR_W];

  display_draw_title("Solar", 0, 0, Black);

  sprintf((char*)&line, "+ %.*fA", 1, sol_a);
  display_draw_text(line, 0, 24, Black);

  sprintf((char*)&line, "%.*fV", 1, sol_v);
  display_draw_text(line, 48, 24, Black);

  sprintf((char*)&line, "%dW", sol_w);
  display_draw_text(line, 86, 24, Black);

  sprintf((char*)&line, "Day +%dAh, -%dAh", 
      rvr40_registers[RVR40_REG_DAY_CHG_AMPHRS],
      rvr40_registers[RVR40_REG_DAY_DCHG_AMPHRS]);
  display_draw_text(line, 0, 36, Black);

  get_temperatures((char*)&line, rvr40_registers[RVR40_REG_TEMPERATURE]);
  display_draw_text(line, 0, 50, Black);
}

void update_page_altenator() {
  char line[32];

  uint16_t alt_a = dcc50s_registers[DCC50S_REG_ALT_A];
  uint16_t alt_v = dcc50s_registers[DCC50S_REG_ALT_V];
  uint16_t alt_w = dcc50s_registers[DCC50S_REG_ALT_W];

  sprintf((char*)&line, "%dA %dV %dW", alt_a, alt_v, alt_w);
  display_draw_title("Altenator", 0, 0, Black);

  display_draw_text(line, 32, 20, Black);
}

void update_page_ui() {
  const uint16_t half_x = DISPLAY_H / 2;
  const uint16_t half_y = DISPLAY_W / 2;
  const uint16_t third_x = DISPLAY_H / 3;
  const uint16_t third_y = DISPLAY_W / 3;

  display_draw_rect(third_x * 2, third_y, DISPLAY_H - 20, third_y * 2);
}

void update_page() {
  // Clear black and red buffers (with White, 0xff);
  display_set_buffer((const uint8_t*)display_buffer_red);
  display_fill_colour(White);
  display_set_buffer((const uint8_t*)display_buffer_black);
  display_fill_colour(White);

  // full refresh after x partials or first draw
  if(display_refresh_count == 0 || display_refresh_count >= EPD_FULL_REFRESH_AFTER) {
    printf("Full refresh: ");
    display_partial_mode = false;
    display_refresh_count = 1;
  } else {
    display_partial_mode = true;
    display_refresh_count++;
  }

  switch(current_page) {
    case Overview: 
      update_page_overview();
      break;

    case Altenator:
      update_page_altenator();
      break;

    case Solar:
      update_page_solar();
      break;

    default:
      display_draw_title("404", 0, 0, Black);
      break;
  }

  update_page_ui();
  
  if(!display_state){
    display_wake();
    display_state = true;
  }
  if(!display_partial_mode) {
    printf("Updating full screen normal refresh\n");
    display_send_buffer(display_buffer_black, SCREEN_W, SCREEN_H, 1);
    display_send_buffer(display_buffer_red, SCREEN_W, SCREEN_H, 2);
    display_refresh(true);
    busy_wait_ms(500);
    display_sleep();
    display_state = false;
  } else {
    display_state = true;
    printf("Updating partial\n");
    coord_t screen_region = { 0, 0, DISPLAY_W - 1, DISPLAY_H - 1 };
    display_draw_partial(display_buffer_black, display_buffer_red, screen_region);
  }
}

void btn_handler(uint gpio, uint32_t events) {
  static uint64_t time_handled;

#ifdef _VERBOSE
  printf("Btn @ %d is now %02x\n", gpio, events);
#endif
/*
  if (events & 0x1) { // Low
  }
  if (events & 0x2) { // High
  }
  if (events & 0x4) { // Fall
  }
*/
  if (events & 0x8) { // Rise
    // debounce
    if(time_us_64() > btn_last_pressed + 350000) // 350ms
    {
      current_page = (++current_page) % PageContentsCount;
      printf("Changed current page to %d\n", current_page);
      update_page();
    }
  }
}

int main() {
  int state;

  stdio_init_all();
 
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_handler);
  current_page = Overview;
  btn_last_pressed = time_us_64();

  state = devices_modbus_init();
  if(state != 0) {
    return state;
  }

  // wait for host... (should be a better way)
  gpio_put(LED_PIN, 0);
  busy_wait_ms(3000);
  gpio_put(LED_PIN, 1);

  state = devices_modbus_uart_init();
  if(state != 0) {
    printf("Unable to initialise modbus: %d", state); 
    return state;
  }

  display_init();
  display_state = true;
  display_clear();
  busy_wait_ms(500);

  gpio_put(LED_PIN, 0);

  while(1) {
    gpio_put(LED_PIN, 1);

    devices_modbus_read_registers(
        RS232_PORT, RS232_RVR40_ADDRESS, RVR40_REG_START, RVR40_REG_END, (uint16_t*)&rvr40_registers);

    devices_modbus_read_registers(
      RS485_PORT, RS485_DCC50S_ADDRESS, DCC50S_REG_START, DCC50S_REG_END, (uint16_t*)&dcc50s_registers);
    
    update_page();
    
    gpio_put(LED_PIN, 0);
    sleep_ms(5000);
  }
}

