#include "devices-modbus.h"
#include "vanny-hub.h"

// Interface State
static PageContents_t current_page;
static uint64_t btn_last_pressed;
static uint64_t time_since_boot;

// Statistics State
static uint16_t stats_count;
static Statshot_t stats_data[STATS_MAX_HISTORY];
static Statshot_t* stats_head = (Statshot_t*)&stats_data;
static uint16_t stats_rolling_count;
static Statshot_t stats_rolling;
static struct repeating_timer timer_stats_historic;

// EPD State
static uint8_t display_buffer_black[SCREEN_W * SCREEN_H];
static uint8_t display_buffer_red[SCREEN_W * SCREEN_H];
static uint8_t display_refresh_count;
static bool display_state;
static bool display_partial_mode;

// Device State
static uint16_t dcc50s_registers[DCC50S_REG_END+1];
static uint16_t rvr40_registers[RVR40_REG_END+1];

char* append_buf(char* s1, char* s2) {
  if(s1 == NULL || s2 == NULL)
    return NULL;

  int n = sizeof(s1);
  char* dest = s1;
  while(*dest != '\0'){
    dest++;
  }
  while(n--){
    if(!(*dest++ = *s2++))
      return s1;
  }
  *dest = '\0';

  return s1;
}

void get_charge_status(char* buffer, uint16_t dcc50s_state, uint16_t rvr40_state) {
  bool charging = false;

  if ((dcc50s_state & DCC50S_CHARGE_STATE_SOLAR) > 0) {
    append_buf(buffer, "Solar ");
    charging = true;
  }
  if ((dcc50s_state & DCC50S_CHARGE_STATE_ALT) > 0) {
    append_buf(buffer, "Altenator ");
    charging = true;
  }
  if ((rvr40_state & RVR40_CHARGE_ACTIVE) > 0) {
    append_buf(buffer, "Solar ");
    charging = true;
  }
  if(!charging){
    sprintf(buffer, "No charge");
    return;
  }

  if ((rvr40_state & DCC50S_CHARGE_STATE_EQ) > 0
      || (rvr40_state & RVR40_CHARGE_EQ) > 0) {
    append_buf(buffer, "=");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_BOOST) > 0
      || (rvr40_state & RVR40_CHARGE_BOOST) > 0) {
    append_buf(buffer, "++");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_FLOAT) > 0
      || (rvr40_state & RVR40_CHARGE_FLOAT) > 0) {
    append_buf(buffer, "~");
  }
  if ((rvr40_state & DCC50S_CHARGE_STATE_LIMITED) > 0
      || (rvr40_state & RVR40_CHARGE_LIMITED) > 0) {
    append_buf(buffer, "+");
  }
}

void get_temperatures(uint16_t state, uint16_t* internal, uint16_t* aux) {
  *internal = (state >> 8);
  *aux = (state & 0xff);
}


void update_page_overview() {
  char line[32];

  uint16_t aux_soc = dcc50s_registers[DCC50S_REG_AUX_SOC];
  float aux_v = (float)dcc50s_registers[DCC50S_REG_AUX_V] / 10.f;   // 0.1v

  uint16_t alt_w = dcc50s_registers[DCC50S_REG_ALT_W];
  uint16_t dcc_charge_state = dcc50s_registers[DCC50S_REG_CHARGE_STATE];

  // Sum any charge from the RVR40 as well
  uint16_t sol_w = rvr40_registers[RVR40_REG_SOLAR_W];
  uint16_t rvr_charge_state = rvr40_registers[RVR40_REG_CHARGE_STATE];

  get_charge_status((char*)&line, dcc_charge_state, rvr_charge_state);
  display_draw_title(line, 5, 12, Black);

  // Battery SOC% and voltage
  sprintf((char*)&line, "%d%%", aux_soc); 
  display_draw_title(line, DISPLAY_H / 2 - 5, DISPLAY_W / 3 + 3, Black);
  sprintf((char*)&line, "%.*fV", 2, aux_v);
  display_draw_text(line, DISPLAY_H / 2, DISPLAY_W / 3 + 30, Black);

  display_draw_text("Altenator", 10, 50, Black);
  sprintf((char*)&line, "%dW", alt_w);
  display_draw_text(line, 100, 50, Black);

  display_draw_text("Solar", 10, 70, Black);
  sprintf((char*)&line, "%dW", sol_w);
  display_draw_text(line, 100, 70, Black);
}

void update_page_solar() {
  char line[32];

  float sol_v = (float)rvr40_registers[RVR40_REG_SOLAR_V] / 10.f;
  float sol_a = (float)rvr40_registers[RVR40_REG_SOLAR_A] / 100.f;
  uint16_t sol_w = rvr40_registers[RVR40_REG_SOLAR_W];
  uint16_t temperature_ctrl, temperature_aux;

  display_draw_title("Solar", 5, 12, Black);

  sprintf((char*)&line, "+ %.*fA", 1, sol_a);
  display_draw_text(line, 5, 50, Black);

  sprintf((char*)&line, "%.*fV", 1, sol_v);
  display_draw_text(line, 50, 50, Black);

  sprintf((char*)&line, "%dW", sol_w);
  display_draw_title(line, 100, 50, Black);

  display_draw_text("Daily Stats", DISPLAY_H / 2 + 15, 30, Black);
  display_draw_text("Charged", DISPLAY_H / 2 + 25, 45, Black);
  display_draw_text("Discharged", DISPLAY_H / 2 + 25, 60, Black);

  sprintf((char*)&line, "%dAh", 
      rvr40_registers[RVR40_REG_DAY_CHG_AMPHRS]);
  display_draw_text(line, DISPLAY_H - 35, 45, Black);
  
  sprintf((char*)&line, "%dAh", 
      rvr40_registers[RVR40_REG_DAY_DCHG_AMPHRS]);
  display_draw_text(line, DISPLAY_H - 35, 60, Black);

  display_draw_text("Temperatures (C)", DISPLAY_H / 2 + 15, 80, Black);
  get_temperatures(rvr40_registers[RVR40_REG_TEMPERATURE], &temperature_ctrl, &temperature_aux);
  sprintf((char*)&line, "RVR40: %d, Aux: %d", temperature_ctrl, temperature_aux);
  display_draw_text(line, DISPLAY_H / 2 + 25, 95, Black);
}

void update_page_altenator() {
  char line[32];

  uint16_t alt_a = dcc50s_registers[DCC50S_REG_ALT_A];
  uint16_t alt_v = dcc50s_registers[DCC50S_REG_ALT_V];
  uint16_t alt_w = dcc50s_registers[DCC50S_REG_ALT_W];
  uint16_t day_total_ah = dcc50s_registers[DCC50S_REG_DAY_TOTAL_AH];
  uint16_t temperatures = dcc50s_registers[RVR40_REG_TEMPERATURE];
  uint16_t temperature_ctrl, temperature_aux;

  display_draw_title("Altenator", 5, 12, Black);
  display_draw_text("Charge Status", DISPLAY_H / 2 + 20, 30, Black);

  sprintf((char*)&line, "%dA", alt_a);
  display_draw_text(line, DISPLAY_H / 2 + 20, 53, Black);

  sprintf((char*)&line, "%dV", alt_v);
  display_draw_text(line, DISPLAY_H / 2 + 40, 53, Black);

  sprintf((char*)&line, "%dW", alt_w);
  display_draw_title(line, DISPLAY_H / 2 + 80, 50, Black);

  sprintf((char*)&line, "%dAh today", day_total_ah);
  display_draw_text(line, DISPLAY_H / 2 + 25, 65, Black);
  
  display_draw_text("Temperatures (C)", 10, 40, Black);
  get_temperatures(temperatures, &temperature_ctrl, &temperature_aux);
  sprintf((char*)&line, "DCC50S: %d, Aux: %d", temperature_ctrl, temperature_aux);
  display_draw_text(line, 20, 55, Black);
}

void update_menu() {
  const uint16_t size = MENU_IMAGE_SIZE;
  const uint16_t menu_y = DISPLAY_W - MENU_IMAGE_SIZE;

  display_draw_xbitmap(0, menu_y, size, size, menu_home_bits);
  display_draw_xbitmap(size, menu_y, size, size, menu_solar_bits);
  display_draw_xbitmap(size * 2, menu_y, size, size, menu_altenator_bits);
  display_draw_xbitmap(size * 3, menu_y, size, size, menu_stats_bits);

  switch(current_page) {
    case Overview:
      display_draw_rect(0, menu_y, size, DISPLAY_W - 1);
      break;
    case Solar:
      display_draw_rect(size, menu_y, size * 2, DISPLAY_W - 1);
      break;
    case Altenator:
      display_draw_rect(size * 2, menu_y, size * 3, DISPLAY_W - 1);
      break;
    case Statistics:
      display_draw_rect(size * 3, menu_y, size * 4, DISPLAY_W - 1);
      break;
  }
}

void update_page_overview_battery() {
  const uint16_t third_x = DISPLAY_H / 3;
  const uint16_t third_y = DISPLAY_W / 3;
  char line[32];
  
  // Draw battery
  float percent = (float)dcc50s_registers[DCC50S_REG_AUX_SOC] / 100.f;
  uint16_t battery_width = (uint16_t)(((DISPLAY_H - 20) - (third_x * 2 - 2)) * percent);

  display_draw_rect(third_x * 2, third_y, DISPLAY_H - 20, third_y * 2);
  if(percent > 0.25) {
    display_set_buffer(display_buffer_black);
  } else {
    display_set_buffer(display_buffer_red);
  }
  display_draw_fill(third_x * 2 + 2, third_y + 2, third_x * 2 + battery_width, third_y * 2 - 1);

  // Draw Ah meter
  sprintf((char*)&line, "%d / %d Ah", (uint16_t)(LFP12S_AH * percent), LFP12S_AH);
  display_draw_text(line, third_x * 2, third_y * 1 - 20, Black);

  // Determine wattage charge over all units and display it
  uint16_t sol_w = rvr40_registers[RVR40_REG_SOLAR_W];
  uint16_t alt_w = dcc50s_registers[DCC50S_REG_ALT_W];
  uint16_t charge_w = sol_w + alt_w;

  if(charge_w != 0) {
    sprintf((char*)&line, "+%dW", charge_w);
    
    display_set_buffer(display_buffer_black);
    display_draw_text(line, third_x * 2, third_y * 2 + 4, Black);
  }

  // Determine wattage discharge over all units (Better to do from battery BMS once we hook that unit up...)
  uint16_t sol_load_w = rvr40_registers[RVR40_REG_LOAD_W];
  uint16_t load_w = sol_load_w;

  if(load_w != 0) {
    sprintf((char*)&line, "-%dW", sol_load_w);
    
    display_set_buffer(display_buffer_red);
    display_draw_text(line, third_x * 2 + 32, third_y * 2 + 4, Red);
  }

  // Determine time until discharged or full
  if(load_w > charge_w) {
    // DUMMY TODO: BMS hookup
    uint16_t chg_a = rvr40_registers[RVR40_REG_CHG_A] + dcc50s_registers[DCC50S_REG_ALT_A];
    float load_a = (float)(chg_a - rvr40_registers[RVR40_REG_LOAD_A]) / 10.f;
    float hrs_left = ((1 - percent) * LFP12S_AH) / load_a;

    if(hrs_left != INFINITY) {
      sprintf((char*)&line, "empty in %.2fh", hrs_left);
      display_set_buffer(display_buffer_red);
      display_draw_text(line, third_x * 2, third_y * 2 + 10, Red);
    }
  } else {
    uint16_t chg_a = dcc50s_registers[DCC50S_REG_ALT_A] + rvr40_registers[RVR40_REG_CHG_A];
    float load_a = (float)(chg_a - rvr40_registers[RVR40_REG_LOAD_A]) / 10.f;    
    float hrs_full = ((1 - percent) * LFP12S_AH) / load_a;
    
    if(hrs_full != INFINITY && hrs_full != 0) {
      sprintf((char*)&line, "full in %.2fh", hrs_full);
      display_set_buffer(display_buffer_black);
      display_draw_text(line, third_x * 2, third_y * 2 + 10, Black);
    }
  }
}

inline static bool should_draw_stat_in_days() {
  return (stats_count >= 48);
}

void draw_stat(uint16_t i, uint16_t plot_x_start, uint16_t plot_x_iter, uint16_t plot_height) {
  char line[3];
  uint16_t value = plot_height - ((float)stats_data[i].aux_soc / 100.f) * plot_height;
  uint16_t x = plot_x_start + plot_x_iter * i;

  if(i > 0) {
    uint16_t last_value = plot_height - ((float)stats_data[i - 1].aux_soc / 100.f) * plot_height;
    
    display_set_buffer(display_buffer_black);
    display_draw_line(x - plot_x_iter, last_value, x, value);
  }

  if(should_draw_stat_in_days()) {
    if((stats_count - i) % 24 == 0) {
      uint16_t avg, sum;
      for(uint16_t v = i - 24; v < i; v++) {
        sum += stats_data[v].aux_soc;
      }
      avg = plot_height - ((float)sum / 24.f) * plot_height;
      
      display_set_buffer(display_buffer_red);
      display_draw_fill(x - 1, avg - 1, x + 2, avg + 2);
      
      sprintf((char*)line, "%d", (stats_count - i) / 24);
      display_set_buffer(display_buffer_black);
      display_draw_text(line, x, plot_height + 7, Black);
    }
  } else {
    display_set_buffer(display_buffer_red);
    display_draw_fill(x - 1, value - 1, x + 2, value + 2);

    if(stats_count < 12 || (stats_count - i) % 5 == 0) {
      sprintf((char*)line, "%d", (stats_count - i));
      display_set_buffer(display_buffer_black);
      display_draw_text(line, x - 5, plot_height + 7, Black);
    }
  }
}

void update_page_statistics() {
  const uint16_t plot_x_start = 25;
  const uint16_t plot_x_iter = stats_count == 0 ? 1 : (DISPLAY_H - plot_x_start) / stats_count;
  const uint16_t plot_height = DISPLAY_W - MENU_IMAGE_SIZE - 25;

  uint16_t join_index;

  // Draw chart with axis
  display_draw_rect(plot_x_start, 0, DISPLAY_H - 1, plot_height);
  display_draw_text("0", 5, plot_height - 5, Black);
  display_draw_text("%", 5, plot_height / 2, Black);
  display_draw_text("100", 0, 0, Black);

  // Determine if we are rendering days or hours
  if(should_draw_stat_in_days()) {
    display_draw_title("Daily", DISPLAY_H - 80, DISPLAY_W - 20, Black);
  } else {
    display_draw_title("Hourly", DISPLAY_H - 96, DISPLAY_W - 20, Black);
  }

  // iterate to find where beginning meets end (ring buffer)
  for(uint16_t i = 0; i < stats_count; i++) {
    if(stats_data[i].index == 0) {
      join_index = i;
      break;
    }
  }
  // draw from beginning to end of the buffer (regardless of starting index)
  for(uint16_t i = 0; i < join_index; i++) {
    draw_stat(i, plot_x_start, plot_x_iter, plot_height);
  }
  for(uint16_t i = join_index; i < stats_count; i++) {
    draw_stat(i, plot_x_start, plot_x_iter, plot_height);
  }
}

void update_page() {
  // Clear black and red buffers (with White, 0xff);
  display_set_buffer(display_buffer_red);
  display_fill_colour(White);
  display_set_buffer(display_buffer_black);
  display_fill_colour(White);

#ifdef EPD_UPDATE_PARTIAL
  // full refresh after x partials or first draw
  if(display_refresh_count == 0 || display_refresh_count >= EPD_FULL_REFRESH_AFTER) {
    printf("Full refresh \n");
    display_partial_mode = false;
    display_refresh_count = 1;
  } else {
    display_partial_mode = true;
    display_refresh_count++;
  }
#endif
  
  update_menu();

  switch(current_page) {
    case Overview: 
      update_page_overview();
      update_page_overview_battery();
      break;

    case Altenator:
      update_page_altenator();
      break;

    case Solar:
      update_page_solar();
      break;

    case Statistics:
      update_page_statistics();
      break;

    default:
      display_draw_title("404", 5, 12, Black);
      break;
  }

  if(!display_state){
    display_wake();
    display_state = true;
  }
  
#ifdef EPD_UPDATE_PARTIAL
  if(!display_partial_mode) {
#endif

    printf("Updating full screen normal refresh\n");
    display_send_buffer(display_buffer_black, SCREEN_W, SCREEN_H, 1);
    display_send_buffer(display_buffer_red, SCREEN_W, SCREEN_H, 2);
    busy_wait_ms(20);
    display_refresh(true);
    busy_wait_ms(200);
    display_sleep();
    display_state = false;
    busy_wait_ms(1000);

#ifdef EPD_UPDATE_PARTIAL
  } else {
    printf("Updating partial\n");
    const coord_t screen_region = { 0, 0, DISPLAY_W - 1, DISPLAY_H - 1 };
    display_draw_partial(display_buffer_black, display_buffer_red, screen_region);
  }
#endif
}

void btn_handler(uint gpio, uint32_t events) {
#ifdef _VERBOSE
  printf("Btn @ %d is now %02x\n", gpio, events);
#endif

  if (events & 0x1) { // Low
  }
  if (events & 0x2) { // High
  }
  if (events & 0x4) { // Fall
    // debounce
    if(time_us_64() > btn_last_pressed + 800000) // 800ms
    {
      btn_last_pressed = time_us_64();
      update_page();
    }
  }
  if (events & 0x8) { // Rise
    if(time_us_64() > btn_last_pressed + 350000) // 350ms
    {
      current_page = (++current_page) % PageContentsCount;
      printf("Changed current page to %d\n", current_page);
      btn_last_pressed = time_us_64();
    }
  }
}

inline static uint16_t update_rolling_statistic(uint16_t* avg, uint16_t new_value) {
  if(stats_rolling_count == 0)
    return *avg;

  *avg = (*avg * (stats_rolling_count - 1) + new_value) / stats_rolling_count;

  return *avg;
}

Statshot_t get_latest_stats() {
  Statshot_t latest = {
    (uint8_t)(stats_rolling_count + 1),
    dcc50s_registers[DCC50S_REG_AUX_SOC],
    dcc50s_registers[DCC50S_REG_ALT_W],
    rvr40_registers[RVR40_REG_SOLAR_W],
    dcc50s_registers[DCC50S_REG_DAY_TOTAL_AH] + rvr40_registers[RVR40_REG_DAY_CHG_AMPHRS],
    rvr40_registers[RVR40_REG_DAY_DCHG_AMPHRS]
  };
  return latest;
}

void reset_statistics(Statshot_t* stat) {
  Statshot_t latest = get_latest_stats();

  stat->aux_soc = latest.aux_soc;
  stat->alt_w = latest.alt_w;
  stat->sol_w = latest.sol_w;
  stat->charged_ah = latest.charged_ah;
  stat->discharged_ah = latest.discharged_ah;
}

void update_rolling_statistic_from_latest() {
  Statshot_t latest = get_latest_stats();
  
  // First update?
  if(stats_rolling_count <= 1) {
    reset_statistics(&stats_rolling);  
  }
  printf("Updating rolling with latest %d percent\n", latest.aux_soc);

  // Update using rolling average values
  stats_rolling.aux_soc = update_rolling_statistic(&stats_rolling.aux_soc, latest.aux_soc);
  stats_rolling.alt_w = update_rolling_statistic(&stats_rolling.alt_w, latest.alt_w);
  stats_rolling.sol_w = update_rolling_statistic(&stats_rolling.sol_w, latest.sol_w);
  stats_rolling.charged_ah = update_rolling_statistic(&stats_rolling.charged_ah, latest.charged_ah);
  stats_rolling.discharged_ah = update_rolling_statistic(&stats_rolling.discharged_ah, latest.discharged_ah);

  // Increment rolling average count
  stats_rolling_count++;
  printf("Rolling is now %d percent\n", stats_rolling.aux_soc);
  printf("Head is now %d percent\n", stats_head->aux_soc);
  printf("Stats rolling count: %d, stats count: %d\n", stats_rolling_count, stats_count);
}

void update_historical_statistics() {
  // Idealy should not happen, unless the update timings are not adequate
  //  STATS_UPDATE_ROLLING_MS should be less than STATS_UPDATE_HISTORIC_MS
  //  STATS_UPDATE_HISTORIC_MS should occur at least after the first iteration of rolling updates
  if(stats_rolling_count <= 1) {
    update_rolling_statistic_from_latest();
  } 

  // update latest stats with rolling total before incrementing to the next historic snapshot
  stats_head->aux_soc = stats_rolling.aux_soc;
  stats_head->alt_w = stats_rolling.alt_w;
  stats_head->sol_w = stats_rolling.sol_w;
  stats_head->charged_ah = stats_rolling.charged_ah;
  stats_head->discharged_ah = stats_rolling.discharged_ah;

  // lazy mans ring buffer
  if(stats_count < STATS_MAX_HISTORY - 1) {
    stats_head++;
    stats_count++;
    stats_head->index = stats_count;
  } else {
    // reset to head
    if(stats_head->index >= STATS_MAX_HISTORY-1) {
      stats_head = (Statshot_t*)&stats_data;
      stats_head->index = 0;
    }
    else {
      uint8_t last_index = stats_head->index;
      stats_head++;
      stats_head->index = last_index + 1;
    }
  }

  // Fill using the latest value
  if(stats_count == 0) {
    reset_statistics(stats_head);
  } else {
    // Reset rolling averages for the next historical period
    reset_statistics(&stats_rolling);
    stats_rolling_count = 1;
  }
}

bool alarm_update_historic_statistics_callback(struct repeating_timer* t) {
  printf("ALARM: Historic Statistics timer fired!\n");

  update_historical_statistics();

  return true;
}

int alarms_initialise() {
  printf("Intialising alarms... ");
  if(!add_repeating_timer_ms(STATS_UPDATE_HISTORIC_MS, alarm_update_historic_statistics_callback, NULL, &timer_stats_historic)){
    printf("Unable to initialise historic statistics timer\n");
    return -1;
  }

  printf("Done.\n");
  return 0;
}

int main() {
  uint64_t last_epd_update;
  uint64_t last_rolling_stat_update;
  int state;

  stdio_init_all();
 
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_handler);

  current_page = Statistics; //Overview;
  btn_last_pressed = time_us_64();
  last_rolling_stat_update = time_us_64() + (STATS_UPDATE_ROLLING_MS * 1000);
  last_epd_update = time_us_64() + (EPD_REFRESH_RATE_MS * 1000);

  state = devices_modbus_init();
  if(state != 0) {
    return state;
  }

  state = alarms_initialise();
  if(state != 0) {
    return state;
  }

  // wait for host... (should be a better way)
  gpio_put(LED_PIN, 0);
  busy_wait_ms(3000);
  gpio_put(LED_PIN, 1);

  state = devices_modbus_uart_init();
  if(state != 0) {
    printf("Unable to initialise modbus: %d", state); 
    return state;
  }

  display_init();
  display_state = true;
  display_clear();
  busy_wait_ms(500);

  gpio_put(LED_PIN, 0);

  // Main update loop
  while(1) {
    gpio_put(LED_PIN, 1);

    time_since_boot = time_us_64();

    devices_modbus_read_registers(
        RS232_PORT, RS232_RVR40_ADDRESS, RVR40_REG_START, RVR40_REG_END, (uint16_t*)&rvr40_registers);

    devices_modbus_read_registers(
      RS485_PORT, RS485_DCC50S_ADDRESS, DCC50S_REG_START, DCC50S_REG_END, (uint16_t*)&dcc50s_registers);

    
    // update rolling statistics based on latest data received
    if(time_since_boot > last_rolling_stat_update) {
      update_rolling_statistic_from_latest();

      last_rolling_stat_update = time_since_boot + (STATS_UPDATE_ROLLING_MS * 1000);
    }
    
    if(time_since_boot > last_epd_update ) {
      update_page();

      last_epd_update = time_since_boot + (EPD_REFRESH_RATE_MS * 1000);
    }
    
    gpio_put(LED_PIN, 0);
    sleep_ms(1000);
  }
}

