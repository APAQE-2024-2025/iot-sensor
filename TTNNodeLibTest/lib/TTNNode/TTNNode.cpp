/*
*   By Mathias Vansteensel (04/2025)
*   using MCCI LMIC (v5.0.1)
*/

#include <Arduino.h>
#include "TTNNode.h"
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "Config.h"
#include <Preferences.h>

#if USE_OTAA
void os_getArtEui(u1_t *buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui(u1_t *buf) { memcpy(buf, DEVEUI, 8); }
void os_getDevKey(u1_t *buf) { memcpy_P(buf, APPKEY, 16); }
#else
//these are here for the linker :)
void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}
#endif

bool TTNNode::begin()
{
    return initInternal();
}

bool TTNNode::initInternal()
{   
    bool success = (1 == os_init_ex((const void*)&lmic_pins));
    DBG_MSG("Setting up LoRa hardware & HAL");
    DBG_MSG(USE_OTAA ? "Using OTAA" : "Using ABP");
    DBG_MSG(success ? "HAL hardware init success :)" : "ERROR: HAL hardware init failed, check pinmap and hardware connections (check board/logical pin numbering scheme)")

    if (!success)
    {
        DBG_MSG("Press the reset btn to try again or rewrite the code from scratch... again :)");
        while (true);
    }

    lmic_callbacks.push_back(callback);
    
    joinTTN();
    LMIC_setAdrMode(LORAWAN_ADR);
    LMIC_setLinkCheckMode(LORAWAN_ADR);
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

#if USE_OTAA //OTAA
    DBG_MSG("Attempting join with OTAA");
    LMIC_startJoining();
    Preferences p;
    p.begin("lora", true); // intentionally ignore failure here
    uint32_t netId = p.getUInt("netId", UINT32_MAX);
    uint32_t devAddr = p.getUInt("devAddr", UINT32_MAX);
    uint8_t nwkKey[16], artKey[16];
    bool keysgood = p.getBytes("nwkKey", nwkKey, sizeof(nwkKey)) == sizeof(nwkKey) &&
                    p.getBytes("artKey", artKey, sizeof(artKey)) == sizeof(artKey);
    p.end();
    if (!keysgood)
    {
        DBG_MSG("No session saved, joining from scratch");
        LMIC_startJoining();
    }
    else
    {
        DBG_MSG("Rejoining saved session");
        LMIC_setSession(netId, devAddr, nwkKey, artKey);
        invokeCallback(EV_JOINED);
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
    invokeCallback(EV_JOINED);
#endif
}

void TTNNode::invokeCallback(uint8_t message)
{
    for (uint8_t i = 0; i < lmic_callbacks.size(); i++)
        (lmic_callbacks[i])(message);
}

void onEvent(ev_t event)
{
    TTNNode::invokeCallback(event);
}

void TTNNode::callback(uint8_t message)
{
    bool ttn_joined = false;
    switch (message)
    {
        case EV_JOINED:
            ttn_joined = true; // fallthrough intended – don't fucking touch this or you'll break it
        case EV_JOINING:
            DBG_MSG(ttn_joined ? "Joined TTN!\n" : "TTN joining...\n");
            break;
        case EV_JOIN_FAILED:
            DBG_MSG("TTN join failed\n");
            break;
        case EV_REJOIN_FAILED:
            DBG_MSG("TTN rejoin failed\n");
            break;
        case EV_RESET:
            DBG_MSG("Reset TTN connection\n");
            break;
        case EV_LINK_DEAD:
            DBG_MSG("TTN link dead\n");
            break;
        case EV_ACK:
            DBG_MSG("ACK received\n");
            break;
        case EV_PENDING:
            DBG_MSG("Message discarded\n");
            break;
        case EV_QUEUED:
            DBG_MSG("Message queued\n");
            break;
        case EV_TXCOMPLETE:
            if (!packetQueued)
                break;
            DBG_MSG("Queued message transmitted successfully\n");
            packetQueued = false;
            packetSent = true;
            break;
        case EV_RESPONSE:
            DBG_MSG("[TTN] Response: ");

            char buffer[6];
            for (uint8_t i = 0; i < LMIC.dataLen; i++)
            {
                uint8_t val = LMIC.frame[LMIC.dataBeg + i];
                snprintf(buffer, sizeof(buffer), "%02X", val);
                DBG_MSG(buffer);
            }
            DBG_MSG("\n");
            break;
        default:
            break;
    }
}

/// @brief Call this function periodically (e.g. in loop())
void TTNNode::update()
{
    os_runloop_once();
}

// SendState TTNNode::send(String string, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(char* string, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(const char* string, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(double data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(float data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(uint16_t data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(uint32_t data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(uint64_t data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(int16_t data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(int32_t data, uint8_t port)
// {
//     return SendState::Error;
// }

// SendState TTNNode::send(int64_t data, uint8_t port)
// {
//     return SendState::Error;
// }


SendState TTNNode::send(uint8_t* data, size_t dataLen, uint8_t port)
{
    if (packetQueued)
    {
        DBG_MSG("Failed to add packet to queue as it is full");
        return SendState::QueueFull;
    }
    
    LMIC_setSeqnoUp(fCount);

    if (LMIC.opmode & OP_TXRXPEND)
    {
        invokeCallback(EV_PENDING);
        return SendState::OperationPending;
    }

    LMIC_setTxData2(port, data, dataLen, /*confirmed ? 1 :*/ 0);
    packetQueued = true;
    TTNNode::invokeCallback(EV_QUEUED);
    fCount++;
    return SendState::Success;
}


/*
    TODO LOG:
        - only allow send when no messages queued
        - ^^enforce queue system
        - check if "packetSent" is useful
        - internal txBuffer
        *- store frame counter as static RTC_DATA_ATTR uint32_t count = 0;
*/