#ifndef C64_H
#define C64_H

#define C64_RECORD_SEPARATOR 0xfff5
#define C64_SECTOR_LENGTH    256

class Sector;
class Fluxmap;

class Commodore64Decoder : public AbstractSplitDecoder
{
public:
    virtual ~Commodore64Decoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;
};

#endif
