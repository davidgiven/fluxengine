#ifndef A2R_H
#define A2R_H

// The canonical reference for the A2R format is: https://applesaucefdc.com/a2r2-reference/
// All data is stored little-endian

// Note: The first chunk begins at byte offset 8, not 12 as given in a2r2 reference version 2.0.1

struct A2RHeader
{
    char file_id[8];        // 'A2R' + FF + LF CR LF
};

#define A2R_CHUNK_INFO (0x4F464E49)
#define A2R_CHUNK_STRM (0x4D525453)
#define A2R_CHUNK_META (0x4154454D)

#define A2R_INFO_CHUNK_VERSION (1)

struct A2RChunk
{
    uint32_t chunk_id_le32;
    uint32_t chunk_size_le32;
    uint8_t data[];
};

enum A2RDiskType {
    A2R_DISK_525 = 1,
    A2R_DISK_35 = 2,
};

struct A2RInfoData {
    uint8_t version;
    char creator[32];
    uint8_t disk_type, write_protected, synchronized;
};

enum A2RCaptureType {
    A2R_TIMING = 1,
    A2R_BITS = 2,
    A2R_XTIMING = 3,
};

struct A2RStrmData {
    uint8_t location;
    uint8_t capture_type;
    uint32_t data_length_le32;
    uint32_t estimated_loop_point_le32;
    uint8_t data[];
};

extern uint8_t a2r2_fileheader[8];

#define A2R_NS_PER_TICK (125)

#endif
