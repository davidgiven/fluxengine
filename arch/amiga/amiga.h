#ifndef AMIGA_H
#define AMIGA_H

#define AMIGA_SECTOR_RECORD 0xaaaa44894489LL

#define AMIGA_RECORD_SIZE 0x21f

class Sector;
class Fluxmap;

class AmigaDecoder : public AbstractDecoder
{
public:
    virtual ~AmigaDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

#endif
