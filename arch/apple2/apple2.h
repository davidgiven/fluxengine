#ifndef APPLE2_H
#define APPLE2_H

#include <memory.h>
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"

#define APPLE2_SECTOR_RECORD 0xd5aa96
#define APPLE2_DATA_RECORD 0xd5aaad

#define APPLE2_SECTOR_LENGTH 256
#define APPLE2_ENCODED_SECTOR_LENGTH 342

#define APPLE2_SECTORS 16

extern std::unique_ptr<Decoder> createApple2Decoder(const DecoderProto& config);
extern std::unique_ptr<Encoder> createApple2Encoder(const EncoderProto& config);

#endif
