// #pragma once

// #include <lmic.h>

// #pragma warning "THIS WARNING IS HERE TO REMING YOU TO UPDATE/SET YOUR CREDENTIALS!!!"

// #if defined(USE_OTAA)


// // This key should be in little endian format (lsb)
// static const uint8_t PROGMEM APPEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// // This key should be in little endian format (lsb)
// static uint8_t DEVEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// // This key should be in big endian format (msb)
// static const uint8_t PROGMEM APPKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// #else


// static const uint8_t PROGMEM NWKSKEY[16] = {0x95, 0xB5, 0x39, 0x97, 0x8E, 0x72, 0x25, 0x98, 0x18, 0x3B, 0x62, 0x01, 0x1E, 0x45, 0xC8, 0x85};
// static const uint8_t PROGMEM APPSKEY[16] = {0x21, 0xBD, 0xBA, 0xF9, 0xAD, 0x87, 0xB2, 0x09, 0xBA, 0x43, 0x07, 0x6D, 0xDB, 0x95, 0xB6, 0x91};
// static const uint32_t DEVADDR = 0x260B3EDE; //just randomly generate one or copy the one from the TTN dashboard if theres already one there


// #endif



// #define LORAWAN_PORT 10              // Port the messages will be sent to
// #define LORAWAN_CONFIRMED_EVERY 0    // Send confirmed message every these many messages (0 means never)
// #define LORAWAN_SF DR_SF7            // Spreading factor (recommended DR_SF7 for ttn network map purposes, DR_SF10 works for slow moving trackers)
// #define LORAWAN_ADR 0                // Enable ADR
// #define SHOW_DEBUG false             // Show debug information (very verbose, but lots of serial prints slows down program)
// #define USE_OTAA false               // Uses OTAA if true, ABP if false. tbh if otaa doesnt work, try abp it just may


// // -----------------------------------------------------------------------------
// // Custom messages
// // -----------------------------------------------------------------------------

// #define EV_QUEUED 100
// #define EV_PENDING 101
// #define EV_ACK 102
// #define EV_RESPONSE 103

// // -----------------------------------------------------------------------------
// // LoRa SPI
// // -----------------------------------------------------------------------------

// // Lilygo T-BEAM
// // #define SCK_GPIO 5
// // #define MISO_GPIO 19
// // #define MOSI_GPIO 27
// // #define NSS_GPIO 18
// // #define RESET_GPIO 14
// // #define DIO0_GPIO 26
// // #define DIO1_GPIO 33 // Note: not really used on this board
// // #define DIO2_GPIO 32 // Note: not really used on this board

// // Firebeetle32
// #define SCK_GPIO 18
// #define MISO_GPIO 19
// #define MOSI_GPIO 23
// #define NSS_GPIO D5
// #define RESET_GPIO D4
// #define DIO0_GPIO D2
// #define DIO1_GPIO 16
// #define DIO2_GPIO LMIC_UNUSED_PIN