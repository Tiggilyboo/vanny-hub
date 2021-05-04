#include "vanny-hub.h"

#define LED_PIN 25
#define UART_BR 9600
#define UART_DBITS 8
#define UART_SBITS 2
#define RTS_PIN 22

#define SLAVE_ADDR_DCC50S       0x0F
#define REG_DCC50S_AUX_SOC      0x100
#define REG_DCC50S_AUX_V        0x101
#define REG_DCC50S_MAX          0x102
#define REG_DCC50S_TEMPERATURE  0x103
#define REG_DCC50S_ALT_V        0x104
#define REG_DCC50S_ALT_A        0x105
#define REG_DCC50S_ALT_W        0x106
#define REG_DCC50S_SOL_V        0x107
#define REG_DCC50S_SOL_A        0x108
#define REG_DCC50S_SOL_W        0x109
#define REG_DCC50S_DAY_COUNT    0x10F
#define REG_DCC50S_CHARGE_STATE 0x114

typedef enum {
  Overview,
  Altenator,
  Solar,
} PageContents_t;

static PageContents_t current_page;
static ModbusMaster master;

int init_modbus() {
  uint16_t state;

  uart_init(uart0, UART_BR);
  
  gpio_init(0);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_init(1);
  gpio_set_function(1, GPIO_FUNC_UART);

  uart_set_format(uart0, UART_DBITS, UART_SBITS, UART_PARITY_NONE);
  uart_set_hw_flow(uart0, false, true);
  uart_set_fifo_enabled(uart0, true);

  state = uart_set_baudrate(uart0, UART_BR);
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
  unsigned int length = 0;
  uint8_t frame[255];
    
  printf("Waiting on response... ");
  uart_is_readable_within_us(uart0, 50000);
  if(!uart_is_readable(uart0)) {
    printf("RX empty.");
    return;
  }

  printf("Got: ");
  while(uart_is_readable(uart0)) {
    frame[length] = uart_getc(uart0);
    printf("%02x ", frame[length]);
    if(length >= 255){
      printf("RX frame buffer overflow!");
      break;
    }
    length++;
  
    // char delay
    busy_wait_us(5000);
  }
  printf("\n");

  // Set response frame
  master.response.frame = frame;
  master.response.length = length;
}

void flush_rx() {
  char c;
  printf("Flushing RX: ");
  if(!uart_is_readable(uart0)) {
    printf("empty.");
  }
  while(uart_is_readable(uart0)) {
    c = uart_getc(uart0);
    printf("%02x ", c);
  }
  printf("\n");
}

static inline void send_request() {
  flush_rx();  

  printf("Sending ");
  int i;
  for(i = 0; i < master.request.length; i++){
    printf("%02x ", master.request.frame[i]);
  }

  while(!uart_is_writable(uart0))
    busy_wait_us(5000);

  set_rts(true);
  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(uart0, master.request.frame[i]);
    busy_wait_us(5000);
  }
  set_rts(false);
  printf(" Sent.\n");

  flush_rx();
}

static inline void build_request(uint16_t address, uint16_t count) { 
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

static inline uint16_t* parse_response() {
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

    /*case MODBUS_COIL:
      printf("Coil %x (%d): ", master.data.index, master.data.count);
      for(i = 0; i < master.data.length; i++){
        printf( "%x ", modbusMaskRead(master.data.coils, master.data.length, i) );
      }
      printf("\n");
      return master.data.coils;
    */
    default:
      printf("Unable to parse response of type: %d", master.data.type);
      return NULL;
  }
}

uint16_t read_register(uint16_t address) {
  build_request(address, 1);
  send_request();
  read_response();
  return *parse_response();
}

inline static void count_chars_in_value(uint16_t v, int* c) {
  char buf[32];

  sprintf(buf, "%d", v);
  while(buf[*c] != '\0')
    *c++;
}

void update_page() {
  char line[32];
  uint16_t reg[3];
  int len;

  display_clear();

  switch(current_page) {
    case Overview: 
      reg[0] = read_register(REG_DCC50S_AUX_SOC) / 100;
      reg[1] = read_register(REG_DCC50S_AUX_V) / 100;
      reg[2] = read_register(REG_DCC50S_TEMPERATURE);
      display_draw_text("< ALT", 0, 0);
      display_draw_text("SOL >", 128 - 40, 0);

      sprintf(&line, "%d %", reg[0]);
      display_draw_title(line, 32, 24);

      sprintf(&line, "%d V", reg[1]);
      display_draw_text(line, 0, 42);

      sprintf(&line, "%d degC", reg[2]);
      display_draw_text(line, 128 - 32, 42);
      break;

    case Altenator:
      reg[0] = read_register(REG_DCC50S_ALT_A) / 100;
      reg[1] = read_register(REG_DCC50S_ALT_V) / 100;
      reg[2] = read_register(REG_DCC50S_ALT_W) / 100;
      sprintf(&line, "%dA %dV %dW", reg[0], reg[1], reg[2]);
      display_draw_title("Altenator", 0, 0);
      display_draw_text(line, 32, 20);
      break;

    case Solar:
      reg[0] = read_register(REG_DCC50S_SOL_A) / 100;
      reg[1] = read_register(REG_DCC50S_SOL_V) / 100;
      reg[2] = read_register(REG_DCC50S_SOL_W) / 100;
      sprintf(&line, "%dA %dV %dW", reg[0], reg[1], reg[2]);
      display_draw_title("Solar", 0, 0);
      display_draw_text(line, 32, 20);
      break;

    default:
      return;
  }

  display_update();
}

int main() {
  int state;

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

  state = init_modbus();
  display_init();

  gpio_put(LED_PIN, 0);

  while(1) {
    gpio_put(LED_PIN, 1);
    update_page();
    sleep_ms(1000);

    gpio_put(LED_PIN, 0);
    sleep_ms(9000);
  }
}

