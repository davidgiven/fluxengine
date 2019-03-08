#ifndef APPLE2_H
#define APPLE2_H

#define APPLE2_SECTOR_RECORD   0xd5aa96
#define APPLE2_DATA_RECORD     0xd5aaad

#define APPLE2_SECTOR_LENGTH   256
#define APPLE2_ENCODED_SECTOR_LENGTH 342

class Sector;
class Fluxmap;

class Apple2Decoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~Apple2Decoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif

