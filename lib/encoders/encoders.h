#ifndef ENCODERS_H
#define ENCODERS_H

class FluxSource;
class Fluxmap;
class SectorSet;
class EncoderProto;
class DisassemblingGeometryMapper;

class AbstractEncoder
{
public:
    virtual ~AbstractEncoder() {}

	static std::unique_ptr<AbstractEncoder> create(const EncoderProto& config, const DisassemblingGeometryMapper& mapper);

public:
    virtual std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide) = 0;
};

#endif

