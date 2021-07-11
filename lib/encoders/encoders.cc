#include "globals.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "arch/amiga/amiga.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/northstar/northstar.h"
#include "arch/tids990/tids990.h"
#include "lib/encoders/encoders.pb.h"
#include "protocol.h"

std::unique_ptr<AbstractEncoder> AbstractEncoder::create(const EncoderProto& config)
{
	static const std::map<int,
		std::function<std::unique_ptr<AbstractEncoder>(const EncoderProto&)>> encoders =
	{
		#if 0
		{ EncoderProto::kAmiga,     createAmigaEncoder },
		{ EncoderProto::kBrother,   createBrotherEncoder },
		{ EncoderProto::kC64,       createCommodore64Encoder },
		{ EncoderProto::kIbm,       createIbmEncoder },
		{ EncoderProto::kMacintosh, createMacintoshEncoder },
		{ EncoderProto::kNorthstar, createNorthstarEncoder },
		#endif
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


