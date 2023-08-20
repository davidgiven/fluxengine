#ifndef MICROPOLIS_H
#define MICROPOLIS_H

#define MICROPOLIS_PAYLOAD_SIZE (256)
#define MICROPOLIS_HEADER_SIZE (1 + 2 + 10)
#define MICROPOLIS_ENCODED_SECTOR_SIZE \
    (MICROPOLIS_HEADER_SIZE + MICROPOLIS_PAYLOAD_SIZE + 6)

class Decoder;
class Encoder;
class EncoderProto;
class DecoderProto;

extern std::unique_ptr<Decoder> createMicropolisDecoder(
    const DecoderProto& config);
extern std::unique_ptr<Encoder> createMicropolisEncoder(
    const EncoderProto& config);

extern uint8_t micropolisChecksum(const Bytes& bytes);
extern uint32_t vectorGraphicEcc(const Bytes& bytes);

#endif
