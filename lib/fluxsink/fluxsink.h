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
class DecodedDisk;

class FluxSink
{
public:
    virtual ~FluxSink() {}

    static std::unique_ptr<FluxSink> createHardwareFluxSink(
        const HardwareFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createAuFluxSink(
        const AuFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createA2RFluxSink(
        const A2RFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createVcdFluxSink(
        const VcdFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createScpFluxSink(
        const ScpFluxSinkProto& config);
    static std::unique_ptr<FluxSink> createFl2FluxSink(
        const Fl2FluxSinkProto& config);

    static std::unique_ptr<FluxSink> createFl2FluxSink(
        const std::string& filename);

    static std::unique_ptr<FluxSink> create(Config& config);
    static std::unique_ptr<FluxSink> create(const FluxSinkProto& config);

public:
    class Sink
    {
    public:
        Sink() {}
        virtual ~Sink() {}

    public:
        /* Writes a fluxmap to a track and side. */

        virtual void addFlux(int track, int side, const Fluxmap& fluxmap) = 0;
        void addFlux(const CylinderHead& location, const Fluxmap& fluxmap)
        {
            addFlux(location.cylinder, location.head, fluxmap);
        }
    };

public:
    /* Creates a writer object. */

    virtual std::unique_ptr<Sink> create() = 0;

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

inline std::ostream& operator<<(std::ostream& stream, FluxSink& flushSink)
{
    stream << (std::string)flushSink;
    return stream;
}

#endif
