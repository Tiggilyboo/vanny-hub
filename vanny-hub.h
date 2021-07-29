#include <stdio.h>
#include <math.h>

#include <pico/stdlib.h>
#include <hardware/irq.h>

#include "display/display.h"

//#define _OFFLINE_TESTING
#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

//#define EPD_UPDATE_PARTIAL
#define EPD_FULL_REFRESH_AFTER  3
#define EPD_REFRESH_RATE_MS     5000

#define RS485_DCC50S_ADDRESS    0x01
#define RS485_LFP100S_ADDRESS   0xf7

#define RS232_RVR40_ADDRESS     0x01

// TODO: Until we read proper Ah value from unit addresses above...
#define LFP12S_AH 200

#define STATS_MAX_HISTORY        168
#define STATS_UPDATE_ROLLING_MS  2000  // 60000   // (minutely)
#define STATS_UPDATE_HISTORIC_MS 25000 // 3600000  // (hourly)
  
typedef enum {
  Overview,
  Solar,
  Altenator,
  Statistics,
  PageContentsCount,
} PageContents_t;

typedef struct {
  uint8_t index;
  uint16_t aux_soc;
  uint16_t sol_w;
  uint16_t alt_w;
  uint16_t charged_ah;
  uint16_t discharged_ah;
} Statshot_t;

