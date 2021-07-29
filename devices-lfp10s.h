/* Renogy LFP100S Smart Lithium-Ion Battery Modbus 485 register schema
 *  The following addresses were found to return values in the range,
 *    and have been used to reverse engineer the schema.
 *
 *  5000 - 5033
 *  5035 - 5052
 *  5100 - 5141
 *  5200 - 5223
 */

#define LFP100S_REG_START           5000

// Regions found to work
#define LFP100S_REG_1_END           33
#define LFP100S_REG_2_START         5035
#define LFP100S_REG_2_END           17
#define LFP100S_REG_3_START         5100
#define LFP100S_REG_3_END           41
#define LFP100S_REG_4_START         5200
#define LFP100S_REG_4_END           23

// Offsets of addresses from LFP100S_REG_START
#define LFP100S_REG_CELL_COUNT      1
#define LFP100S_REG_C1_V            2
#define LFP100S_REG_C2_V            3
#define LFP100S_REG_C3_V            4
#define LFP100S_REG_C4_V            5
#define LFP100S_REG_C1_TEMP         19
#define LFP100S_REG_C2_TEMP         20
#define LFP100S_REG_C3_TEMP         21
#define LFP100S_REG_C4_TEMP         22
#define LFP100S_REG_LOAD_A          43
#define LFP100S_REG_VOLTAGE         44

// Total addresses found, used for memory allocation
#define LFP100S_REG_COUNT           114 
