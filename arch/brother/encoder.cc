#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "brother.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"

FlagGroup brotherEncoderFlags;

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	3.83);

static DoubleFlag postIndexGapMs(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (milliseconds).",
	1.0);

static DoubleFlag sectorSpacingMs(
	{ "--sector-spacing" },
	"Time between successive sector headers (milliseconds).",
	16.2);

static DoubleFlag postHeaderSpacingMs(
	{ "--post-header-spacing" },
	"Time between a sector's header and data records (milliseconds).",
	0.69);

static StringFlag sectorSkew(
	{ "--sector-skew" },
	"Order in which to write sectors.",
	"05a3816b4927");

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

static void write_sector_header(std::vector<bool>& bits, unsigned& cursor,
		int track, int sector)
{
	write_bits(bits, cursor, 0xffffffff, 31);
	write_bits(bits, cursor, BROTHER_SECTOR_RECORD, 32);
	write_bits(bits, cursor, encode_header_gcr(track), 16);
	write_bits(bits, cursor, encode_header_gcr(sector), 16);
	write_bits(bits, cursor, encode_header_gcr(0x2f), 16);
}

static void write_sector_data(std::vector<bool>& bits, unsigned& cursor, const Bytes& data)
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

	uint32_t realCrc = crcbrother(data);
	write_byte(realCrc>>16);
	write_byte(realCrc>>8);
	write_byte(realCrc);
	write_byte(0x58); /* magic */
    write_byte(0xd4);
    while (width != 0)
        write_byte(0);
}

static int charToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

std::unique_ptr<Fluxmap> BrotherEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= BROTHER_TRACKS_PER_DISK)
        || (physicalSide != 0))
		return std::unique_ptr<Fluxmap>();

	int bitsPerRevolution = 200000.0 / clockRateUs;
	const std::string& skew = sectorSkew.get();
	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

	for (int sectorCount=0; sectorCount<BROTHER_SECTORS_PER_TRACK; sectorCount++)
	{
		int sectorId = charToInt(skew.at(sectorCount));
		double headerMs = postIndexGapMs + sectorCount*sectorSpacingMs;
		unsigned headerCursor = headerMs*1e3 / clockRateUs;
		double dataMs = headerMs + postHeaderSpacingMs;
		unsigned dataCursor = dataMs*1e3 / clockRateUs;

		const auto& sectorData = allSectors.get(physicalTrack, 0, sectorId);

		fillBitmapTo(bits, cursor, headerCursor, { true, false });
		write_sector_header(bits, cursor, physicalTrack, sectorId);
		fillBitmapTo(bits, cursor, dataCursor, { true, false });
		write_sector_data(bits, cursor, sectorData->data);
	}

	if (cursor >= bits.size())
		Error() << "track data overrun";
	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	// The pre-index gap is not normally reported.
	// std::cerr << "pre-index gap " << 200.0 - (double)cursor*clockRateUs/1e3 << std::endl;
	
	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}
