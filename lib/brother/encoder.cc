#include "globals.h"
#include "record.h"
#include "brother.h"

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
	write_bits(bits, cursor, BROTHER_SECTOR_RECORD, 32);
	write_bits(bits, cursor, encode_header_gcr(track), 16);
	write_bits(bits, cursor, encode_header_gcr(sector), 16);
}

