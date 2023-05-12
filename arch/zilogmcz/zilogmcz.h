#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

extern std::unique_ptr<Decoder> createZilogMczDecoder(
    const DecoderProto& config);

#endif
