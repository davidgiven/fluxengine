#ifndef ENCODERS_H
#define ENCODERS_H

class Fluxmap;
class SectorSet;

class AbstractEncoder
{
public:
    virtual ~AbstractEncoder() {}

public:
    virtual std::unique_ptr<Fluxmap> encode(
        int physicalTrack, int physicalSide, const SectorSet& allSectors) = 0;
};

#endif

