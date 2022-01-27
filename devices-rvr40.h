// Renogy Rover 20, 30, or 40 Amp Solar MPPT Charge Controller register schema

#define RVR40_REG_START             0x100
#define RVR40_REG_AUX_SOC           0         // 0x100
#define RVR40_REG_AUX_V             1
#define RVR40_REG_CHG_A             2
#define RVR40_REG_TEMPERATURE       3
#define RVR40_REG_LOAD_V            4         // * 0.1
#define RVR40_REG_LOAD_A            5         // * 0.1
#define RVR40_REG_LOAD_W            6
#define RVR40_REG_SOLAR_V           7         // * 0.1
#define RVR40_REG_SOLAR_A           8         // * 0.1
#define RVR40_REG_SOLAR_W           9
#define RVR40_REG_DAY_MIN_V         11
#define RVR40_REG_DAY_AUX_MAX_V     12
#define RVR40_REG_DAY_CHG_MAX_A     13
#define RVR40_REG_DAY_DCHG_MAX_A    14
#define RVR40_REG_DAY_CHG_MAX_W     15
#define RVR40_REG_DAY_DCHG_MAX_W    16
#define RVR40_REG_DAY_CHG_AMPHRS    17
#define RVR40_REG_DAY_DCHG_AMPHRS   18
#define RVR40_REG_DAY_CHG_KWH       19
#define RVR40_REG_DAY_DCHG_KWH      20
#define RVR40_REG_DAY_COUNT         21
#define RVR40_REG_OVER_DCHG_COUNT   22
#define RVR40_REG_FULL_CHG_COUNT    23
#define RVR40_REG_TOTAL_CHG_AMPHRS  24
#define RVR40_REG_TOTAL_DCHG_AMPHRS 25
#define RVR40_REG_CUMUL_CHG_KWH     26  // /= 10000
#define RVR40_REG_CUMUL_DCHG_KWH    27  // /= 10000
#define RVR40_REG_CHARGE_STATE      28
#define RVR40_REG_ERR_1             29
#define RVR40_REG_ERR_2             30
#define RVR40_REG_END               31

// For RVR40_REG_CHARGE_STATE
#define RVR40_CHARGE_DEACTIVE   0
#define RVR40_CHARGE_ACTIVE     (1 << 1)
#define RVR40_CHARGE_MPPT       (1 << 2)
#define RVR40_CHARGE_EQ         (1 << 3)
#define RVR40_CHARGE_BOOST      (1 << 4)
#define RVR40_CHARGE_FLOAT      (1 << 5)
#define RVR40_CHARGE_LIMITED    (1 << 6)

// For RVR40_REG_ERR2
#define RVR40_ERR_REVERSE_POLARITY  (1 << 12)
#define RVR40_ERR_WP_OVER_VOLT      (1 << 11)
#define RVR40_ERR_COUNTER_CURRENT   (1 << 10)
#define RVR40_ERR_IN_OVER_VOLT      (1 << 9)
#define RVR40_ERR_IN_SHORT          (1 << 8)
#define RVR40_ERR_IN_OVER_AMP       (1 << 7)
#define RVR40_ERR_AUX_OVERHEAT      (1 << 6)
#define RVR40_ERR_CTRL_OVERHEAT     (1 << 5)
#define RVR40_ERR_LOAD_OVER_AMP     (1 << 4)
#define RVR40_ERR_LOAD_SHORT        (1 << 3)
#define RVR40_ERR_AUX_UNDER_VOLT    (1 << 2)
#define RVR40_ERR_AUX_OVER_VOLT     (1 << 1)
#define RVR40_ERR_AUX_DISCHARGED    (1 << 0)
