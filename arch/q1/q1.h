#ifndef Q1_H
#define Q1_H

#define Q1_ADDRESS_RECORD 0x55424954
#define Q1_DATA_RECORD 0x55424955

extern std::unique_ptr<Decoder> createQ1Decoder(const DecoderProto& config);

#endif
