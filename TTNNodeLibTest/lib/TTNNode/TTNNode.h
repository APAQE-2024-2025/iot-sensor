#ifndef TTNNode_h
#define TTNNode_h

#include <Arduino.h>
#include "Config.h"
#include <vector>
#include <hal/hal.h>

enum SendState
{
    Success = 0,
    QueueFull = 1,
    Error = 2,
    OperationPending = 3
};

class TTNNode 
{

#if SHOW_DEBUG 
#define DBG_MSG(...) Serial.println(__VA_ARGS__);
#else
#define DBG_MSG(...)
#endif

private:
    //no constructors for you hahahaha
    TTNNode();

    static inline const lmic_pinmap lmic_pins = {
        .nss = NSS_GPIO,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = RESET_GPIO,
        .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
    };

    static inline bool packetSent = false;
    static inline bool packetQueued = false;
    static inline RTC_DATA_ATTR uint32_t fCount = 0;

    static void callback(uint8_t message);
    static bool initInternal();
    static void joinTTN();

public:
    static inline std::vector<void (*)(uint8_t message)> lmic_callbacks;

    static bool begin();

    static void update();

    static void invokeCallback(uint8_t message);

    // static SendState send(String string, uint8_t port);
    // static SendState send(char* string, uint8_t port);
    // static SendState send(const char* string, uint8_t port);

    // static SendState send(double data, uint8_t port);
    // static SendState send(float data, uint8_t port);
    // static SendState send(uint16_t data, uint8_t port);
    // static SendState send(uint32_t data, uint8_t port);
    // static SendState send(uint64_t data, uint8_t port);
    // static SendState send(int16_t data, uint8_t port);
    // static SendState send(int32_t data, uint8_t port);
    // static SendState send(int64_t data, uint8_t port);

    template<typename T>
    static SendState send(const T& payload, uint8_t port)
    {
        if (packetQueued)
        {
            DBG_MSG("Failed to add packet to queue as it is full");
            return SendState::QueueFull;
        }

        const int dataLen = sizeof(T);
        uint8_t txBuffer[dataLen];
        memcpy(txBuffer, &payload, dataLen);

        return TTNNode::send(txBuffer, dataLen, port);
    }


    static SendState send(uint8_t* data, size_t dataLen, uint8_t port);
};
#endif