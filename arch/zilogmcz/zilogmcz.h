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

#endif


