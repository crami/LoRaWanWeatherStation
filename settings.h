/*******************************************************************************
 * Things To Change:
 * - set NWKSKEY (value from staging.thethingsnetwork.com)
 * - set APPKSKEY (value from staging.thethingsnetwork.com)
 * - set DEVADDR (value from staging.thethingsnetwork.com)
 * - optionally comment #define DEBUG
 * - optionally comment #define SLEEP
 * - set TX_INTERVAL in seconds
 *
 *******************************************************************************/

// show debug statements; comment next line to disable debug statements
#define DEBUG

// use low power sleep; comment next line to not use low power sleep
#define SLEEP

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 2*60;

// LoRaWAN NwkSKey, your network session key, 16 bytes (from staging.thethingsnetwork.org)
static const PROGMEM u1_t NWKSKEY[16] = { 0x27, 0xB1, 0xD2, 0x70, 0xCF, 0x82, 0xA4, 0x4B, 0x9D, 0x1E, 0x13, 0x88, 0xFA, 0x3D, 0x92, 0x1D };

// LoRaWAN AppSKey, application session key, 16 bytes  (from staging.thethingsnetwork.org)
static const u1_t PROGMEM APPSKEY[16] = { 0x68, 0x0F, 0xE8, 0x44, 0xBD, 0x3F, 0x43, 0xE9, 0xC0, 0x5F, 0x7B, 0x7E, 0xB8, 0x8A, 0x1B, 0x1B };

// LoRaWAN end-device address (DevAddr),  (from staging.thethingsnetwork.org)
static const u4_t DEVADDR = 0x2601109B; // <-- Change this address for every node!

