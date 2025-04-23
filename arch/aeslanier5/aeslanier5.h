#ifndef AESLANIER5_H
#define AESLANIER5_H

#define AESLANIER5_RECORD_SEPARATOR 0x55555122
#define AESLANIER5_SECTOR_LENGTH 256
#define AESLANIER5_RECORD_SIZE (AESLANIER5_SECTOR_LENGTH + 5)

extern std::unique_ptr<Decoder> createAesLanier5Decoder(
    const DecoderProto& config);

#endif
