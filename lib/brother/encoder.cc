#include "globals.h"
#include "record.h"
#include "brother.h"
#include "crc.h"

static int encode_header_gcr(uint16_t word)
{
	switch (word)
	{
		#define GCR_ENTRY(gcr, data) \
			case data: return gcr;
		#include "header_gcr.h"
		#undef GCR_ENTRY
	}                       
	return -1;             
};

static int encode_data_gcr(uint8_t data)
{
	switch (data)
	{
		#define GCR_ENTRY(gcr, data) \
			case data: return gcr;
		#include "data_gcr.h"
		#undef GCR_ENTRY
	}                       
	return -1;             
};

static void write_bits(std::vector<bool>& bits, unsigned& cursor, uint32_t data, int width)
{
	cursor += width;
	for (int i=0; i<width; i++)
	{
		unsigned pos = cursor - i - 1;
		if (pos < bits.size())
			bits[pos] = data & 1;
		data >>= 1;
	}
}

void writeBrotherSectorHeader(std::vector<bool>& bits, unsigned& cursor,
		int track, int sector)
{
	write_bits(bits, cursor, 0xffffffff, 31);
	write_bits(bits, cursor, BROTHER_SECTOR_RECORD, 32);
	write_bits(bits, cursor, encode_header_gcr(track), 16);
	write_bits(bits, cursor, encode_header_gcr(sector), 16);
	write_bits(bits, cursor, encode_header_gcr(0x2f), 16);
}

void writeBrotherSectorData(std::vector<bool>& bits, unsigned& cursor,
		const std::vector<uint8_t>& data)
{
	write_bits(bits, cursor, 0xffffffff, 32);
	write_bits(bits, cursor, BROTHER_DATA_RECORD, 32);

	uint16_t fifo = 0;
	int width = 0;

	if (data.size() != BROTHER_DATA_RECORD_PAYLOAD)
		Error() << "unsupported sector size";

	auto write_byte = [&](uint8_t byte)
	{
		fifo |= (byte << (8 - width));
		width += 8;

		while (width >= 5)
		{
			uint8_t quintet = fifo >> 11;
			fifo <<= 5;
			width -= 5;

			write_bits(bits, cursor, encode_data_gcr(quintet), 8);
		}
	};

	for (uint8_t byte : data)
		write_byte(byte);

	uint32_t realCrc = crcbrother(&data[0], &data[256]);
	write_byte(realCrc>>16);
	write_byte(realCrc>>8);
	write_byte(realCrc);
	write_byte(0x58); /* magic */
    write_byte(0xd4);
    while (width != 0)
        write_byte(0);
}


