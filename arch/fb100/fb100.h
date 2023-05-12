#ifndef FB100_H
#define FB100_H

#define FB100_RECORD_SIZE 0x516 /* bytes */
#define FB100_ID_SIZE 17
#define FB100_PAYLOAD_SIZE 0x500

extern std::unique_ptr<Decoder> createFb100Decoder(const DecoderProto& config);

#endif
