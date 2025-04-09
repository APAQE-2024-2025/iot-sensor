/*

Credentials file

*/

#pragma once

// Only one of these settings must be defined
#define USE_ABP
//#define USE_OTAA

#ifdef USE_ABP

//pain-and-suffering

static const u1_t PROGMEM NWKSKEY[16] = {0x4B, 0xDC, 0x68, 0x91, 0xD0, 0x94, 0x10, 0xC7, 0x04, 0x69, 0x5C, 0x3C, 0x0B, 0x62, 0x45, 0x59};
// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = {0x25, 0x45, 0x88, 0xD5, 0xCA, 0xE8, 0xB6, 0xF1, 0x25, 0x9E, 0x03, 0x4F, 0xB4, 0x66, 0x83, 0x78};
// LoRaWAN end-device address (DevAddr)
// This has to be unique for every node
static const u4_t DEVADDR = 0x260B0C50;

// //ik-wil-dood

// static const u1_t PROGMEM NWKSKEY[16] = {0xBC, 0xD7, 0xED, 0x5B, 0x2F, 0xDE, 0xDD, 0x47, 0x6E, 0x7F, 0xAB, 0x2F, 0x21, 0x5F, 0x92, 0x36};
// // LoRaWAN AppSKey, application session key
// static const u1_t PROGMEM APPSKEY[16] = {0x7C, 0xC0, 0x31, 0x64, 0x25, 0xF0, 0x8C, 0xE9, 0x89, 0x25, 0x0B, 0xC5, 0xFB, 0x2F, 0x81, 0x5F};
// // LoRaWAN end-device address (DevAddr)
// // This has to be unique for every node
// static const u4_t DEVADDR = 0x260BFEAB;

#endif

#ifdef USE_OTAA

// send-help-pls

// This EUI must be in little-endian format, so least-significant-byte (lsb)
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0x00, 0x00,
// 0x00.
static const u1_t PROGMEM APPEUI[8] = {0xBB, 0xAA, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};

// This should also be in little endian format (lsb), see above.
// Note: You do not need to set this field, if unset it will be generated automatically based on the device macaddr
static u1_t DEVEUI[8] = {0x28, 0xFC, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};

// This key should be in big endian format (msb) (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = {0x7D, 0xAF, 0x09, 0x06, 0x01, 0xD0, 0x4F, 0xA8, 0xC0, 0x24, 0x0A, 0xC3, 0xE0, 0x9E, 0x8E, 0xC2};

#endif
