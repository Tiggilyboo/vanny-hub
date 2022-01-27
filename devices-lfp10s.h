/* Renogy LFP100S Smart Lithium-Ion Battery Modbus 485 register schema
 *  The following addresses were found to return values in the range,
 *    and have been used to reverse engineer the schema.
 *
 *  5000 - 5033
 *  5035 - 5052
 *  5100 - 5141
 *  5200 - 5223
);*/

#define LFP100S_REG_START           5042

// Offsets of addresses from LFP100S_REG_START
#define LFP100S_REG_LOAD_A          0 // 5042
#define LFP100S_REG_VOLTAGE         1 // 5043
#define LFP100S_REG_CAPACITY_1      2 // 5044
#define LFP100S_REG_CAPACITY_2      3 // 5045
#define LFP100S_REG_MAX_CAPACITY_1  4 // 5046
#define LFP100S_REG_MAX_CAPACITY_2  5 // 5047
#define LFP100S_REG_END             6

float battery_max_capacity();
float battery_capacity();
float battery_percentage();
float battery_amperes();
float battery_voltage();
