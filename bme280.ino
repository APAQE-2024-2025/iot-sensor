#include <Wire.h>
#include <Adafruit_BME280.h>
#include <math.h>

#define LOCAL_ALTITUDE 35.0  // Your altitude in meters

Adafruit_BME280 bme;  // I2C (default: 0x77)

// ----- Settings for the DO sensor -----
#define DO_PIN A1          // Analog pin for the DO sensor
#define VREF 5000          // Reference voltage in mV
#define ADC_RES 1024       // ADC resolution

// Single point calibration: at CAL1_T°C the sensor should output a voltage of CAL1_v (in mV)
#define CAL1_v 1600        // mV
#define CAL1_T 25          // °C

// ----- pH sensor (example) -----
#define PH_PIN 4           // Connected to analog pin 4

// Table with saturation values (in mg/L * 1000) for temperatures from 0°C to 40°C
const uint16_t Do_Table[41] = {
  14460, 13940, 13500, 13090, 12700, 12310, 12010, 11710, 11440, 11170,
  10920, 10690, 10460, 10250, 10040, 9840, 9650, 9460, 9270, 9080,
  8910, 8740, 8570, 8410, 8250, 8090, 7930, 7780, 7630, 7490,
  7350, 7210, 7070, 6940, 6810, 6680, 6560, 6440, 6320, 6200,
  6090
};

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for the serial monitor
  delay(1000);

  Serial.println("Initializing BME280...");
  if (!bme.begin(0x77)) {  // Try 0x76 if this doesn't work
    Serial.println("BME280 not found! Check the wiring.");
    while (1);  // Hang if the sensor fails
  }
  Serial.println("BME280 OK!");

  // Set the pins
  pinMode(PH_PIN, INPUT);
  pinMode(DO_PIN, INPUT);
}

void loop() {
  Serial.println();

  // Read and print the BME280 values
  float temperature = bme.readTemperature();
  Serial.print("Temperature (C): ");
  Serial.println(temperature, 2);

  float pressure = bme.readPressure();
  Serial.print("Pressure (hPa): ");
  Serial.println(pressure / 100.0, 2);  // Convert Pa to hPa

  float humidity = bme.readHumidity();
  Serial.print("Humidity (%RH): ");
  Serial.println(humidity, 2);

  // Calculate the altitude, corrected for your LOCAL_ALTITUDE
  float seaLevelPressure = pressure / pow(1 - (LOCAL_ALTITUDE / 44330.0), 5.255);
  float altitude = bme.readAltitude(seaLevelPressure / 100.0);
  Serial.print("Altitude (m): ");
  Serial.println(altitude, 2);

  // Example pH measurements (apply your own calibration if needed)
  int rawPH = analogRead(PH_PIN);
  int phValue = map(rawPH, 0, 613, 0, 14);
  Serial.print("pH: ");
  Serial.println(phValue);

  // ----- DO sensor measurement -----
  int rawDO = analogRead(DO_PIN);
  // Calculate the measured voltage in mV
  float voltage = (rawDO * VREF) / float(ADC_RES);

  // Use the measured temperature as water temperature (constrain between 0°C and 40°C)
  int waterTemp = constrain(int(temperature + 0.5), 0, 40);
  // Retrieve the saturation value (in mg/L) and divide by 1000
  float saturation = Do_Table[waterTemp] / 1000.0;

  // Calculate the DO value (mg/L):
  // The ratio measured voltage / calibration voltage gives the saturation fraction,
  // multiplied by the saturation value yields the DO concentration.
  float doValue = (voltage / CAL1_v) * saturation;
  Serial.print("Dissolved Oxygen (mg/L): ");
  Serial.println(doValue, 2);
  // -----------------------------

  delay(2000);  // Wait 2 seconds before the next measurement
}
