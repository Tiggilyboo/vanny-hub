#include "devices-modbus.h"

const uint16_t char_delay = ((1 / (float)RS485_BR) * 11.f * 1.5f) * 1000000; // 1.5t calc_delay_us(1.5f);
const uint16_t frame_timeout =((1 / (float)RS485_BR) * 11.f * 3.5f) * 1000000; // 3.5t calc_delay_us(3.5f);

// clean this garbage hack
static bool ready_to_parse = false;
static uint16_t frame_length = 0;
static char frame[255];

inline static void set_rts(bool on) {
  gpio_put(RS485_PIN_RTS, on ? 1 : 0);
}

int devices_modbus_init() {
  gpio_init(RS485_PIN_RTS);
  gpio_set_dir(RS485_PIN_RTS, GPIO_OUT);
  set_rts(false);

  return 0;
}

void on_rs485_rx() {
  while(1) {
    if(frame_length >= 255) {
      printf("RS485 RX frame buffer overflow!\n");
      return;
    }

    frame[frame_length] = uart_getc(RS485_PORT);
    frame_length++;

    if(!uart_is_readable_within_us(RS485_PORT, frame_timeout)){
      break;
    }
  }

  ready_to_parse = true;
}

void on_rs232_rx() {
  if(frame_length >= 255) {
    printf("RS232 RX frame buffer overflow\n");
    return;
  }

  frame[frame_length] = uart_getc(RS232_PORT);
  frame_length++;

  ready_to_parse = true;
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

  int RS485_IRQ = RS485_PORT == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_exclusive_handler(RS485_IRQ, on_rs485_rx);
  irq_set_enabled(RS485_IRQ, true);

  uart_set_irq_enables(RS485_PORT, true, false);
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

  int RS232_IRQ = RS232_PORT == uart0 ? UART0_IRQ : UART1_IRQ;
  irq_set_exclusive_handler(RS232_IRQ, on_rs232_rx);
  irq_set_enabled(RS232_IRQ, true);

  uart_set_irq_enables(RS232_PORT, true, false);
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

  // clear response
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

void parse_response(uint16_t* data) {
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
    return;
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
      
      memcpy(data, master.data.regs, master.data.length);
    
    default:
      printf("Unable to parse response of type: %d", master.data.type);
      return;
  }
}

inline static void flush_rx(uart_inst_t* inst) {
  printf("Flushing uart%d... ", uart_get_index(inst));

  while(uart_is_readable(inst)) {
    printf("%02x", uart_getc(inst)); 
  }
  printf("\n");
}

#ifndef _OFFLINE_TESTING

void devices_modbus_read_registers(uart_inst_t* inst, uint8_t unit, uint16_t address, uint16_t count, uint16_t* returned_data) {
  uint32_t timeout;

  build_request(unit, address, count);

  flush_rx(inst);
  send_request(inst);

  if(inst == RS485_PORT) {
    flush_rx(inst);
  }

  while(!ready_to_parse){
    busy_wait_us(frame_timeout);
    timeout += frame_timeout;

    if(timeout > UART_RX_TIMEOUT) {
      printf("Timeout.\n");
      return;
    }
  }

  parse_response(returned_data);
}

#else 

static char dcc50s_test_frame[] = {
   0x64,0x00,0x8e,0x00,0x00,0x14,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x91,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
void devices_modbus_read_registers(uint8_t unit, uint16_t address, uint16_t count, uint16_t* returned_data) {
  memcpy(returned_data, &dcc50s_test_frame, sizeof(dcc50s_test_frame) / sizeof(dcc50s_test_frame[0]));
}

#endif
