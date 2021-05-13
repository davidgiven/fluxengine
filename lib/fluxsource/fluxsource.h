#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "flags.h"

extern FlagGroup hardwareFluxSourceFlags;

class Fluxmap;
class FluxSpec;
class Config_InputDisk;
class HardwareInput;
class TestPatternInput;

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createSqliteFluxSource(const std::string& filename);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(const HardwareInput& config);
    static std::unique_ptr<FluxSource> createStreamFluxSource(const std::string& path);
    static std::unique_ptr<FluxSource> createTestPatternFluxSource(const TestPatternInput& config);

public:
    static std::unique_ptr<FluxSource> create(const FluxSpec& spec);
    static std::unique_ptr<FluxSource> create(const Config_InputDisk& spec);

public:
    virtual std::unique_ptr<Fluxmap> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
    virtual bool retryable() { return false; }
};

#endif

