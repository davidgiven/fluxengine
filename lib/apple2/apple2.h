#ifndef APPLE2_H
#define APPLE2_H

#define APPLE2_SECTOR_RECORD   0xd5aa96
#define APPLE2_DATA_RECORD     0xd5aaad

#define APPLE2_SECTOR_LENGTH   256
#define APPLE2_ENCODED_SECTOR_LENGTH 342

class Sector;
class Fluxmap;

class Apple2Decoder : public AbstractSplitDecoder
{
public:
    virtual ~Apple2Decoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;
};


#endif

