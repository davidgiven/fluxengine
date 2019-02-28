#ifndef VICTOR_H
#define VICTOR_H

#define VICTOR_RECORD_SEPARATOR 0xffffea

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
