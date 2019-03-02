#include "globals.h"
#include "crc.h"

uint16_t sumBytes(const uint8_t* start, const uint8_t* end)
{
	uint16_t i = 0;
	while (start != end)
		i += *start++;
	return i;
}

uint8_t xorBytes(const uint8_t* start, const uint8_t* end)
{
	uint8_t i = 0;
	while (start != end)
		i ^= *start++;
	return i;
}

uint16_t crc16(uint16_t poly, const uint8_t* start, const uint8_t* end)
{
	uint16_t crc = 0xffff;

	while (start != end)
	{
		crc ^= *start++ << 8;
		for (int i=0; i<8; i++)
			crc = (crc & 0x8000) ? ((crc<<1)^poly) : (crc<<1);
	}

	return crc;
}

/* Thanks to user202729 on StackOverflow for miraculously reverse engineering
 * this. */
uint32_t crcbrother(const uint8_t* start, const uint8_t* end)
{
	uint32_t crc = *start++;

	while (start != end)
	{
		for (int i=0; i<8; i++)
			crc = (crc & 0x800000) ? ((crc<<1)^BROTHER_POLY) : (crc<<1);
		crc ^= *start++;
	}

	return crc & 0xFFFFFF;
}

