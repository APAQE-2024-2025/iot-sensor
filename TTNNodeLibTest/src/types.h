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
  ERR_BME_TIMEOUT = 5,
  ERR_LORA_SEND_FAIL = 6
};

enum PayloadFlags
{
  PAYLOAD_FULL = 0b00000001
};

//BOTH STRUCTS HAVE TO BE THE SAME UP UNTIL THE "additional data" PART!!!
#pragma pack(1)
struct PayloadDataFull
{
  uint8_t flags = PayloadFlags::PAYLOAD_FULL;
  float pressure;
  float rawHumidity;
  float rawGasResistance;
  float stabilizationStatus;
  float runInStatus;
  float rawTemp;
  float batteryVoltage;

  //additional data
  float airQualityIndex;
  float airQualityIndexAccuracy;
  float staticAirQualityIndex;
  float co2Eq;
  float voc;
  float gasPercentage;
  float temperature;
  float humidity;
  // float ph;
  // float doValue;
  // float seaLevel;
};

#pragma pack(1)
struct PayloadData
{
  uint8_t flags = 0;
  float pressure;
  float humidity;
  float gasResistance;
  float stabilizationStatus;
  float runInStatus;
  float temperature;
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