#include <stdio.h>

#include <pico/stdlib.h>
#include <hardware/irq.h>
#include <hardware/uart.h>

#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/master.h>

#define RS485_RTS_PIN 22

#define RS485_PORT uart0
#define RS485_BR 9600
#define RS485_DBITS 8
#define RS485_SBITS 1
#define RS485_PIN_TX 0
#define RS485_PIN_RX 1
#define RS485_RX_TIMEOUT 3000000

#define DCC50S_REG_START               0x100
#define DCC50S_REG_AUX_SOC             0       // 0x100
#define DCC50S_REG_AUX_V               1
#define DCC50S_REG_AUX_A               2
#define DCC50S_REG_TEMPERATURE         3
#define DCC50S_REG_ALT_V               4
#define DCC50S_REG_ALT_A               5
#define DCC50S_REG_ALT_W               6
#define DCC50S_REG_SOL_V               7
#define DCC50S_REG_SOL_A               8
#define DCC50S_REG_SOL_W               9     // 0x109
#define DCC50S_REG_DAY_MIN_V           11    // 0x10B
#define DCC50S_REG_DAY_MAX_V           12    // 0x10C
#define DCC50S_REG_DAY_MAX_A           13    // 0x10D
#define DCC50S_REG_DAY_MAX_W           15    // 0x10F
#define DCC50S_REG_DAY_TOTAL_AH        17    // 0x113
#define DCC50S_REG_DAY_COUNT           21    // 0x115 
#define DCC50S_REG_CHARGE_STATE        32    // 0x120
#define DCC50S_REG_ERR_1               33    // 0x121
#define DCC50S_REG_ERR_2               34    // 0x122
#define DCC50S_REG_END                 35

#define DCC50S_CHARGE_STATE_NONE     0
#define DCC50S_CHARGE_STATE_SOLAR    (1 << 2)
#define DCC50S_CHARGE_STATE_EQ       (1 << 3)
#define DCC50S_CHARGE_STATE_BOOST    (1 << 4)
#define DCC50S_CHARGE_STATE_FLOAT    (1 << 5)
#define DCC50S_CHARGE_STATE_LIMITED  (1 << 6)
#define DCC50S_CHARGE_STATE_ALT      (1 << 7)

// For DCC50S_REG_ERR_1
#define DCC50S_ERR_TOO_COLD          (1 << 11)
#define DCC50S_ERR_OVERCHARGE        (1 << 10)
#define DCC50S_ERR_RPOLARITY         (1 << 9)
#define DCC50S_ERR_ALT_OVER_VOLT      (1 << 8)
#define DCC50S_ERR_ALT_OVER_AMP       (1 << 5)

// For DCC50S_REG_ERR_2
#define DCC50S_ERR_AUX_DISCHARGED     (1 << 0)
#define DCC50S_ERR_AUX_OVER_VOLT      (1 << 1)
#define DCC50S_ERR_AUX_UNDER_VOLT     (1 << 2)
#define DCC50S_ERR_CTRL_OVERHEAT      (1 << 5)
#define DCC50S_ERR_AUX_OVERHEAT       (1 << 6)
#define DCC50S_ERR_SOL_OVER_AMP       (1 << 7)
#define DCC50S_ERR_SOL_OVER_VOLT      (1 << 9)
#define DCC50S_ERR_SOL_RPOLARITY      (1 << 12)

static ModbusMaster master;

int devices_modbus_init();
int devices_modbus_uart_init();
uint16_t* devices_modbus_read_registers(uint8_t unit, uint16_t address, uint16_t count);
