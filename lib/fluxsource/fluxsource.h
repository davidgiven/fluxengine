#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "flags.h"

extern FlagGroup hardwareFluxSourceFlags;

class Fluxmap;
class FluxSpec;
class InputDiskProto;
class HardwareInputProto;
class TestPatternInputProto;

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createSqliteFluxSource(const std::string& filename);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(const HardwareInputProto& config);
    static std::unique_ptr<FluxSource> createStreamFluxSource(const std::string& path);
    static std::unique_ptr<FluxSource> createTestPatternFluxSource(const TestPatternInputProto& config);

public:
    static std::unique_ptr<FluxSource> create(const InputDiskProto& spec);

public:
    virtual std::unique_ptr<Fluxmap> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
    virtual bool retryable() { return false; }
};

#endif

