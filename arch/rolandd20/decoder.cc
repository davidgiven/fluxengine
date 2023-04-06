#include "lib/globals.h"
#include "lib/decoders/decoders.h"
#include "lib/crc.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "lib/sector.h"
#include "lib/bytes.h"
#include "rolandd20.h"
#include <string.h>

/* Sector header record:
 *
 * BF FF FF FF FF FF FE AB
 *
 * This encodes to:
 *
 *    e    d    5    5    5    5    5    5
 * 1110 1101 0101 0101 0101 0101 0101 0101
 *    5    5    5    5    5    5    5    5
 * 0101 0101 0101 0101 0101 0101 0101 0101
 *    5    5    5    5    5    5    5    5
 * 0101 0101 0101 0101 0101 0101 0101 0101
 *    5    5    5    4    4    4    4    5
 * 0101 0101 0101 0100 0100 0100 0100 0101
 */

static const FluxPattern SECTOR_PATTERN(64, 0xed55555555555555LL);

class RolandD20Decoder : public Decoder
{
public:
	RolandD20Decoder(const DecoderProto& config):
		Decoder(config)
	{}

    nanoseconds_t advanceToNextRecord() override
	{
		return seekToPattern(SECTOR_PATTERN);
	}

    void decodeSectorRecord() override
	{
		auto rawbits = readRawBits(256);
		const auto& bytes = decodeFmMfm(rawbits);
		fmt::print("{} ", _sector->clock);
		hexdump(std::cout, bytes);
	}
};

std::unique_ptr<Decoder> createRolandD20Decoder(const DecoderProto& config)
{
	return std::unique_ptr<Decoder>(new RolandD20Decoder(config));
}



