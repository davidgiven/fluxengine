#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

class Fluxmap;
class DataSpec;

class FluxSource
{
public:
    virtual ~FluxSource() {}

private:
    static std::unique_ptr<FluxSource> createSqliteFluxSource(const std::string& filename);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(unsigned drive);
    static std::unique_ptr<FluxSource> createStreamFluxSource(const std::string& path);

public:
    static std::unique_ptr<FluxSource> create(const DataSpec& spec);

public:
    virtual std::unique_ptr<Fluxmap> readFlux(int track, int side) = 0;
    virtual void recalibrate() {}
    virtual bool retryable() { return false; }
};

extern void setHardwareFluxSourceRevolutions(int revolutions);
extern void setHardwareFluxSourceDensity(bool high_density);

#endif

