#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "flags.h"
#include <ostream>

class Fluxmap;
class FluxSinkProto;
class HardwareFluxSinkProto;

class FluxSink
{
public:
    virtual ~FluxSink() {}

    static std::unique_ptr<FluxSink> createSqliteFluxSink(const std::string& filename);
    static std::unique_ptr<FluxSink> createHardwareFluxSink(const HardwareFluxSinkProto& config);

    static std::unique_ptr<FluxSink> create(const FluxSinkProto& config);

public:
    virtual void writeFlux(int track, int side, Fluxmap& fluxmap) = 0;

	virtual operator std::string () const = 0;
};

inline std::ostream& operator << (std::ostream& stream, FluxSink& flushSink)
{
	stream << (std::string)flushSink;
	return stream;
}

#endif

