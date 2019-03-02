#ifndef CRC_H
#define CRC_H

#define CCITT_POLY 0x1021
#define BROTHER_POLY 0x000201

extern uint16_t sumBytes(const uint8_t* start, const uint8_t* end);
extern uint8_t xorBytes(const uint8_t* start, const uint8_t* end);
extern uint16_t crc16(uint16_t poly, const uint8_t* start, const uint8_t* end);
extern uint32_t crcbrother(const uint8_t* start, const uint8_t* end);

#endif

