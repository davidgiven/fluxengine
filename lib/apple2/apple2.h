#ifndef APPLE2_H
#define APPLE2_H

#define MAC_SECTOR_RECORD   0xd5aa96
#define MAC_DATA_RECORD     0xd5aaad

#define MAC_SECTOR_LENGTH   524 /* yes, really */

class Sector;
class Fluxmap;

class Apple2Decoder : public AbstractDecoder
{
public:
    virtual ~Apple2Decoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif

