#ifndef READER_H
#define READER_H

class AbstractDecoder;
class FluxSink;
class FluxSource;
class Fluxmap;
class AssemblingGeometryMapper;
class Track;

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(FluxSource& source, AbstractDecoder& decoder, AssemblingGeometryMapper& geometryMapper);
extern void rawReadDiskCommand(FluxSource& source, FluxSink& sink);

#endif
