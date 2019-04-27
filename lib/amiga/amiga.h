#ifndef AMIGA_H
#define AMIGA_H

#define AMIGA_SECTOR_RECORD 0xaaaa44894489LL

#define AMIGA_RECORD_SIZE 0x21f

class Sector;
class Fluxmap;

class AmigaDecoder : public AbstractStatefulDecoder
{
public:
    virtual ~AmigaDecoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track);
    void decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector);
};

#endif
