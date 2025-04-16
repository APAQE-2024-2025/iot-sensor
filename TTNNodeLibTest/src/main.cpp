#include "main.h"
#include "types.h"
#include "TTNNode.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

//unsigned long nextSendTime = SEND_INTERVAL;
bool transmitComplete = false;
bool sentError = false;
bool disableSleep = false;

Adafruit_BME680 bme(&Wire); // I2C

void setup()
{
  Serial.begin(115200);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (isDeepSleepWakeCause(wakeup_reason))
    Serial.println("Awoke from deep sleep!");
  
  TTNNode::lmic_callbacks.push_back(lmicCallback);
  bool setupSuccess = TTNNode::begin();
  Serial.println(setupSuccess ? "Setup success :)" : "Setup failed :(");
  
  if (!bme.begin()) 
  {
    Serial.println("Failed to init BME680, reporting!");
    sendError(ERROR::ERR_BME_INIT_FAIL);
  }

  bool setPropSuccess = true;
  setPropSuccess =  bme.setTemperatureOversampling(BME680_OS_8X);
  setPropSuccess =  bme.setHumidityOversampling(BME680_OS_2X);
  setPropSuccess =  bme.setPressureOversampling(BME680_OS_4X);
  setPropSuccess =  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  if (!setPropSuccess)
    sendError(ERROR::ERR_BME_SET_PROP_FAIL);
  if (!bme.setGasHeater(320, 150)) // 320*C for 150 ms
    sendError(ERROR::ERR_BME_HEATER_FAIL);

  pinMode(PH_PIN, INPUT);
  pinMode(DO_PIN, INPUT);

  TTNNode::update();

  if (!bme.performReading())
  {
    Serial.println("Reading BME680 failed! skipping");
    sendError(ERROR::ERR_BME_CONN_FAIL);
    return;
  }
  sendMessage();
}

void loop()
{
  TTNNode::update();

  if (transmitComplete)
  {
    unsigned long long sleepTime = (sentError ? REBOOT_SLEEP_TIME_MS : SEND_INTERVAL) * 1000ULL;
    transmitComplete = sentError = false;
    delay(100); //TODO: remove with preprocessor directives its here to allow serial to flush before sleeping
    sleepFor(sleepTime, true); //will restart after
  }
  

  // unsigned long now = millis();
  // if (now >= nextSendTime)
  // {
  //   nextSendTime = now + SEND_INTERVAL;
  //   if (!bme.performReading()) 
  //   {
  //     Serial.println("Reading BME680 failed! skipping");
  //     sendError(ERROR::ERR_BME_CONN_FAIL);
  //     return;
  //   }
  //   sendMessage();
  // }
}

bool isDeepSleepWakeCause(esp_sleep_wakeup_cause_t reason)
{
  switch(reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :
    case ESP_SLEEP_WAKEUP_EXT1 :
    case ESP_SLEEP_WAKEUP_TIMER :
      return true;
    default:
      return false;
  }
}

void sendMessage()
{
  float temperature = bme.temperature;
  Serial.print(F("Temperature = "));
  Serial.print(temperature);
  Serial.println(F(" *C"));

  float pressure = bme.pressure / 100.0;
  Serial.print(F("Pressure = "));
  Serial.print(pressure);
  Serial.println(" hPa");

  float humidity = bme.humidity;
  Serial.print(F("Humidity = "));
  Serial.print(humidity);
  Serial.println(" %");

  float gasResistance = bme.gas_resistance / 1000.0;
  Serial.print(F("Gas = "));
  Serial.print(gasResistance);
  Serial.println(F(" KOhms"));

  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  Serial.print(F("Approx. Altitude = "));
  Serial.print(altitude);
  Serial.println(F(" m"));

  PayloadData payload;
  payload.temperature = temperature;
  payload.pressure = pressure;
  payload.humidity = humidity;
  payload.gasResistance = gasResistance;
  payload.altitude = altitude;
  //payload.ph = analogRead(PH_PIN); // TODO: calibrate better :)

  // int rawDO = analogRead(DO_PIN);
  // // Calculate the measured voltage in mV
  // float voltage = (rawDO * VREF) / float(ADC_RES);

  // // Use the measured temperature as water temperature (constrain between 0°C and 40°C)
  // int waterTemp = max(min(int(temperature + 0.5), 0), 40);
  // // Retrieve the saturation value (in mg/L) and divide by 1000
  // float saturation = Do_Table[waterTemp] / 1000.0;

  // // Calculate the DO value (mg/L):
  // // The ratio of measured voltage to calibration voltage gives the saturation fraction,
  // // multiplied by the saturation value yields the DO concentration.
  // payload.doValue = (voltage / CAL1_v) * saturation;

  TTNNode::send<PayloadData>(payload, DATA_PORT);
}

void sendError(ERROR error)
{
  const char* msg = ERROR_MSG[(int)error];
  Serial.println(msg);
  uint8_t* buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(msg));
  sentError = true;
  TTNNode::send(buffer, strlen(msg), (uint8_t)ERROR_PORT);
}

void lmicCallback(uint8_t message)
{
  if (message == EV_TXCOMPLETE)
  {
    transmitComplete = true;
  }
}

bool sleepFor(unsigned long long us, bool deepSleep)
{
  if (disableSleep)
  {
    disableSleep = false;
    delayMicroseconds(us);
    if (deepSleep)
      ESP.restart();
    return true;
  }
  
  bool success = true;
  success = ESP_OK == esp_sleep_enable_timer_wakeup(us);
  if (deepSleep)
    esp_deep_sleep_start();
  success = ESP_OK == esp_light_sleep_start();
  //disableSleep = !success;

  if (!success)
    sendError(ERROR::ERR_INSOMNIA);

  return success;
}


//Here for if i ever need it again
// switch (error)
  // {
  //   case ERROR::ERR_BME_INIT_FAIL:
  //     break;
  //   case ERROR::ERR_BME_HEATER_FAIL:
  //     break;
  //   case ERROR::ERR_BME_CONN_FAIL:
  //     break;
  //   case ERROR::ERR_BME_SET_PROP_FAIL:
  //     break;    
  //   default:
  //     break;
  // }