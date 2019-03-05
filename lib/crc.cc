#include "globals.h"
#include "bytes.h"
#include "crc.h"

uint16_t sumBytes(const Bytes& bytes)
{
	ByteReader br(bytes);

	uint16_t i = 0;
	while (!br.eof())
		i += br.read_8();
	return i;
}

uint8_t xorBytes(const Bytes& bytes)
{
	ByteReader br(bytes);

	uint8_t i = 0;
	while (!br.eof())
		i ^= br.read_8();
	return i;
}

uint16_t crc16(uint16_t poly, const Bytes& bytes)
{
	ByteReader br(bytes);

	uint16_t crc = 0xffff;
	while (!br.eof())
	{
		crc ^= br.read_8() << 8;
		for (int i=0; i<8; i++)
			crc = (crc & 0x8000) ? ((crc<<1)^poly) : (crc<<1);
	}

	return crc;
}

uint16_t crc16ref(uint16_t poly, const Bytes& bytes)
{
	ByteReader br(bytes);

	uint16_t crc = 0xffff;
	while (!br.eof())
	{
		crc ^= br.read_8();
		for (int i=0; i<8; i++)
			crc = (crc & 0x0001) ? ((crc>>1)^poly) : (crc>>1);
	}

	return crc;
}

/* Thanks to user202729 on StackOverflow for miraculously reverse engineering
 * this. */
uint32_t crcbrother(const Bytes& bytes)
{
	ByteReader br(bytes);

	uint32_t crc = br.read_8();
	while (!br.eof())
	{
		for (int i=0; i<8; i++)
			crc = (crc & 0x800000) ? ((crc<<1)^BROTHER_POLY) : (crc<<1);
		crc ^= br.read_8();
	}

	return crc & 0xFFFFFF;
}

