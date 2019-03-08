#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

#define ZILOGMCZ_RECORD_SEPARATOR 0xffff5d75

class Sector;
class Fluxmap;

class ZilogMczDecoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~ZilogMczDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
    RawRecordVector extractRecords(std::vector<bool> bits) const;
};

#endif


