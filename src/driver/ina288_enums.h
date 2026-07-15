//  for setMode() and getMode()
enum ina228_mode_enum {
  INA228_MODE_SHUTDOWN            = 0x00,
  INA228_MODE_TRIG_BUS            = 0x01,
  INA228_MODE_TRIG_SHUNT          = 0x02,
  INA228_MODE_TRIG_BUS_SHUNT      = 0x03,
  INA228_MODE_TRIG_TEMP           = 0x04,
  INA228_MODE_TRIG_TEMP_BUS       = 0x05,
  INA228_MODE_TRIG_TEMP_SHUNT     = 0x06,
  INA228_MODE_TRIG_TEMP_BUS_SHUNT = 0x07,

  INA228_MODE_SHUTDOWN2           = 0x08,
  INA228_MODE_CONT_BUS            = 0x09,
  INA228_MODE_CONT_SHUNT          = 0x0A,
  INA228_MODE_CONT_BUS_SHUNT      = 0x0B,
  INA228_MODE_CONT_TEMP           = 0x0C,
  INA228_MODE_CONT_TEMP_BUS       = 0x0D,
  INA228_MODE_CONT_TEMP_SHUNT     = 0x0E,
  INA228_MODE_CONT_TEMP_BUS_SHUNT = 0x0F
};


//  for setAverage() and getAverage()
enum ina228_average_enum {
    INA228_1_SAMPLE     = 0,
    INA228_4_SAMPLES    = 1,
    INA228_16_SAMPLES   = 2,
    INA228_64_SAMPLES   = 3,
    INA228_128_SAMPLES  = 4,
    INA228_256_SAMPLES  = 5,
    INA228_512_SAMPLES  = 6,
    INA228_1024_SAMPLES = 7
};


//  for Bus, shunt and temperature conversion timing.
enum ina228_timing_enum {
    INA228_50_us   = 0,
    INA228_84_us   = 1,
    INA228_150_us  = 2,
    INA228_280_us  = 3,
    INA228_540_us  = 4,
    INA228_1052_us = 5,
    INA228_2074_us = 6,
    INA228_4120_us = 7
};


//  for diagnose/alert() bit fields.
//  TODO bit masks?
enum ina228_diag_enum {
  INA228_DIAG_MEMORY_STATUS      = 0,
  INA228_DIAG_CONVERT_COMPLETE   = 1,
  INA228_DIAG_POWER_OVER_LIMIT   = 2,
  INA228_DIAG_BUS_UNDER_LIMIT    = 3,
  INA228_DIAG_BUS_OVER_LIMIT     = 4,
  INA228_DIAG_SHUNT_UNDER_LIMIT  = 5,
  INA228_DIAG_SHUNT_OVER_LIMIT   = 6,
  INA228_DIAG_TEMP_OVER_LIMIT    = 7,
  INA228_DIAG_RESERVED           = 8,
  INA228_DIAG_MATH_OVERFLOW      = 9,
  INA228_DIAG_CHARGE_OVERFLOW    = 10,
  INA228_DIAG_ENERGY_OVERFLOW    = 11,
  INA228_DIAG_ALERT_POLARITY     = 12,
  INA228_DIAG_SLOW_ALERT         = 13,
  INA228_DIAG_CONVERT_READY      = 14,
  INA228_DIAG_ALERT_LATCH        = 15
};