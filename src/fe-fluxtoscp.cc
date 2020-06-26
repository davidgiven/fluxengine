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

static SettableFlag indexedMode(
	{ "--indexed", "-i" },
	"align data to track boundaries"
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

static void appendChecksum(uint32_t& checksum, const Bytes& bytes)
{
	ByteReader br(bytes);
	while (!br.eof())
		checksum += br.read_8();
}

static int strackno(int track, int side)
{
    if (fortyTrackMode)
        track /= 2;
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
    fileheader.flags = (indexedMode ? SCP_FLAG_INDEXED : 0)
			| (fortyTrackMode ? 0 : SCP_FLAG_96TPI);
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
            std::cout << fmt::format("{}.{}: ", track, side) << std::flush;

            auto fluxmap = sqlReadFlux(inputDb, track, side);
			if (fluxmap->bytes() == 0)
			{
				std::cout << "missing\n";
				continue;
			}

            ScpTrack trackheader = {0};
            trackheader.track_id[0] = 'T';
            trackheader.track_id[1] = 'R';
            trackheader.track_id[2] = 'K';
            trackheader.strack = strack;

            FluxmapReader fmr(*fluxmap);
            Bytes fluxdata;
            ByteWriter fluxdataWriter(fluxdata);

			if (indexedMode)
				fmr.findEvent(F_BIT_INDEX);

            int revolution = 0;
            unsigned revTicks = 0;
            unsigned totalTicks = 0;
            unsigned ticksSinceLastPulse = 0;
            uint32_t startOffset = 0;
            while (revolution < 5)
            {
                unsigned ticks;
                uint8_t bits = fmr.getNextEvent(ticks);
				ticksSinceLastPulse += ticks;
				totalTicks += ticks;
				revTicks += ticks;

				if (fmr.eof() || (bits & F_BIT_INDEX))
				{
					auto* revheader = &trackheader.revolution[revolution];
					write_le32(revheader->offset, startOffset + sizeof(ScpTrack));
					write_le32(revheader->length, (fluxdataWriter.pos - startOffset) / 2);
					write_le32(revheader->index, revTicks * NS_PER_TICK / 25);
					revolution++;
					revheader++;
					revTicks = 0;
					startOffset = fluxdataWriter.pos;
				}

				if (bits & F_BIT_PULSE)
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

            write_le32(fileheader.track[strack], trackdataWriter.pos + sizeof(ScpHeader));
            trackdataWriter += Bytes((uint8_t*)&trackheader, sizeof(trackheader));
            trackdataWriter += fluxdata;

            std::cout << fmt::format("{:.3f} ms in {} bytes\n",
                totalTicks * MS_PER_TICK,
                fluxdata.size());
        }
    }

    sqlClose(inputDb);
    
	uint32_t checksum = 0;
	appendChecksum(checksum,
		Bytes((const uint8_t*) &fileheader, sizeof(fileheader))
			.slice(0x10));
	appendChecksum(checksum, trackdata);
	write_le32(fileheader.checksum, checksum);

    std::cout << "Writing output file...\n";
    std::ofstream of(filenames[1], std::ios::out | std::ios::binary);
    if (!of.is_open())
        Error() << "cannot open output file";
    of.write((const char*) &fileheader, sizeof(fileheader));
    of.write((const char*) trackdata.begin(), trackdata.size());
    of.close();

    return 0;
}
