#ifndef ENCODERS_H
#define ENCODERS_H

class Fluxmap;
class SectorSet;
class Encoder;

class AbstractEncoder
{
public:
    virtual ~AbstractEncoder() {}

	static std::unique_ptr<AbstractEncoder> create(const Encoder& config);

public:
    virtual std::unique_ptr<Fluxmap> encode(
        int physicalTrack, int physicalSide, const SectorSet& allSectors) = 0;
};

#endif

