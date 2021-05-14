#include "vanny-hub.h"

#define UART_PORT uart0
#define UART_BR 9600
#define UART_DBITS 8
#define UART_SBITS 2
#define UART_PIN_TX 0
#define UART_PIN_RX 1
#define UART_RX_TIMEOUT 3000000

#define LED_PIN 25
#define RTS_PIN 22
#define BTN_PIN 21

#define SLAVE_ADDR_DCC50S       1
#define REG_START               0x100
#define REG_AUX_SOC             0       // 0x100
#define REG_AUX_V               1
#define REG_AUX_A               2
#define REG_TEMPERATURE         3
#define REG_ALT_V               4
#define REG_ALT_A               5
#define REG_ALT_W               6
#define REG_SOL_V               7
#define REG_SOL_A               8
#define REG_SOL_W               9     // 0x109
#define REG_DAY_MIN_V           11    // 0x10B
#define REG_DAY_MAX_V           12    // 0x10C
#define REG_DAY_MAX_A           13    // 0x10D
#define REG_DAY_MAX_W           15    // 0x10F
#define REG_DAY_TOTAL_AH        17    // 0x113
#define REG_DAY_COUNT           21    // 0x115 
#define REG_CHARGE_STATE        32    // 0x120
#define REG_ERR_1               33    // 0x121
#define REG_ERR_2               34    // 0x122
#define REG_END                 35

#define CHARGE_STATE_NONE     0
#define CHARGE_STATE_SOLAR    (1 << 2)
#define CHARGE_STATE_EQ       (1 << 3)
#define CHARGE_STATE_BOOST    (1 << 4)
#define CHARGE_STATE_FLOAT    (1 << 5)
#define CHARGE_STATE_LIMITED  (1 << 6)
#define CHARGE_STATE_ALT      (1 << 7)

// For REG_ERR_1
#define ERR_TOO_COLD          (1 << 11)
#define ERR_OVERCHARGE        (1 << 10)
#define ERR_RPOLARITY         (1 << 9)
#define ERR_ALT_OVER_VOLT      (1 << 8)
#define ERR_ALT_OVER_AMP       (1 << 5)

// For REG_ERR_2
#define ERR_AUX_DISCHARGED     (1 << 0)
#define ERR_AUX_OVER_VOLT      (1 << 1)
#define ERR_AUX_UNDER_VOLT     (1 << 2)
#define ERR_CTRL_OVERHEAT      (1 << 5)
#define ERR_AUX_OVERHEAT       (1 << 6)
#define ERR_SOL_OVER_AMP       (1 << 7)
#define ERR_SOL_OVER_VOLT      (1 << 9)
#define ERR_SOL_RPOLARITY      (1 << 12)

typedef enum {
  Overview,
  Altenator,
  Solar,
  PageContentsCount,
} PageContents_t;

static PageContents_t current_page;
static ModbusMaster master;
static uint64_t btn_last_pressed;
static uint16_t* dcc50s_registers;

// clean this garbage hack
static bool ready_to_parse = false;
static uint16_t frame_length = 0;
static char frame[255];

const uint16_t char_delay = ((1 / (float)UART_BR) * 11.f * 1.5f) * 1000000; // 1.5t calc_delay_us(1.5f);
const uint16_t frame_timeout =((1 / (float)UART_BR) * 11.f * 3.5f) * 1000000; // 3.5t calc_delay_us(3.5f);

void on_uart_rx() {
  while(uart_is_readable_within_us(UART_PORT, frame_timeout)) {
    if(frame_length >= 255) {
      printf("RX frame buffer overflow!\n");
      break;
    }

    frame[frame_length] = uart_getc(UART_PORT);
    frame_length++;
  }
  ready_to_parse = true;
}

static inline int uart_modbus_init() {
  uint16_t state;

  uart_init(UART_PORT, UART_BR);
  
  gpio_set_function(UART_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(UART_PIN_RX, GPIO_FUNC_UART);
  
  state = uart_set_baudrate(UART_PORT, UART_BR);
  printf("Actual baudrate set to: %d\n", state);

  uart_set_hw_flow(UART_PORT, false, false);
  uart_set_format(UART_PORT, UART_DBITS, UART_SBITS, UART_PARITY_NONE);
  uart_set_fifo_enabled(UART_PORT, true);

  int UART_IRQ = UART_PORT == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);

  uart_set_irq_enables(UART_PORT, true, false);

  state = modbusMasterInit(&master);
  if(state != MODBUS_OK) {
    printf("Unable to initialise modbus: %d", state); 
    return -1;
  }

  return 0;
}

static inline void set_rts(bool on) {
  gpio_put(RTS_PIN, on ? 1 : 0);
}

void send_request() {
  int i;

  for(i = 0; i < master.request.length; i++){
    printf("%02x ", master.request.frame[i]);
  }
  // clear response
  for(i = 0; i < 255; i++) {
    frame[i] = 0;
  }

  set_rts(true);

  // pre-delay 5-10ms
  busy_wait_us(5000);

  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(UART_PORT, master.request.frame[i]);
    busy_wait_us(char_delay);
  }
  set_rts(false);
  frame_length = 0;
  ready_to_parse = false;

  printf(" Sent.\n");
}

inline void build_request(uint16_t address, uint16_t count) { 
  uint8_t resp;
  resp = modbusBuildRequest03(&master, SLAVE_ADDR_DCC50S, address, count);
  if(resp != MODBUS_OK) {
    printf("Unable to build request: %d\n", resp);
    switch (resp) {
      case MODBUS_ERROR_BUILD:
        printf("build error: %d\n", master.buildError);
      default:
        printf("unable to build request!\n");
    }
  }
}

uint16_t* parse_response() {
  int i;
  ModbusError err;

  printf("RX: ");
  for(i = 0; i < frame_length; i++) {
    printf("%02x ", frame[i]);
  }
  printf("\n");

  master.response.frame = frame;
  master.response.length = frame_length;

  err = modbusParseResponse(&master);
  if(err != MODBUS_OK) {
    // TODO
    switch(err) {
      case MODBUS_ERROR_EXCEPTION:
        printf("Exception: %d\n", master.exception.code);
        break;
      case MODBUS_ERROR_PARSE:
        printf("Parse exception: %d\n", master.parseError);
        break;
      case MODBUS_ERROR_BUILD:
        printf("Build exception: %d\n", master.buildError);
        break;
      default:
        printf("Unhandled exception: %d\n", err);
    }
    return NULL;
  }

  switch(master.data.type){
    case MODBUS_HOLDING_REGISTER:
    case MODBUS_INPUT_REGISTER:
    case MODBUS_DISCRETE_INPUT:
      printf("Register %x (%d): ", master.data.index, master.data.count);
      for(i = 0; i < master.data.length; i++){
        printf("%02x ", master.data.regs[i]);
      }
      printf("\n");
      return master.data.regs;
    
    default:
      printf("Unable to parse response of type: %d", master.data.type);
      return NULL;
  }
}

uint16_t* read_registers(uint16_t address, uint16_t count) {
  build_request(address, count);
  send_request();

  while(!ready_to_parse){
    busy_wait_us(frame_timeout);
  }

  return parse_response();
}

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

void get_charge_status(char* buffer, uint16_t state) {
  bool charging = false;

  if ((state & CHARGE_STATE_SOLAR) > 0) {
    append_buf(buffer, "Sol ");
    charging = true;
  }
  if ((state & CHARGE_STATE_ALT) > 0) {
    append_buf(buffer, "Alt ");
    charging = true;
  }
  if(!charging){
    sprintf(buffer, "No charge");
    return;
  }

  if ((state & CHARGE_STATE_EQ) > 0) {
    append_buf(buffer, "=");
  }
  if ((state & CHARGE_STATE_BOOST) > 0) {
    append_buf(buffer, "++");
  }
  if ((state & CHARGE_STATE_FLOAT) > 0) {
    append_buf(buffer, "~");
  }
  if ((state & CHARGE_STATE_LIMITED) > 0) {
    append_buf(buffer, "+");
  }
}

void get_temperatures(char* buffer, uint16_t state) {
  uint16_t internal_temp = (state >> 8);
  uint16_t aux_temp = (state << 8) >> 8;

  sprintf(buffer, "C %d*C B %d*C", internal_temp, aux_temp);
}

void update_page_overview() {
  char line[32];

  uint16_t aux_soc = dcc50s_registers[REG_AUX_SOC];
  float aux_v = (float)dcc50s_registers[REG_AUX_V] / 10.f;   // 0.1v
  float aux_a = (float)dcc50s_registers[REG_AUX_A] / 100.f;  // 0.01a

  uint16_t charge_state = dcc50s_registers[REG_CHARGE_STATE];
  uint16_t temperature = dcc50s_registers[REG_TEMPERATURE];

  get_charge_status(&line, charge_state);
  display_draw_title(line, 0, 0);

  sprintf(&line, "%d%%", aux_soc); 
  display_draw_title(line, 128 - 56, 28);

  sprintf(&line, "+%.*fA", 2, aux_a);
  display_draw_text(line, 0, 24);
  sprintf(&line, "%.*fV", 1, aux_v);
  display_draw_text(line, 0, 36);

  get_temperatures(&line, temperature);
  display_draw_text(line, 0, 50);
}

void update_page_solar() {
  char line[32];

  uint16_t sol_a = dcc50s_registers[REG_SOL_A];
  uint16_t sol_v = dcc50s_registers[REG_SOL_V];
  uint16_t sol_w = dcc50s_registers[REG_SOL_W];

  sprintf(&line, "%dA %dV %dW", sol_a, sol_v, sol_w);
  display_draw_title("Solar", 0, 0);
  display_draw_text(line, 32, 20);
}

void update_page_altenator() {
  char line[32];

  uint16_t alt_a = dcc50s_registers[REG_ALT_A];
  uint16_t alt_v = dcc50s_registers[REG_ALT_V];
  uint16_t alt_w = dcc50s_registers[REG_ALT_W];

  sprintf(&line, "%dA %dV %dW", alt_a, alt_v, alt_w);
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

  printf("Btn @ %d is now %02x\n", gpio, events);

  if (events & 0x1) { // Low
  }
  if (events & 0x2) { // High
  }
  if (events & 0x4) { // Fall
    time_handled = time_us_64(); 
    if(time_handled >= btn_last_pressed + 5000000) // 5s
    {
      btn_last_pressed = time_handled;
      printf("Turn off the screen!\n");
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
  stdio_init_all();
 
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_handler);
  current_page = Overview;
  btn_last_pressed = time_us_64();

  gpio_init(RTS_PIN);
  gpio_set_dir(RTS_PIN, GPIO_OUT);
  set_rts(false);

  // wait for host... (should be a better way)
  gpio_put(LED_PIN, 0);
  sleep_ms(3000);
  gpio_put(LED_PIN, 1);

  uart_modbus_init();
  display_init();
  display_clear();
  display_update();
  gpio_put(LED_PIN, 0);

  while(1) {
    gpio_put(LED_PIN, 1);
    dcc50s_registers = read_registers(REG_START, 1);
    update_page();
    gpio_put(LED_PIN, 0);
    sleep_ms(4000);
  }
}

