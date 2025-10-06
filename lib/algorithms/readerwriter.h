#ifndef WRITER_H
#define WRITER_H

#include "lib/data/locations.h"

class Decoder;
class Encoder;
class DecodedDisk;
class FluxSink;
class FluxSource;
class FluxSourceIteratorHolder;
class Fluxmap;
class Image;
class ImageReader;
class ImageWriter;
class TrackInfo;
class DecodedTrack;
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
    std::vector<std::shared_ptr<const DecodedTrack>> fluxes;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

struct DiskReadLogMessage
{
    std::shared_ptr<const DecodedDisk> disk;
};

struct BeginReadOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndReadOperationLogMessage
{
    std::shared_ptr<const DecodedTrack> trackDataFlux;
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
    const std::vector<CylinderHead>& locations);

extern void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder = nullptr,
    FluxSource* fluxSource = nullptr);

extern void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink);

struct FluxAndSectors
{
    std::vector<std::shared_ptr<const DecodedTrack>> fluxes;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

extern FluxAndSectors readAndDecodeTrack(FluxSource& fluxSource,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& layout);

extern void readDiskCommand(
    FluxSource& fluxsource, Decoder& decoder, DecodedDisk& diskflux);
extern void readDiskCommand(
    FluxSource& source, Decoder& decoder, ImageWriter& writer);

#endif
