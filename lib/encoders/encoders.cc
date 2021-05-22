#include "globals.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "arch/amiga/amiga.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/tids990/tids990.h"
#include "lib/encoders/encoders.pb.h"
#include "protocol.h"

std::unique_ptr<AbstractEncoder> AbstractEncoder::create(const EncoderProto& config)
{
	switch (config.format_case())
	{
		case EncoderProto::kAmiga:
			return std::unique_ptr<AbstractEncoder>(new AmigaEncoder(config.amiga()));

		case EncoderProto::kIbm:
			return std::unique_ptr<AbstractEncoder>(new IbmEncoder(config.ibm()));

		case EncoderProto::kBrother:
			return std::unique_ptr<AbstractEncoder>(new BrotherEncoder(config.brother()));

		case EncoderProto::kMacintosh:
			return std::unique_ptr<AbstractEncoder>(new MacintoshEncoder(config.macintosh()));

		case EncoderProto::kC64:
			return std::unique_ptr<AbstractEncoder>(new Commodore64Encoder(config.c64()));

		default:
			Error() << "no input disk format specified";
	}
	return std::unique_ptr<AbstractEncoder>();
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


