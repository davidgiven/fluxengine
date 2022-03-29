#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "bytes.h"
#include "protocol.h"
#include "fluxsink/fluxsink.h"
#include "decoders/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/logger.h"
#include "proto.h"
#include "fmt/format.h"
#include "fmt/chrono.h"
#include "fluxmap.h"
#include "a2r.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace
{
uint32_t ticks_to_a2r(uint32_t ticks) {
    return ticks * NS_PER_TICK / A2R_NS_PER_TICK;
}

bool singlesided(void) {
	return config.heads().start() == config.heads().end();
}

class A2RFluxSink : public FluxSink
{
public:
	A2RFluxSink(const A2RFluxSinkProto& lconfig):
		_config(lconfig),
		_bytes{},
		_writer{_bytes.writer()}
	{

		Logger() << fmt::format("A2R: writing A2R {} file containing {} tracks\n",
			singlesided() ? "single sided" : "double sided",
			config.cylinders().end() - config.cylinders().start() + 1
		);

		time_t now{std::time(nullptr)};
		auto t = gmtime(&now);
		_metadata["image_date"] = fmt::format("{:%FT%TZ}", *t);
	}

	~A2RFluxSink()
	{
		writeHeader();
		writeInfo();
		writeStream();
		writeMeta();

		Logger() << "A2R: writing output file...\n";
		std::ofstream of(_config.filename(), std::ios::out | std::ios::binary);
		if (!of.is_open())
			Error() << "cannot open output file";
		_bytes.writeTo(of);
		of.close();
	}

private:
	void writeData(const auto data, size_t size) {
		_writer += Bytes(reinterpret_cast<const uint8_t*>(data), size);
	}
	void writeChunkAndData(uint32_t chunk_id, const auto data, size_t size) {
		_writer.write_le32(chunk_id);
		_writer.write_le32(size);
		writeData(data, size);
	}

	void writeHeader() {
		writeData(a2r2_fileheader, sizeof(a2r2_fileheader));		  
	}

	void writeInfo() {
		A2RInfoData info{
		    A2R_INFO_CHUNK_VERSION,
		    {}, // to be filled
		    singlesided() ? A2R_DISK_525 : A2R_DISK_35,
		    1,
		    1,
		};
		auto version_str = fmt::format("Fluxengine {}", FLUXENGINE_VERSION);
		auto version_str_padded = fmt::format("{: <32}", version_str);
		assert(version_str_padded.size() == sizeof(info.creator));
		memcpy(info.creator, version_str_padded.data(), sizeof(info.creator));
		writeChunkAndData(A2R_CHUNK_INFO, &info, sizeof(info));
	}

	void writeMeta() {
		std::stringstream ss;
		for(auto &i : _metadata) {
			ss << i.first << '\t' << i.second << '\n';
		}
		writeChunkAndData(A2R_CHUNK_META, ss.str().data(), ss.str().size());
	}

	void writeStream() {
		// A STRM always ends with a 255, even though this could ALSO indicate the first byte of a multi-byte sequence
		_strmWriter.write_8(255);

		writeChunkAndData(A2R_CHUNK_STRM, _strmBytes.cbegin(), _strmBytes.size());
	}

	void writeFlux(int cylinder, int head, const Fluxmap& fluxmap) override
	{
		// Note: this assumes that 'cylinder' numbers are in the PC high density 5.25 drive numbering, NOT apple numbering
		unsigned location = cylinder * 2 + head;

		if(!fluxmap.bytes()) {
		    return;
		}

		if (location > 255) {
		    Error() << fmt::format("A2R: cannot write track {} head {}, "
			    "{} does not fit within the location field\n",
			    cylinder, head, location);
		    return;
		}


		// Writing from an image (as opposed to from a floppy) will contain exactly one revolution and no index events.
		auto is_image = [](auto &fluxmap) {
			FluxmapReader fmr(fluxmap);
			fmr.skipToEvent(F_BIT_INDEX);
			// but maybe there is no index, if we're writing from an image to an a2r
			return fmr.eof();
		};

		// Write the flux data into its own Bytes
		Bytes trackBytes;
		auto trackWriter = trackBytes.writer();

		auto write_one_flux = [&](unsigned ticks) {
			auto value = ticks_to_a2r(ticks);
			while(value > 254) {
				trackWriter.write_8(255);
				value -= 255;
			}
			trackWriter.write_8(value);
		};

		int revolution = 0;
		uint32_t loopPoint = 0;
		uint32_t totalTicks = 0;
		FluxmapReader fmr(fluxmap);

		auto write_flux = [&](unsigned maxTicks = ~0u) {
			unsigned ticksSinceLastPulse = 0;

			while(!fmr.eof() && totalTicks < maxTicks) {
				unsigned ticks;
				int event;
				fmr.getNextEvent(event, ticks);

				ticksSinceLastPulse += ticks;
				totalTicks += ticks;

				if(event & F_BIT_PULSE) {
					write_one_flux(ticksSinceLastPulse);
					ticksSinceLastPulse = 0;
				}

				if(event & F_BIT_INDEX && revolution == 0) {
					loopPoint = totalTicks;
					revolution += 1;
				}
			}

		};

		if(is_image(fluxmap)) {
			// A timing stream with no index represents exactly one
			// revolution with no index. However, a2r nominally contains 450
			// degress of rotation, 250ms at 300rpm.
			write_flux();
			loopPoint = totalTicks;
			fmr.rewind();
			revolution += 1;
			write_flux(totalTicks*5/4);
		} else {
			// We have an index, so this is real from a floppy and should be "one revolution plus a bit"
			fmr.skipToEvent(F_BIT_INDEX);
			write_flux();
		}

		uint32_t chunk_size = 10 + trackBytes.size();

		_strmWriter.write_8(location);
		_strmWriter.write_8(A2R_TIMING);
		_strmWriter.write_le32(trackBytes.size());
		_strmWriter.write_le32(ticks_to_a2r(loopPoint));
		_strmWriter += trackBytes;
	}

	operator std::string () const
	{
		return fmt::format("a2r({})", _config.filename());
	}

private:
	const A2RFluxSinkProto& _config;
	A2RHeader _fileheader = {0};
	Bytes _bytes;
	ByteWriter _writer;
	Bytes _strmBytes;
	ByteWriter _strmWriter{_strmBytes.writer()};
	std::map<std::string, std::string> _metadata;
};
}

std::unique_ptr<FluxSink> FluxSink::createA2RFluxSink(const A2RFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new A2RFluxSink(config));
}

uint8_t a2r2_fileheader[] = {'A', '2', 'R', '2', 0xff, 0x0a, 0x0d, 0x0a };
