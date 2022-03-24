#ifndef WRITER_H
#define WRITER_H

class AbstractDecoder;
class AbstractEncoder;
class FluxSink;
class FluxSource;
class Fluxmap;
class Image;
class ImageReader;
class Location;

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

#endif
