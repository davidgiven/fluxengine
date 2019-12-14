#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "amiga.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"

FlagGroup amigaEncoderFlags;

static DoubleFlag clockRateUs(
	{ "--clock-rate" },
	"Encoded data clock rate (microseconds).",
	2.00);

static DoubleFlag postIndexGapMs(
	{ "--post-index-gap" },
	"Post-index gap before first sector header (milliseconds).",
	0.5);

static int charToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, const std::vector<bool>& src)
{
	for (bool bit : src)
	{
		if (cursor < bits.size())
			bits[cursor++] = bit;
	}
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
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

static void write_interleaved_bytes(std::vector<bool>& bits, unsigned& cursor, const Bytes& bytes)
{
	assert(!(bytes.size() & 3));
	Bytes interleaved = amigaInterleave(bytes);
	encodeMfm(bits, cursor, interleaved);
}

static void write_interleaved_bytes(std::vector<bool>& bits, unsigned& cursor, uint32_t data)
{
	Bytes b(4);
	ByteWriter bw(b);
	bw.write_be32(data);
	write_interleaved_bytes(bits, cursor, b);
}

static void write_sector(std::vector<bool>& bits, unsigned& cursor, const Sector* sector)
{
	if ((sector->data.size() != 512) && (sector->data.size() != 528))
		Error() << "unsupported sector size --- you must pick 512 or 528";

    write_bits(bits, cursor, AMIGA_SECTOR_RECORD, 6*8);

	std::vector<bool> headerBits(20*16);
	unsigned headerCursor = 0;

	Bytes header = 
		{
			0xff, /* Amiga 1.0 format byte */
			(uint8_t) ((sector->logicalTrack<<1) | sector->logicalSide),
			(uint8_t) sector->logicalSector,
			(uint8_t) (AMIGA_SECTORS_PER_TRACK - sector->logicalSector)
		};
	write_interleaved_bytes(headerBits, headerCursor, header);
	Bytes recoveryInfo(16);
	if (sector->data.size() == 528)
		recoveryInfo = sector->data.slice(512, 16);
	write_interleaved_bytes(headerBits, headerCursor, recoveryInfo);

	std::vector<bool> dataBits(512*16);
	unsigned dataCursor = 0;
	write_interleaved_bytes(dataBits, dataCursor, sector->data);

	write_bits(bits, cursor, headerBits);
	uint32_t headerChecksum = amigaChecksum(toBytes(headerBits));
	write_interleaved_bytes(bits, cursor, headerChecksum);
	uint32_t dataChecksum = amigaChecksum(toBytes(dataBits));
	write_interleaved_bytes(bits, cursor, dataChecksum);
	write_bits(bits, cursor, dataBits);
}

std::unique_ptr<Fluxmap> AmigaEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	if ((physicalTrack < 0) || (physicalTrack >= AMIGA_TRACKS_PER_DISK))
		return std::unique_ptr<Fluxmap>();

	int bitsPerRevolution = 200000.0 / clockRateUs;
	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

    fillBitmapTo(bits, cursor, postIndexGapMs * 1000 / clockRateUs, { true, false });

	for (int sectorId=0; sectorId<AMIGA_SECTORS_PER_TRACK; sectorId++)
	{
		const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
		write_sector(bits, cursor, sectorData);
    }

	if (cursor >= bits.size())
		Error() << "track data overrun";
	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;
}

