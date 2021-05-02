#include <stdio.h>

#include <pico/stdlib.h>

#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/master.h>

#define LED_PIN 25
#define UART_BR 9600
#define UART_DBITS 8
#define UART_SBITS 2
#define RTS_PIN 22

ModbusMaster master;
uint8_t resp;

void debug_master()
{
	int i;
	printf( "\n" );

  if (master.data.type != 0 || master.data.count > 0){
	  printf( "Received data: slave: %02x, addr: %02x, count: %d, type: %02x\n",
		  master.data.address, master.data.index, master.data.count, master.data.type );
	  printf( "\tValues:" );
  }
	switch ( master.data.type )
	{
		case MODBUS_HOLDING_REGISTER:
		case MODBUS_INPUT_REGISTER:
			for ( i = 0; i < master.data.count; i++ )
				printf( " %02x", master.data.regs[i] );
			break;

		case MODBUS_COIL:
		case MODBUS_DISCRETE_INPUT:
			for ( i = 0; i < master.data.count; i++ )
				printf( " %02x", modbusMaskRead( master.data.coils, master.data.length, i ) );
			break;
	}

  if(master.request.length > 0){
    printf( "\nRequest:" );
    for ( i = 0; i < master.request.length; i++ )
      printf( " %02x", master.request.frame[i] );
    printf( "\n" );
  }

  if(master.response.length > 0){
    printf( "Response:" );
    for ( i = 0; i < master.response.length; i++ )
      printf( " %02x", master.response.frame[i] );
    printf( "\n" );
  }

  if(resp != 0)
    printf("Code: %d\n", resp);
}

void destroy_modbus() {
  modbus_close(&master);
  modbus_free(&master);
}

void init_modbus() {
  uart_init(uart0, UART_BR);
  
  gpio_init(0);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_init(1);
  gpio_set_function(1, GPIO_FUNC_UART);

  uart_set_format(uart0, UART_DBITS, UART_SBITS, UART_PARITY_NONE);
  uart_set_hw_flow(uart0, false, true);
  uart_set_fifo_enabled(uart0, true);

  int actual = uart_set_baudrate(uart0, UART_BR);
  printf("Actual baudrate set to: %d\n", actual);

  modbusMasterInit(&master);
}

inline void set_rts(bool on) {
  gpio_put(RTS_PIN, on ? 1 : 0);
}

// Example: 0f03020000d185 
void read_response() {
  unsigned int length = 0;
  unsigned short expected_length = 0;
  unsigned char buf[255];
    
  printf("Waiting on response... ");
  uart_is_readable_within_us(uart0, 500000);
  if(!uart_is_readable(uart0)) {
    printf("RX empty.");
    return;
  }

  printf("Got: ");
  while(uart_is_readable(uart0)) {
    buf[length] = uart_getc(uart0);
    printf("%02x ", buf[length]);
    if(length >= 255){
      printf("Read buffer overflow!");
      break;
    }
    length++;
  
    // char delay
    busy_wait_us(5000);
  }

  printf("\n");
}

void flush_rx() {
  char c;
  printf("Flushing RX: ");
  if(!uart_is_readable(uart0)) {
    printf("uart0 is not readable...");
    return;
  }
  while(uart_is_readable(uart0)) {
    c = uart_getc(uart0);
    printf("%02x ", c);
  }
  printf("\n");
}

void send_request() {
  flush_rx();  

  printf("Sending Request: ");
  int i;
  for(i = 0; i < master.request.length; i++){
    printf("%02x ", master.request.frame[i]);
  }
  printf("\n");

  while(!uart_is_writable(uart0))
    busy_wait_us(5000);

  set_rts(true);
  for(i = 0; i < master.request.length; i++){
    uart_putc_raw(uart0, master.request.frame[i]);
    busy_wait_us(5000);
  }
  set_rts(false);

  printf("Sent.\n");

  flush_rx();

  debug_master();
}

int main() {
  stdio_init_all();
 
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  gpio_init(RTS_PIN);
  gpio_set_dir(RTS_PIN, GPIO_OUT);
  set_rts(false);

  // wait for host... (should be a better way)
  sleep_ms(3000);
  init_modbus();

  resp = modbusBuildRequest03(&master, 0x0F, 0, 1);
  if(resp != MODBUS_OK) {
    printf("Unable to build request: %d\n", resp);
    switch (resp) {
      case MODBUS_ERROR_BUILD:
        printf("build error: %d\n", master.buildError);
    }
  }

  while(1) {
    if(uart_is_enabled(uart0)) {
      send_request();
    
      debug_master();
      read_response();

    } else {
      printf("uart0 is not enabled...");
    }

    gpio_put(LED_PIN, 1);
    sleep_ms(5000);
    gpio_put(LED_PIN, 0);
  }

  destroy_modbus();
}

