#include <stdio.h>
#include <math.h>

#include <pico/stdlib.h>
#include <hardware/irq.h>

#include "display/display.h"

#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

//#define EPD_UPDATE_PARTIAL
#define EPD_FULL_REFRESH_AFTER  3
#define EPD_REFRESH_RATE_MS     60000

#define RS485_DCC50S_ADDRESS    0x01
#define RS485_LFP100S_ADDRESS   0xf7
#define RS232_RVR40_ADDRESS     0x01

#define STATS_MAX_HISTORY        168
#define STATS_UPDATE_ROLLING_MS  10000     // (secondly)
#define STATS_UPDATE_HISTORIC_MS 3600000  // (hourly)

typedef enum {
  Overview,
  Solar,
  Alternator,
  Statistics,
  PageContentsCount,
} PageContents_t;

typedef struct {
  uint16_t index;
  float bat_soc;
  float bat_v;
  float load_w;
  uint16_t sol_w;
  uint16_t alt_w;
  uint16_t charged_ah;
  uint16_t discharged_ah;
} Statshot_t;
