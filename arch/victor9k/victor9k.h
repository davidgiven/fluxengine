#ifndef VICTOR9K_H
#define VICTOR9K_H

class Encoder;
class Decoder;
class EncoderProto;
class DecoderProto;

/* ... 1101 0101 0111
 *       ^^ ^^^^ ^^^^ ten bit IO byte */
#define VICTOR9K_SECTOR_RECORD 0xfffffd57
#define VICTOR9K_HEADER_ID 0x7

/* ... 1101 0100 1001
 *       ^^ ^^^^ ^^^^ ten bit IO byte */
#define VICTOR9K_DATA_RECORD 0xfffffd49
#define VICTOR9K_DATA_ID 0x8

#define VICTOR9K_SECTOR_LENGTH 512

extern std::unique_ptr<Decoder> createVictor9kDecoder(
    const DecoderProto& config);
extern std::unique_ptr<Encoder> createVictor9kEncoder(
    const EncoderProto& config);

#endif
