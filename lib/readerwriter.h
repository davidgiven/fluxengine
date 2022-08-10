#ifndef WRITER_H
#define WRITER_H

class AbstractDecoder;
class AbstractEncoder;
class DiskFlux;
class FluxSink;
class FluxSource;
class Fluxmap;
class Image;
class ImageReader;
class ImageWriter;
class Location;
class TrackDataFlux;

extern void writeTracks(FluxSink& fluxSink,
    const std::function<std::unique_ptr<const Fluxmap>(const Location& location)>
        producer);

extern void fillBitmapTo(std::vector<bool>& bitmap,
    unsigned& cursor,
    unsigned terminateAt,
    const std::vector<bool>& pattern);

extern void writeDiskCommand(std::shared_ptr<const Image> image,
    AbstractEncoder& encoder,
    FluxSink& fluxSink,
    AbstractDecoder* decoder = nullptr,
    FluxSource* fluxSource = nullptr);

extern void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink);

extern std::unique_ptr<TrackDataFlux> readAndDecodeTrack(
		FluxSource& source, AbstractDecoder& decoder, unsigned track, unsigned head);

extern std::shared_ptr<const DiskFlux> readDiskCommand(FluxSource& fluxsource, AbstractDecoder& decoder);
extern void readDiskCommand(FluxSource& source, AbstractDecoder& decoder, ImageWriter& writer, FluxSink* solvedFlux);
extern void rawReadDiskCommand(FluxSource& source, FluxSink& sink);

#endif
