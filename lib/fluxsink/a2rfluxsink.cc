#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "protocol.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/data/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/core/logger.h"
#include "lib/config/proto.h"
#include "lib/data/fluxmap.h"
#include "lib/data/layout.h"
#include "lib/external/a2r.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "fmt/chrono.h"

namespace
{
    uint32_t ticks_to_a2r(uint32_t ticks)
    {
        return ticks * NS_PER_TICK / A2R_NS_PER_TICK;
    }

    class A2RFluxSink : public FluxSink
    {
    public:
        A2RFluxSink(const A2RFluxSinkProto& lconfig):
            _config(lconfig),
            _bytes{},
            _writer{_bytes.writer()}
        {
            log("A2R: collecting data");

            time_t now{std::time(nullptr)};
            auto t = gmtime(&now);
            _metadata["image_date"] = fmt::format("{:%FT%TZ}", *t);
        }

        ~A2RFluxSink()
        {
            auto locations = Layout::computeLocations();
            Layout::getBounds(
                locations, _minTrack, _maxTrack, _minSide, _maxSide);

            log("A2R: writing A2R {} file containing {} tracks...",
                (_minSide == _maxSide) ? "single sided" : "double sided",
                _maxTrack - _minTrack + 1);

            writeHeader();
            writeInfo();
            writeStream();
            writeMeta();

            std::ofstream of(
                _config.filename(), std::ios::out | std::ios::binary);
            if (!of.is_open())
                error("cannot open output file");
            _bytes.writeTo(of);
            of.close();
        }

    private:
        void writeChunkAndData(uint32_t chunk_id, const Bytes& data)
        {
            _writer.write_le32(chunk_id);
            _writer.write_le32(data.size());
            _writer += data;
        }

        void writeHeader()
        {
            static const uint8_t a2r2_fileheader[] = {
                'A', '2', 'R', '2', 0xff, 0x0a, 0x0d, 0x0a};
            _writer += Bytes(a2r2_fileheader, sizeof(a2r2_fileheader));
        }

        void writeInfo()
        {
            Bytes info;
            auto writer = info.writer();
            writer.write_8(A2R_INFO_CHUNK_VERSION);
            auto version_str_padded = fmt::format("{: <32}", "FluxEngine");
            assert(version_str_padded.size() == 32);
            writer.append(version_str_padded);
            writer.write_8(A2R_DISK_35);
            writer.write_8(1); // write protected
            writer.write_8(1); // synchronized
            writeChunkAndData(A2R_CHUNK_INFO, info);
        }

        void writeMeta()
        {
            Bytes meta;
            auto writer = meta.writer();
            for (auto& i : _metadata)
            {
                writer.append(i.first);
                writer.write_8('\t');
                writer.append(i.second);
                writer.write_8('\n');
            }
            writeChunkAndData(A2R_CHUNK_META, meta);
        }

        void writeStream()
        {
            // A STRM always ends with a 255, even though this could ALSO
            // indicate the first byte of a multi-byte sequence
            _strmWriter.write_8(255);

            writeChunkAndData(A2R_CHUNK_STRM, _strmBytes);
        }

        void writeFlux(int cylinder, int head, const Fluxmap& fluxmap) override
        {
            if (!fluxmap.bytes())
            {
                return;
            }

            // Writing from an image (as opposed to from a floppy) will contain
            // exactly one revolution and no index events.
            auto is_image = [](auto& fluxmap)
            {
                FluxmapReader fmr(fluxmap);
                fmr.skipToEvent(F_BIT_INDEX);
                // but maybe there is no index, if we're writing from an image
                // to an a2r
                return fmr.eof();
            };

            // Write the flux data into its own Bytes
            Bytes trackBytes;
            auto trackWriter = trackBytes.writer();

            auto write_one_flux = [&](unsigned ticks)
            {
                auto value = ticks_to_a2r(ticks);
                while (value > 254)
                {
                    trackWriter.write_8(255);
                    value -= 255;
                }
                trackWriter.write_8(value);
            };

            int revolution = 0;
            uint32_t loopPoint = 0;
            uint32_t totalTicks = 0;
            FluxmapReader fmr(fluxmap);

            auto write_flux = [&](unsigned maxTicks = ~0u)
            {
                unsigned ticksSinceLastPulse = 0;

                while (!fmr.eof() && totalTicks < maxTicks)
                {
                    unsigned ticks;
                    int event;
                    fmr.getNextEvent(event, ticks);

                    ticksSinceLastPulse += ticks;
                    totalTicks += ticks;

                    if (event & F_BIT_PULSE)
                    {
                        write_one_flux(ticksSinceLastPulse);
                        ticksSinceLastPulse = 0;
                    }

                    if (event & F_BIT_INDEX && revolution == 0)
                    {
                        loopPoint = totalTicks;
                        revolution += 1;
                    }
                }
            };

            if (is_image(fluxmap))
            {
                // A timing stream with no index represents exactly one
                // revolution with no index. However, a2r nominally contains 450
                // degress of rotation, 250ms at 300rpm.
                write_flux();
                loopPoint = totalTicks;
                fmr.rewind();
                revolution += 1;
                write_flux(totalTicks * 5 / 4);
            }
            else
            {
                // We have an index, so this is a real read from a floppy and
                // should be "one revolution plus a bit"
                fmr.skipToEvent(F_BIT_INDEX);
                write_flux();
            }

            uint32_t chunk_size = 10 + trackBytes.size();

            _strmWriter.write_8((cylinder << 1) | head);
            _strmWriter.write_8(A2R_TIMING);
            _strmWriter.write_le32(trackBytes.size());
            _strmWriter.write_le32(ticks_to_a2r(loopPoint));
            _strmWriter += trackBytes;
        }

        operator std::string() const override
        {
            return fmt::format("a2r({})", _config.filename());
        }

    private:
        const A2RFluxSinkProto& _config;
        Bytes _bytes;
        ByteWriter _writer;
        Bytes _strmBytes;
        ByteWriter _strmWriter{_strmBytes.writer()};
        std::map<std::string, std::string> _metadata;
        int _minSide;
        int _maxSide;
        int _minTrack;
        int _maxTrack;
    };
} // namespace

std::unique_ptr<FluxSink> FluxSink::createA2RFluxSink(
    const A2RFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new A2RFluxSink(config));
}
