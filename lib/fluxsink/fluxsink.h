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

class FluxSinkFactory
{
public:
    virtual ~FluxSinkFactory() {}

    static std::unique_ptr<FluxSinkFactory> createHardwareFluxSinkFactory(
        const HardwareFluxSinkProto& config);
    static std::unique_ptr<FluxSinkFactory> createAuFluxSinkFactory(
        const AuFluxSinkProto& config);
    static std::unique_ptr<FluxSinkFactory> createA2RFluxSinkFactory(
        const A2RFluxSinkProto& config);
    static std::unique_ptr<FluxSinkFactory> createVcdFluxSinkFactory(
        const VcdFluxSinkProto& config);
    static std::unique_ptr<FluxSinkFactory> createScpFluxSinkFactory(
        const ScpFluxSinkProto& config);
    static std::unique_ptr<FluxSinkFactory> createFl2FluxSinkFactory(
        const Fl2FluxSinkProto& config);

    static std::unique_ptr<FluxSinkFactory> createFl2FluxSinkFactory(
        const std::string& filename);

    static std::unique_ptr<FluxSinkFactory> create(Config& config);
    static std::unique_ptr<FluxSinkFactory> create(const FluxSinkProto& config);

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
    std::ostream& stream, FluxSinkFactory& flushSink)
{
    stream << (std::string)flushSink;
    return stream;
}

#endif
