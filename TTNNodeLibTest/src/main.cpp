#include "main.h"
#include "types.h"
#include "TTNNode.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "bsec.h"
#include <string>

// unsigned long nextSendTime = SEND_INTERVAL;
bool transmitComplete = false;
bool sentError = false;
bool bmeCooked = false;

void setup()
{
    Serial.begin(115200);

#pragma region DisableWifiBT

    if (esp_wifi_deinit() != ESP_OK)
        Serial.println("Disabling wifi failed, already disabled?");

    Serial.print("Bluetooth mode status: ");
    switch (esp_bt_controller_get_status())
    {
    case ESP_BT_CONTROLLER_STATUS_IDLE:
        Serial.println("IDLE");
        break;
    case ESP_BT_CONTROLLER_STATUS_INITED:
        Serial.println("INITED");
        break;
    case ESP_BT_CONTROLLER_STATUS_ENABLED:
        Serial.println("ENABLED");
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }

    if (esp_bt_controller_disable() != ESP_OK)
        Serial.println("Failed to turn off Bluetooth");
#pragma endregion

    loadState();
    // pinMode(PH_PIN, INPUT);
    // pinMode(DO_PIN, INPUT);
    pinMode(A0, INPUT);
    pinMode(BME_PWR_PIN, OUTPUT);
    digitalWrite(BME_PWR_PIN, true); 

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (isDeepSleepWakeCause(wakeup_reason))
        Serial.println("Awoke from deep sleep!");

    TTNNode::lmic_callbacks.push_back(lmicCallback);
    bool setupSuccess = TTNNode::begin();
    if (!setupSuccess)
    {
        Serial.println("Setup failed :(");
        sleepFor(SEND_INTERVAL * 1000ULL, true);
        ESP.restart(); //unreachable, but just a catch-all
    }
    
    Serial.println("Setup success :)");
    
    TTNNode::update();

    //skip to loop
    if (!bme680Begin())
    {
        bmeCooked = true;
        return;
    }

    if (!bme680Subscribe())
    {
        bmeCooked = true;
        return;
    }
    
    sendMessage();
}

void loop()
{
    TTNNode::update();

    if (transmitComplete)
    {
        unsigned long long sleepTime = (sentError ? ERROR_SLEEP_TIME : SEND_INTERVAL) * 1000ULL;
        transmitComplete = sentError = false;
        // delay(100); //TODO: remove with preprocessor directives its here to allow serial to flush before sleeping
        sleepFor(sleepTime, true); // will restart after
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
    switch (reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
    case ESP_SLEEP_WAKEUP_TIMER:
        return true;
    default:
        return false;
    }
}

void sendMessage()
{   
    PayloadDataFull payload = readPayload();

    // if (payload == nullptr) //TODO
    // {
        
    // }

    Serial.print(F("Temperature = "));
    Serial.print(payload.temperature);
    Serial.println(F(" *C"));

    Serial.print(F("Pressure = "));
    Serial.print(payload.pressure);
    Serial.println(" hPa");

    float humidity = bme.humidity;
    Serial.print(F("Humidity = "));
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print(F("IAQ = "));
    Serial.println(payload.airQualityIndex);

    Serial.print(F("CO2 = "));
    Serial.print(payload.co2Eq);
    Serial.println(F(" ppm"));

    float batteryVoltage = (float)multiSampleAnalogRead(A0, 20) / 3661.922727272727 * 4.2;
    payload.batteryVoltage = batteryVoltage;

    Serial.print(F("Approx. voltage = "));
    Serial.print(batteryVoltage);
    Serial.println(F(" V"));

    if (!payload.runInStatus)
    {
        Serial.println("Sending small message...");
        PayloadData* trimmed = reinterpret_cast<PayloadData*>(&payload);
        (*trimmed).flags = 0;

        if(TTNNode::send<PayloadData>(*trimmed, DATA_PORT))
        {
            sendError(5 ,ERROR_MSG[(int)ERROR::ERR_LORA_SEND_FAIL]);
        }
        return;
    }

    payload.flags |= PayloadFlags::PAYLOAD_FULL;
    Serial.println("Sending full size message :)");

    if(TTNNode::send<PayloadDataFull>(payload, DATA_PORT))
    {
        sendError(5 ,ERROR_MSG[(int)ERROR::ERR_LORA_SEND_FAIL]);
    }
    // payload.ph = analogRead(PH_PIN); // TODO: calibrate better :)

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
}

void sendError(int idNum, const char *msg)
{
    std::string fullMsg = std::to_string(idNum) + ": " + msg;

    Serial.println(fullMsg.c_str());

    uint8_t *buffer = const_cast<uint8_t *>(
        reinterpret_cast<const uint8_t *>(fullMsg.c_str())
    );

    sentError = true;
    
    if (TTNNode::send(buffer, fullMsg.length(), (uint8_t)ERROR_PORT)) //true if error
    {
        sleepFor(ERROR_SLEEP_TIME * 1000ULL, true);
        ESP.restart(); //unreachable, but catch-all
    }
}


int multiSampleAnalogRead(uint8_t pin, uint8_t samples)
{
    int total = 0;
    for (size_t i = 0; i < samples; i++)
        total += analogRead(pin);
    return total / samples;
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
#if DISABLE_SLEEP
    delayMicroseconds(us);
    if (deepSleep)
        ESP.restart();
    return true;
#endif

    bool success = true;
    success = ESP_OK == esp_sleep_enable_timer_wakeup(us);
    if (deepSleep)
    {
        if (!bmeCooked) saveState();
        digitalWrite(BME_PWR_PIN, false);
        esp_deep_sleep_start();
    }
        
    success = ESP_OK == esp_light_sleep_start();

    if (!success)
        sendError(3, ERROR_MSG[(int)ERROR::ERR_INSOMNIA]);

    return success;
}

// Here for if i ever need it again
//  switch (error)
//  {
//    case ERROR::ERR_BME_INIT_FAIL:
//      break;
//    case ERROR::ERR_BME_HEATER_FAIL:
//      break;
//    case ERROR::ERR_BME_CONN_FAIL:
//      break;
//    case ERROR::ERR_BME_SET_PROP_FAIL:
//      break;
//    default:
//      break;
//  }