#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD 0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD 256
#define BROTHER_DATA_RECORD_CHECKSUM 3
#define BROTHER_DATA_RECORD_ENCODED_SIZE 415

#define BROTHER_TRACKS_PER_240KB_DISK 78
#define BROTHER_TRACKS_PER_120KB_DISK 39
#define BROTHER_SECTORS_PER_TRACK 12

extern std::unique_ptr<Decoder> createBrotherDecoder(
    const DecoderProto& config);
extern std::unique_ptr<Encoder> createBrotherEncoder(
    const EncoderProto& config);

#endif
