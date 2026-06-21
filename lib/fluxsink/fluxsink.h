#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "lib/config/flags.h"
#include "lib/data/locations.h"
#include <ostream>
#include <filesystem>

class Fluxmap;
class FluxSinkProto;
class HardwareFluxSinkProto;
class AuFluxSinkProto;
class A2RFluxSinkProto;
class VcdFluxSinkProto;
class ScpFluxSinkProto;
class Fl2FluxSinkProto;
class Config;
class Disk;

class FluxSink
{
public:
    FluxSink() {}
    virtual ~FluxSink() {}

public:
    /* Writes a fluxmap to a track and side. */

    virtual void addFlux(int track, int side, const Fluxmap& fluxmap) = 0;
    void addFlux(const CylinderHead& location, const Fluxmap& fluxmap)
    {
        addFlux(location.cylinder, location.head, fluxmap);
    }
};

class FluxSinkFactory : public gc
{
public:
    virtual ~FluxSinkFactory() {}

    static FluxSinkFactory* createHardwareFluxSinkFactory(
        const HardwareFluxSinkProto& config);
    static FluxSinkFactory* createAuFluxSinkFactory(
        const AuFluxSinkProto& config);
    static FluxSinkFactory* createA2RFluxSinkFactory(
        const A2RFluxSinkProto& config);
    static FluxSinkFactory* createVcdFluxSinkFactory(
        const VcdFluxSinkProto& config);
    static FluxSinkFactory* createScpFluxSinkFactory(
        const ScpFluxSinkProto& config);
    static FluxSinkFactory* createFl2FluxSinkFactory(
        const Fl2FluxSinkProto& config);

    static FluxSinkFactory* createFl2FluxSinkFactory(
        const std::string& filename);

    static FluxSinkFactory* create(Config& config);
    static FluxSinkFactory* create(const FluxSinkProto& config);

public:
    /* Creates a writer object. */

    virtual std::unique_ptr<FluxSink> create() = 0;

    /* Returns whether this is writing to real hardware or not. */

    virtual bool isHardware() const
    {
        return false;
    }

    /* Returns the path (filename or directory) being written to, if there is
     * one. */

    virtual std::optional<std::filesystem::path> getPath() const
    {
        return {};
    }

    virtual operator std::string() const = 0;
};

inline std::ostream& operator<<(
    std::ostream& stream, FluxSinkFactory* flushSink)
{
    stream << (std::string)*flushSink;
    return stream;
}

#endif
