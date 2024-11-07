#ifndef AGAT_H
#define AGAT_H

#define AGAT_SECTOR_SIZE 256

static constexpr uint64_t SECTOR_ID = 0x8924555549111444;
static constexpr uint64_t DATA_ID = 0x8924555514444911;

class Encoder;
class EncoderProto;
class Decoder;
class DecoderProto;

extern std::unique_ptr<Decoder> createAgatDecoder(const DecoderProto& config);
extern std::unique_ptr<Encoder> createAgatEncoder(const EncoderProto& config);

extern uint8_t agatChecksum(const Bytes& bytes);

#endif
