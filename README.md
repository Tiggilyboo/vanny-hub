# vanny-hub

![Hub](https://i.imgur.com/v27shz1.jpg)

## What?
My van's embedded monitoring platform, using Modbus over RS-485 and RS-232 on the Raspberry Pi Pico microcontroller.

This is a DIY open source project of my own creation, and as such take no responsibility for this code...

## Features

1. Interface with a range of Renogy Products
  a) Renogy Rover 40A (RVR40) over Modbus RTU 232
  b) Renogy Smart Lithium Battery (LFP100S) over Modbus RTU 485
  c) Renogy DC-DC Battery Charger (DCC50S) over Modbus RTU 485
2. Display information from each of these products on a WaveShare 2.9" E-Ink display
3. Display statistics over time (Rolling average for the hour, and after 48 hours, 7 days).

## Configuration

- `device-lfp10s.h`: Register mappings for the LFP100S battery
- `device-dcc50s.h`: Register mappings for the DCC50S unit
- `device-rvr40.h`: Register mappings for the RVR40 controller

### Pinouts

`devices-modbus.h` contains the various pinouts for the Raspberry Pi Pico UART pins. It makes use of both UARTs (one for modbus 485, and one for 232).

```c
#define UART_RX_TIMEOUT 1000000

#define RS485_PORT      uart0
#define RS485_BR        9600
#define RS485_DBITS     8
#define RS485_SBITS     1
#define RS485_PIN_TX    0
#define RS485_PIN_RX    1
#define RS485_PIN_RTS   22

#define RS232_PORT      uart1
#define RS232_BR        9600
#define RS232_DBITS     8
#define RS232_SBITS     1
#define RS232_PIN_TX    4
#define RS232_PIN_RX    5
```

`vanny-hub.h` contains the modbus node configuration, as well as refresh rates and statistic storing rates, as well as the GPIO pin used for changing the screen view.

```c
#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

//#define EPD_UPDATE_PARTIAL
#define EPD_FULL_REFRESH_AFTER    3
#define EPD_REFRESH_RATE_MS       60000

#define RS485_DCC50S_ADDRESS      0x01
#define RS485_LFP100S_ADDRESS     0xf7
#define RS232_RVR40_ADDRESS       0x01

#define STATS_MAX_HISTORY         168
#define STATS_UPDATE_ROLLING_MS   10000     // (secondly)
#define STATS_UPDATE_HISTORIC_MS  3600000  // (hourly)
```

And finally, the display SPI pinout configuration is defined in `display/display-ws-eink.h`

```c
#define SPI_PORT      spi1
#define SPI_BD        4000000

#define SPI_PIN_DC    8
#define SPI_PIN_CS    9
#define SPI_PIN_CLK   10
#define SPI_PIN_DIN   11 // Aka. MOSI
#define SPI_PIN_RST   12
#define SPI_PIN_BSY   13
```

## Setup

This project uses the Raspberry Pi Pico, and as such uses it's C API as well as a modbus library (stored as git submodules). This requires the use of:

```bash
$ git submodule sync
$ git submodule update
```

Currently I have only written bash scripts for building and deploying the binaries (Ubuntu / Debian or Arch with minor adjustments of paths). But to use them, simply:

```bash
$ ./build.sh
$ ./deploy.sh
```
