#ifndef SMAKY6_H
#define SMAKY6_H

#define SMAKY6_SECTOR_SIZE 256
#define SMAKY6_RECORD_SIZE (1 + SMAKY6_SECTOR_SIZE + 1)

extern Decoder* createSmaky6Decoder(const DecoderProto& config);

#endif
