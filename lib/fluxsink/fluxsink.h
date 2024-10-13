#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "lib/config/flags.h"
#include <ostream>

class Fluxmap;
class FluxSinkProto;
class HardwareFluxSinkProto;
class AuFluxSinkProto;
class A2RFluxSinkProto;
class VcdFluxSinkProto;
class ScpFluxSinkProto;
class Fl2FluxSinkProto;
class Config;

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
    /* Writes a fluxmap to a track and side. */

    virtual void writeFlux(int track, int side, const Fluxmap& fluxmap) = 0;

    /* Returns whether this is writing to real hardware or not. */

    virtual bool isHardware() const
    {
        return false;
    }

    virtual operator std::string() const = 0;
};

inline std::ostream& operator<<(std::ostream& stream, FluxSink& flushSink)
{
    stream << (std::string)flushSink;
    return stream;
}

#endif
