#ifndef A2R_H
#define A2R_H

// The canonical reference for the A2R format is:
// https://applesaucefdc.com/a2r2-reference/ All data is stored little-endian

// Note: The first chunk begins at byte offset 8, not 12 as given in a2r2
// reference version 2.0.1

#define A2R_CHUNK_INFO (0x4F464E49)
#define A2R_CHUNK_STRM (0x4D525453)
#define A2R_CHUNK_META (0x4154454D)

#define A2R_INFO_CHUNK_VERSION (1)

enum A2RDiskType
{
    A2R_DISK_525 = 1,
    A2R_DISK_35 = 2,
};

enum A2RCaptureType
{
    A2R_TIMING = 1,
    A2R_BITS = 2,
    A2R_XTIMING = 3,
};

extern const uint8_t a2r2_fileheader[8];

#define A2R_NS_PER_TICK (125)

#endif
