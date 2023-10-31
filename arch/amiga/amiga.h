#ifndef AMIGA_H
#define AMIGA_H

#include "lib/encoders/encoders.h"

#define AMIGA_SECTOR_RECORD 0xaaaa44894489LL

#define AMIGA_TRACKS_PER_DISK 80
#define AMIGA_SECTORS_PER_TRACK 11
#define AMIGA_RECORD_SIZE 0x21c

extern std::unique_ptr<Decoder> createAmigaDecoder(const DecoderProto& config);
extern std::unique_ptr<Encoder> createAmigaEncoder(const EncoderProto& config);

extern uint32_t amigaChecksum(const Bytes& bytes);
extern Bytes amigaInterleave(const Bytes& input);
extern Bytes amigaDeinterleave(const uint8_t*& input, size_t len);
extern Bytes amigaDeinterleave(const Bytes& input);

#endif
