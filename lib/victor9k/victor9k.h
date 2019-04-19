#ifndef VICTOR9K_H
#define VICTOR9K_H

#define VICTOR9K_SECTOR_RECORD 0xfffffeab
#define VICTOR9K_DATA_RECORD   0xfffffea4

#define VICTOR9K_SECTOR_LENGTH 512

class Sector;
class Fluxmap;

class Victor9kDecoder : public AbstractSplitDecoder
{
public:
    virtual ~Victor9kDecoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;
};

#endif
