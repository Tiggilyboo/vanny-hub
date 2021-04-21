from micropython import const
from machine import Pin, I2C
from ssd1306 import SSD1306_I2C

I2C_DEVICE = const(1)

class Display:
    def poweroff(self):
        self.oled.poweroff()
    def poweron(self):
        self.oled.poweron()
        
    def contrast(self, val):
        self.oled.contrast(val) 

    def clear(self):
        self.oled.fill(0)
    
    def show(self):
        self.oled.show()
        
    def text(self, text, x, y):
        self.oled.text(text, x, y)
        
    def hline(self, y):
        line = ''
        for i in self.char_width:
            line += '_'
        self.text(line, 0, y)
    
    def __init__(self, width, height, pin_scl, pin_sda):
        i2c_dev = I2C(I2C_DEVICE, scl=pin_scl, sda=pin_sda,freq=200000)
        i2c_addr = [hex(ii) for ii in i2c_dev.scan()]
        if i2c_addr == []:
            print('No i2c device found')
            sys.exit()
        else:
            print("I2C Address      : {}".format(i2c_addr[0]))
            print("I2C Configuration: {}".format(i2c_dev))

        self.width = const(width)
        self.height = const(height)
        self.char_width = const(int(width / 8))
        self.i2c_dev = i2c_dev
        self.oled = SSD1306_I2C(width, height, i2c_dev)
        self.clear()