#ifndef AMIGA_H
#define AMIGA_H

#define AMIGA_SECTOR_RECORD 0xaaaaaaaa44894489LL

class Sector;
class Fluxmap;

class AmigaDecoder : public AbstractDecoder
{
public:
    virtual ~AmigaDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords);
	nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    int recordMatcher(uint64_t fifo) const;
};

#endif
