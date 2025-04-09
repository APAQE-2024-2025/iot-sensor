/*
*   By Mathias Vansteensel (7/2025)
*   using MCCI LMIC (v5.0.1)
*/

#include <Adruino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "Config.h"

#if SHOW_DEBUG 
#define DBG_MSG(...) Serial.println(__VA_ARGS__);
#else
#define DBG_MSG
#endif


std::vector<void (*)(uint8_t message)> TTNNode::_lmic_callbacks;



#if defined(USE_OTAA)
void TTNNode::begin(uint8_t joinEUI[8], uint8_t devEUI[8], uint8_t nwkKey[16], uint8_t appKey[16], const lmic_pinmap lmic_pins)
{
    //OTAA
    initInternal(lmic_pins);
}
#else
void TTNNode::begin(uint8_t devAdr[4], uint8_t nwkSKey[16], uint8_t appSKey[16], const lmic_pinmap lmic_pins, bool enableAddr)
{
    //ABP
    initInternal(lmic_pins);
}
#endif

bool TTNNode::initInternal(lmic_pinmap lmic_pins, void (*callback)(uint8_t message))
{   
    bool success = (1 == os_init_ex((const void *)&lmic_pins))
#ifdef SHOW_DEBUG
    DBG_MSG("Setting up LoRa hardware & HAL");
    DBG_MSG(_useABP ? "Using ABP" : "Using OTAA");
    DBG_MSG(sucess ? "HAL hardware init success :)" : "ERROR: HAL hardware init failed, check pinmap and hardware connections (check board/logical pin numbering scheme)")
#endif

    if (!success)
    {
        DBG_MSG("Press the reset btn to try again or rewrite the code from scratch... again :)");
        while (true);
    }

    _lmic_callbacks.push_back(callback)
    
    joinTTN();
    LMIC_setAdrMode(LORAWAN_ADR);
    LMIC_setLinkCheckMode(LORAWAN_ADR);
    DBG_MSG();
    return success;
}

void TTNNode::joinTTN()
{
    LMIC_reset();

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI); // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);  // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);   // g2-band

    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(LORAWAN_SF, 14);

#if defined(USE_OTAA) //OTAA
    DBG_MSG("Attempting join with OTAA");
    LMIC_startJoining();
    Preferences p;
    p.begin("lora", true); // we intentionally ignore failure here
    uint32_t netId = p.getUInt("netId", UINT32_MAX);
    uint32_t devAddr = p.getUInt("devAddr", UINT32_MAX);
    uint8_t nwkKey[16], artKey[16];
    bool keysgood = p.getBytes("nwkKey", nwkKey, sizeof(nwkKey)) == sizeof(nwkKey) &&
                    p.getBytes("artKey", artKey, sizeof(artKey)) == sizeof(artKey);
    p.end();
    if (!keysgood)
    {
        DEBUG_PORT.println("No session saved, joining from scratch");
        LMIC_startJoining();
    }
    else
    {
        DEBUG_PORT.println("Rejoining saved session");
        LMIC_setSession(netId, devAddr, nwkKey, artKey);
        _ttn_callback(EV_JOINED);
    }
#else //ABP
    DBG_MSG("Attempting join with ABP");
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    DBG_MSG("");
    LMIC_setSession(0x1, DEVADDR, nwkskey, appskey);

    LMIC.dn2Dr = DR_SF9;
    _ttn_callback(EV_JOINED);
#endif
}

//TODO: implement propper
void TTNNode::callback(uint8_t message)
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

void TTNNode::send(String string)
{
    
}

void TTNNode::send(char* string)
{

}

void TTNNode::send(uint8_t* data)
{
    
}