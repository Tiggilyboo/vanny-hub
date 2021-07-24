#include <stdio.h>
#include <math.h>

#include <pico/stdlib.h>
#include <hardware/irq.h>

#include "display/display.h"

#define _OFFLINE_TESTING
#define _VERBOSE

#define LED_PIN 25
#define BTN_PIN 21

//#define EPD_UPDATE_PARTIAL
#define EPD_FULL_REFRESH_AFTER  3
#define MAX_STATS_HISTORY 168

#define RS485_DCC50S_ADDRESS    1
#define RS485_LFP12S_ADDRESS_A  2
#define RS485_LFP12S_ADDRESS_B  3

#define RS232_RVR40_ADDRESS     1

// TODO: Until we read proper Ah value from unit addresses above...
#define LFP12S_AH 200
  
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

#ifdef _OFFLINE_TESTING
uint32_t seed = 22925;
uint32_t rand() {
   return seed = (seed * 1103515245 + 12345) & ((1U << 31) - 1);
}
#endif
