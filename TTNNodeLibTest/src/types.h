#pragma once
#ifndef TYPES_H
#define TYPES_H

// Update ERROR_MSG in main.h if changing this
enum ERROR
{
  ERR_BME_INIT_FAIL = 0, //not really used anymore
  ERR_BME_SET_PROP_FAIL = 1, //not really used anymore
  ERR_BME_HEATER_FAIL = 2, //not really used anymore
  ERR_BME_CONN_FAIL = 3, //not really used anymore
  ERR_INSOMNIA = 4,
  ERR_BME_TIMEOUT = 5
};

struct PayloadData
{
  float airQualityIndex;
  float airQualityIndexAccuracy;
  float staticAirQualityIndex;
  float co2Eq;
  float voc;
  float rawTemp;
  float pressure;
  float rawHumidity;
  float rawGasResistance;
  float stabilizationStatus;
  float runInStatus;
  float temperature;
  float humidity;
  float gasPercentage;
  float batteryVoltage;
  // float ph;
  // float doValue;
  // float seaLevel;
};

struct BME680_Status
{
  bool success;
  const char *errorMsg;

  BME680_Status()
  {
    success = true;
    errorMsg = nullptr;
  }

  BME680_Status(const char *msg)
  {
    success = false;
    errorMsg = msg;
  }
};

#endif