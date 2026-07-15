//    FILE: INA228.cpp
//  AUTHOR: Rob Tillaart
//  MODIFIED BY Andreas Bresser for ESA Space Resources Challenge
// VERSION: 0.4.0-ESA-SRC
//    DATE: 2025-10-01
// PURPOSE: Arduino library for the INA228, I2C, 20 bit, voltage, current and power sensor.
//     URL: https://github.com/RobTillaart/INA228
//          https://www.adafruit.com/product/5832           ( 10 A version)
//          https://www.mateksys.com/?portfolio=i2c-ina-bm  (200 A version))

//
//  Read the datasheet for the details


#include "ina228.hpp"

//      REGISTERS                   ADDRESS    BITS  RW
#define INA228_CONFIG               0x00    //  16   RW
#define INA228_ADC_CONFIG           0x01    //  16   RW
#define INA228_SHUNT_CAL            0x02    //  16   RW
#define INA228_SHUNT_TEMP_CO        0x03    //  16   RW
#define INA228_SHUNT_VOLTAGE        0x04    //  24   R-
#define INA228_BUS_VOLTAGE          0x05    //  24   R-
#define INA228_TEMPERATURE          0x06    //  16   R-
#define INA228_CURRENT              0x07    //  24   R-
#define INA228_POWER                0x08    //  24   R-
#define INA228_ENERGY               0x09    //  40   R-
#define INA228_CHARGE               0x0A    //  40   R-
#define INA228_DIAG_ALERT           0x0B    //  16   RW
#define INA228_SOVL                 0x0C    //  16   RW
#define INA228_SUVL                 0x0D    //  16   RW
#define INA228_BOVL                 0x0E    //  16   RW
#define INA228_BUVL                 0x0F    //  16   RW
#define INA228_TEMP_LIMIT           0x10    //  16   RW
#define INA228_POWER_LIMIT          0x11    //  16   RW
#define INA228_MANUFACTURER         0x3E    //  16   R-
#define INA228_DEVICE_ID            0x3F    //  16   R-


//  CONFIG MASKS (register 0)
#define INA228_CFG_RST              0x8000
#define INA228_CFG_RSTACC           0x4000
#define INA228_CFG_CONVDLY          0x3FC0
#define INA228_CFG_TEMPCOMP         0x0020
#define INA228_CFG_ADCRANGE         0x0010
#define INA228_CFG_RESERVED         0x000F  //  all unused bits


//  ADC MASKS (register 1)
#define INA228_ADC_MODE             0xF000
#define INA228_ADC_VBUSCT           0x0E00
#define INA228_ADC_VSHCT            0x01C0
#define INA228_ADC_VTCT             0x0038
#define INA228_ADC_AVG              0x0007


////////////////////////////////////////////////////////
//
//  CONSTRUCTOR
//
INA228::INA228(const uint8_t address)
{
  _address     = address;
  //  no calibrated values by default.
  _shunt       = 0.015;
  _maxCurrent  = 10.0;
  _current_LSB = _maxCurrent * 1.9073486328125e-06;
  _error       = 0;
}


bool INA228::begin()
{
  fd_i2c = wiringPiI2CSetup(_address);
  if (fd_i2c == -1) {
    std::cerr << "Failed to init I2C communication." << std::endl;
    return false;
  }

  if (! isConnected()) {
    std::cerr << "Failed to init I2C communication." << std::endl;
    return false;
  }

  std::cout << "I2C communication successfully setup." << std::endl;
  getADCRange();
  
  return true;
}


bool INA228::isConnected()
{
  if (fd_i2c < 0) return false;
  // Try reading a known register. A return value of -1 indicates an error.
  return (wiringPiI2CReadReg16(fd_i2c, INA228_MANUFACTURER) != -1);
}



uint8_t INA228::getAddress()
{
  return _address;
}


////////////////////////////////////////////////////////
//
//  CORE FUNCTIONS
//
//  PAGE 25
float INA228::getBusVoltage()
{
  //  always positive, remove reserved bits.
  int32_t value = _readRegister(INA228_BUS_VOLTAGE, 3) >> 4;
  float bus_LSB = 195.3125e-6;  //  195.3125 uV
  float voltage = value * bus_LSB;
  return voltage;
}

//  PAGE 25
float INA228::getShuntVoltage()
{
  //  shunt_LSB depends on ADCRANGE in INA228_CONFIG register.
  float shunt_LSB = 312.5e-9;  //  312.5 nV
  if (_ADCRange == true)
  {
    shunt_LSB = 78.125e-9;     //  78.125 nV
  }

  //  remove reserved bits.
  int32_t value = _readRegister(INA228_SHUNT_VOLTAGE, 3) >> 4;
  //  handle negative values (20 bit)
  if (value & 0x00080000)
  {
    value |= 0xFFF00000;
  }
  float voltage = value * shunt_LSB;
  return voltage;
}

int32_t INA228::getShuntVoltageRAW()
{
  //  remove reserved bits.
  uint32_t value = _readRegister(INA228_SHUNT_VOLTAGE, 3) >> 4;
  //  handle negative values (20 bit)
  if (value & 0x00080000)
  {
    value |= 0xFFF00000;
  }
  return (int32_t)value;
}

//  PAGE 25 + 8.1.2
// in Ampere
float INA228::getCurrent()
{
  //  remove reserved bits.
  int32_t value = _readRegister(INA228_CURRENT, 3) >> 4;
  //  handle negative values (20 bit)
  if (value & 0x00080000)
  {
    value |= 0xFFF00000;
  }
  float current = value * _current_LSB;
  return current;
}

//  PAGE 26 + 8.1.2
// in Watt
float INA228::getPower()
{
  uint32_t value = _readRegister(INA228_POWER, 3);
  //  PAGE 31 (8.1.2)
  return value * 3.2 * _current_LSB;
}

//  PAGE 25
float INA228::getTemperature()
{
  uint32_t value = _readRegister(INA228_TEMPERATURE, 2);
  float LSB = 7.8125e-3;  //  milli degree Celsius
  return value * LSB;
}

//  PAGE 26 + 8.1.2
double INA228::getEnergy()
{
  //  read 40 bit UNSIGNED as a double to prevent 64 bit integers
  //  double might be 8 or 4 byte, depends on platform
  //  40 bit ==> O(10^12)
  double value = _readRegisterF(INA228_ENERGY, 'U');
  //  PAGE 31 (8.1.2)
  return value * (16 * 3.2) * _current_LSB;
}


//  PAGE 26 + 8.1.2
double INA228::getCharge()
{
  //  read 40 bit SIGNED as a float to prevent 64 bit integers
  //  double might be 8 or 4 byte, depends on platform
  //  40 bit ==> O(10^12)
  double value = _readRegisterF(INA228_CHARGE, 'S');
  //  PAGE 32 (8.1.2)
  return value * _current_LSB;
}


////////////////////////////////////////////////////////
//
//  CONFIG REGISTER 0
//
void INA228::reset()
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  value |= INA228_CFG_RST;
  _writeRegister(INA228_CONFIG, value);
}

bool INA228::setAccumulation(uint8_t value)
{
  if (value > 1) return false;
  uint16_t reg = _readRegister(INA228_CONFIG, 2);
  if (value == 1) reg |= INA228_CFG_RSTACC;
  else            reg &= ~INA228_CFG_RSTACC;
  _writeRegister(INA228_CONFIG, reg);
  return true;
}

bool INA228::getAccumulation()
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  return (value & INA228_CFG_RSTACC) > 0;
}

void INA228::setConversionDelay(uint8_t steps)
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  value &= ~INA228_CFG_CONVDLY;
  value |= (steps << 6);
  _writeRegister(INA228_CONFIG, value);
}

uint8_t INA228::getConversionDelay()
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  return (value >> 6) & 0xFF;
}

void INA228::setTemperatureCompensation(bool on)
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  if (on) value |= INA228_CFG_TEMPCOMP;
  else    value &= ~INA228_CFG_TEMPCOMP;
  _writeRegister(INA228_CONFIG, value);
}

bool INA228::getTemperatureCompensation()
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  return (value & INA228_CFG_TEMPCOMP) > 0;
}

bool INA228::setADCRange(bool flag)
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  _ADCRange = (value & INA228_CFG_ADCRANGE) > 0;
  //  nothing changed ==> we're done.
  if (flag == _ADCRange) return true;

  _ADCRange = flag;
  if (flag) value |= INA228_CFG_ADCRANGE;
  else      value &= ~INA228_CFG_ADCRANGE;
  _writeRegister(INA228_CONFIG, value);
  //  Fix #26, issue where shunt_cal was not modified
  bool rv = setMaxCurrentShunt(getMaxCurrent(), getShunt()) == 0;
  return rv;
}

bool INA228::getADCRange()
{
  uint16_t value = _readRegister(INA228_CONFIG, 2);
  _ADCRange = (value & INA228_CFG_ADCRANGE) > 0;
  return _ADCRange;
}


////////////////////////////////////////////////////////
//
//  CONFIG ADC REGISTER 1
//
bool INA228::setMode(uint8_t mode)
{
  if (mode > 0x0F) return false;
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  value &= ~INA228_ADC_MODE;
  value |= (mode << 12);
  _writeRegister(INA228_ADC_CONFIG, value);
  return true;
}

uint8_t INA228::getMode()
{
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  return (value & INA228_ADC_MODE) >> 12;
}

bool INA228::setBusVoltageConversionTime(uint8_t bvct)
{
  if (bvct > 7) return false;
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  value &= ~INA228_ADC_VBUSCT;
  value |= (bvct << 9);
  _writeRegister(INA228_ADC_CONFIG, value);
  return true;
}

uint8_t INA228::getBusVoltageConversionTime()
{
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  return (value & INA228_ADC_VBUSCT) >> 9;
}

bool INA228::setShuntVoltageConversionTime(uint8_t svct)
{
  if (svct > 7) return false;
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  value &= ~INA228_ADC_VSHCT;
  value |= (svct << 6);
  _writeRegister(INA228_ADC_CONFIG, value);
  return true;
}

uint8_t INA228::getShuntVoltageConversionTime()
{
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  return (value & INA228_ADC_VSHCT) >> 6;
}

bool INA228::setTemperatureConversionTime(uint8_t tct)
{
  if (tct > 7) return false;
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  value &= ~INA228_ADC_VTCT;
  value |= (tct << 3);
  _writeRegister(INA228_ADC_CONFIG, value);
  return true;
}

uint8_t INA228::getTemperatureConversionTime()
{
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  return (value & INA228_ADC_VTCT) >> 3;
}

bool INA228::setAverage(uint8_t avg)
{
  if (avg > 7) return false;
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  value &= ~INA228_ADC_AVG;
  value |= avg;
  _writeRegister(INA228_ADC_CONFIG, value);
  return true;
}

uint8_t INA228::getAverage()
{
  uint16_t value = _readRegister(INA228_ADC_CONFIG, 2);
  return (value & INA228_ADC_AVG);
}


////////////////////////////////////////////////////////
//
//  SHUNT CALIBRATION REGISTER 2
//
int INA228::setMaxCurrentShunt(float maxCurrent, float shunt)
{
  //  Shunt can be really small
  if (shunt < 0.0001) return -2;   //  TODO error code
  _maxCurrent = maxCurrent;
  _shunt = shunt;
  _current_LSB = _maxCurrent * 1.9073486328125e-6;  //  pow(2, -19);

  //  PAGE 31 (8.1.2)
  float shunt_cal = 13107.2e6 * _current_LSB * _shunt;
  //  depends on ADCRANGE in INA228_CONFIG register.
  if (_ADCRange == true)
  {
    shunt_cal *= 4;
  }
  //  shunt_cal must be written to REGISTER.
  //  work in progress PR #7
  _writeRegister(INA228_SHUNT_CAL, shunt_cal);

  return 0;
}

float INA228::getMaxCurrent()
{
  return _maxCurrent;
}

float INA228::getShunt()
{
  return _shunt;
}

float INA228::getCurrentLSB()
{
  return _current_LSB;
}


////////////////////////////////////////////////////////
//
//  SHUNT TEMPERATURE COEFFICIENT REGISTER 3
//
bool INA228::setShuntTemperatureCoefficent(uint16_t ppm)
{
  if (ppm > 16383) return false;
  _writeRegister(INA228_SHUNT_TEMP_CO, ppm);
  return true;
}

uint16_t INA228::getShuntTemperatureCoefficent()
{
  uint16_t value = _readRegister(INA228_SHUNT_TEMP_CO, 2);
  return value;
}


////////////////////////////////////////////////////////
//
//  DIAGNOSE ALERT REGISTER 11
//
void INA228::setDiagnoseAlert(uint16_t flags)
{
  _writeRegister(INA228_DIAG_ALERT, flags);
}

uint16_t INA228::getDiagnoseAlert()
{
  return _readRegister(INA228_DIAG_ALERT, 2);
}

//  INA228.h has an enum for the bit fields.
void INA228::setDiagnoseAlertBit(uint8_t bit)
{
  uint16_t value = _readRegister(INA228_DIAG_ALERT, 2);
  uint16_t mask = (1 << bit);
  //  only write new value if bit not set
  if ((value & mask) == 0)
  {
    value |= mask;
    _writeRegister(INA228_DIAG_ALERT, value);
  }
}

void INA228::clearDiagnoseAlertBit(uint8_t bit)
{
  uint16_t value = _readRegister(INA228_DIAG_ALERT, 2);
  uint16_t mask = (1 << bit);
  //  only write new value if bit not set.
  if ((value & mask ) != 0)
  {
    value &= ~mask;
    _writeRegister(INA228_DIAG_ALERT, value);
  }
}

uint16_t INA228::getDiagnoseAlertBit(uint8_t bit)
{
  uint16_t value = _readRegister(INA228_DIAG_ALERT, 2);
  return (value >> bit) & 0x01;
}


////////////////////////////////////////////////////////
//
//  THRESHOLD AND LIMIT REGISTERS 12-17
//
//  TODO - API ?

void INA228::setShuntOvervoltageTH(uint16_t threshold)
{
  //  TODO ADCRANGE DEPENDENT
  _writeRegister(INA228_SOVL, threshold);
}

uint16_t INA228::getShuntOvervoltageTH()
{
  //  TODO ADCRANGE DEPENDENT
  return _readRegister(INA228_SOVL, 2);
}

void INA228::setShuntUndervoltageTH(uint16_t threshold)
{
  //  TODO ADCRANGE DEPENDENT
  _writeRegister(INA228_SUVL, threshold);
}

uint16_t INA228::getShuntUndervoltageTH()
{
  //  TODO ADCRANGE DEPENDENT
  return _readRegister(INA228_SUVL, 2);
}

void INA228::setBusOvervoltageTH(uint16_t threshold)
{
  if (threshold > 0x7FFF) return;
  //float LSB = 3.125e-3;  //  3.125 mV/LSB.
  _writeRegister(INA228_BOVL, threshold);
}

uint16_t INA228::getBusOvervoltageTH()
{
  //float LSB = 3.125e-3;  //  3.125 mV/LSB.
  return _readRegister(INA228_BOVL, 2);
}

void INA228::setBusUndervoltageTH(uint16_t threshold)
{
  if (threshold > 0x7FFF) return;
  //float LSB = 3.125e-3;  //  3.125 mV/LSB.
  _writeRegister(INA228_BUVL, threshold);
}

uint16_t INA228::getBusUndervoltageTH()
{
  //float LSB = 3.125e-3;  //  3.125 mV/LSB.
  return _readRegister(INA228_BUVL, 2);
}

void INA228::setTemperatureOverLimitTH(uint16_t threshold)
{
  //float LSB = 7.8125e-3;  //  milliCelsius
  _writeRegister(INA228_TEMP_LIMIT, threshold);
}

uint16_t INA228::getTemperatureOverLimitTH()
{
  //float LSB = 7.8125e-3;  //  milliCelsius
  return _readRegister(INA228_TEMP_LIMIT, 2);
}

void INA228::setPowerOverLimitTH(uint16_t threshold)
{
  //  P29
  //  Conversion factor: 256 × Power LSB.
  _writeRegister(INA228_POWER_LIMIT, threshold);
}

uint16_t INA228::getPowerOverLimitTH()
{
  //  P29
  //  Conversion factor: 256 × Power LSB.
  return _readRegister(INA228_POWER_LIMIT, 2);
}


////////////////////////////////////////////////////////
//
//  MANUFACTURER and ID REGISTER 3E/3F
//
uint16_t INA228::getManufacturer()
{
  uint16_t value = _readRegister(INA228_MANUFACTURER, 2);
  return value;
}

uint16_t INA228::getDieID()
{
  uint16_t value = _readRegister(INA228_DEVICE_ID, 2);
  return (value >> 4) & 0x0FFF;
}

uint16_t INA228::getRevision()
{
  uint16_t value = _readRegister(INA228_DEVICE_ID, 2);
  return value & 0x000F;
}


////////////////////////////////////////////////////////
//
//  ERROR HANDLING
//
int INA228::getLastError()
{
  int e = _error;
  _error = 0;
  return e;
}


////////////////////////////////////////////////////////
//
//  PRIVATE
//
uint32_t INA228::_readRegister(uint8_t reg, uint8_t bytes)
{
  _error = 0;
  // _wire->beginTransmission(_address);
  // _wire->write(reg);
  // int n = _wire->endTransmission();
  // if (n != 0)
  // {
  //   _error = -1;
  //   return 0;
  // }
  if (fd_i2c < 0) {
    _error = -1;
    return 0;
  }
  

  // uint32_t value = 0;
  // if (bytes == _wire->requestFrom(_address, (uint8_t)bytes))
  // {
  //   for (int i = 0; i < bytes; i++)
  //   {
  //     value <<= 8;
  //     value |= _wire->read();
  //   }
  // }
  // else
  // {
  //   _error = -2;
  //   return 0;
  // }
  // return value;
  // For 2-byte reads, wiringPi has a convenient helper function.
  if (bytes == 2) {
    int result = wiringPiI2CReadReg16(fd_i2c, reg);
    if (result < 0) {
      _error = -1; // Read error
      return 0;
    }
    // The INA228 is big-endian, and wiringPiI2CReadReg16 returns the value
    // with bytes swapped to match the host's endianness (usually little-endian).
    // ntohs (network to host short) to swap them back.
    return ntohs((uint16_t)result);
  }

  // For other lengths (1, 3, 4, 5 bytes), we use raw read/write.
  // write the register address we want to read from.
  if (write(fd_i2c, &reg, 1) != 1) {
    _error = -1; // Write error
    return 0;
  }

  uint8_t buffer[5] = {0}; // Max 5 bytes for energy/charge
  if (read(fd_i2c, buffer, bytes) != bytes) {
    _error = -2; // Read error
    return 0;
  }

  uint32_t value = 0;
  for (int i = 0; i < bytes; i++) {
    value = (value << 8) | buffer[i];
  }
  return value;
}


//  always 5 bytes
double INA228::_readRegisterF(uint8_t reg, char mode)
{
  // _error = 0;
  // _wire->beginTransmission(_address);
  // _wire->write(reg);
  // int n = _wire->endTransmission();
  // if (n != 0)
  // {
  //   _error = -1;
  //   return 0;
  // }

  // double value = 0;
  // if (5 == _wire->requestFrom(_address, (uint8_t)5))
  // {
  //   uint32_t val = 0;
  //   //  fetch 4 MSB bytes first.
  //   for (int i = 0; i < 4; i++)
  //   {
  //     val <<= 8;
  //     val |= _wire->read();
  //   }
  //   //  handle signed / unsigned by casting.
  //   if (mode == 'U') value = val;
  //   else             value = (int32_t) val;
  //   //  process last byte
  //   value *= 256;
  //   //  note: mar05c
  //   uint8_t n = _wire->read();
  //   value += n;
  // }
  // else
  // {
  //   _error = -2;
  //   return 0;
  // }
  // return value;
  _error = 0;
  if (fd_i2c < 0) {
      _error = -10; // Not initialized
      return 0;
  }
  
  // First, write the register address we want to read from.
  if (write(fd_i2c, &reg, 1) != 1) {
      _error = -1; // Write error
      return 0;
  }

  uint8_t buffer[5];
  if (read(fd_i2c, buffer, 5) != 5) {
      _error = -2; // Read error
      return 0;
  }
  
  // Assemble the 40-bit value
  uint64_t raw_value = 0;
  for (int i = 0; i < 5; i++) {
      raw_value = (raw_value << 8) | buffer[i];
  }
  
  double value = 0;
  if (mode == 'S') { // Signed
      // Sign-extend if the 40th bit is set
      if (raw_value & 0x8000000000ULL) {
          value = (double)((int64_t)(raw_value | 0xFFFFFF0000000000ULL));
      } else {
          value = (double)raw_value;
      }
  } else { // Unsigned
      value = (double)raw_value;
  }

  return value;
}


uint16_t INA228::_writeRegister(uint8_t reg, uint16_t value)
{
  // _wire->beginTransmission(_address);
  // _wire->write(reg);
  // _wire->write(value >> 8);
  // _wire->write(value & 0xFF);
  // int n = _wire->endTransmission();
  // if (n != 0)
  // {
  //   _error = -1;
  // }
  // return n;
  if (fd_i2c < 0) {
    _error = -10; // Not initialized
    return -1;
  }
  
  // The INA228 expects data in big-endian format (MSB first).
  // wiringPiI2CWriteReg16 sends data in little-endian format (LSB first).
  // So, we need to swap the bytes of 'value' before sending.
  uint16_t swapped_value = (value >> 8) | (value << 8);

  int result = wiringPiI2CWriteReg16(fd_i2c, reg, swapped_value);
  if (result != 0) {
    _error = -1; // Write error
  }
  return result;
}


//  -- END OF FILE --

