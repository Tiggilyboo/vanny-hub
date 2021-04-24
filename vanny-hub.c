#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25
#define UART_BR 9600
#define UART_DBITS 8
#define UART_SBITS 2

void init_modbus() {
  uart_init(uart0, UART_BR);
  uart_set_format(uart0, UART_DBITS, UART_SBITS, UART_PARITY_NONE);
}

int main() {
  stdio_init_all();
  
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  // wait for host... (should be a better way)
  sleep_ms(1000);
  init_modbus();

  while(1) {
    gpio_put(LED_PIN, 1);

    if(uart_is_enabled(uart0)) {

    } else {
      printf("uart0 is not enabled...");
    }

    gpio_put(LED_PIN, 0);
    sleep_ms(500);
  }

}

