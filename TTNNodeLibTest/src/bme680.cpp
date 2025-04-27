// now sensor code is split up to save my sanity :)
#include "main.h"
#include "bsec.h"
#include "types.h"

Bsec bme;

bool bme680Begin()
{
    bme.begin(BME68X_I2C_ADDR_LOW, Wire);
    checkAndSendBmeError(1);

    return true;
}

void bme680Subscribe()
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
            BSEC_OUTPUT_GAS_PERCENTAGE};

    bme.updateSubscription(sensorList, 13, BSEC_SAMPLE_RATE_LP);
    checkAndSendBmeError(2);
}

void checkAndSendBmeError(int idNum)
{
    BME680_Status status = checkSensorStatus();
    if (!status.success)
    {
        sendError(idNum, status.errorMsg);
    }
}

PayloadData readPayload()
{
    PayloadData payload;
    unsigned long startTime = millis();
    bool timeout = false;

    while (true)
    {
        if (millis() - startTime >= MAX_BME680_READ_TIME)
        {
            timeout = true;
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
            payload.pressure = bme.pressure;
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

    if (timeout)
    {
        sendError(4, ERROR_MSG[(int)ERROR::ERR_BME_TIMEOUT]);
    }
    
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