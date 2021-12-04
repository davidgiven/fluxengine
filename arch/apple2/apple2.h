#ifndef APPLE2_H
#define APPLE2_H

#define APPLE2_SECTOR_RECORD   0xd5aa96
#define APPLE2_DATA_RECORD     0xd5aaad

#define APPLE2_SECTOR_LENGTH   256
#define APPLE2_ENCODED_SECTOR_LENGTH 342

extern std::unique_ptr<AbstractDecoder> createApple2Decoder(const DecoderProto& config);

#endif

