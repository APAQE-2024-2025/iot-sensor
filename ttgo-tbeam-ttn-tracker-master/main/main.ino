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
    // Buttons & LED
    pinMode(BUTTON_PIN, INPUT_PULLUP);

#ifdef LED_PIN
    pinMode(LED_PIN, OUTPUT);
#endif

    DEBUG_MSG(APP_NAME " " APP_VERSION "\n");
    DEBUG_PORT.println("Init TTN...");

    if (!ttn_setup())
    {
        DEBUG_PORT.println("lora not found");
        if (REQUIRE_RADIO)
        {
            delay(MESSAGE_TO_SLEEP_DELAY);
            sleep_forever();
        }
    }
    else
    {
        DEBUG_PORT.println("ttn setup successful");
        gps_setup();
        ttn_register(callback);
        DEBUG_PORT.println("Joining TTN");
        ttn_join();
        ttn_adr(LORAWAN_ADR);
        DEBUG_PORT.println("Done!");
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
        DEBUG_PORT.println("sleep");
    }

    // if user presses button for more than 3 secs, discard our network prefs and reboot (FIXME, use a debounce lib instead of this boilerplate)
 /*   static bool wasPressed = false;
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
    }*/

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