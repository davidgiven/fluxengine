#ifndef AESLANIER_H
#define AESLANIER_H

#define AESLANIER_RECORD_SEPARATOR 0x55555122
#define AESLANIER_SECTOR_LENGTH    256
#define AESLANIER_RECORD_SIZE      (AESLANIER_SECTOR_LENGTH + 5)

class Sector;
class Fluxmap;

class AesLanierDecoder : public AbstractStatefulDecoder
{
public:
    virtual ~AesLanierDecoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track);
    void decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector);
};

#endif
