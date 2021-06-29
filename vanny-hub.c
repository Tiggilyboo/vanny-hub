#include "devices-modbus.h"
#include "vanny-hub.h"

#define _OFFLINE_TESTING
#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

#define RS485_DCC50S_ADDRESS    1
#define RS485_LFP12S_ADDRESS_A  2
#define RS485_LFP12S_ADDRESS_B  3

typedef enum {
  Overview,
  Altenator,
  Solar,
  PageContentsCount,
} PageContents_t;

// Interface State
static PageContents_t current_page;
static uint64_t btn_last_pressed;
static bool display_state;

// Device State
static uint16_t* modbus_registers;

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

void dcc50s_get_charge_status(char* buffer, uint16_t state) {
  bool charging = false;

  if ((state & DCC50S_CHARGE_STATE_SOLAR) > 0) {
    append_buf(buffer, "Sol ");
    charging = true;
  }
  if ((state & DCC50S_CHARGE_STATE_ALT) > 0) {
    append_buf(buffer, "Alt ");
    charging = true;
  }
  if(!charging){
    sprintf(buffer, "No charge");
    return;
  }

  if ((state & DCC50S_CHARGE_STATE_EQ) > 0) {
    append_buf(buffer, "=");
  }
  if ((state & DCC50S_CHARGE_STATE_BOOST) > 0) {
    append_buf(buffer, "++");
  }
  if ((state & DCC50S_CHARGE_STATE_FLOAT) > 0) {
    append_buf(buffer, "~");
  }
  if ((state & DCC50S_CHARGE_STATE_LIMITED) > 0) {
    append_buf(buffer, "+");
  }
}

void dcc50s_get_temperatures(char* buffer, uint16_t state) {
  uint16_t internal_temp = (state >> 8);
  uint16_t aux_temp = (state & 0xFF);

  sprintf(buffer, "C %d*C B %d*C", internal_temp, aux_temp);
}

void update_page_overview() {
  char line[32];

  uint16_t aux_soc = modbus_registers[DCC50S_REG_AUX_SOC];
  float aux_v = (float)modbus_registers[DCC50S_REG_AUX_V] / 10.f;   // 0.1v
  float aux_a = (float)modbus_registers[DCC50S_REG_AUX_A] / 100.f;  // 0.01a

  uint16_t charge_state = modbus_registers[DCC50S_REG_CHARGE_STATE];
  uint16_t temperature = modbus_registers[DCC50S_REG_TEMPERATURE];

  dcc50s_get_charge_status((char*)&line, charge_state);
  display_draw_title(line, 0, 0);

  sprintf((char*)&line, "%d%%", aux_soc); 
  display_draw_title(line, 128 - 56, 28);

  sprintf((char*)&line, "+%.*fA", 2, aux_a);
  display_draw_text(line, 0, 24);
  sprintf((char*)&line, "%.*fV", 1, aux_v);
  display_draw_text(line, 0, 36);

  dcc50s_get_temperatures((char*)&line, temperature);
  display_draw_text(line, 0, 50);
}

void update_page_solar() {
  char line[32];

  uint16_t sol_a = modbus_registers[DCC50S_REG_SOL_A];
  uint16_t sol_v = modbus_registers[DCC50S_REG_SOL_V];
  uint16_t sol_w = modbus_registers[DCC50S_REG_SOL_W];

  sprintf((char*)&line, "%dA %dV %dW", sol_a, sol_v, sol_w);
  display_draw_title("Solar", 0, 0);
  display_draw_text(line, 32, 20);
}

void update_page_altenator() {
  char line[32];

  uint16_t alt_a = modbus_registers[DCC50S_REG_ALT_A];
  uint16_t alt_v = modbus_registers[DCC50S_REG_ALT_V];
  uint16_t alt_w = modbus_registers[DCC50S_REG_ALT_W];

  sprintf((char*)&line, "%dA %dV %dW", alt_a, alt_v, alt_w);
  display_draw_title("Altenator", 0, 0);
  display_draw_text(line, 32, 20);
}

void update_page() {

  display_clear();

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
      display_draw_title("404", 0, 0);
      break;
  }

  display_update();
}

void btn_handler(uint gpio, uint32_t events) {
  static uint64_t time_handled;

#ifdef _VERBOSE
  printf("Btn @ %d is now %02x\n", gpio, events);
#endif

  if (events & 0x1) { // Low
  }
  if (events & 0x2) { // High
  }
  if (events & 0x4) { // Fall
    time_handled = time_us_64(); 
    if(time_handled >= btn_last_pressed + 5000000) // 5s
    {
      btn_last_pressed = time_handled;
      if(display_state) {
        display_turn_off();
      } else {
        display_turn_on();
      }
      display_state = !display_state;
    }
  }
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
  sleep_ms(3000);
  gpio_put(LED_PIN, 1);

  state = devices_modbus_uart_init();
  if(state != 0) {
    printf("Unable to initialise modbus: %d", state); 
    return state;
  }

  display_init();
  display_clear();
  display_update();
  gpio_put(LED_PIN, 0);

  while(1) {
    gpio_put(LED_PIN, 1);
    modbus_registers = devices_modbus_read_registers(RS485_DCC50S_ADDRESS, DCC50S_REG_START, DCC50S_REG_END);
    update_page();
    gpio_put(LED_PIN, 0);
    sleep_ms(2000);

    gpio_put(LED_PIN, 1);

  }
}

