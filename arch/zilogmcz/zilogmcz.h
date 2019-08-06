#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

class Sector;
class Fluxmap;

class ZilogMczDecoder : public AbstractDecoder
{
public:
    virtual ~ZilogMczDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

#endif


