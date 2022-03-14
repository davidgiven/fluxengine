#include "globals.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "arch/amiga/amiga.h"
#include "arch/apple2/apple2.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/micropolis/micropolis.h"
#include "arch/northstar/northstar.h"
#include "arch/tids990/tids990.h"
#include "arch/victor9k/victor9k.h"
#include "lib/encoders/encoders.pb.h"
#include "protocol.h"

std::unique_ptr<AbstractEncoder> AbstractEncoder::create(const EncoderProto& config)
{
	static const std::map<int,
		std::function<std::unique_ptr<AbstractEncoder>(const EncoderProto&)>> encoders =
	{
		{ EncoderProto::kAmiga,     createAmigaEncoder },
		{ EncoderProto::kApple2,    createApple2Encoder },
		{ EncoderProto::kBrother,   createBrotherEncoder },
		{ EncoderProto::kC64,       createCommodore64Encoder },
		{ EncoderProto::kIbm,       createIbmEncoder },
		{ EncoderProto::kMacintosh, createMacintoshEncoder },
		{ EncoderProto::kMicropolis,createMicropolisEncoder },
		{ EncoderProto::kNorthstar, createNorthstarEncoder },
		{ EncoderProto::kTids990,   createTids990Encoder },
		{ EncoderProto::kVictor9K,  createVictor9kEncoder },
	};

	auto encoder = encoders.find(config.format_case());
	if (encoder == encoders.end())
		Error() << "no encoder specified";

	return (encoder->second)(config);
}

Fluxmap& Fluxmap::appendBits(const std::vector<bool>& bits, nanoseconds_t clock)
{
	nanoseconds_t now = duration();
	for (unsigned i=0; i<bits.size(); i++)
	{
		now += clock;
		if (bits[i])
		{
			unsigned delta = (now - duration()) / NS_PER_TICK;
            appendInterval(delta);
			appendPulse();
		}
	}

	return *this;
}

