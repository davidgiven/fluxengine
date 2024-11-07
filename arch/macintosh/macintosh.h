#ifndef MACINTOSH_H
#define MACINTOSH_H

#define MAC_SECTOR_RECORD 0xd5aa96 /* 1101 0101 1010 1010 1001 0110 */
#define MAC_DATA_RECORD 0xd5aaad   /* 1101 0101 1010 1010 1010 1101 */

#define MAC_SECTOR_LENGTH 524 /* yes, really */
#define MAC_ENCODED_SECTOR_LENGTH 703
#define MAC_FORMAT_BYTE 0x22

#define MAC_TRACKS_PER_DISK 80

class Encoder;
class Decoder;
class DecoderProto;
class EncoderProto;

extern std::unique_ptr<Decoder> createMacintoshDecoder(
    const DecoderProto& config);
extern std::unique_ptr<Encoder> createMacintoshEncoder(
    const EncoderProto& config);

#endif
