#include "Arduino.h"
#include "types.h"

#ifndef MAIN_H
#define MAIN_H

// CONFIG
#define SEND_INTERVAL 10000ULL//300000ULL
#define DATA_PORT 10
#define ERROR_PORT 69
#define ERROR_SLEEP_TIME 1200000ULL //20min

// ----- Settings for the DO sensor -----
// #define DO_PIN A1    // Analog pin for the DO sensor
// #define VREF 3300    // Reference voltage in mV
// #define ADC_RES 4096 // ADC resolution

// Single point calibration: at CAL1_T°C the sensor should output a voltage of CAL1_v (in mV)
#define CAL1_v 1600 // mV
#define CAL1_T 25   // °C

// ----- pH sensor (example) -----
//#define PH_PIN A4 // Connected to analog pin 4

// BME680
#define SEALEVELPRESSURE_HPA (1013.25)

// Table with saturation values (in mg/L * 1000) for temperatures from 0°C to 40°C
// const uint16_t Do_Table[41] = 
// {
//   14460, 13940, 13500, 13090, 12700, 12310, 12010, 11710, 11440, 11170,
//   10920, 10690, 10460, 10250, 10040, 9840, 9650, 9460, 9270, 9080,
//   8910, 8740, 8570, 8410, 8250, 8090, 7930, 7780, 7630, 7490,
//   7350, 7210, 7070, 6940, 6810, 6680, 6560, 6440, 6320, 6200,
//   6090
// };

//TODO: dont hold these in memory use F(); or smth similar
const char* ERROR_MSG[] = 
{
  "Failed to initialize BME680, can't send data!",
  "Failed to set property on BME680, check wiring!",
  "Failed to start BME680 heater, won't send ironious data!",
  "Connection to BME680 failed, check wiring!",
  "Help I'm scared i can't sleep! (sleep rejected)"
};


void sendMessage();

void sendError(ERROR error);

bool sleepFor(unsigned long long us, bool deepSleep);

void lmicCallback(uint8_t message);

bool isDeepSleepWakeCause(esp_sleep_wakeup_cause_t reason);

int multiSampleAnalogRead(uint8_t pin, uint8_t samples);

#endif