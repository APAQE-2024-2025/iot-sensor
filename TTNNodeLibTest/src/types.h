#ifndef TYPES_H
#define TYPES_H

enum ERROR
{
  ERR_BME_INIT_FAIL = 0,
  ERR_BME_SET_PROP_FAIL = 1,
  ERR_BME_HEATER_FAIL = 2,
  ERR_BME_CONN_FAIL = 3,
  ERR_INSOMNIA = 4
};

struct PayloadData
{
  float temperature;
  float pressure;
  float humidity;
  float gasResistance;
  float altitude;
  float battery_V;
  //float ph;
  //float doValue;
  //float seaLevel;
};

#endif