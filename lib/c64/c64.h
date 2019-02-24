#ifndef C64_H
#define C64_H

#define C64_SECTOR_RECORD 0xfff527
#define C64_DATA_RECORD   0xfff55d

class Sector;
class Fluxmap;

class Commodore64Decoder : public AbstractDecoder
{
public:
    virtual ~Commodore64Decoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack);
	nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    int recordMatcher(uint64_t fifo) const;
};

#endif
