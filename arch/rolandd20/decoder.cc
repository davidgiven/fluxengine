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
 *       f       f       f       f       f       e       a       b
 * 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1 0 1 0 1 1
 * 1111111111111111111111111111111111111111111111011101110111011111
 *    f   f   f   f   f   f   f   f   f   f   f   d   d   d   d   f
 */

static const FluxPattern SECTOR_PATTERN(64, 0xfffffffffffddddfLL);

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
		auto rawbits = readRawBits(128);
		const auto& bytes = decodeFmMfm(rawbits);
		hexdump(std::cout, bytes);
	}
};

std::unique_ptr<Decoder> createRolandD20Decoder(const DecoderProto& config)
{
	return std::unique_ptr<Decoder>(new RolandD20Decoder(config));
}



