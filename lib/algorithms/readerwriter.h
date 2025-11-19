#ifndef WRITER_H
#define WRITER_H

#include "lib/data/locations.h"

class Disk;
class Track;
class Decoder;
class DiskLayout;
class Encoder;
class FluxSinkFactory;
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
    std::vector<std::shared_ptr<const Track>> tracks;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

struct DiskReadLogMessage
{
    std::shared_ptr<const Disk> disk;
};

struct BeginReadOperationLogMessage
{
    unsigned track;
    unsigned head;
};

struct EndReadOperationLogMessage
{
    std::shared_ptr<const Track> trackDataFlux;
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

extern void writeTracks(const DiskLayout& diskLayout,
    FluxSinkFactory& fluxSinkFactory,
    const std::function<std::unique_ptr<const Fluxmap>(
        const LogicalTrackLayout& ltl)> producer,
    const std::vector<CylinderHead>& locations);

extern void writeTracksAndVerify(const DiskLayout& diskLayout,
    FluxSinkFactory& fluxSinkFactory,
    Encoder& encoder,
    FluxSource& fluxSource,
    Decoder& decoder,
    const Image& image,
    const std::vector<CylinderHead>& locations);

extern void writeDiskCommand(const DiskLayout& diskLayout,
    const Image& image,
    Encoder& encoder,
    FluxSinkFactory& fluxSinkFactory,
    Decoder* decoder,
    FluxSource* fluxSource,
    const std::vector<CylinderHead>& locations);

extern void writeDiskCommand(const DiskLayout& diskLayout,
    const Image& image,
    Encoder& encoder,
    FluxSinkFactory& fluxSinkFactory,
    Decoder* decoder = nullptr,
    FluxSource* fluxSource = nullptr);

extern void writeRawDiskCommand(const DiskLayout& diskLayout,
    FluxSource& fluxSource,
    FluxSinkFactory& fluxSinkFactory);

/* Reads a single group of tracks. tracks and combinedSectors are populated.
 * tracks may contain preexisting data which will be taken into account. */

extern void readAndDecodeTrack(const DiskLayout& diskLayout,
    FluxSource& fluxSource,
    Decoder& decoder,
    const std::shared_ptr<const LogicalTrackLayout>& ltl,
    std::vector<std::shared_ptr<const Track>>& tracks,
    std::vector<std::shared_ptr<const Sector>>& combinedSectors);

extern void readDiskCommand(const DiskLayout& diskLayout,
    FluxSource& fluxSource,
    Decoder& decoder,
    Disk& disk);
extern void readDiskCommand(const DiskLayout& diskLayout,
    FluxSource& source,
    Decoder& decoder,
    ImageWriter& writer);

#endif
