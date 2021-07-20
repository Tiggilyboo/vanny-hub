
/* Display based upon Waveshare epd2in13b_V3
 * https://github.com/waveshare/e-Paper/tree/master/Arduino/epd2in13b_V3
 */

#define SPI_PORT      spi1
#define SPI_BD        4000000

#define SPI_PIN_DC    8
#define SPI_PIN_CS    9
#define SPI_PIN_CLK   10
#define SPI_PIN_DIN   11 // Aka. MOSI
#define SPI_PIN_RST   12
#define SPI_PIN_BSY   13

#define DISPLAY_W     128
#define DISPLAY_H     296
#define SCREEN_W      (DISPLAY_W / 8)
#define SCREEN_H      DISPLAY_H

#define EPD_PANEL_SETTING                               0x00
#define EPD_POWER_SETTING                               0x01
#define EPD_POWER_OFF                                   0x02
#define EPD_POWER_OFF_SEQUENCE_SETTING                  0x03
#define EPD_POWER_ON                                    0x04
#define EPD_POWER_ON_MEASURE                            0x05
#define EPD_BOOSTER_SOFT_START                          0x06
#define EPD_DEEP_SLEEP                                  0x07
#define EPD_DATA_START_TRANSMISSION_1                   0x10
#define EPD_DATA_STOP                                   0x11
#define EPD_DISPLAY_REFRESH                             0x12
#define EPD_DATA_START_TRANSMISSION_2                   0x13
#define EPD_PLL_CONTROL                                 0x30
#define EPD_TEMPERATURE_SENSOR_COMMAND                  0x40
#define EPD_TEMPERATURE_SENSOR_CALIBRATION              0x41
#define EPD_TEMPERATURE_SENSOR_WRITE                    0x42
#define EPD_TEMPERATURE_SENSOR_READ                     0x43
#define EPD_VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define EPD_LOW_POWER_DETECTION                         0x51
#define EPD_TCON_SETTING                                0x60
#define EPD_TCON_RESOLUTION                             0x61
#define EPD_GET_STATUS                                  0x71
#define EPD_AUTO_MEASURE_VCOM                           0x80
#define EPD_VCOM_VALUE                                  0x81
#define EPD_VCM_DC_SETTING_REGISTER                     0x82
#define EPD_PARTIAL_WINDOW                              0x90
#define EPD_PARTIAL_IN                                  0x91
#define EPD_PARTIAL_OUT                                 0x92
#define EPD_PROGRAM_MODE                                0xA0
#define EPD_ACTIVE_PROGRAM                              0xA1
#define EPD_READ_OTP_DATA                               0xA2
#define EPD_CHECK_CODE                                  0xA5
#define EPD_POWER_SAVING                                0xE3

