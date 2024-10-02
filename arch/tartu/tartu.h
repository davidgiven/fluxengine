#ifndef TARTU_H
#define TARTU_H

extern std::unique_ptr<Decoder> createTartuDecoder(const DecoderProto& config);
extern std::unique_ptr<Encoder> createTartuEncoder(const EncoderProto& config);

#endif
