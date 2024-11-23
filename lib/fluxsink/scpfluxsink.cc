#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "protocol.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/data/fluxmapreader.h"
#include "lib/fluxsink/fluxsink.pb.h"
#include "lib/config/proto.h"
#include "lib/data/fluxmap.h"
#include "lib/data/layout.h"
#include "lib/external/scp.h"
#include "lib/core/logger.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

static int strackno(int track, int side)
{
    return (track << 1) | side;
}

static void write_le32(uint8_t dest[4], uint32_t v)
{
    dest[0] = v;
    dest[1] = v >> 8;
    dest[2] = v >> 16;
    dest[3] = v >> 24;
}

static void appendChecksum(uint32_t& checksum, const Bytes& bytes)
{
    ByteReader br(bytes);
    while (!br.eof())
        checksum += br.read_8();
}

class ScpFluxSink : public FluxSink
{
public:
    ScpFluxSink(const ScpFluxSinkProto& lconfig): _config(lconfig)
    {
        int minTrack;
        int maxTrack;
        int minSide;
        int maxSide;
        Layout::getBounds(
            Layout::computeLocations(), minTrack, maxTrack, minSide, maxSide);

        _fileheader.file_id[0] = 'S';
        _fileheader.file_id[1] = 'C';
        _fileheader.file_id[2] = 'P';
        _fileheader.version = 0x18; /* Version 1.8 of the spec */
        _fileheader.type = _config.type_byte();
        _fileheader.start_track = strackno(minTrack, minSide);
        _fileheader.end_track = strackno(maxTrack, maxSide);
        _fileheader.flags = SCP_FLAG_INDEXED;
        if (globalConfig()->drive().drive_type() != DRIVETYPE_40TRACK)
            _fileheader.flags |= SCP_FLAG_96TPI;
        _fileheader.cell_width = 0;
        if ((minSide == 0) && (maxSide == 0))
            _fileheader.heads = 1;
        else if ((minSide == 1) && (maxSide == 1))
            _fileheader.heads = 2;
        else
            _fileheader.heads = 0;

        log("SCP: writing {} tpi {} file containing {} tracks",
            (_fileheader.flags & SCP_FLAG_96TPI) ? 96 : 48,
            (minSide == maxSide) ? "single sided" : "double sided",
            _fileheader.end_track - _fileheader.start_track + 1);
    }

    ~ScpFluxSink()
    {
        uint32_t checksum = 0;
        appendChecksum(checksum,
            Bytes((const uint8_t*)&_fileheader, sizeof(_fileheader))
                .slice(0x10));
        appendChecksum(checksum, _trackdata);
        write_le32(_fileheader.checksum, checksum);

        log("SCP: writing output file");
        std::ofstream of(_config.filename(), std::ios::out | std::ios::binary);
        if (!of.is_open())
            error("cannot open output file");
        of.write((const char*)&_fileheader, sizeof(_fileheader));
        _trackdata.writeTo(of);
        of.close();
    }

public:
    void writeFlux(int track, int head, const Fluxmap& fluxmap) override
    {
        ByteWriter trackdataWriter(_trackdata);
        trackdataWriter.seekToEnd();
        int strack = strackno(track, head);

        if (strack >= std::size(_fileheader.track))
        {
            log("SCP: cannot write track {} head {}, there are not not enough "
                "Track Data Headers.",
                track,
                head);
            return;
        }
        ScpTrack trackheader = {0};
        trackheader.header.track_id[0] = 'T';
        trackheader.header.track_id[1] = 'R';
        trackheader.header.track_id[2] = 'K';
        trackheader.header.strack = strack;

        FluxmapReader fmr(fluxmap);
        Bytes fluxdata;
        ByteWriter fluxdataWriter(fluxdata);

        int revolution =
            -1; // -1 indicates that we are before the first index pulse
        if (_config.align_with_index())
        {
            fmr.skipToEvent(F_BIT_INDEX);
            revolution = 0;
        }
        unsigned revTicks = 0;
        unsigned totalTicks = 0;
        unsigned ticksSinceLastPulse = 0;
        uint32_t startOffset = 0;
        while (revolution < 5)
        {
            unsigned ticks;
            int event;
            fmr.getNextEvent(event, ticks);

            ticksSinceLastPulse += ticks;
            totalTicks += ticks;
            revTicks += ticks;

            // if we haven't output any revolutions yet by the end of the track,
            // assume that the whole track is one rev
            // also discard any duplicate index pulses
            if (((fmr.eof() && revolution <= 0) ||
                    ((event & F_BIT_INDEX)) && revTicks > 0))
            {
                if (fmr.eof() && revolution == -1)
                    revolution = 0;
                if (revolution >= 0)
                {
                    auto* revheader = &trackheader.revolution[revolution];
                    write_le32(
                        revheader->offset, startOffset + sizeof(ScpTrack));
                    write_le32(revheader->length,
                        (fluxdataWriter.pos - startOffset) / 2);
                    write_le32(revheader->index, revTicks * NS_PER_TICK / 25);
                    revheader++;
                }
                revolution++;
                revTicks = 0;
                startOffset = fluxdataWriter.pos;
            }
            if (fmr.eof())
                break;

            if (event & F_BIT_PULSE)
            {
                unsigned t = ticksSinceLastPulse * NS_PER_TICK / 25;
                while (t >= 0x10000)
                {
                    fluxdataWriter.write_be16(0);
                    t -= 0x10000;
                }
                fluxdataWriter.write_be16(t);
                ticksSinceLastPulse = 0;
            }
        }

        _fileheader.revolutions = revolution;
        write_le32(
            _fileheader.track[strack], trackdataWriter.pos + sizeof(ScpHeader));
        trackdataWriter += Bytes((uint8_t*)&trackheader, sizeof(trackheader));
        trackdataWriter += fluxdata;
    }

    operator std::string() const override
    {
        return fmt::format("scp({})", _config.filename());
    }

private:
    const ScpFluxSinkProto& _config;
    ScpHeader _fileheader = {0};
    Bytes _trackdata;
};

std::unique_ptr<FluxSink> FluxSink::createScpFluxSink(
    const ScpFluxSinkProto& config)
{
    return std::unique_ptr<FluxSink>(new ScpFluxSink(config));
}
