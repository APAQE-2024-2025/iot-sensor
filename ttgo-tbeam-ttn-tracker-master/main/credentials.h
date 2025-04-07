/*

Credentials file

*/

#pragma once

// Only one of these settings must be defined
#define USE_ABP
// #define USE_OTAA

#ifdef USE_ABP

// LoRaWAN NwkSKey, network session key
static const u1_t PROGMEM NWKSKEY[16] = {0x4B, 0xDC, 0x68, 0x91, 0xD0, 0x94, 0x10, 0xC7, 0x04, 0x69, 0x5C, 0x3C, 0x0B, 0x62, 0x45, 0x59};
// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = {0x25, 0x45, 0x88, 0xD5, 0xCA, 0xE8, 0xB6, 0xF1, 0x25, 0x9E, 0x03, 0x4F, 0xB4, 0x66, 0x83, 0x78};
// LoRaWAN end-device address (DevAddr)
// This has to be unique for every node
static const u4_t DEVADDR = 0x260B0C50;

#endif

#ifdef USE_OTAA

// This EUI must be in little-endian format, so least-significant-byte (lsb)
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0x00, 0x00,
// 0x00.
static const u1_t PROGMEM APPEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// This should also be in little endian format (lsb), see above.
// Note: You do not need to set this field, if unset it will be generated automatically based on the device macaddr
static u1_t DEVEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// This key should be in big endian format (msb) (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif
