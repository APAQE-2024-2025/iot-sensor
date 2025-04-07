// #include "Config.h"

// class TTNNode 
// {
//     private:
//         std::vector<void (*)(uint8_t message)> _lmic_callbacks;

//         void callback(uint8_t message);

//         bool initInternal(lmic_pinmap lmic_pins, void (*callback)(uint8_t message));
        
//         void joinTTN();

//     public:
//     #if defined(USE_OTAA)
//         void begin(uint8_t joinEUI[8], uint8_t devEUI[8], uint8_t nwkKey[16], uint8_t appKey[16], const lmic_pinmap lmic_pins, bool enableAddr); //OTAA
//     #else
//         void begin(uint8_t devAdr[4], uint8_t nwkSKey[16], uint8_t appSKey[16], const lmic_pinmap lmic_pins); //ABP
//     #endif
//         void send(String string);

//         void send(char* string);

//         void send(nint data);