#pragma once

class Encoder;
class Decoder;
class DecoderProto;
class EncoderProto;
class Config;

namespace Arch
{
    Decoder* createDecoder(Config& config);
    Decoder* createDecoder(const DecoderProto& config);

    Encoder* createEncoder(Config& config);
    Encoder* createEncoder(const EncoderProto& config);
}
