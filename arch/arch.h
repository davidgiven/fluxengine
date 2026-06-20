#pragma once

class Encoder;
class Decoder;
class DecoderProto;
class EncoderProto;
class Config;

namespace Arch
{
    std::unique_ptr<Decoder> createDecoder(Config& config);
    std::unique_ptr<Decoder> createDecoder(const DecoderProto& config);

    Encoder* createEncoder(Config& config);
    Encoder* createEncoder(const EncoderProto& config);
}
