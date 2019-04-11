#ifndef FB100_H
#define FB100_H

#define FB100_SECTOR_SIZE 0x500

class Sector;
class Fluxmap;

class Fb100Decoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~Fb100Decoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif

