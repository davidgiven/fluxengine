#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

class Sector;
class Fluxmap;
class ZilogMczDecoderProto;

class ZilogMczDecoder : public AbstractDecoder
{
public:
	ZilogMczDecoder(const ZilogMczDecoderProto&) {}
    virtual ~ZilogMczDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

extern std::unique_ptr<AbstractDecoder> createZilogMczDecoder(const DecoderProto& config);

#endif


