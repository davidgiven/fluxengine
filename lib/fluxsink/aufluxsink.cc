#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sql.h"
#include "bytes.h"
#include "protocol.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "decoders/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "proto.h"
#include "fmt/format.h"
#include <fstream>

class AuFluxSink : public FluxSink
{
public:
	AuFluxSink(const AuFluxSinkProto& config):
		_config(config)
	{
		if ((::config.cylinders().start() != ::config.cylinders().end())
			|| (::config.heads().start() != ::config.heads().end()))
			Error() << "exactly one track must be written (two sides of one cylinder count as two tracks)";
	}

public:
	void writeFlux(int cylinder, int head, Fluxmap& fluxmap)
	{
		unsigned totalTicks = fluxmap.ticks() + 2;
		unsigned channels = _config.index_markers() ? 2 : 1;

		std::cerr << "Writing output file...\n";
		std::ofstream of(_config.filename(), std::ios::out | std::ios::binary);
		if (!of.is_open())
			Error() << "cannot open output file";

		/* Write header */

		{
			Bytes header;
			header.resize(24);
			ByteWriter bw(header);

			bw.write_be32(0x2e736e64);
			bw.write_be32(24);
			bw.write_be32(totalTicks * channels);
			bw.write_be32(2); /* 8-bit PCM */
			bw.write_be32(TICK_FREQUENCY);
			bw.write_be32(channels); /* channels */

			of.write((const char*) header.cbegin(), header.size());
		}

		/* Write data */

		{
			Bytes data;
			data.resize(totalTicks * channels);
			memset(data.begin(), 0x80, data.size());

			FluxmapReader fmr(fluxmap);
			unsigned timestamp = 0;
			while (!fmr.eof())
			{
				unsigned ticks;
				uint8_t bits = fmr.getNextEvent(ticks);
				if (fmr.eof())
					break;
				timestamp += ticks;

				if (bits & F_BIT_PULSE)
					data[timestamp*channels + 0] = 0x7f;
				if (_config.index_markers() && (bits & F_BIT_INDEX))
					data[timestamp*channels + 1] = 0x7f;
			}

			of.write((const char*) data.cbegin(), data.size());
		}

		std::cerr << "Done. Warning: do not play this file, or you will break your speakers"
				 " and/or ears!\n";
	}

	operator std::string () const
	{
		return fmt::format("au({})", _config.filename());
	}

private:
	const AuFluxSinkProto& _config;
};

std::unique_ptr<FluxSink> FluxSink::createAuFluxSink(const AuFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new AuFluxSink(config));
}


