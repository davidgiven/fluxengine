#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "flags.h"

extern FlagGroup hardwareFluxSinkFlags;
extern FlagGroup sqliteFluxSinkFlags;

class Fluxmap;
class FluxSpec;

class FluxSink
{
public:
    virtual ~FluxSink() {}

    static std::unique_ptr<FluxSink> createSqliteFluxSink(const std::string& filename);
    static std::unique_ptr<FluxSink> createHardwareFluxSink(unsigned drive);

    static std::unique_ptr<FluxSink> create(const FluxSpec& spec);

public:
    virtual void writeFlux(int track, int side, Fluxmap& fluxmap) = 0;
};

extern void setHardwareFluxSinkDensity(bool high_density);

#endif

