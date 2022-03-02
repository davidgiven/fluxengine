#include "globals.h"
#include "arch/apple2/apple2.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "sector.h"
#include "writer.h"
#include "image.h"
#include "fmt/format.h"
#include "lib/encoders/encoders.pb.h"
#include <ctype.h>
#include "bytes.h"

static int encode_data_gcr(uint8_t data) {
    switch(data)
    {
	#define GCR_ENTRY(gcr, data) \
	case data: return gcr;
	#include "data_gcr.h"
	#undef GCR_ENTRY
    }
    return -1;
}

class Apple2Encoder : public AbstractEncoder
{
public:
    Apple2Encoder(const EncoderProto& config):
	AbstractEncoder(config)
    {}

public:
    std::vector<std::shared_ptr<const Sector>> collectSectors(int physicalTrack, int physicalSide, const Image& image) override
    {
	std::vector<std::shared_ptr<const Sector>> sectors;
	constexpr auto numSectors = 16;
	if (physicalSide == 0)
	{
	    int logicalTrack = physicalTrack / 2;
	    unsigned numSectors = 16;
	    for (int sectorId=0; sectorId<numSectors; sectorId++)
	    {
		const auto& sector = image.get(logicalTrack, 0, sectorId);
		if (sector)
		    sectors.push_back(sector);
	    }
	}

	return sectors;
    }

    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide,
	    const std::vector<std::shared_ptr<const Sector>>& sectors, const Image& image) override
    {
	if (physicalSide != 0)
	    return std::unique_ptr<Fluxmap>();

	int logicalTrack = physicalTrack / 2;
	double clockRateUs = 4.;

	int bitsPerRevolution = 200000.0 / clockRateUs;

	std::vector<bool> bits(bitsPerRevolution);
	unsigned cursor = 0;

	for (const auto& sector : sectors) {
	    if(sector) {
		writeSector(bits, cursor, *sector);
	    }
	}

	if (cursor >= bits.size())
	    Error() << fmt::format("track data overrun by {} bits", cursor - bits.size());
	fillBitmapTo(bits, cursor, bits.size(), { true, false });

	std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
	fluxmap->appendBits(bits, clockRateUs*1e3);
	return fluxmap;

    }

private:
    uint8_t volume_id = 254;

    /* This is extremely inspired by the MESS implementation, written by Nathan Woods
     * and R. Belmont: https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib/formats/ap2_dsk.cpp
     * as well as Understanding the Apple II (1983) Chapter 9
     * https://archive.org/details/Understanding_the_Apple_II_1983_Quality_Software/page/n230/mode/1up?view=theater
     */

    void writeSector(std::vector<bool>& bits, unsigned& cursor, const Sector& sector) const
    {
	if ((sector.status == Sector::OK) or (sector.status == Sector::BAD_CHECKSUM))
	{
	    auto write_bit = [&](bool val) {
		if(cursor <= bits.size()) { bits[cursor] = val; }
		cursor++;
	    };

	    auto write_bits = [&](uint32_t bits, int width) {
		for(int i=width; i--;) {
		    write_bit(bits & (1u << i));
		}
	    };

	    auto write_gcr44 = [&](uint8_t value) {
		write_bits((value << 7) | value | 0xaaaa, 16);
	    };

	    auto write_gcr6 = [&](uint8_t value) {
		write_bits(encode_data_gcr(value), 8);
	    };

	    auto write_ff40 = [&]() {
		write_bits(0xff0, 12);
	    };

	    auto write_ff36 = [&]() {
		write_bits(0xff << 2, 10);
	    };

	    auto write_ff32 = [&]() {
		write_bits(0xff, 8);
	    };

	    // There is data to encode to disk.
	    if ((sector.data.size() != APPLE2_SECTOR_LENGTH))
		Error() << fmt::format("unsupported sector size {} --- you must pick 256", sector.data.size());    

	    // Write address syncing leader : A sequence of "FF40"s followed by an "FF32", 5 to 40 of them
	    // "FF40" seems to indicate that the actual data written is "1111 1111 0000" i.e., 8 1s and a total of 40 microseconds
	    for(int i=0; i<4; i++) { write_ff40(); }
	    write_ff32();

	    // Write address field: APPLE2_SECTOR_RECORD + sector identifier + DE AA EB
	    write_bits(APPLE2_SECTOR_RECORD, 24);
	    write_gcr44(volume_id);
	    write_gcr44(sector.logicalTrack);
	    write_gcr44(sector.logicalSector);
	    write_gcr44(volume_id ^ sector.logicalTrack ^ sector.logicalSector);
	    write_bits(0xDEAAEB, 24);

	    // Write the "zip": a gap of 50 (we actually do 52, hopefully it's OK).
	    // In real HW this is actually turning OFF the write head for 50 cycles
	    write_bits(0, 13);

	    // Write data syncing leader: FF40 x4 + FF36 + APPLE2_DATA_RECORD + sector data + sum + DE AA EB (+ mystery bits cut off of the scan?)
	    for(int i=0; i<4; i++) write_ff40();
	    write_ff36();
	    write_bits(APPLE2_DATA_RECORD, 24);

	    // Convert the sector data to GCR, append the checksum, and write it out
	    constexpr auto TWOBIT_COUNT = 0x56; // Size of the 'twobit' area at the start of the GCR data
	    uint8_t checksum = 0;
	    for(int i=0; i<APPLE2_ENCODED_SECTOR_LENGTH; i++) {
		int value;
		if(i >= TWOBIT_COUNT) {
		    value = sector.data[i - TWOBIT_COUNT] >> 2;
		} else {
		    uint8_t tmp = sector.data[i];
		    value = ((tmp & 1) << 1) | ((tmp & 2) >> 1);

		    tmp = sector.data[i + TWOBIT_COUNT];
		    value |= ((tmp & 1) << 3) | ((tmp & 2) << 1);

		    if(i + 2*TWOBIT_COUNT < APPLE2_SECTOR_LENGTH) {
			tmp = sector.data[i + 2*TWOBIT_COUNT];
			value |= ((tmp & 1) << 5) | ((tmp & 2) << 3);
		    }
		}
		checksum ^= value;
		// assert(checksum & ~0x3f == 0);
		write_gcr6(checksum);
		checksum = value;
	    }
	    if(sector.status == Sector::BAD_CHECKSUM) checksum ^= 0x3f;
	    write_gcr6(checksum);
	    write_bits(0xDEAAEB, 24);
	}
    }
};

std::unique_ptr<AbstractEncoder> createApple2Encoder(const EncoderProto& config)
{
	return std::unique_ptr<AbstractEncoder>(new Apple2Encoder(config));
}


