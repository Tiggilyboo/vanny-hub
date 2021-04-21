# Vanny Hub
#  Monitors RS-485 components and displays the data to the OLED screen
#  Currently set up for:
#    - DCC50S DC DC charger and MPPT solar charge controller
#    - RBT100LFP12S x 2 in parallel

from machine import Pin
from display import Display
from modbus import ModbusSerial

OLED_WIDTH = const(128)
OLED_HEIGHT = const(64)
OLED_SDA = const(14)
OLED_SCL = const(15)
UART_ID = const(0)

#display = Display(OLED_WIDTH, OLiED_HEIGHT, OLED_SDA, OLED_SCL)

led = Pin(25, Pin.OUT)
led.value(0)
ctrl_flush = Pin(22, Pin.OUT)
ctrl_flush.value(0)
machine.idle()

bus = ModbusSerial(UART_ID, baudrate=9600, data_bits=8, parity=None, stop_bits=2, ctrl_pin=22)
bus.read_coils(0x00, 0x01, 1)
