#include "globals.h"
#include "crc.h"

uint16_t crc16(uint16_t poly, const uint8_t* start, const uint8_t* end)
{
	uint16_t crc = 0xffff;

	while (start != end)
	{
		crc ^= *start++ << 8;
		for (int i=0; i < 8; i++)
			crc = (crc & 0x8000) ? ((crc<<1)^poly) : (crc<<1);
	}

	return crc;
}

