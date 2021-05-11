#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "flags.h"

extern FlagGroup hardwareFluxSourceFlags;

class Fluxmap;
class FluxSpec;
class Config_InputDisk;

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createSqliteFluxSource(const std::string& filename);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(unsigned drive);
    static std::unique_ptr<FluxSource> createStreamFluxSource(const std::string& path);

public:
    static std::unique_ptr<FluxSource> create(const FluxSpec& spec);
    static std::unique_ptr<FluxSource> create(const Config_InputDisk& spec);

public:
    virtual std::unique_ptr<Fluxmap> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
    virtual bool retryable() { return false; }
};

extern void setHardwareFluxSourceRevolutions(double revolutions);
extern void setHardwareFluxSourceSynced(bool synced);
extern void setHardwareFluxSourceHardSectorCount(int sectorCount);

#endif

