#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "tids990.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"
#include <fmt/format.h>

FlagGroup tids990EncoderFlags;

static IntFlag trackLengthMs(
	{ "--tids990-track-length-ms" },
	"Length of a track in milliseconds.",
	166);

static IntFlag sectorCount(
	{ "--tids990-sector-count" },
	"Number of sectors per track.",
	26);

static IntFlag clockRateKhz(
	{ "--tids990-clock-rate-khz" },
	"Clock rate of data to write.",
	500);

static HexIntFlag am1Byte(
	{ "--tids990-am1-byte" },
	"16-bit RAW bit pattern to use for the AM1 ID byte",
	0x2244);

static HexIntFlag am2Byte(
	{ "--tids990-am2-byte" },
	"16-bit RAW bit pattern to use for the AM2 ID byte",
	0x2245);

static IntFlag gap1(
	{ "--tids990-gap1-bytes" },
	"Size of gap 1 (the post-index gap).",
	80);

static IntFlag gap2(
	{ "--tids990-gap2-bytes" },
	"Size of gap 2 (the post-ID gap).",
	21);

static IntFlag gap3(
	{ "--tids990-gap3-bytes" },
	"Size of gap 3 (the post-data or format gap).",
	51);

static StringFlag sectorSkew(
	{ "--tids990-sector-skew" },
	"Order to emit sectors.",
	"1mhc72nid83oje94pkfa50lgb6");

static int charToInt(char c)
{
	if (isdigit(c))
		return c - '0';
	return 10 + tolower(c) - 'a';
}

void TiDs990Encoder::writeRawBits(uint32_t data, int width)
{
	_cursor += width;
	_lastBit = data & 1;
	for (int i=0; i<width; i++)
	{
		unsigned pos = _cursor - i - 1;
		if (pos < _bits.size())
			_bits[pos] = data & 1;
		data >>= 1;
	}
}

void TiDs990Encoder::writeBytes(const Bytes& bytes)
{
	encodeMfm(_bits, _cursor, bytes, _lastBit);
}

void TiDs990Encoder::writeBytes(int count, uint8_t byte)
{
	Bytes bytes = { byte };
	for (int i=0; i<count; i++)
		writeBytes(bytes);
}

static uint8_t decodeUint16(uint16_t raw)
{
	Bytes b;
	ByteWriter bw(b);
	bw.write_be16(raw);
	return decodeFmMfm(b.toBits())[0];
}

std::unique_ptr<Fluxmap> TiDs990Encoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	double clockRateUs = 1e3 / clockRateKhz / 2.0;
	int bitsPerRevolution = (trackLengthMs * 1000.0) / clockRateUs;
	_bits.resize(bitsPerRevolution);
	_cursor = 0;

	uint8_t am1Unencoded = decodeUint16(am1Byte);
	uint8_t am2Unencoded = decodeUint16(am2Byte);

	writeBytes(gap1, 0x55);

	bool first = true;
	for (char sectorChar : sectorSkew.get())
	{
		int sectorId = charToInt(sectorChar);
		if (!first)
			writeBytes(gap3, 0x55);
		first = false;

		const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
		if (!sectorData)
			Error() << fmt::format("format tried to find sector {} which wasn't in the input file", sectorId);

		/* Writing the sector and data records are fantastically annoying.
		 * The CRC is calculated from the *very start* of the record, and
		 * include the malformed marker bytes. Our encoder doesn't know
		 * about this, of course, with the result that we have to construct
		 * the unencoded header, calculate the checksum, and then use the
		 * same logic to emit the bytes which require special encoding
		 * before encoding the rest of the header normally. */

		{
			Bytes header;
			ByteWriter bw(header);

			writeBytes(12, 0x55);
			bw.write_8(am1Unencoded);
			bw.write_8(sectorData->logicalSide << 3);
			bw.write_8(sectorData->logicalTrack);
			bw.write_8(sectorCount);
			bw.write_8(sectorData->logicalSector);
			bw.write_be16(sectorData->data.size());
			uint16_t crc = crc16(CCITT_POLY, header);
			bw.write_be16(crc);

			writeRawBits(am1Byte, 16);
			writeBytes(header.slice(1));
		}

		writeBytes(gap2, 0x55);

		{
			Bytes data;
			ByteWriter bw(data);

			writeBytes(12, 0x55);
			bw.write_8(am2Unencoded);

			bw += sectorData->data;
			uint16_t crc = crc16(CCITT_POLY, data);
			bw.write_be16(crc);

			writeRawBits(am2Byte, 16);
			writeBytes(data.slice(1));
		}
    }

	if (_cursor >= _bits.size())
		Error() << "track data overrun";
	while (_cursor < _bits.size())
		writeBytes(1, 0x55);
	
	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(_bits, clockRateUs*1e3);
	return fluxmap;
}

