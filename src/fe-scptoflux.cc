#include "globals.h"
#include "fluxmap.h"
#include "sql.h"
#include "bytes.h"
#include "protocol.h"
#include "fmt/format.h"
#include <fstream>

struct ScpHeader
{
    char file_id[3];       // file ID - 'SCP'
    uint8_t version;       // major/minor in nibbles
    uint8_t type;          // disk type - subclass/class in nibbles
    uint8_t revolutions;   // up to 5
    uint8_t start_track;   // 0..165
    uint8_t end_track;     // 0..165
    uint8_t flags;         // see below
    uint8_t cell_width;    // in bits, 0 meaning 16
    uint8_t heads;         // 0 = both, 1 = side 0 only, 2 = side 1 only
    uint8_t resolution;    // 25ns * (resolution+1)
    uint8_t checksum[4];   // of data after this point
    uint8_t track[165][4]; // track offsets, not necessarily 165
};

struct ScpTrack
{
    char track_id[3];      // 'TRK'
    uint8_t strack;        // SCP track number
    struct
    {
        uint8_t index[4];  // time for one revolution
        uint8_t length[4]; // number of bitcells
        uint8_t offset[4]; // offset to bitcell data, relative to track header
    }
    revolution[5];
};

static std::ifstream inputFile;
static sqlite3* outputDb;
static ScpHeader header;
static nanoseconds_t resolution;
static int startSide;
static int endSide;

static void syntax()
{
    std::cout << "Syntax: fluxengine convert cwftoflux <cwffile> <fluxfile>\n";
    exit(0);
}

static void check_for_error()
{
    if (inputFile.fail())
        Error() << fmt::format("I/O error: {}", strerror(errno));
}

static int trackno(int strack)
{
    if (startSide == endSide)
        return strack;
    return strack >> 1;
}

static int headno(int strack)
{
    if (startSide == endSide)
        return startSide;
    return strack & 1;
}

static void read_header()
{
    inputFile.read((char*) &header, sizeof(header));
    check_for_error();

    if ((header.file_id[0] != 'S')
            || (header.file_id[1] != 'C')
            || (header.file_id[2] != 'P'))
        Error() << "input not a SCP file";

    resolution = 25 * (header.resolution + 1);
    startSide = (header.heads == 2) ? 1 : 0;
    endSide = (header.heads == 1) ? 0 : 1;

    if ((header.cell_width != 0) && (header.cell_width != 16))
        Error() << "currently only 16-bit cells are supported";

    std::cout << fmt::format("tracks {}-{}, heads {}-{}\n",
        trackno(header.start_track), trackno(header.end_track), startSide, endSide);
    std::cout << fmt::format("sample resolution: {} ns\n", resolution);
}

static void read_track(int strack)
{
    uint32_t offset = Bytes(header.track[strack], 4).reader().read_le32();

    ScpTrack trackheader;
    inputFile.seekg(offset, std::ios::beg);
    inputFile.read((char*) &trackheader, sizeof(trackheader));
    check_for_error();

    if ((trackheader.track_id[0] != 'T')
            || (trackheader.track_id[1] != 'R')
            || (trackheader.track_id[2] != 'K'))
        Error() << "corrupt SCP file";

    std::cout << fmt::format("{}.{}: ", trackno(strack), headno(strack))
        << std::flush;

    Fluxmap fluxmap;
    nanoseconds_t pending = 0;
    for (int revolution = 0; revolution < header.revolutions; revolution++)
    {
        if (revolution != 0)
            fluxmap.appendIndex();

        uint32_t datalength = Bytes(trackheader.revolution[revolution].length, 4).reader().read_le32();
        uint32_t dataoffset = Bytes(trackheader.revolution[revolution].offset, 4).reader().read_le32();

        Bytes data(datalength*2);
        inputFile.seekg(dataoffset + offset, std::ios::beg);
        inputFile.read((char*) data.begin(), data.size());
        check_for_error();

        ByteReader br(data);
        for (int cell = 0; cell < datalength; cell++)
        {
            uint16_t interval = br.read_be16();
            if (interval)
            {
                fluxmap.appendInterval((interval + pending) * resolution / NS_PER_TICK);
                fluxmap.appendPulse();
                pending = 0;
            }
            else
                pending += interval;
        }
    }

    std::cout << fmt::format(" {} ms in {} output bytes\n",
        fluxmap.duration() / 1e6, fluxmap.bytes());
    sqlWriteFlux(outputDb, trackno(strack), headno(strack), fluxmap);
}

int mainConvertScpToFlux(int argc, const char* argv[])
{
    if (argc != 3)
        syntax();
    
    inputFile.open(argv[1], std::ios::in | std::ios::binary);
    if (!inputFile.is_open())
		Error() << fmt::format("cannot open input file '{}'", argv[1]);

    outputDb = sqlOpen(argv[2], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    sqlPrepareFlux(outputDb);
    sqlWriteIntProperty(outputDb, "version", FLUX_VERSION_CURRENT);
    sqlStmt(outputDb, "BEGIN;");

    read_header();
    inputFile.seekg(sizeof(header), std::ios::beg);
    for (unsigned i=header.start_track; i<=header.end_track; i++)
        read_track(i);

    sqlStmt(outputDb, "COMMIT;");
    sqlClose(outputDb);
    return 0;
}
