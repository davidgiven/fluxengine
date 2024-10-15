#ifndef SCP_H
#define SCP_H

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
    uint8_t track[168][4]; // track offsets, not necessarily 168
};

enum
{
    SCP_FLAG_INDEXED = (1 << 0),
    SCP_FLAG_96TPI = (1 << 1),
    SCP_FLAG_360RPM = (1 << 2),
    SCP_FLAG_NORMALIZED = (1 << 3),
    SCP_FLAG_READWRITE = (1 << 4),
    SCP_FLAG_FOOTER = (1 << 5)
};

struct ScpTrackHeader
{
    char track_id[3]; // 'TRK'
    uint8_t strack;   // SCP track number
};

struct ScpTrackRevolution
{
    uint8_t index[4];  // time for one revolution
    uint8_t length[4]; // number of bitcells
    uint8_t offset[4]; // offset to bitcell data, relative to track header
};

struct ScpTrack
{
    ScpTrackHeader header;
    ScpTrackRevolution revolution[5];
};

#endif
