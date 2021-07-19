#include "globals.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "amiga.h"
#include "crc.h"
#include "writer.h"
#include "image.h"
#include "arch/amiga/amiga.pb.h"
#include "lib/encoders/encoders.pb.h"

static bool lastBit;

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
			lastBit = bits[cursor++] = bit;
	}
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
	cursor += width;
	lastBit = data & 1;
	for (int i=0; i<width; i++)
	{
		unsigned pos = cursor - i - 1;
		if (pos < bits.size())
			bits[pos] = data & 1;
		data >>= 1;
	}
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, const Bytes& bytes)
{
	ByteReader br(bytes);
	BitReader bitr(br);

	while (!bitr.eof())
	{
		if (cursor < bits.size())
			bits[cursor++] = bitr.get();
	}
}

static void write_sector(std::vector<bool>& bits, unsigned& cursor, const std::shared_ptr<Sector>& sector)
{
	if ((sector->data.size() != 512) && (sector->data.size() != 528))
		Error() << "unsupported sector size --- you must pick 512 or 528";

	uint32_t checksum = 0;

	auto write_interleaved_bytes = [&](const Bytes& bytes)
	{
		Bytes interleaved = amigaInterleave(bytes);
		Bytes mfm = encodeMfm(interleaved, lastBit);
		checksum ^= amigaChecksum(mfm);
		checksum &= 0x55555555;
		write_bits(bits, cursor, mfm);
	};

	auto write_interleaved_word = [&](uint32_t word)
	{
		Bytes b(4);
		b.writer().write_be32(word);
		write_interleaved_bytes(b);
	};

    write_bits(bits, cursor, AMIGA_SECTOR_RECORD, 6*8);

	checksum = 0;
	Bytes header = 
		{
			0xff, /* Amiga 1.0 format byte */
			(uint8_t) ((sector->logicalTrack<<1) | sector->logicalSide),
			(uint8_t) sector->logicalSector,
			(uint8_t) (AMIGA_SECTORS_PER_TRACK - sector->logicalSector)
		};
	write_interleaved_bytes(header);
	Bytes recoveryInfo(16);
	if (sector->data.size() == 528)
		recoveryInfo = sector->data.slice(512, 16);
	write_interleaved_bytes(recoveryInfo);
	write_interleaved_word(checksum);

	Bytes data = sector->data.slice(0, 512);
	write_interleaved_word(amigaChecksum(encodeMfm(amigaInterleave(data), lastBit)));
	write_interleaved_bytes(data);
}

class AmigaEncoder : public AbstractEncoder
{
public:
	AmigaEncoder(const EncoderProto& config):
		AbstractEncoder(config),
		_config(config.amiga()) {}

public:
	std::vector<std::shared_ptr<Sector>> collectSectors(int physicalTrack, int physicalSide, const Image& image) override
	{
		std::vector<std::shared_ptr<Sector>> sectors;

		if ((physicalTrack >= 0) && (physicalTrack < AMIGA_TRACKS_PER_DISK))
		{
			for (int sectorId=0; sectorId<AMIGA_SECTORS_PER_TRACK; sectorId++)
			{
				const auto& sector = image.get(physicalTrack, physicalSide, sectorId);
				if (sector)
					sectors.push_back(sector);
			}
		}

		return sectors;
	}

    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide,
			const std::vector<std::shared_ptr<Sector>>& sectors, const Image& image) override
	{
		if ((physicalTrack < 0) || (physicalTrack >= AMIGA_TRACKS_PER_DISK))
			return std::unique_ptr<Fluxmap>();

		int bitsPerRevolution = 200000.0 / _config.clock_rate_us();
		std::vector<bool> bits(bitsPerRevolution);
		unsigned cursor = 0;

		fillBitmapTo(bits, cursor, _config.post_index_gap_ms() * 1000 / _config.clock_rate_us(), { true, false });
		lastBit = false;

		for (int sectorId=0; sectorId<AMIGA_SECTORS_PER_TRACK; sectorId++)
		{
			const auto& sectorData = image.get(physicalTrack, physicalSide, sectorId);
			if (sectorData)
				write_sector(bits, cursor, sectorData);
		}

		if (cursor >= bits.size())
			Error() << "track data overrun";
		fillBitmapTo(bits, cursor, bits.size(), { true, false });

		std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
		fluxmap->appendBits(bits, _config.clock_rate_us()*1e3);
		return fluxmap;
	}

private:
	const AmigaEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createAmigaEncoder(const EncoderProto& config)
{
	return std::unique_ptr<AbstractEncoder>(new AmigaEncoder(config));
}


