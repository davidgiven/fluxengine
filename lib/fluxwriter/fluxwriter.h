#ifndef FLUXWRITER_H
#define FLUXWRITER_H

class Fluxmap;
class DataSpec;

class FluxWriter
{
public:
    virtual ~FluxWriter() {}

private:
    static std::unique_ptr<FluxWriter> createSqliteFluxWriter(const std::string& filename);
    static std::unique_ptr<FluxWriter> createHardwareFluxWriter(unsigned drive);

public:
    static std::unique_ptr<FluxWriter> create(const DataSpec& spec);

public:
    virtual void writeFlux(int track, int side, Fluxmap& fluxmap) = 0;
};

extern void setHardwareFluxWriterDensity(bool high_density);

#endif

