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

#define NO_CONNECTION_PRINT_COUNT 200000
#define GPS_POLL_INTERVAL 3000

bool isDataFresh = true;
uint32_t latitude;
uint32_t longitude;
uint32_t altitude;
uint8_t hdop;
uint8_t sats;

// unsigned long noConnectionCounter = 0;
long lastGPSCheck = 0;
long gpsCheckDeltaCounter = 0;
volatile bool allowGPSPoll = false;

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
}

void gps_loop()
{
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
#if !DUMMY_DATA
    allowGPSPoll = true;
#endif
}

bool buildPacket(uint8_t* txBuffer)
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
    txBuffer[i++] = (char)'t';
#else
    DEBUG_PORT.println("DEBUG: Generating packet!");
    for(; i < sizeof(txBuffer); i++)
        txBuffer[i] = i;
#endif
    isDataFresh = false;
    return true;
}