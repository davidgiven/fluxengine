#ifndef VICTOR9K_H
#define VICTOR9K_H

#define VICTOR9K_SECTOR_RECORD 0xfffffeab
#define VICTOR9K_DATA_RECORD   0xfffffea4

#define VICTOR9K_SECTOR_LENGTH 512

extern std::unique_ptr<AbstractDecoder> createVictor9kDecoder(const DecoderProto& config);

#endif
