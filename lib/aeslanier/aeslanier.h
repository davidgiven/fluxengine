#ifndef AESLANIER_H
#define AESLANIER_H

#define AESLANIER_RECORD_SEPARATOR 0x55555122
#define AESLANIER_SECTOR_LENGTH    252

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
