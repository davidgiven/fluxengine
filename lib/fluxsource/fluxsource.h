#ifndef FLUXSOURCE_H
#define FLUXSOURCE_H

#include "lib/config/flags.h"
#include "lib/config/config.pb.h"

class A2rFluxSourceProto;
class CwfFluxSourceProto;
class DiskFlux;
class EraseFluxSourceProto;
class Fl2FluxSourceProto;
class FluxSourceProto;
class FluxSpec;
class Fluxmap;
class HardwareFluxSourceProto;
class KryofluxFluxSourceProto;
class ScpFluxSourceProto;
class TestPatternFluxSourceProto;
class FlxFluxSourceProto;
class Config;

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
    static std::unique_ptr<FluxSource> createA2rFluxSource(
        const A2rFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createCwfFluxSource(
        const CwfFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createDmkFluxSource(
        const DmkFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createEraseFluxSource(
        const EraseFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createFl2FluxSource(
        const Fl2FluxSourceProto& config);
    static std::unique_ptr<FluxSource> createFlxFluxSource(
        const FlxFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createHardwareFluxSource(
        const HardwareFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createKryofluxFluxSource(
        const KryofluxFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createScpFluxSource(
        const ScpFluxSourceProto& config);
    static std::unique_ptr<FluxSource> createTestPatternFluxSource(
        const TestPatternFluxSourceProto& config);

public:
    static std::unique_ptr<FluxSource> createMemoryFluxSource(
        const DiskFlux& flux);

    static std::unique_ptr<FluxSource> create(Config& config);
    static std::unique_ptr<FluxSource> create(const FluxSourceProto& spec);

public:
    /* Returns any configuration this flux source might be carrying (e.g. tpi
     * of the drive which made the capture). */

    const ConfigProto& getExtraConfig() const
    {
        return _extraConfig;
    }

    /* Read flux from a given track and side. */

    virtual std::unique_ptr<FluxSourceIterator> readFlux(
        int track, int side) = 0;

    /* Recalibrates; seeks to track 0 and ensures the head is in the right
     * place. */

    virtual void recalibrate() {}

    /* Seeks to a given track (without recalibrating). */

    virtual void seek(int track) {}

    /* Is this real hardware? If so, then flux can be read indefinitely (among
     * other things). */

    virtual bool isHardware()
    {
        return false;
    }

protected:
    ConfigProto _extraConfig;
};

class EmptyFluxSourceIterator : public FluxSourceIterator
{
    bool hasNext() const override
    {
        return false;
    }

    std::unique_ptr<const Fluxmap> next() override
    {
        error("no flux to read");
    }
};

class TrivialFluxSource : public FluxSource
{
public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int side) override;
    virtual std::unique_ptr<const Fluxmap> readSingleFlux(
        int track, int side) = 0;
};

#endif
