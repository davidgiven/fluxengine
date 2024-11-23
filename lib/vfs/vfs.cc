#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "vfs.h"
#include "lib/config/proto.h"
#include "lib/config/layout.pb.h"
#include "lib/data/layout.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"
#include "arch/arch.h"

Path::Path(const std::vector<std::string> other):
    std::vector<std::string>(other)
{
}

Path::Path(const std::string& path)
{
    if (path == "")
        return;
    auto p = path;
    if (p[0] == '/')
        p = p.substr(1);

    std::stringstream ss(p);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (item.empty())
            throw BadPathException();
        push_back(item);
    }
}

Path Path::parent() const
{
    Path p;
    if (!empty())
    {
        for (int i = 0; i < (size() - 1); i++)
            p.push_back((*this)[i]);
    }
    return p;
}

Path Path::concat(const std::string& s) const
{
    Path p(*this);
    p.push_back(s);
    return p;
}

std::string Path::to_str(const std::string sep) const
{
    return join(*this, sep);
}

uint32_t Filesystem::capabilities() const
{
    return 0;
}

void Filesystem::create(bool quick, const std::string& volumeName)
{
    throw UnimplementedFilesystemException();
}

FilesystemStatus Filesystem::check()
{
    throw UnimplementedFilesystemException();
}

std::map<std::string, std::string> Filesystem::getMetadata()
{
    throw UnimplementedFilesystemException();
}

void Filesystem::putMetadata(const std::map<std::string, std::string>& metadata)
{
    throw UnimplementedFilesystemException();
}

std::vector<std::shared_ptr<Dirent>> Filesystem::list(const Path& path)
{
    throw UnimplementedFilesystemException();
}

Bytes Filesystem::getFile(const Path& path)
{
    throw UnimplementedFilesystemException();
}

void Filesystem::putFile(const Path& path, const Bytes& data)
{
    throw UnimplementedFilesystemException();
}

std::shared_ptr<Dirent> Filesystem::getDirent(const Path& path)
{
    throw UnimplementedFilesystemException();
}

void Filesystem::putMetadata(
    const Path& path, const std::map<std::string, std::string>& metadata)
{
    throw UnimplementedFilesystemException();
}

void Filesystem::createDirectory(const Path& path)
{
    throw UnimplementedFilesystemException();
}

void Filesystem::deleteFile(const Path& path)
{
    throw UnimplementedFilesystemException();
}

void Filesystem::moveFile(const Path& oldName, const Path& newName)
{
    throw UnimplementedFilesystemException();
}

bool Filesystem::isReadOnly()
{
    return _sectors->isReadOnly();
}

bool Filesystem::needsFlushing()
{
    return _sectors->needsFlushing();
}

void Filesystem::flushChanges()
{
    _sectors->flushChanges();
}

void Filesystem::discardChanges()
{
    _sectors->discardChanges();
}

Filesystem::Filesystem(std::shared_ptr<SectorInterface> sectors):
    _sectors(sectors)
{
    auto& layout = globalConfig()->layout();
    if (!layout.has_tracks() || !layout.has_sides())
        error(
            "FS: filesystem support cannot be used without concrete layout "
            "information");

    unsigned block = 0;
    for (const auto& p :
        Layout::getTrackOrdering(layout.tracks(), layout.sides()))
    {
        int track = p.first;
        int side = p.second;

        auto trackLayout = Layout::getLayoutOfTrack(track, side);
        if (trackLayout->numSectors == 0)
            error(
                "FS: filesystem support cannot be used without concrete "
                "layout information");

        for (int sectorId : trackLayout->filesystemSectorOrder)
            _locations.push_back(std::make_tuple(track, side, sectorId));
    }
}

std::unique_ptr<Filesystem> Filesystem::createFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> image)
{
    switch (config.type())
    {
        case FilesystemProto::BROTHER120:
            return Filesystem::createBrother120Filesystem(config, image);

        case FilesystemProto::ACORNDFS:
            return Filesystem::createAcornDfsFilesystem(config, image);

        case FilesystemProto::FATFS:
            return Filesystem::createFatFsFilesystem(config, image);

        case FilesystemProto::CPMFS:
            return Filesystem::createCpmFsFilesystem(config, image);

        case FilesystemProto::AMIGAFFS:
            return Filesystem::createAmigaFfsFilesystem(config, image);

        case FilesystemProto::MACHFS:
            return Filesystem::createMacHfsFilesystem(config, image);

        case FilesystemProto::CBMFS:
            return Filesystem::createCbmfsFilesystem(config, image);

        case FilesystemProto::PRODOS:
            return Filesystem::createProdosFilesystem(config, image);

        case FilesystemProto::APPLEDOS:
            return Filesystem::createAppledosFilesystem(config, image);

        case FilesystemProto::SMAKY6:
            return Filesystem::createSmaky6Filesystem(config, image);

        case FilesystemProto::PHILE:
            return Filesystem::createPhileFilesystem(config, image);

        case FilesystemProto::LIF:
            return Filesystem::createLifFilesystem(config, image);

        case FilesystemProto::MICRODOS:
            return Filesystem::createMicrodosFilesystem(config, image);

        case FilesystemProto::ZDOS:
            return Filesystem::createZDosFilesystem(config, image);

        case FilesystemProto::ROLAND:
            return Filesystem::createRolandFsFilesystem(config, image);

        default:
            error("no filesystem configured");
            return std::unique_ptr<Filesystem>();
    }
}

std::unique_ptr<Filesystem> Filesystem::createFilesystemFromConfig()
{
    std::shared_ptr<SectorInterface> sectorInterface;
    if (globalConfig().hasFluxSource() || globalConfig().hasFluxSink())
    {
        std::shared_ptr<FluxSource> fluxSource;
        std::shared_ptr<Decoder> decoder;
        std::shared_ptr<FluxSink> fluxSink;
        std::shared_ptr<Encoder> encoder;
        if (globalConfig().hasFluxSource())
        {
            fluxSource = FluxSource::create(globalConfig());
            decoder = Arch::createDecoder(globalConfig());
        }
        if (globalConfig()->flux_sink().type() == FLUXTYPE_DRIVE)
        {
            fluxSink = FluxSink::create(globalConfig());
            encoder = Arch::createEncoder(globalConfig());
        }
        sectorInterface = SectorInterface::createFluxSectorInterface(
            fluxSource, fluxSink, encoder, decoder);
    }
    else
    {
        std::shared_ptr<ImageReader> reader;
        std::shared_ptr<ImageWriter> writer;
        if (globalConfig().hasImageReader() &&
            doesFileExist(globalConfig()->image_reader().filename()))
            reader = ImageReader::create(globalConfig());
        if (globalConfig().hasImageWriter())
            writer = ImageWriter::create(globalConfig());

        sectorInterface =
            SectorInterface::createImageSectorInterface(reader, writer);
    }

    return createFilesystem(globalConfig()->filesystem(), sectorInterface);
}

Bytes Filesystem::getSector(unsigned track, unsigned side, unsigned sector)
{
    auto s = _sectors->get(track, side, sector);
    if (!s)
        throw BadFilesystemException();
    return s->data;
}

Bytes Filesystem::getLogicalSector(uint32_t number, uint32_t count)
{
    if ((number + count) > _locations.size())
        throw BadFilesystemException(fmt::format(
            "invalid filesystem: sector {} is out of bounds ({} maximum)",
            number + count - 1,
            _locations.size()));

    Bytes data;
    ByteWriter bw(data);
    for (int i = 0; i < count; i++)
    {
        auto& it = _locations[number + i];
        int track = std::get<0>(it);
        int side = std::get<1>(it);
        int sector = std::get<2>(it);
        auto trackLayout = Layout::getLayoutOfTrack(track, side);
        bw += _sectors->get(track, side, sector)
                  ->data.slice(0, trackLayout->sectorSize);
    }
    return data;
}

void Filesystem::putLogicalSector(uint32_t number, const Bytes& data)
{
    if (number >= _locations.size())
        throw BadFilesystemException(fmt::format(
            "invalid filesystem: sector {} is out of bounds", number));

    unsigned pos = 0;
    while (pos < data.size())
    {
        auto& it = _locations[number];
        int track = std::get<0>(it);
        int side = std::get<1>(it);
        int sector = std::get<2>(it);
        int sectorSize = Layout::getLayoutOfTrack(track, side)->sectorSize;

        _sectors->put(track, side, sector)->data = data.slice(pos, sectorSize);
        pos += sectorSize;
        number++;
    }
}

unsigned Filesystem::getOffsetOfSector(
    unsigned track, unsigned side, unsigned sector)
{
    location_t key = {track, side, sector};

    for (int i = 0; i < _locations.size(); i++)
    {
        if (_locations[i] >= key)
            return i;
    }

    throw BadFilesystemException();
}

unsigned Filesystem::getLogicalSectorCount()
{
    return _locations.size();
}

unsigned Filesystem::getLogicalSectorSize(unsigned track, unsigned side)
{
    return Layout::getLayoutOfTrack(track, side)->sectorSize;
}

void Filesystem::eraseEverythingOnDisk()
{
    for (int i = 0; i < getLogicalSectorCount(); i++)
        putLogicalSector(i, Bytes(1));
}
