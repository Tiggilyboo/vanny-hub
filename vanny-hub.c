#include "vanny-hub.h"

#define UART_PORT uart0
#define UART_BR 9600
#define UART_DBITS 8
#define UART_SBITS 2
#define LED_PIN 25
#define RTS_PIN 22

#define SLAVE_ADDR_DCC50S       0x01
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

typedef enum {
  Overview,
  Altenator,
  Solar,
} PageContents_t;

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

#define MAX_CHARGE_VOLTS 14.8f

static PageContents_t current_page;
static ModbusMaster master;

static inline int init_modbus() {
  uint16_t state;

  uart_init(UART_PORT, UART_BR);
  
  gpio_init(0);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_init(1);
  gpio_set_function(1, GPIO_FUNC_UART);

  uart_set_format(UART_PORT, UART_DBITS, UART_SBITS, UART_PARITY_NONE);
  uart_set_hw_flow(UART_PORT, false, true);
  uart_set_fifo_enabled(UART_PORT, true);

  state = uart_set_baudrate(UART_PORT, UART_BR);
  printf("Actual baudrate set to: %d\n", state);

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

static inline void read_response() {
  uint16_t length = 0;
  uint8_t frame[255];
    
  printf("Waiting on response... ");
  uart_is_readable_within_us(UART_PORT, 50000);
  if(!uart_is_readable(UART_PORT)) {
    printf("RX empty.\n");
    return;
  }

  printf("Got: ");
  while(uart_is_readable(UART_PORT)) {
    frame[length] = uart_getc(UART_PORT);
    printf("%02x ", frame[length]);
    if(length >= 255){
      printf("RX frame buffer overflow!\n");
      break;
    }
    length++;
  
    // char delay
    uart_is_readable_within_us(UART_PORT, 5000);
  }
  printf("\n");

  // Set response frame
  master.response.frame = frame;
  master.response.length = length;
}

void flush_rx() {
  char c;
  printf("Flushing RX: ");
  if(!uart_is_readable(UART_PORT)) {
    printf("empty.");
  }
  while(uart_is_readable(UART_PORT)) {
    c = uart_getc(UART_PORT);
    printf("%02x ", c);
  }
  printf("\n");
}

void send_request() {
  flush_rx();  

  printf("Sending ");
  int i;
  for(i = 0; i < master.request.length; i++){
    printf("%02x ", master.request.frame[i]);
  }

  while(!uart_is_writable(UART_PORT))
    busy_wait_us(5000);

  set_rts(true);
  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(UART_PORT, master.request.frame[i]);
    busy_wait_us(5000);
  }
  set_rts(false);
  printf(" Sent.\n");

  flush_rx();
}

void build_request(uint16_t address, uint16_t count) { 
  uint8_t resp;
  resp = modbusBuildRequest03(&master, SLAVE_ADDR_DCC50S, address, count);
  if(resp != MODBUS_OK) {
    printf("Unable to build request: %d\n", resp);
    switch (resp) {
      case MODBUS_ERROR_BUILD:
        printf("build error: %d\n", master.buildError);
    }
  }
}

uint16_t* parse_response() {
  int i;
  ModbusError err;

  err = modbusParseResponse(&master);
  if(err != MODBUS_OK) {
    // TODO
    printf("Slave threw exception: %d\n", master.exception.code);
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

uint16_t* read_register(uint16_t address, uint16_t count) {
  build_request(address, count);
  send_request();
  read_response();
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

void update_page_overview(uint16_t* reg) {
  char line[32];

  uint16_t aux_soc = reg[REG_AUX_SOC];
  float aux_v = (float)reg[REG_AUX_V] / 10.f;   // 0.1v
  float aux_a = (float)reg[REG_AUX_A] / 100.f;  // 0.01a

  uint16_t charge_state = reg[REG_CHARGE_STATE];
  uint16_t temperature = reg[REG_TEMPERATURE];

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

void update_page_solar(uint16_t* reg) {
  char line[32];

  uint16_t sol_a = reg[REG_SOL_A];
  uint16_t sol_v = reg[REG_SOL_V];
  uint16_t sol_w = reg[REG_SOL_W];

  sprintf(&line, "%dA %dV %dW", sol_a, sol_v, sol_w);
  display_draw_title("Solar", 0, 0);
  display_draw_text(line, 32, 20);
}

void update_page_altenator(uint16_t* reg) {
  char line[32];

  uint16_t alt_a = reg[REG_ALT_A];
  uint16_t alt_v = reg[REG_ALT_V];
  uint16_t alt_w = reg[REG_ALT_W];

  sprintf(&line, "%dA %dV %dW", alt_a, alt_v, alt_w);
  display_draw_title("Altenator", 0, 0);
  display_draw_text(line, 32, 20);
}

void update_page() {
  uint16_t* reg = read_register(REG_START, REG_END);

  display_clear();

  switch(current_page) {
    case Overview: 
      update_page_overview(reg);
      break;

    case Altenator:
      update_page_altenator(reg);
      break;

    case Solar:
      update_page_solar(reg);
      break;

    default:
      display_draw_title("404", 0, 0);
      break;
  }

  display_update();
}

int main() {
  stdio_init_all();
 
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  gpio_init(RTS_PIN);
  gpio_set_dir(RTS_PIN, GPIO_OUT);
  set_rts(false);

  // wait for host... (should be a better way)
  gpio_put(LED_PIN, 0);
  sleep_ms(3000);
  gpio_put(LED_PIN, 1);

  init_modbus();
  display_init();
  display_clear();
  display_update();
  gpio_put(LED_PIN, 0);

  while(1) {
    gpio_put(LED_PIN, 1);
    update_page();
    gpio_put(LED_PIN, 0);
    sleep_ms(10000);
  }
}

