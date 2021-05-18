#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "flags.h"

class CwfFluxSourceProto;
class EraseFluxSourceProto;
class FluxSourceProto;
class FluxSpec;
class Fluxmap;
class HardwareFluxSourceProto;
class KryofluxFluxSourceProto;
class ScpFluxSourceProto;
class TestPatternFluxSourceProto;

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createCwfFluxSource(const CwfFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createEraseFluxSource(const EraseFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(const HardwareFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createKryofluxFluxSource(const KryofluxFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createScpFluxSource(const ScpFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createSqliteFluxSource(const std::string& filename);
    static std::unique_ptr<FluxSource> createTestPatternFluxSource(const TestPatternFluxSourceProto& config);

public:
    static std::unique_ptr<FluxSource> create(const FluxSourceProto& spec);
	static void updateConfigForFilename(const std::string& filename);

public:
    virtual std::unique_ptr<Fluxmap> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
    virtual bool retryable() { return false; }
};

#endif

