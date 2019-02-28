#ifndef MACINTOSH_H
#define MACINTOSH_H

#define MAC_SECTOR_RECORD   0xd5aa96
#define MAC_DATA_RECORD     0xd5aaad

#define MAC_SECTOR_LENGTH   524 /* yes, really */
#define MAC_ENCODED_SECTOR_LENGTH 700

class Sector;
class Fluxmap;

class MacintoshDecoder : public AbstractDecoder
{
public:
    virtual ~MacintoshDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif

