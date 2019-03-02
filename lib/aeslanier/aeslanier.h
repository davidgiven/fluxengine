#ifndef AESLANIER_H
#define AESLANIER_H

class Sector;
class Fluxmap;

class AesLanierDecoder : public AbstractDecoder
{
public:
    virtual ~AesLanierDecoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack);
	nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    int recordMatcher(uint64_t fifo) const;
};

#endif
