#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "flags.h"

class CwfFluxSourceProto;
class EraseFluxSourceProto;
class Fl2FluxSourceProto;
class FluxSourceProto;
class FluxSpec;
class Fluxmap;
class HardwareFluxSourceProto;
class KryofluxFluxSourceProto;
class ScpFluxSourceProto;
class TestPatternFluxSourceProto;

class FluxSourceIterator
{
public:
	virtual ~FluxSourceIterator() {}

	virtual bool hasNext() const = 0;
	virtual std::unique_ptr<const Fluxmap> next() = 0;
};

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createCwfFluxSource(const CwfFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createEraseFluxSource(const EraseFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createFl2FluxSource(const Fl2FluxSourceProto& config);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(const HardwareFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createKryofluxFluxSource(const KryofluxFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createScpFluxSource(const ScpFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createTestPatternFluxSource(const TestPatternFluxSourceProto& config);

public:
    static std::unique_ptr<FluxSource> create(const FluxSourceProto& spec);
	static void updateConfigForFilename(FluxSourceProto* proto, const std::string& filename);

public:
    virtual std::unique_ptr<FluxSourceIterator> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
	virtual bool isHardware() { return false; }
};

class TrivialFluxSource : public FluxSource
{
public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int side);
	virtual std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) = 0;
};

#endif

