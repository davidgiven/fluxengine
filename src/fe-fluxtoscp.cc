#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sql.h"
#include "bytes.h"
#include "protocol.h"
#include "dataspec.h"
#include "fmt/format.h"
#include "decoders/fluxmapreader.h"
#include "scp.h"
#include <fstream>
#include <algorithm>

static FlagGroup flags { };

static SettableFlag fortyTrackMode(
    { "--48", "-4" },
    "set 48 tpi mode; only every other physical track is emitted"
);

static SettableFlag singleSided(
    { "--single-sided", "-s" },
    "only emit side 0"
);

static IntFlag diskType(
    { "--disk-type" },
    "sets the SCP disk type byte",
    0xff
);

static sqlite3* inputDb;

static void syntax()
{
    std::cout << "Syntax: fluxengine convert fluxtoscp <fluxfile> <scpfile>\n";
    exit(0);
}

static void write_le32(uint8_t dest[4], uint32_t v)
{
    dest[0] = v;
    dest[1] = v >> 8;
    dest[2] = v >> 16;
    dest[3] = v >> 24;
}

static int strackno(int track, int side)
{
    if (fortyTrackMode)
        track /= 2;
    if (singleSided)
        return track;
    else
        return (track << 1) | side;
}

int mainConvertFluxToScp(int argc, const char* argv[])
{
    auto filenames = flags.parseFlagsWithFilenames(argc, argv);
    if (filenames.size() != 2)
        syntax();

    inputDb = sqlOpen(filenames[0], SQLITE_OPEN_READONLY);
    auto tracks = sqlFindFlux(inputDb);

    int maxTrack = 0;
    int maxSide = 0;
    for (auto p : tracks)
    {
        if (singleSided && (p.second == 1))
            continue;
        maxTrack = std::max(maxTrack, (int)p.first);
        maxSide = std::max(maxSide, (int)p.second);
    }
    int maxStrack = strackno(maxTrack, maxSide);

    std::cout << fmt::format("Writing {} {} SCP file containing {} SCP tracks\n",
        fortyTrackMode ? "48 tpi" : "96 tpi",
        singleSided ? "single sided" : "double sided",
        maxStrack + 1
    );

    ScpHeader fileheader = {0};
    fileheader.file_id[0] = 'S';
    fileheader.file_id[1] = 'C';
    fileheader.file_id[2] = 'P';
    fileheader.version = 0x18; /* Version 1.8 of the spec */
    fileheader.type = diskType;
    fileheader.revolutions = 5;
    fileheader.start_track = 0;
    fileheader.end_track = maxStrack;
    fileheader.flags = SCP_FLAG_INDEXED | (fortyTrackMode ? 0 : SCP_FLAG_96TPI);
    fileheader.cell_width = 0;
    fileheader.heads = singleSided ? 1 : 0;

    Bytes trackdata;
    ByteWriter trackdataWriter(trackdata);

    int trackstep = 1 + fortyTrackMode;
    int maxside = singleSided ? 0 : 1;
    for (int track = 0; track <= maxTrack; track += trackstep)
    {
        for (int side = 0; side <= maxside; side++)
        {
            int strack = strackno(track, side);
            std::cout << fmt::format("FE track {}.{}, SCP track {}: ", track, side, strack) << std::flush;

            auto fluxmap = sqlReadFlux(inputDb, track, side);
            ScpTrack trackheader = {0};
            trackheader.track_id[0] = 'T';
            trackheader.track_id[1] = 'R';
            trackheader.track_id[2] = 'K';
            trackheader.strack = strack;

            FluxmapReader fmr(*fluxmap);
            Bytes fluxdata;
            ByteWriter fluxdataWriter(fluxdata);

            int revolution = 0;
            unsigned revTicks = 0;
            unsigned totalTicks = 0;
            unsigned ticksSinceLastPulse = 0;
            uint32_t startOffset = 0;
            while (revolution < 5)
            {
                unsigned ticks;
                int opcode = fmr.readOpcode(ticks);
                if (ticks)
                {
                    ticksSinceLastPulse += ticks;
                    totalTicks += ticks;
                    revTicks += ticks;
                }

                switch (opcode)
                {
                    case -1: /* end of flux, treat like an index marker */
                    case F_OP_INDEX:
                    {
                        auto* revheader = &trackheader.revolution[revolution];
                        write_le32(revheader->offset, startOffset + sizeof(ScpTrack));
                        write_le32(revheader->length, (fluxdataWriter.pos - startOffset) / 2);
                        write_le32(revheader->index, revTicks * NS_PER_TICK / 25);
                        revolution++;
                        revheader++;
                        revTicks = 0;
                        startOffset = fluxdataWriter.pos;
                        break;
                    }

                    case F_OP_PULSE:
                    {
                        unsigned t = ticksSinceLastPulse * NS_PER_TICK / 25;
                        while (t >= 0x10000)
                        {
                            fluxdataWriter.write_be16(0);
                            t -= 0x10000;
                        }
                        fluxdataWriter.write_be16(t);
                        ticksSinceLastPulse = 0;
                        break;
                    }
                }
            }

            write_le32(fileheader.track[strack], trackdataWriter.pos + sizeof(ScpHeader));
            trackdataWriter += Bytes((uint8_t*)&trackheader, sizeof(trackheader));
            trackdataWriter += fluxdata;

            std::cout << fmt::format("{} ms in {} bytes\n",
                totalTicks * MS_PER_TICK,
                fluxdata.size());
        }
    }

    sqlClose(inputDb);
    
    std::cout << "Writing output file...\n";
    std::ofstream of(filenames[1], std::ios::out | std::ios::binary);
    if (!of.is_open())
        Error() << "cannot open output file";
    of.write((const char*) &fileheader, sizeof(fileheader));
    of.write((const char*) trackdata.begin(), trackdata.size());
    of.close();

    return 0;
}
