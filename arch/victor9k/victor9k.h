#ifndef VICTOR9K_H
#define VICTOR9K_H

class AbstractEncoder;
class AbstractDecoder;
class EncoderProto;
class DecoderProto;

#define VICTOR9K_SECTOR_RECORD 0xfffffeab
#define VICTOR9K_DATA_RECORD   0xfffffea4

#define VICTOR9K_SECTOR_LENGTH 512

extern std::unique_ptr<AbstractDecoder> createVictor9kDecoder(const DecoderProto& config);
extern std::unique_ptr<AbstractEncoder> createVictor9kEncoder(const EncoderProto& config);

#endif
