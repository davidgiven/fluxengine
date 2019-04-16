#ifndef VICTOR9K_H
#define VICTOR9K_H

#define VICTOR9K_SECTOR_RECORD 0xffeab
#define VICTOR9K_DATA_RECORD   0xffea4

#define VICTOR9K_SECTOR_LENGTH 512

class Sector;
class Fluxmap;

class Victor9kDecoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~Victor9kDecoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack, unsigned physicalSide);
    int recordMatcher(uint64_t fifo) const;

    nanoseconds_t guessClockImpl(Fluxmap& fluxmap, unsigned physicalTrack, unsigned physicalSide) const;
};

#endif
