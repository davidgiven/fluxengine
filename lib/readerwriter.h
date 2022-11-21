#ifndef WRITER_H
#define WRITER_H

class Decoder;
class Encoder;
class DiskFlux;
class FluxSink;
class FluxSource;
class FluxSourceIteratorHolder;
class Fluxmap;
class Image;
class ImageReader;
class ImageWriter;
class TrackInfo;
class TrackFlux;

extern void measureDiskRotation(
    nanoseconds_t& oneRevolution, nanoseconds_t& hardSectorThreshold);

extern void writeTracks(FluxSink& fluxSink,
    const std::function<std::unique_ptr<const Fluxmap>(
        std::shared_ptr<const TrackInfo>& layout)> producer,
    std::vector<std::shared_ptr<const TrackInfo>>& locations);

extern void writeTracksAndVerify(FluxSink& fluxSink,
    Encoder& encoder,
    FluxSource& fluxSource,
    Decoder& decoder,
    const Image& image,
    std::vector<std::shared_ptr<const TrackInfo>>& locations);

extern void fillBitmapTo(std::vector<bool>& bitmap,
    unsigned& cursor,
    unsigned terminateAt,
    const std::vector<bool>& pattern);

extern void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource,
    std::vector<std::shared_ptr<const TrackInfo>>& locations);

extern void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder = nullptr,
    FluxSource* fluxSource = nullptr);

extern void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink);

extern std::shared_ptr<TrackFlux> readAndDecodeTrack(FluxSource& fluxSource,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& layout);

extern std::shared_ptr<const DiskFlux> readDiskCommand(
    FluxSource& fluxsource, Decoder& decoder);
extern void readDiskCommand(
    FluxSource& source, Decoder& decoder, ImageWriter& writer);
extern void rawReadDiskCommand(FluxSource& source, FluxSink& sink);

#endif
