#include <stdio.h>

#include <pico/stdlib.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/master.h>

#include "devices-dcc50s.h"
#include "devices-rvr40.h"
#include "devices-lfp10s.h"

#define UART_RX_TIMEOUT 1000000

#define RS485_PORT uart0
#define RS485_BR 9600
#define RS485_DBITS 8
#define RS485_SBITS 1
#define RS485_PIN_TX 0
#define RS485_PIN_RX 1
#define RS485_PIN_RTS 22

#define RS232_PORT uart1
#define RS232_BR 9600
#define RS232_DBITS 8
#define RS232_SBITS 1
#define RS232_PIN_TX 4
#define RS232_PIN_RX 5

static ModbusMaster master;

int devices_modbus_init();
int devices_modbus_uart_init();
uint8_t devices_modbus_read_registers(uart_inst_t* inst, uint8_t unit, uint16_t address, uint16_t count, uint16_t* returned_data);
