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
class TrackDataFlux;
class Sector;

struct BeginSpeedOperationLogMessage
{
};

struct EndSpeedOperationLogMessage
{
    nanoseconds_t rotationalPeriod;
};

struct TrackReadLogMessage
{
    std::shared_ptr<const TrackFlux> track;
};

struct DiskReadLogMessage
{
    std::shared_ptr<const DiskFlux> disk;
};

struct BeginReadOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndReadOperationLogMessage
{
    std::shared_ptr<const TrackDataFlux> trackDataFlux;
    std::set<std::shared_ptr<const Sector>> sectors;
};

struct BeginWriteOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndWriteOperationLogMessage
{
};

struct BeginOperationLogMessage
{
    std::string message;
};

struct EndOperationLogMessage
{
    std::string message;
};

struct OperationProgressLogMessage
{
    unsigned progress;
};

extern void measureDiskRotation();

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
