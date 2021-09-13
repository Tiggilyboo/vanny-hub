#include <string.h>
#include "devices-modbus.h"

const uint16_t char_delay = ((1 / (float)RS485_BR) * 11.f * 1.5f) * 1000000; 
const uint16_t frame_timeout = ((1 / (float)RS485_BR) * 11.f * 3.5f) * 1000000; 

static char frame[255];
static uint16_t frame_length = 0;

inline static void set_rts(bool on) {
  gpio_put(RS485_PIN_RTS, on ? 1 : 0);
}

int devices_modbus_init() {
  gpio_init(RS485_PIN_RTS);
  gpio_set_dir(RS485_PIN_RTS, GPIO_OUT);
  set_rts(false);

  return 0;
}

inline static void on_rx(uart_inst_t* inst) {
  frame_length = 0;

#ifdef _VERBOSE
  printf("RX: ");
#endif
  while(uart_is_readable_within_us(inst, frame_timeout)) {
    if(frame_length >= 255) {
      printf("RS485 RX frame buffer overflow!\n");
      break;
    }

    frame[frame_length] = uart_getc(inst);
#ifdef _VERBOSE
    printf("%02x ", frame[frame_length]);
#endif
    frame_length++;
  }
  printf("\n");
}

int devices_modbus485_uart_init() {
  uint16_t state;

  uart_init(RS485_PORT, RS485_BR);
  
  gpio_set_function(RS485_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RS485_PIN_RX, GPIO_FUNC_UART);
  
  state = uart_set_baudrate(RS485_PORT, RS485_BR);
  printf("RS485 - Actual baudrate set to: %d\n", state);

  uart_set_hw_flow(RS485_PORT, false, false);
  uart_set_format(RS485_PORT, RS485_DBITS, RS485_SBITS, UART_PARITY_NONE);
  uart_set_fifo_enabled(RS485_PORT, true);

  return 0;
}

int devices_modbus232_uart_init() {
  uint16_t state;

  uart_init(RS232_PORT, RS232_BR);

  gpio_set_function(RS232_PIN_TX, GPIO_FUNC_UART);
  gpio_set_function(RS232_PIN_RX, GPIO_FUNC_UART);

  state = uart_set_baudrate(RS232_PORT, RS232_BR);
  printf("RS232 - Actual baudrate set to: %d\n", state);

  uart_set_hw_flow(RS232_PORT, false, false);
  uart_set_format(RS232_PORT, RS232_DBITS, RS232_SBITS, UART_PARITY_NONE);
  uart_set_fifo_enabled(RS232_PORT, true);

  return 0;
}

int devices_modbus_uart_init() {
  uint16_t state;

  state = devices_modbus485_uart_init();
  if(state != 0)
    return state;

  state = devices_modbus232_uart_init();
  if(state != 0)
    return state;

  state = modbusMasterInit(&master);

  return state;
}

void send_request(uart_inst_t* inst) {
  int i;

  for(i = 0; i < master.request.length; i++){
    printf("%02x ", master.request.frame[i]);
  }

  frame_length = 0;
  for(i = 0; i < 255; i++) {
    frame[i] = 0;
  }

  if(inst == RS485_PORT) {
    set_rts(true);
  }

  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(inst, master.request.frame[i]);
    busy_wait_us(char_delay);
  }

  if(inst == RS485_PORT) {
    set_rts(false);
  }

  printf(" Sent.\n");
}

void build_request(uint8_t unit, uint16_t address, uint16_t count) { 
  uint8_t resp = modbusBuildRequest03(&master, unit, address, count);
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

uint8_t parse_response(uint16_t* parsed_data) {
  int i;
  ModbusError err;

  master.response.frame = frame;
  master.response.length = frame_length;

  if(frame_length == 5) {
    if(frame[1] == 0x80 + master.request.frame[1]) {
      printf("Error code response (%d): Code %d\n", frame[1], frame[2]);
      return frame[1];
    }
  }

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
    return 0;
  }

  switch(master.data.type){
    case MODBUS_HOLDING_REGISTER:
    case MODBUS_INPUT_REGISTER:
    case MODBUS_DISCRETE_INPUT:
#ifdef _VERBOSE
      printf("Register %x (%d): ", master.data.index, master.data.count);
      for(i = 0; i < master.data.length; i++){
        printf("%02x ", master.data.regs[i]);
      }
      printf("\n");
#endif
      memcpy(parsed_data, master.data.regs, master.data.length);
      return frame[1];
    
    default:
      printf("Unable to parse response of type: %d", master.data.type);
      return 0;
  }
}

inline static void flush_rx(uart_inst_t* inst) {
#ifdef _VERBOSE
  printf("Flushing uart%d... ", uart_get_index(inst));
#endif

  while(uart_is_readable(inst)) {
    char flushed = uart_getc(inst);
#ifdef _VERBOSE
    printf("%02x", flushed); 
#endif
  }
  printf("\n");
}

uint8_t devices_modbus_read_registers(uart_inst_t* inst, uint8_t unit, uint16_t address, uint16_t count, uint16_t* returned_data) {

  build_request(unit, address, count);
  flush_rx(inst);
  send_request(inst);

  if(inst == RS485_PORT) {
    flush_rx(inst);
  }

  if(uart_is_readable_within_us(inst, UART_RX_TIMEOUT)) {
    on_rx(inst);
  } else {
    printf("Timeout.\n");
    return 0;
  }

  return parse_response(returned_data);
}

