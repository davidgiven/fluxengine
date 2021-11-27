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

		for (int sectorId : trackdata.sectors().sector())
        {
			const auto& sector = image.get(physicalTrack, physicalSide, sectorId);
			if (sector)
				sectors.push_back(sector);
        }

		return sectors;
	}

    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide,
            const std::vector<std::shared_ptr<Sector>>& sectors, const Image& image)
    {
		Victor9kEncoderProto::TrackdataProto trackdata;
		getTrackFormat(trackdata, physicalTrack, physicalSide);

        std::vector<bool> bits(trackdata.bits_per_revolution());
        unsigned clockRateUs = 166666.0 / trackdata.bits_per_revolution();
        unsigned cursor = 0;

        fillBitmapTo(bits, cursor, trackdata.post_index_gap_us() / clockRateUs, { true, false });
        lastBit = false;

        for (const auto& sector : sectors)
            writeSector(bits, cursor, trackdata, *sector);

        if (cursor >= bits.size())
            Error() << fmt::format("track data overrun by {} bits", cursor - bits.size());
        fillBitmapTo(bits, cursor, bits.size(), { true, false });

        std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
        fluxmap->appendBits(bits, clockRateUs*1e3);
        return fluxmap;
    }

private:
    void writeSector(std::vector<bool>& bits, unsigned& cursor,
        const Victor9kEncoderProto::TrackdataProto& trackdata, const Sector& sector)
    {
    }

private:
	const Victor9kEncoderProto& _config;
};

std::unique_ptr<AbstractEncoder> createVictor9kEncoder(const EncoderProto& config)
{
	return std::unique_ptr<AbstractEncoder>(new Victor9kEncoder(config));
}

// vim: sw=4 ts=4 et

