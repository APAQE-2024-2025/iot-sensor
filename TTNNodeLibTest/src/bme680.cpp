// now sensor code is split up to save my sanity :)
#include "main.h"
#include "bsec.h"
#include "types.h"

Bsec bme;
RTC_DATA_ATTR uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
RTC_DATA_ATTR bool hasState = false;

bool loadState()
{
    if (!hasState)
    {
        Serial.println("No state loaded");
    }    
    bme.setState(bsecState);
    return checkAndSendBmeError(6);
}

bool saveState()
{
    Serial.println("Saving state...");
    hasState = true;   
    bme.getState(bsecState);
    return checkAndSendBmeError(7);
}

bool bme680Begin()
{
    digitalWrite(BME_PWR_PIN, true);
    delay(10); //let power stabilize
    bme.begin(BME68X_I2C_ADDR_HIGH, Wire);
    return checkAndSendBmeError(1);
}

bool bme680Subscribe()
{
    bsec_virtual_sensor_t sensorList[13] =
    {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_GAS_PERCENTAGE
    };

    bme.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_CONT);
    return checkAndSendBmeError(2);
}

bool checkAndSendBmeError(int idNum)
{
    BME680_Status status = checkSensorStatus();
    bool success = status.success;
    if (!success)
    {
        sendError(idNum, status.errorMsg);
    }
    return success;
}

PayloadDataFull readPayload()
{
    int timeout = 0;
    PayloadDataFull payload;
    Retry:
    unsigned long startTime = millis();
    

    while (true)
    {
        if (millis() - startTime >= MAX_BME680_READ_TIME)
        {
            timeout++;
            break;
        }

        if (bme.run())
        {
            payload.airQualityIndex = bme.iaq;
            payload.airQualityIndexAccuracy = bme.iaqAccuracy;
            payload.staticAirQualityIndex = bme.staticIaq;
            payload.co2Eq = bme.co2Equivalent;
            payload.voc = bme.breathVocEquivalent;
            payload.rawTemp = bme.rawTemperature;
            payload.pressure = bme.pressure / 100.0;
            payload.rawHumidity = bme.rawHumidity;
            payload.rawGasResistance = bme.gasResistance;
            payload.stabilizationStatus = bme.stabStatus;
            payload.runInStatus = bme.runInStatus;
            payload.temperature = bme.temperature;
            payload.humidity = bme.humidity;
            payload.gasPercentage = bme.gasPercentage;
            return payload;
        }
        else
        {
            sleepFor(100000, false);
        }
    }

    if (timeout < 2)
    {
        digitalWrite(BME_PWR_PIN, false);
        delay(100);
        bme680Begin();
        bme680Subscribe();
        delay(100);
        goto Retry;
    } 

    if (timeout)
    {
        sendError(4, ERROR_MSG[(int)ERROR::ERR_BME_TIMEOUT]);
    }

    return payload;
}

BME680_Status checkSensorStatus()
{
    // skip bme.bsecStatus for now

    if (bme.bme68xStatus != BME68X_OK)
    {
        if (bme.bme68xStatus < BME68X_OK) // error
        {
            BME680_Status status(BME680_ERROR_MSG[bme.bme68xStatus + 5]);
            return status;
        }
        // else //warning
        // {

        // }
        // skip warnings for now
    }
    BME680_Status status;
    return status;
}