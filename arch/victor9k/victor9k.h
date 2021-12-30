#ifndef VICTOR9K_H
#define VICTOR9K_H

class AbstractEncoder;
class AbstractDecoder;
class EncoderProto;
class DecoderProto;

/* ... 1101 0101 0111
 *       ^^ ^^^^ ^^^^ ten bit IO byte */
#define VICTOR9K_SECTOR_RECORD 0xfffffffffffffd57
#define VICTOR9K_HEADER_ID 0x7

/* ... 1101 0100 1001
 *       ^^ ^^^^ ^^^^ ten bit IO byte */
#define VICTOR9K_DATA_RECORD   0xfffffffffffffd49
#define VICTOR9K_SECTOR_ID 0x8

#define VICTOR9K_SECTOR_LENGTH 512

extern std::unique_ptr<AbstractDecoder> createVictor9kDecoder(const DecoderProto& config);
extern std::unique_ptr<AbstractEncoder> createVictor9kEncoder(const EncoderProto& config);

#endif
