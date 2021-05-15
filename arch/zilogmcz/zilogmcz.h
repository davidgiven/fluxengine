#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

class Sector;
class Fluxmap;
class ZilogMczInputProto;

class ZilogMczDecoder : public AbstractDecoder
{
public:
	ZilogMczDecoder(const ZilogMczInputProto&) {}
    virtual ~ZilogMczDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

#endif


