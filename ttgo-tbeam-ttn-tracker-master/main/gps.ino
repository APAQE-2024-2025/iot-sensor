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

uint32_t latitude;
uint32_t longitude;
uint16_t altitude;
uint8_t hdop;
uint8_t sats;
bool isDataFresh = true;
unsigned long infoCounter = 0;

TinyGPSPlus gps;
HardwareSerial _serial_gps(GPS_SERIAL_NUM);

void gps_time(char *buffer, uint8_t size)
{
    snprintf(buffer, size, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
}

void gps_setup()
{
    _serial_gps.begin(9600, SERIAL_8N1, 34, 12);
}

void gps_loop()
{   
    infoCounter++;
    while (_serial_gps.available() > 0)
    {
        if (gps.encode(_serial_gps.read()))
        {
            if (gps.location.isUpdated())
            {
                isDataFresh = true;
                latitude = gps.location.lat();
                longitude = gps.location.lng();
                sats = gps.satellites.value();
                hdop = gps.hdop.value();
                altitude = gps.altitude.meters();
                Serial.print("Latitude: ");
                Serial.println(latitude);
                Serial.print("Longitude: ");
                Serial.println(longitude);
                Serial.print("Altitude: ");
                Serial.println(altitude);
                Serial.print("hdop: ");
                Serial.println(hdop);
                Serial.print("Sats: ");
                Serial.println(sats);
            }
        }
    }
}

bool buildPacket(uint8_t txBuffer[10])
{
    if (!isDataFresh || ((latitude | longitude) == 0) ) 
        return false;

    int i = 0;
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
    isDataFresh = false;
    return true;
}