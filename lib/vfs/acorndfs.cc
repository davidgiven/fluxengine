#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"

class AcornDfsFilesystem;

class AcornDfsDirent : public Dirent
{
public:
    AcornDfsDirent(int inode, const Bytes& bytes0, const Bytes& bytes1)
    {
        filename += (char)(bytes0[7] & 0x7f);
        filename += '.';
        for (int j = 0; j < 7; j++)
            filename += bytes0[j] & 0x7f;
        filename = filename.substr(0, filename.find(' '));

        this->inode = inode;
        path = {filename};
        start_sector = ((bytes1[6] & 0x03) << 8) | bytes1[7];
        load_address =
            ((bytes1[6] & 0x0c) << 14) | (bytes1[1] << 8) | bytes1[0];
        exec_address =
            ((bytes1[6] & 0xc0) << 10) | (bytes1[3] << 8) | bytes1[2];
        locked = bytes0[7] & 0x80;
        length = ((bytes1[6] & 0x30) << 12) | (bytes1[5] << 8) | bytes1[4];
        sector_count = (length + 255) / 256;
        file_type = TYPE_FILE;
        mode = locked ? "L" : "";

        attributes[Filesystem::FILENAME] = filename;
        attributes[Filesystem::LENGTH] = std::to_string(length);
        attributes[Filesystem::FILE_TYPE] = "file";
        attributes[Filesystem::MODE] = mode;
        attributes["acorndfs.inode"] = std::to_string(inode);
        attributes["acorndfs.start_sector"] = std::to_string(start_sector);
        attributes["acorndfs.load_address"] =
            fmt::format("0x{:x}", load_address);
        attributes["acorndfs.exec_address"] =
            fmt::format("0x{:x}", exec_address);
        attributes["acorndfs.locked"] = std::to_string(locked);
    }

public:
    int inode;
    uint32_t start_sector;
    uint32_t load_address;
    uint32_t exec_address;
    uint32_t sector_count;
    bool locked;
};

class AcornDfsDirectory
{
public:
    AcornDfsDirectory(Filesystem* fs)
    {
        auto sector0 = fs->getLogicalSector(0);
        auto sector1 = fs->getLogicalSector(1);

        if (sector1[5] & 7)
            throw BadFilesystemException();
        int fileCount = sector1[5] / 8;

        for (int i = 0; i < fileCount; i++)
        {
            auto bytes0 = sector0.slice(i * 8 + 8, 8);
            auto bytes1 = sector1.slice(i * 8 + 8, 8);

            auto de = std::make_shared<AcornDfsDirent>(i, bytes0, bytes1);
            usedSectors += de->sector_count;
            dirents.push_back(de);
        }

        {
            std::stringstream ss;
            for (uint8_t b : sector0.slice(0, 8))
                ss << (char)(b & 0x7f);
            for (uint8_t b : sector1.slice(0, 4))
                ss << (char)(b & 0x7f);
            volumeName = tohex(rightTrimWhitespace(ss.str()));
        }
    }

    std::shared_ptr<AcornDfsDirent> findFile(const Path& path)
    {
        if (path.size() != 1)
            throw BadPathException();

        for (auto& dirent : dirents)
        {
            if (dirent->filename == path[0])
                return dirent;
        }

        throw FileNotFoundException();
    }

public:
    std::string volumeName;
    unsigned usedSectors = 0;
    std::vector<std::shared_ptr<AcornDfsDirent>> dirents;
};

class AcornDfsFilesystem : public Filesystem
{
public:
    AcornDfsFilesystem(
        const AcornDfsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_GETDIRENT;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        AcornDfsDirectory dir(this);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = dir.volumeName;
        attributes[TOTAL_BLOCKS] = std::to_string(getLogicalSectorCount());
        attributes[USED_BLOCKS] = std::to_string(dir.usedSectors);
        attributes[BLOCK_SIZE] = "256";
        return attributes;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        if (!path.empty())
            throw FileNotFoundException();
        AcornDfsDirectory dir(this);

        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& dirent : dir.dirents)
            result.push_back(dirent);

        return result;
    }

    Bytes getFile(const Path& path) override
    {
        AcornDfsDirectory dir(this);
        auto dirent = dir.findFile(path);
        int sectors = (dirent->length + 255) / 256;

        Bytes data;
        ByteWriter bw(data);
        for (int i = 0; i < sectors; i++)
        {
            auto sector = getLogicalSector(dirent->start_sector + i);
            bw.append(sector);
        }

        data.resize(dirent->length);
        return data;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        AcornDfsDirectory dir(this);
        return dir.findFile(path);
    }

private:
private:
    const AcornDfsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createAcornDfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<AcornDfsFilesystem>(config.acorndfs(), sectors);
}
