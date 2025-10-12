#ifndef WRITER_H
#define WRITER_H

#include "lib/data/locations.h"

class DecodedDisk;
class DecodedTrack;
class Decoder;
class DiskLayout;
class Encoder;
class FluxSink;
class FluxSource;
class FluxSourceIteratorHolder;
class Fluxmap;
class Image;
class ImageReader;
class ImageWriter;
class LogicalTrackLayout;
class PhysicalTrackLayout;
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
    std::vector<std::shared_ptr<const DecodedTrack>> tracks;
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

extern void writeTracks(const DiskLayout& diskLayout,
    FluxSink& fluxSink,
    const std::function<std::unique_ptr<const Fluxmap>(
        const LogicalTrackLayout& ltl)> producer,
    const std::vector<CylinderHead>& locations);

extern void writeTracksAndVerify(const DiskLayout& diskLayout,
    FluxSink& fluxSink,
    Encoder& encoder,
    FluxSource& fluxSource,
    Decoder& decoder,
    const Image& image,
    const std::vector<CylinderHead>& locations);

extern void writeDiskCommand(const DiskLayout& diskLayout,
    const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource,
    const std::vector<CylinderHead>& locations);

extern void writeDiskCommand(const DiskLayout& diskLayout,
    const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder = nullptr,
    FluxSource* fluxSource = nullptr);

extern void writeRawDiskCommand(
    const DiskLayout& diskLayout, FluxSource& fluxSource, FluxSink& fluxSink);

struct TracksAndSectors
{
    std::vector<std::shared_ptr<const DecodedTrack>> tracks;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

extern TracksAndSectors readAndDecodeTrack(const DiskLayout& diskLayout,
    FluxSource& fluxSource,
    Decoder& decoder,
    const std::shared_ptr<const LogicalTrackLayout>& ltl);

extern void readDiskCommand(const DiskLayout& diskLayout,
    FluxSource& fluxsource,
    Decoder& decoder,
    DecodedDisk& diskflux);
extern void readDiskCommand(const DiskLayout& diskLayout,
    FluxSource& source,
    Decoder& decoder,
    ImageWriter& writer);

#endif
