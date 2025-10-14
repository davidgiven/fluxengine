#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/vfs/vfs.h"
#include "lib/config/proto.h"
#include "lib/config/layout.pb.h"
#include "lib/data/layout.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "lib/data/layout.h"
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

Filesystem::Filesystem(const std::shared_ptr<const DiskLayout>& diskLayout,
    std::shared_ptr<SectorInterface> sectors):
    _diskLayout(diskLayout),
    _blockCount(diskLayout->logicalSectorLocationsInFilesystemOrder.size()),
    _sectors(sectors)
{
}

std::unique_ptr<Filesystem> Filesystem::createFilesystem(
    const FilesystemProto& config,
    const std::shared_ptr<const DiskLayout>& diskLayout,
    std::shared_ptr<SectorInterface> image)
{
    switch (config.type())
    {
        case FilesystemProto::BROTHER120:
            return Filesystem::createBrother120Filesystem(
                config, diskLayout, image);

        case FilesystemProto::ACORNDFS:
            return Filesystem::createAcornDfsFilesystem(
                config, diskLayout, image);

        case FilesystemProto::FATFS:
            return Filesystem::createFatFsFilesystem(config, diskLayout, image);

        case FilesystemProto::CPMFS:
            return Filesystem::createCpmFsFilesystem(config, diskLayout, image);

        case FilesystemProto::AMIGAFFS:
            return Filesystem::createAmigaFfsFilesystem(
                config, diskLayout, image);

        case FilesystemProto::MACHFS:
            return Filesystem::createMacHfsFilesystem(
                config, diskLayout, image);

        case FilesystemProto::CBMFS:
            return Filesystem::createCbmfsFilesystem(config, diskLayout, image);

        case FilesystemProto::PRODOS:
            return Filesystem::createProdosFilesystem(
                config, diskLayout, image);

        case FilesystemProto::APPLEDOS:
            return Filesystem::createAppledosFilesystem(
                config, diskLayout, image);

        case FilesystemProto::SMAKY6:
            return Filesystem::createSmaky6Filesystem(
                config, diskLayout, image);

        case FilesystemProto::PHILE:
            return Filesystem::createPhileFilesystem(config, diskLayout, image);

        case FilesystemProto::LIF:
            return Filesystem::createLifFilesystem(config, diskLayout, image);

        case FilesystemProto::MICRODOS:
            return Filesystem::createMicrodosFilesystem(
                config, diskLayout, image);

        case FilesystemProto::ZDOS:
            return Filesystem::createZDosFilesystem(config, diskLayout, image);

        case FilesystemProto::ROLAND:
            return Filesystem::createRolandFsFilesystem(
                config, diskLayout, image);

        default:
            error("no filesystem configured");
            return std::unique_ptr<Filesystem>();
    }
}

std::unique_ptr<Filesystem> Filesystem::createFilesystemFromConfig()
{
    std::shared_ptr<SectorInterface> sectorInterface;
    auto diskLayout = createDiskLayout(globalConfig());
    if (globalConfig().hasFluxSource() || globalConfig().hasFluxSink())
    {
        std::shared_ptr<FluxSource> fluxSource;
        std::shared_ptr<Decoder> decoder;
        std::shared_ptr<FluxSinkFactory> fluxSinkFactory;
        std::shared_ptr<Encoder> encoder;
        if (globalConfig().hasFluxSource())
        {
            fluxSource = FluxSource::create(globalConfig());
            decoder = Arch::createDecoder(globalConfig());
        }
        if (globalConfig()->flux_sink().type() == FLUXTYPE_DRIVE)
        {
            fluxSinkFactory = FluxSinkFactory::create(globalConfig());
            encoder = Arch::createEncoder(globalConfig());
        }
        sectorInterface = SectorInterface::createFluxSectorInterface(
            diskLayout, fluxSource, fluxSinkFactory, encoder, decoder);
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

        sectorInterface = SectorInterface::createImageSectorInterface(
            diskLayout, reader, writer);
    }

    return createFilesystem(
        globalConfig()->filesystem(), diskLayout, sectorInterface);
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
    if ((number + count) > _blockCount)
        throw BadFilesystemException(fmt::format(
            "invalid filesystem: sector {} is out of bounds ({} maximum)",
            number + count - 1,
            _diskLayout->logicalSectorLocationsInFilesystemOrder.size()));

    Bytes data;
    ByteWriter bw(data);
    for (int i = 0; i < count; i++)
    {
        auto& [cylinder, head, sectorId] =
            _diskLayout->logicalSectorLocationsInFilesystemOrder.at(number + i);
        auto& ltl = _diskLayout->layoutByLogicalLocation.at({cylinder, head});
        bw += _sectors->get(cylinder, head, sectorId)
                  ->data.slice(0, ltl->sectorSize);
    }
    return data;
}

void Filesystem::putLogicalSector(uint32_t number, const Bytes& data)
{
    if (number >= _blockCount)
        throw BadFilesystemException(fmt::format(
            "invalid filesystem: sector {} is out of bounds", number));

    unsigned pos = 0;
    while (pos < data.size())
    {
        const auto& [cylinder, head, sectorId] =
            _diskLayout->logicalSectorLocationsInFilesystemOrder.at(number);
        const auto& ltl =
            _diskLayout->layoutByLogicalLocation.at({cylinder, head});
        const auto& sector = _sectors->put(cylinder, head, sectorId);
        sector->status = Sector::OK;
        sector->data = data.slice(pos, ltl->sectorSize);
        pos += ltl->sectorSize;
        number++;
    }
}

unsigned Filesystem::getOffsetOfSector(
    unsigned track, unsigned side, unsigned sector)
{
    unsigned offset =
        findOrDefault(_diskLayout->sectorOffsetByLogicalSectorLocation,
            {track, side, sector},
            UINT_MAX);
    if (offset == UINT_MAX)
        throw BadFilesystemException();
    return offset;
}

unsigned Filesystem::getLogicalSectorCount()
{
    return _blockCount;
}

unsigned Filesystem::getLogicalSectorSize(unsigned cylinder, unsigned head)
{
    auto& ltl = _diskLayout->layoutByLogicalLocation.at({cylinder, head});
    return ltl->sectorSize;
}

void Filesystem::eraseEverythingOnDisk()
{
    for (int i = 0; i < getLogicalSectorCount(); i++)
        putLogicalSector(i, Bytes(1));
}
