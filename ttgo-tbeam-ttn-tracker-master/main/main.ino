/*

  Main module

  # Modified by Kyle T. Gabriel to fix issue with incorrect GPS data for TTNMapper

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

#include "configuration.h"
#include "rom/rtc.h"
#include <TinyGPS++.h>
#include <Wire.h>

#include "axp20x.h"
AXP20X_Class axp;
bool pmu_irq = false;
String baChStatus = "No charging";

bool axp192_found = false;

bool packetSent, packetQueued;

#if defined(PAYLOAD_USE_FULL)
// includes number of satellites and accuracy
static uint8_t txBuffer[11];
#elif defined(PAYLOAD_USE_CAYENNE)
// CAYENNE DF
static uint8_t txBuffer[11] = {0x03, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

// deep sleep support
RTC_DATA_ATTR int bootCount = 0;
esp_sleep_source_t wakeCause; // the reason we booted this time

// -----------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------

// bool buildPacket(uint8_t txBuffer[]); // needed for platformio

/**
 * If we have a valid position send it to the server.
 * @return true if we decided to send.
 */
bool trySend()
{
        packetSent = false;

#if LORAWAN_CONFIRMED_EVERY > 0
    bool confirmed = (ttn_get_count() % LORAWAN_CONFIRMED_EVERY == 0);
    if (confirmed)
        DEBUG_PORT.println("confirmation enabled");
#else
    bool confirmed = false;
#endif

    packetQueued = true;

    if (!buildPacket(txBuffer))
        return false;

    ttn_send(txBuffer, sizeof(txBuffer), LORAWAN_PORT, confirmed);
    DEBUG_PORT.println("Sending packet...");
    return true;
}

void doDeepSleep(uint64_t msecToWake)
{
    DEBUG_PORT.printf("Entering deep sleep for %llu seconds\n", msecToWake / 1000);

    // not using wifi yet, but once we are this is needed to shutoff the radio hw
    // esp_wifi_stop();

    LMIC_shutdown(); // cleanly shutdown the radio

    if (axp192_found)
    {
        // turn on after initial testing with real hardware
        axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF); // LORA radio
        axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // GPS main power
    }

    // FIXME - use an external 10k pulldown so we can leave the RTC peripherals powered off
    // until then we need the following lines
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Only GPIOs which are have RTC functionality can be used in this bit map: 0,2,4,12-15,25-27,32-39.
    uint64_t gpioMask = (1ULL << BUTTON_PIN);

    // FIXME change polarity so we can wake on ANY_HIGH instead - that would allow us to use all three buttons (instead of just the first)
    gpio_pullup_en((gpio_num_t)BUTTON_PIN);

    esp_sleep_enable_ext1_wakeup(gpioMask, ESP_EXT1_WAKEUP_ALL_LOW);

    esp_sleep_enable_timer_wakeup(msecToWake * 1000ULL); // call expects usecs
    esp_deep_sleep_start();                              // TBD mA sleep current (battery)
}

void sleep()
{
#if SLEEP_BETWEEN_MESSAGES

    

    // Set the user button to wake the board
    sleep_interrupt(BUTTON_PIN, LOW);

    // We sleep for the interval between messages minus the current millis
    // this way we distribute the messages evenly every SEND_INTERVAL millis
    uint32_t sleep_for = (millis() < SEND_INTERVAL) ? SEND_INTERVAL - millis() : SEND_INTERVAL;
    doDeepSleep(sleep_for);

#endif
}

void callback(uint8_t message)
{
    bool ttn_joined = false;
    if (EV_JOINED == message)
    {
        ttn_joined = true;
    }
    if (EV_JOINING == message)
    {
        if (ttn_joined)
        {
            DEBUG_PORT.println("TTN joining...\n");
        }
        else
        {
            DEBUG_PORT.println("Joined TTN!\n");
        }
    }
    if (EV_JOIN_FAILED == message)
        DEBUG_PORT.println("TTN join failed\n");
    if (EV_REJOIN_FAILED == message)
        DEBUG_PORT.println("TTN rejoin failed\n");
    if (EV_RESET == message)
        DEBUG_PORT.println("Reset TTN connection\n");
    if (EV_LINK_DEAD == message)
        DEBUG_PORT.println("TTN link dead\n");
    if (EV_ACK == message)
        DEBUG_PORT.println("ACK received\n");
    if (EV_PENDING == message)
        DEBUG_PORT.println("Message discarded\n");
    if (EV_QUEUED == message)
        DEBUG_PORT.println("Message queued\n");

    // We only want to say 'packetSent' for our packets (not packets needed for joining)
    if (EV_TXCOMPLETE == message && packetQueued)
    {
        DEBUG_PORT.println("Message sent\n");
        packetQueued = false;
        packetSent = true;
    }

    if (EV_RESPONSE == message)
    {
        DEBUG_PORT.println("[TTN] Response: ");

        size_t len = ttn_response_len();
        uint8_t data[len];
        ttn_response(data, len);

        char buffer[6];
        for (uint8_t i = 0; i < len; i++)
        {
            snprintf(buffer, sizeof(buffer), "%02X", data[i]);
            DEBUG_PORT.println(buffer);
        }
        DEBUG_PORT.println("\n");
    }
}

void scanI2Cdevice(void)
{
    byte err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        err = Wire.endTransmission();
        if (err == 0)
        {
            DEBUG_PORT.print("I2C device found at address 0x");
            if (addr < 16)
                DEBUG_PORT.print("0");
            DEBUG_PORT.print(addr, HEX);
            DEBUG_PORT.println(" !");
            nDevices++;

            if (addr == AXP192_SLAVE_ADDRESS)
            {
                axp192_found = true;
                DEBUG_PORT.println("axp192 PMU found");
            }
        }
        else if (err == 4)
        {
            DEBUG_PORT.print("Unknow error at address 0x");
            if (addr < 16)
                DEBUG_PORT.print("0");
            DEBUG_PORT.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        DEBUG_PORT.println("No I2C devices found\n");
    else
        DEBUG_PORT.println("done\n");
}

/**
 * Init the power manager chip
 *
 * axp192 power
    DCDC1 0.7-3.5V @ 1200mA max -> OLED  // If you turn this off you'll lose comms to the axp192 because the OLED and the axp192 share the same i2c bus, instead use ssd1306 sleep mode
    DCDC2 -> unused
    DCDC3 0.7-3.5V @ 700mA max -> ESP32 (keep this on!)
    LDO1 30mA -> charges GPS backup battery  // charges the tiny J13 battery by the GPS to power the GPS ram (for a couple of days), can not be turned off
    LDO2 200mA -> LORA
    LDO3 200mA -> GPS
 */

void axp192Init()
{
    if (axp192_found)
    {
        if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS))
        {
            DEBUG_PORT.println("AXP192 Begin PASS");
        }
        else
        {
            DEBUG_PORT.println("AXP192 Begin FAIL");
        }
        // axp.setChgLEDMode(LED_BLINK_4HZ);
        DEBUG_PORT.printf("DCDC1: %s\n", axp.isDCDC1Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("DCDC2: %s\n", axp.isDCDC2Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("LDO2: %s\n", axp.isLDO2Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("LDO3: %s\n", axp.isLDO3Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("DCDC3: %s\n", axp.isDCDC3Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("Exten: %s\n", axp.isExtenEnable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.println("----------------------------------------");

        axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); // LORA radio
        axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); // GPS main power
        axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
        axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
        axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
        axp.setDCDC1Voltage(3300); // for the OLED power

        DEBUG_PORT.printf("DCDC1: %s\n", axp.isDCDC1Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("DCDC2: %s\n", axp.isDCDC2Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("LDO2: %s\n", axp.isLDO2Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("LDO3: %s\n", axp.isLDO3Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("DCDC3: %s\n", axp.isDCDC3Enable() ? "ENABLE" : "DISABLE");
        DEBUG_PORT.printf("Exten: %s\n", axp.isExtenEnable() ? "ENABLE" : "DISABLE");

        pinMode(PMU_IRQ, INPUT_PULLUP);
        attachInterrupt(PMU_IRQ, []
                        { pmu_irq = true; }, FALLING);

        axp.adc1Enable(AXP202_BATT_CUR_ADC1, 1);
        axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ, 1);
        axp.clearIRQ();

        if (axp.isCharging())
        {
            baChStatus = "Charging";
        }
    }
    else
    {
        DEBUG_PORT.println("AXP192 not found");
    }
}

// Perform power on init that we do on each wake from deep sleep
void initDeepSleep()
{
    bootCount++;
    wakeCause = esp_sleep_get_wakeup_cause();
    /*
    Not using yet because we are using wake on all buttons being low

    wakeButtons = esp_sleep_get_ext1_wakeup_status();        // If one of these buttons is set it was the reason we woke
    if (wakeCause == ESP_SLEEP_WAKEUP_EXT1 && !wakeButtons)  // we must have been using the 'all buttons rule for waking' to support busted boards, assume button one was pressed
        wakeButtons = ((uint64_t)1) << buttons.gpios[0];
    */

    DEBUG_PORT.printf("booted, wake cause %d (boot count %d)\n", wakeCause, bootCount);
}

void setup()
{

// Debug
#ifdef DEBUG_PORT
    DEBUG_PORT.begin(SERIAL_BAUD);
#endif

    initDeepSleep();

    Wire.begin(I2C_SDA, I2C_SCL);
    scanI2Cdevice();
    axp192Init();

    // Buttons & LED
    pinMode(BUTTON_PIN, INPUT_PULLUP);

#ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
#endif

    // Hello
    DEBUG_MSG(APP_NAME " " APP_VERSION "\n");

    // TTN setup
    if (!ttn_setup())
    {
        if (REQUIRE_RADIO)
        {
            delay(MESSAGE_TO_SLEEP_DELAY);
            sleep_forever();
        }
    }
    else
    {
        gps_setup();
        ttn_register(callback);
        ttn_join();
        ttn_adr(LORAWAN_ADR);
    }
    DEBUG_PORT.println("Setup finished!");
}

void loop()
{
    ttn_loop();
    if (packetSent)
    {
        packetSent = false;
        sleep();
    }

    // if user presses button for more than 3 secs, discard our network prefs and reboot (FIXME, use a debounce lib instead of this boilerplate)
    static bool wasPressed = false;
    static uint32_t minPressMs; // what tick should we call this press long enough
    if (!digitalRead(BUTTON_PIN))
    {
        if (!wasPressed)
        {
            // just started a new press
            DEBUG_PORT.println("pressing");
            wasPressed = true;
            minPressMs = millis() + 3000;
        }
    }
    else if (wasPressed)
    {
        // we just did a release
        wasPressed = false;
        if (millis() > minPressMs)
        {
// held long enough
#ifndef PREFS_DISCARD
            DEBUG_PORT.println("Discarding prefs disabled\n");
#endif

#ifdef PREFS_DISCARD
            DEBUG_PORT.println("Discarding prefs!\n");
            ttn_erase_prefs();
            delay(5000); // Give some time to read the screen
            ESP.restart();
#endif
        }
    }

    gps_loop();

    // Send every SEND_INTERVAL millis
    static uint32_t last = 0;
    static bool first = true;
    if (millis() - last > SEND_INTERVAL)
    {
        if (trySend())
        {
            last = millis();
            first = false;
            DEBUG_PORT.println("TRANSMITTED");
        }
        
    }
}