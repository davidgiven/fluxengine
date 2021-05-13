#ifndef FLUXSINK_H
#define FLUXSINK_H

#include "flags.h"
#include <ostream>

extern FlagGroup hardwareFluxSinkFlags;
extern FlagGroup sqliteFluxSinkFlags;

class Fluxmap;
class OutputDiskProto;

class FluxSink
{
public:
    virtual ~FluxSink() {}

    static std::unique_ptr<FluxSink> createSqliteFluxSink(const std::string& filename);
    static std::unique_ptr<FluxSink> createHardwareFluxSink(unsigned drive);

    static std::unique_ptr<FluxSink> create(const OutputDiskProto& config);

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

