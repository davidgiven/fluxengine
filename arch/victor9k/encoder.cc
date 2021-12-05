#include "globals.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "victor9k.h"
#include "crc.h"
#include "sector.h"
#include "writer.h"
#include "image.h"
#include "fmt/format.h"
#include "arch/victor9k/victor9k.pb.h"
#include "lib/encoders/encoders.pb.h"
#include <ctype.h>
#include "bytes.h"

static bool lastBit;

static void write_zero_bits(std::vector<bool>& bits, unsigned& cursor, unsigned count)
{
    while (count--)
	{
		if (cursor < bits.size())
			lastBit = bits[cursor++] = 0;
	}
}

static void write_one_bits(std::vector<bool>& bits, unsigned& cursor, unsigned count)
{
    while (count--)
	{
		if (cursor < bits.size())
			lastBit = bits[cursor++] = 1;
	}
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

static int encode_data_gcr(uint8_t data)
{
    switch (data & 0x0f)
    {
        #define GCR_ENTRY(gcr, data) \
            case data: return gcr;
        #include "data_gcr.h"
        #undef GCR_ENTRY
    }
    return -1;
}

static void write_bytes(std::vector<bool>& bits, unsigned& cursor, const Bytes& bytes)
{
    for (uint8_t b : bytes)
    {
        write_bits(bits, cursor, encode_data_gcr(b>>4), 5);
        write_bits(bits, cursor, encode_data_gcr(b),    5);
    }
}

static void write_sector(std::vector<bool>& bits, unsigned& cursor,
		const Victor9kEncoderProto::TrackdataProto& trackdata,
        const Sector& sector)
{
    write_one_bits(bits, cursor, trackdata.pre_header_sync_bits());
    write_bits(bits, cursor, VICTOR9K_SECTOR_RECORD, 10);

    uint8_t encodedTrack = sector.logicalTrack | (sector.logicalSide<<7);
    uint8_t encodedSector = sector.logicalSector;
    write_bytes(bits, cursor, Bytes {
        encodedTrack,
        encodedSector,
        (uint8_t)(encodedTrack + encodedSector),
    });


    write_zero_bits(bits, cursor, trackdata.post_header_gap_bits());
    write_one_bits(bits, cursor, trackdata.pre_data_sync_bits());
    write_bits(bits, cursor, VICTOR9K_DATA_RECORD, 10);

    write_bytes(bits, cursor, sector.data);

    Bytes checksum(2);
    checksum.writer().write_le16(sumBytes(sector.data));
    write_bytes(bits, cursor, checksum);

    write_zero_bits(bits, cursor, trackdata.post_data_gap_bits());
}

class Victor9kEncoder : public AbstractEncoder
{
public:
	Victor9kEncoder(const EncoderProto& config):
        AbstractEncoder(config),
		_config(config.victor9k())
	{}

private:

	void getTrackFormat(Victor9kEncoderProto::TrackdataProto& trackdata, unsigned cylinder, unsigned head)
	{
		trackdata.Clear();
		for (const auto& f : _config.trackdata())
		{
			if (f.has_min_cylinder() && (cylinder < f.min_cylinder()))
				continue;
			if (f.has_max_cylinder() && (cylinder > f.max_cylinder()))
				continue;
			if (f.has_head() && (head != f.head()))
				continue;

			trackdata.MergeFrom(f);
		}
	}

public:
	std::vector<std::shared_ptr<Sector>> collectSectors(int physicalTrack, int physicalSide, const Image& image) override
	{
		std::vector<std::shared_ptr<Sector>> sectors;

		Victor9kEncoderProto::TrackdataProto trackdata;
		getTrackFormat(trackdata, physicalTrack, physicalSide);

        for (int i = 0; i < trackdata.sector_range().sector_count(); i++)
        {
            int sectorId = trackdata.sector_range().start_sector() + i;
			const auto& sector = image.get(physicalTrack, physicalSide, sectorId);
			if (sector)
				sectors.push_back(sector);
        }

		return sectors;
	}

    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide,
            const std::vector<std::shared_ptr<Sector>>& sectors, const Image& image) override
    {
		Victor9kEncoderProto::TrackdataProto trackdata;
		getTrackFormat(trackdata, physicalTrack, physicalSide);

        unsigned bitsPerRevolution = trackdata.original_data_rate_khz() * trackdata.original_period_ms();
        std::vector<bool> bits(bitsPerRevolution);
        double clockRateUs = 166666.0 / bitsPerRevolution;
        unsigned cursor = 0;

        fillBitmapTo(bits, cursor, trackdata.post_index_gap_us() / clockRateUs, { true, false });
        lastBit = false;

        for (const auto& sector : sectors)
            write_sector(bits, cursor, trackdata, *sector);

        if (cursor >= bits.size())
            Error() << fmt::format("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), { true, false });

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits, clockRateUs*1e3);
        return fluxmap;
    }

private:
	const Victor9kEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createVictor9kEncoder(const EncoderProto& config)
{
	return std::unique_ptr<AbstractEncoder>(new Victor9kEncoder(config));
}

// vim: sw=4 ts=4 et

