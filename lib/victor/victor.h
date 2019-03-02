#ifndef VICTOR_H
#define VICTOR_H

#define VICTOR_SECTOR_RECORD 0xffeab
#define VICTOR_DATA_RECORD   0xffea4

class Sector;
class Fluxmap;

class VictorDecoder : public AbstractDecoder
{
public:
    virtual ~VictorDecoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif
