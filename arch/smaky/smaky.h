#ifndef SMAKY_H
#define SMAKY_H

#define SMAKY_SECTOR_SIZE 256
#define SMAKY_RECORD_SIZE (1 + SMAKY_SECTOR_SIZE + 1)

extern std::unique_ptr<Decoder> createSmakyDecoder(const DecoderProto& config);

#endif

