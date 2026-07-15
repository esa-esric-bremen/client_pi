#pragma once
//    FILE: INA228.h
//  AUTHOR: Rob Tillaart
//  MODIFIED BY: Andreas Bresser for ESA Space Resources Challenge
// VERSION: 0.4.0-ESA-SRC
//    DATE: 2025-10-01
// PURPOSE: Arduino library for the INA228, I2C, 20 bit, voltage, current and power sensor.
//     URL: https://github.com/RobTillaart/INA228
//          https://www.adafruit.com/product/5832           ( 10 A version)
//          https://www.mateksys.com/?portfolio=i2c-ina-bm  (200 A version))
//
//
//  Read the datasheet for the details


#include <iomanip>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>


#define INA228_LIB_VERSION          (F("0.4.0"))


#include "ina288_enums.h"


class INA228
{
public:
  //  address between 0x40 and 0x4F
  explicit INA228(const uint8_t address);

  bool     begin();
  bool     isConnected();
  uint8_t  getAddress();

  //
  //  CORE FUNCTIONS + scale wrappers.
  //
  //       BUS VOLTAGE
  float    getBusVoltage();     //  Volt
  float    getBusVolt()         { return getBusVoltage(); };
  float    getBusMilliVolt()    { return getBusVoltage()   * 1e3; };
  float    getBusMicroVolt()    { return getBusVoltage()   * 1e6; };

  //       SHUNT VOLTAGE
  float    getShuntVoltage();   //  Volt
  float    getShuntVolt()       { return getShuntVoltage(); };
  float    getShuntMilliVolt()  { return getShuntVoltage() * 1e3; };
  float    getShuntMicroVolt()  { return getShuntVoltage() * 1e6; };

  //  raw integer interface.
  int32_t  getShuntVoltageRAW();
  
  //       SHUNT CURRENT
  float    getCurrent();        //  Ampere
  float    getAmpere()          { return getCurrent(); };
  float    getMilliAmpere()     { return getCurrent()      * 1e3; };
  float    getMicroAmpere()     { return getCurrent()      * 1e6; };

  //       POWER
  float    getPower();          //  Watt
  float    getWatt()            { return getPower(); };
  float    getMilliWatt()       { return getPower()        * 1e3; };
  float    getMicroWatt()       { return getPower()        * 1e6; };
  float    getKiloWatt()        { return getPower()        * 1e-3; };

  //       TEMPERATURE
  float    getTemperature();    //  Celsius

  //  the Energy and Charge functions are returning double as they have higher accuracy.
  //       ENERGY
  double   getEnergy();         //  Joule or watt second
  double   getJoule()           { return getEnergy(); };
  double   getMegaJoule()       { return getEnergy()       * 1e-6; };
  double   getKiloJoule()       { return getEnergy()       * 1e-3; };
  double   getMilliJoule()      { return getEnergy()       * 1e3; };
  double   getMicroJoule()      { return getEnergy()       * 1e6; };
  double   getWattHour()        { return getEnergy()       * (1.0  / 3600.0); };
  double   getKiloWattHour()    { return getEnergy()       * (1e-3 / 3600.0); };

  //       CHARGE
  double   getCharge();         //  Coulombs
  double   getCoulomb()         { return getCharge(); };
  double   getMilliCoulomb()    { return getCharge()       * 1e3; };
  double   getMicroCoulomb()    { return getCharge()       * 1e6; };


  //
  //  CONFIG REGISTER 0
  //  read datasheet for details, section 7.6.1.1, page 22
  //
  void     reset();
  //  value: 0 == normal operation,  1 = clear registers
  //   bool     resetEnergyCharge(uint8_t value);  better name for setAccumulation()??
  //  [[deprecated("Use ")]]  ??
  bool     setAccumulation(uint8_t value);  //  idem
  bool     getAccumulation();
  //  Conversion delay in 0..255 steps of 2 ms
  void     setConversionDelay(uint8_t steps);
  uint8_t  getConversionDelay();
  void     setTemperatureCompensation(bool on);
  bool     getTemperatureCompensation();
  //  flag = false => 164 mV, true => 41 mV
  bool     setADCRange(bool flag);
  bool     getADCRange();

  //
  //  CONFIG ADC REGISTER 1
  //  read datasheet for details, section 7.6.1.2, page 22++
  //
  bool     setMode(uint8_t mode = INA228_MODE_CONT_TEMP_BUS_SHUNT);
  uint8_t  getMode();
  //  default value = ~1 milliseconds for all.
  bool     setBusVoltageConversionTime(uint8_t bvct = INA228_1052_us);
  uint8_t  getBusVoltageConversionTime();
  bool     setShuntVoltageConversionTime(uint8_t svct = INA228_1052_us);
  uint8_t  getShuntVoltageConversionTime();
  bool     setTemperatureConversionTime(uint8_t tct = INA228_1052_us);
  uint8_t  getTemperatureConversionTime();
  bool     setAverage(uint8_t avg = INA228_1_SAMPLE);
  uint8_t  getAverage();

  //
  //  SHUNT CALIBRATION REGISTER 2
  //  read datasheet for details. use with care.
  //  maxCurrent <= 204, (in fact no limit)
  //  shunt >= 0.0001.
  //  returns _current_LSB;
  int      setMaxCurrentShunt(float maxCurrent, float shunt);
  bool     isCalibrated()    { return _current_LSB != 0.0; };
  float    getMaxCurrent();
  float    getShunt();
  float    getCurrentLSB();

  //
  //  SHUNT TEMPERATURE COEFFICIENT REGISTER 3
  //  read datasheet for details, page 16.
  //  ppm = 0..16383.
  bool     setShuntTemperatureCoefficent(uint16_t ppm = 0);
  uint16_t getShuntTemperatureCoefficent();


  //
  //  DIAGNOSE ALERT REGISTER 11  (0x0B)
  //  read datasheet for details, section 7.6.1.12, page 26++.
  //
  void     setDiagnoseAlert(uint16_t flags);
  uint16_t getDiagnoseAlert();
  //  INA228.h has an enum for the bit fields.
  //  See ina228_diag_enum above
  void     setDiagnoseAlertBit(uint8_t bit);
  void     clearDiagnoseAlertBit(uint8_t bit);
  uint16_t getDiagnoseAlertBit(uint8_t bit);


  //
  //  THRESHOLD AND LIMIT REGISTERS 12-17
  //  read datasheet for details, section 7.3.7, page 16++
  //
  //  TODO - design and implement better API?
  //
  void     setShuntOvervoltageTH(uint16_t threshold);
  uint16_t getShuntOvervoltageTH();
  void     setShuntUndervoltageTH(uint16_t threshold);
  uint16_t getShuntUndervoltageTH();
  void     setBusOvervoltageTH(uint16_t threshold);
  uint16_t getBusOvervoltageTH();
  void     setBusUndervoltageTH(uint16_t threshold);
  uint16_t getBusUndervoltageTH();
  void     setTemperatureOverLimitTH(uint16_t threshold);
  uint16_t getTemperatureOverLimitTH();
  void     setPowerOverLimitTH(uint16_t threshold);
  uint16_t getPowerOverLimitTH();


  //
  //  MANUFACTURER and ID REGISTER 3E and 3F
  //
  //                               typical value
  uint16_t getManufacturer();  //  0x5449
  uint16_t getDieID();         //  0x0228
  uint16_t getRevision();      //  0x0001

  //
  //  ERROR HANDLING
  //
  int      getLastError();

private:
  //  max 4 bytes
  uint32_t _readRegister(uint8_t reg, uint8_t bytes);
  //  always 5 bytes, mode == U ==> unsigned, otherwise signed
  double   _readRegisterF(uint8_t reg, char mode);
  uint16_t _writeRegister(uint8_t reg, uint16_t value);

  float    _current_LSB;
  float    _shunt;
  float    _maxCurrent;
  bool     _ADCRange;

  uint8_t   _address;
  // use I2C from wiringPi instead of TwoWire Arduino library
  int       fd_i2c;

  int       _error;
};


//  -- END OF FILE --


