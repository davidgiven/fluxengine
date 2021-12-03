#ifndef MICROPOLIS_H
#define MICROPOLIS_H

#define MICROPOLIS_ENCODED_SECTOR_SIZE (1+2+266+6)

class AbstractDecoder;
class AbstractEncoder;
class EncoderProto;
class DecoderProto;

extern std::unique_ptr<AbstractDecoder> createMicropolisDecoder(const DecoderProto& config);
extern std::unique_ptr<AbstractEncoder> createMicropolisEncoder(const EncoderProto& config);

extern uint8_t micropolisChecksum(const Bytes& bytes);

#endif
