#pragma once
#ifndef MAIN_H
#define MAIN_H

#include "Arduino.h"
#include "types.h"
#include "bsec.h"

// CONFIG
#define SEND_INTERVAL 300000ULL //5min
#define DATA_PORT 10
#define ERROR_PORT 69
#define ERROR_SLEEP_TIME 1200000ULL //20min
#define MAX_BME680_READ_TIME 200000UL //20sec 
#define DISABLE_SLEEP false
#define BME_PWR_PIN D8

// ----- Settings for the DO sensor -----
// #define DO_PIN A1    // Analog pin for the DO sensor
// #define VREF 3300    // Reference voltage in mV
// #define ADC_RES 4096 // ADC resolution

// Single point calibration: at CAL1_T°C the sensor should output a voltage of CAL1_v (in mV)
#define CAL1_v 1600 // mV
#define CAL1_T 25   // °C

// ----- pH sensor (example) -----
//#define PH_PIN A4 // Connected to analog pin 4

#pragma region LUTs
// Table with saturation values (in mg/L * 1000) for temperatures from 0°C to 40°C
// inline const uint16_t Do_Table[41] = 
// {
//   14460, 13940, 13500, 13090, 12700, 12310, 12010, 11710, 11440, 11170,
//   10920, 10690, 10460, 10250, 10040, 9840, 9650, 9460, 9270, 9080,
//   8910, 8740, 8570, 8410, 8250, 8090, 7930, 7780, 7630, 7490,
//   7350, 7210, 7070, 6940, 6810, 6680, 6560, 6440, 6320, 6200,
//   6090
// };

//F() or PROGMEM macros are not needed on ESP32 afaik they're even dummy macros
//const arrays are always stored in flash and mapped into memory as needed (=what flashstringhelper does)
//but change this on any other arduino :)
inline const char* ERROR_MSG[] = 
{
  "Failed to initialize BME680, can't send data!",
  "Failed to set property on BME680, check wiring!",
  "Failed to start BME680 heater, won't send ironious data!",
  "Connection to BME680 failed, check wiring!",
  "Help I'm scared i can't sleep! (sleep rejected)",
  "Reading BME680 timed out, probably bad connection!",
  "Failed to send data, probably LoRa hardware issue, maybe TTN"
};

//Error code + 5 = error message
//just a little sketchy...
inline const char* BME680_ERROR_MSG[] = 
{
  "BME680 (Err): Self test failed",
  "BME680 (Err): Incorrect length parameter",
  "BME680 (Err): Sensor not found",
  "BME680 (Err): Communication failure",
  "BME680 (Err): Null pointer passed",

  "BME680 (OK): this should never be sent this error code is not an error",

  "BME680 (Warn): Invalid operation mode",
  "BME680 (Warn): No new data was found ",
  "BME680 (Warn): Heating duration not defined"
};
#pragma endregion

// BME680
#define SEALEVELPRESSURE_HPA (1013.25)

//iotSensor.cpp
void sendMessage();

void sendError(int idNum, const char* msg);

bool sleepFor(unsigned long long us, bool deepSleep);

void lmicCallback(uint8_t message);

bool isDeepSleepWakeCause(esp_sleep_wakeup_cause_t reason);

int multiSampleAnalogRead(uint8_t pin, uint8_t samples);

//bme680.cpp
extern Bsec bme;
// extern RTC_DATA_ATTR bool hasState;
// extern RTC_DATA_ATTR uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE];

bool bme680Begin();

BME680_Status checkSensorStatus();

PayloadDataFull readPayload();

bool checkAndSendBmeError(int idNum);

bool bme680Subscribe();

bool saveState();

bool loadState();

#endif