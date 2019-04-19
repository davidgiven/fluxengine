#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

class Sector;
class Fluxmap;

class ZilogMczDecoder : public AbstractStatefulDecoder
{
public:
    virtual ~ZilogMczDecoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track);
    void decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector);
};

#endif


