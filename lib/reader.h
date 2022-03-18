#ifndef READER_H
#define READER_H

class AbstractDecoder;
class DiskFlux;
class FluxSink;
class FluxSource;
class Fluxmap;
class ImageWriter;
class TrackDataFlux;

extern std::unique_ptr<TrackDataFlux> readAndDecodeTrack(
		FluxSource& source, AbstractDecoder& decoder, unsigned cylinder, unsigned head);

extern std::shared_ptr<const DiskFlux> readDiskCommand(FluxSource& fluxsource, AbstractDecoder& decoder);
extern void readDiskCommand(FluxSource& source, AbstractDecoder& decoder, ImageWriter& writer);
extern void rawReadDiskCommand(FluxSource& source, FluxSink& sink);

#endif
