#ifndef FLUXSINK_H
#define FLUXSINK_H

class Fluxmap;
class DataSpec;

class FluxSink
{
public:
    virtual ~FluxSink() {}

private:
    static std::unique_ptr<FluxSink> createSqliteFluxSink(const std::string& filename);
    static std::unique_ptr<FluxSink> createHardwareFluxSink(unsigned drive);

public:
    static std::unique_ptr<FluxSink> create(const DataSpec& spec);

public:
    virtual void writeFlux(int track, int side, Fluxmap& fluxmap) = 0;
};

extern void setHardwareFluxSinkDensity(bool high_density);

#endif

