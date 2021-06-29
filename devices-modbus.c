#include "devices-modbus.h"

const uint16_t char_delay = ((1 / (float)RS485_BR) * 11.f * 1.5f) * 1000000; // 1.5t calc_delay_us(1.5f);
const uint16_t frame_timeout =((1 / (float)RS485_BR) * 11.f * 3.5f) * 1000000; // 3.5t calc_delay_us(3.5f);

// clean this garbage hack
static bool ready_to_parse = false;
static uint16_t frame_length = 0;
static char frame[255];

inline static void set_rts(bool on) {
  gpio_put(RS485_RTS_PIN, on ? 1 : 0);
}

int devices_modbus_init() {
  gpio_init(RS485_RTS_PIN);
  gpio_set_dir(RS485_RTS_PIN, GPIO_OUT);
  set_rts(false);

  return 0;
}

void on_uart_rx() {
  while(1) {
    if(frame_length >= 255) {
      printf("RX frame buffer overflow!\n");
      break;
    }

    frame[frame_length] = uart_getc(RS485_PORT);
    frame_length++;

    if(!uart_is_readable_within_us(RS485_PORT, frame_timeout)) {
      break;
    }
  }

  ready_to_parse = true;
}

int devices_modbus_uart_init() {
  uint16_t state;

  uart_init(RS485_PORT, RS485_BR);
  
  gpio_set_function(RS485_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RS485_PIN_RX, GPIO_FUNC_UART);
  
  state = uart_set_baudrate(RS485_PORT, RS485_BR);
  printf("Actual baudrate set to: %d\n", state);

  uart_set_hw_flow(RS485_PORT, false, false);
  uart_set_format(RS485_PORT, RS485_DBITS, RS485_SBITS, UART_PARITY_NONE);
  uart_set_fifo_enabled(RS485_PORT, true);

  int RS485_IRQ = RS485_PORT == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_exclusive_handler(RS485_IRQ, on_uart_rx);
  irq_set_enabled(RS485_IRQ, true);

  uart_set_irq_enables(RS485_PORT, true, false);

  state = modbusMasterInit(&master);

  return state;
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

  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(RS485_PORT, master.request.frame[i]);
    busy_wait_us(char_delay);
  }
  set_rts(false);
  frame_length = 0;
  ready_to_parse = false;

  printf(" Sent.\n");
}

void build_request(uint8_t unit, uint16_t address, uint16_t count) { 
  uint8_t resp;
  resp = modbusBuildRequest03(&master, unit, address, count);
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

inline static void flush_rx() {
  printf("Flushing... ");

  while(uart_is_readable(RS485_PORT)) {
    printf("%02x", uart_getc(RS485_PORT)); 
  }
  printf("\n");
}

#ifndef _OFFLINE_TESTING

uint16_t* devices_modbus_read_registers(uint8_t unit, uint16_t address, uint16_t count) {
  uint32_t timeout;

  build_request(unit, address, count);

  flush_rx();
  send_request();
  flush_rx();

  while(!ready_to_parse){
    busy_wait_us(frame_timeout);
    timeout += frame_timeout;

    if(timeout > RS485_RX_TIMEOUT) {
      printf("Timeout.\n");
      return NULL;
    }
  }

  return parse_response();
}

#else 

static char dcc50s_test_frame[] = {
   0x64,0x00,0x8e,0x00,0x00,0x14,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x91,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
uint16_t* devices_modbus_read_registers(uint8_t unit, uint16_t address, uint16_t count) {
  return &dcc50s_test_frame;
}

#endif
