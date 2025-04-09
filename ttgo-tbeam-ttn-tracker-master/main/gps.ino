/*

  GPS module

  Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <TinyGPS++.h>
#include <TimerOne.h>
// #include <DFRobot_BME680_I2C.h>

#define NO_CONNECTION_PRINT_COUNT 200000
#define GPS_POLL_INTERVAL 3000

bool isDataFresh = true;
uint32_t latitude;
uint32_t longitude;
uint32_t altitude;
uint8_t hdop;
uint8_t sats;

long lastGPSCheck = 0;
long gpsCheckDeltaCounter = 0;
volatile bool allowGPSPoll = false;
volatile int measureState = 0;


float seaLevel;

// ----- Settings for the DO sensor -----
//#define DO_PIN A1          // Analog pin for the DO sensor
#define VREF 5000          // Reference voltage in mV
#define ADC_RES 1024       // ADC resolution
#define CALIBRATE_PRESSURE 15  // Use an accurate altitude (in meters) for sea level pressure calibration

// Single point calibration: at CAL1_T°C the sensor should output a voltage of CAL1_v (in mV)
#define CAL1_v 1600        // mV
#define CAL1_T 25          // °C

#define PH_PIN 4           // Connected to analog pin 4

// Table with saturation values (in mg/L * 1000) for temperatures from 0°C to 40°C
const uint16_t Do_Table[41] = {
  14460, 13940, 13500, 13090, 12700, 12310, 12010, 11710, 11440, 11170,
  10920, 10690, 10460, 10250, 10040, 9840, 9650, 9460, 9270, 9080,
  8910, 8740, 8570, 8410, 8250, 8090, 7930, 7780, 7630, 7490,
  7350, 7210, 7070, 6940, 6810, 6680, 6560, 6440, 6320, 6200,
  6090
};

//DFRobot_BME680_I2C bme(0x77);
TinyGPSPlus gps;
HardwareSerial _serial_gps(GPS_SERIAL_NUM);

void gps_time(char *buffer, uint8_t size)
{
    snprintf(buffer, size, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
}

void gps_setup()
{
#if !DUMMY_DATA
    _serial_gps.begin(9600, SERIAL_8N1, 34, 12);
#endif
    Timer1.initialize(GPS_POLL_INTERVAL * 1000);
    Timer1.attachInterrupt(pollGPS);
    // if (!bme.begin())
    // {
    //     DEBUG_PORT.println("Failed to find BME680");
    //     while (true);
    // }

// #ifdef CALIBRATE_PRESSURE
//     bme.startConvert();
//     delay(1000);
//     bme.update();
//     // Calibrate sea level pressure using an accurate altitude (15m in this case)
//     seaLevel = bme.readSeaLevel(CALIBRATE_PRESSURE);
//     Serial.print("Sea level pressure: ");
//     Serial.println(seaLevel);
// #endif

    //pinMode(PH_PIN, INPUT);
    //pinMode(DO_PIN, INPUT);
}

PayloadData payload;

void gps_loop()
{
    switch (measureState)
    {
        case 0:
            //bme.startConvert();
            break;
        case 1:
            //bme.update();
            payload.temperature = 26.4;//bme.readTemperature();
            payload.pressure = 101325;//bme.readPressure();
            payload.humidity = 80.0;//bme.readHumidity();
            payload.gasResistance = 420.69;//bme.readGasResistance();
            payload.altitude = 40.0;//bme.readAltitude();
            payload.phValue = 7.2;//map(analogRead(PH_PIN), 0, 613, 0, 14);
            payload.doValue = 69.0;//((analogRead(DO_PIN) * VREF) / float(ADC_RES) / CAL1_v) *  Do_Table[constrain(int(payload.temperature + 0.5), 0, 40)] / 1000.0;
            break;
        default:
            measureState = 0;
            break;
    }

    if (allowGPSPoll)
    {
        allowGPSPoll = false;
        if (_serial_gps.available() > 0)
        {
            while (_serial_gps.available() > 0)
            {
                if (gps.encode(_serial_gps.read()))
                {
                    if (gps.location.isUpdated())
                    {
                        // noConnectionCounter = 0;
                        isDataFresh = true;
                        latitude = gps.location.lat();
                        longitude = gps.location.lng();
                        sats = gps.satellites.value();
                        hdop = gps.hdop.value();
                        altitude = gps.altitude.meters();
                        DEBUG_PORT.print("Latitude: ");
                        DEBUG_PORT.println(latitude);
                        DEBUG_PORT.print("Longitude: ");
                        DEBUG_PORT.println(longitude);
                        DEBUG_PORT.print("Altitude: ");
                        DEBUG_PORT.println(altitude);
                        DEBUG_PORT.print("hdop: ");
                        DEBUG_PORT.println(hdop);
                        DEBUG_PORT.print("Sats: ");
                        DEBUG_PORT.println(sats);
                    }
                }
            }
        }
        else
            DEBUG_PORT.println("NO GPS LOCK. Refusing send.");
    }
}

void pollGPS()
{
    measureState++;
#if !DUMMY_DATA
    allowGPSPoll = true;
#endif
}

bool buildPacket()
{
    int i = 0;
#if !DUMMY_DATA
    bool ironiousData = ((latitude | longitude) == 0);
    if (!isDataFresh || ironiousData)
        return false;
    else if (ironiousData)
        DEBUG_PORT.println("Stale/ironious GPS data. Aborting send.");

    txBuffer[i++] = latitude >> 16;
    txBuffer[i++] = latitude >> 8;
    txBuffer[i++] = latitude;
    txBuffer[i++] = longitude >> 16;
    txBuffer[i++] = longitude >> 8;
    txBuffer[i++] = longitude;
    txBuffer[i++] = altitude >> 16;
    txBuffer[i++] = altitude >> 8;
    txBuffer[i++] = altitude;
    txBuffer[i++] = sats;
    txBuffer[i++] = 't';
#else
    //DEBUG_PORT.println("DEBUG: Generating packet!");
    // PayloadData payload;
    // payload.longitude = 69.420;
    // payload.latitude = 420.69;
    // payload.altitude = 1234567890;
    // payload.satellites = (uint8_t)random(0, UINT8_MAX);
    // payload.hdop = 3.1416;

    Serial.println((int)&txBuffer, HEX);
    
    memcpy(&txBuffer, &payload, sizeof(payload));
    for (int j = 0; j < sizeof(PayloadData); j++)
    {
        Serial.print("0x");
        Serial.print(txBuffer[j], HEX);
        Serial.print(' ');
    }
    Serial.println(/*(int)&txBuffer, HEX*/);
#endif
    isDataFresh = false;
    return true;
}