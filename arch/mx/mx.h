#ifndef MX_H
#define MX_H

#include "decoders/decoders.h"

extern std::unique_ptr<Decoder> createMxDecoder(const DecoderProto& config);

#endif
