#ifndef TIDS990_H
#define TIDS990_H

#define TIDS990_PAYLOAD_SIZE       288 /* bytes */
#define TIDS990_SECTOR_RECORD_SIZE 10 /* bytes */
#define TIDS990_DATA_RECORD_SIZE   (TIDS990_PAYLOAD_SIZE + 4) /* bytes */

class AbstractEncoder;
class AbstractDecoder;
class DecoderProto;
class EncoderProto;

extern std::unique_ptr<AbstractDecoder> createTids990Decoder(const DecoderProto& config);

#endif


