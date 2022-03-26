#ifndef AGAT_H
#define AGAT_H

#define AGAT_SECTOR_SIZE 256

extern std::unique_ptr<AbstractDecoder> createAgatDecoder(const DecoderProto& config);

extern uint8_t agatChecksum(const Bytes& bytes);

#endif

