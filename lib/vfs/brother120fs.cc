#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include <fmt/format.h>

/* Number of sectors on a 120kB disk. */
static constexpr int SECTOR_COUNT = 468;

/* Start sector for the FAT (after the directory) */
static constexpr int FAT_START_SECTOR = 8;

/* Start sector for data (after the FAT) */
static constexpr int DATA_START_SECTOR = 14;

/* Size of a sector */
static constexpr int SECTOR_SIZE = 256;

/* Number of dirents in a directory. */
static constexpr int DIRECTORY_SIZE = 128;

/* Number of sectors in the FAT. */
static constexpr int FAT_SECTORS = 4;

/* Number of sectors in a directory. */
static constexpr int DIRECTORY_SECTORS = 8;

class Brother120Dirent : public Dirent
{
public:
    Brother120Dirent(int inode, const Bytes& bytes)
    {
        ByteReader br(bytes);
        filename = br.read(8);
        filename = filename.substr(0, filename.find(' '));

        this->inode = inode;
        brother_type = br.read_8();
        start_sector = br.read_be16();
        sector_length = br.read_8();
        length = sector_length * SECTOR_SIZE;
        file_type = TYPE_FILE;
        mode = "";
    }

public:
    int inode;
    int brother_type;
    uint32_t start_sector;
    uint32_t sector_length;
};

class BrotherDirectory
{
public:
    BrotherDirectory(Filesystem* fs)
    {
		/* Read directory. */

        int inode = 0;
        for (int block = 0; block < DIRECTORY_SECTORS; block++)
        {
            auto bytes = fs->getLogicalSector(block);
            for (int d = 0; d < SECTOR_SIZE / 16; d++, inode++)
            {
                Bytes buffer = bytes.slice(d * 16, 16);
                if (buffer[0] == 0xf0)
                    continue;

                auto de = std::make_unique<Brother120Dirent>(inode, buffer);
                usedSectors += de->sector_length;
                //dirents.push_back(std::move(de));
            }
        }

		/* Read FAT. */

        Bytes bytes = fs->getLogicalSector(FAT_START_SECTOR, FAT_SECTORS);
        ByteReader br(bytes);

        fat.push_back(0xffff);
        for (int sector = 1; sector != SECTOR_COUNT; sector++)
            fat.push_back(br.read_be16());
    }

    std::unique_ptr<Brother120Dirent> findFile(const Path& path)
    {
        if (path.size() != 1)
            throw BadPathException();

        for (auto& dirent : dirents)
        {
            if (dirent->filename == path[0])
                return std::move(dirent);
        }

        throw FileNotFoundException();
    }

public:
	std::vector<uint16_t> fat;
    std::vector<std::unique_ptr<Brother120Dirent>> dirents;
    uint32_t usedSectors = 0;
};

class Brother120Filesystem : public Filesystem
{
public:
    Brother120Filesystem(const Brother120FsProto& config,
        std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    std::map<std::string, std::string> getMetadata()
    {
        BrotherDirectory dir(this);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = fmt::format("{}", getLogicalSectorCount());
        attributes[USED_BLOCKS] = fmt::format("{}", dir.usedSectors);
        attributes[BLOCK_SIZE] = fmt::format("{}", SECTOR_SIZE);
        return attributes;
    }

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
        if (!path.empty())
            throw FileNotFoundException();

		BrotherDirectory dir(this);

        std::vector<std::unique_ptr<Dirent>> result;
        for (auto& dirent : dir.dirents)
            result.push_back(std::move(dirent));

        return result;
    }

    Bytes getFile(const Path& path)
    {
		BrotherDirectory dir(this);
        auto dirent = dir.findFile(path);
        int sector = dirent->start_sector;

        Bytes data;
        ByteWriter bw(data);
        while ((sector != 0) && (sector != 0xffff))
        {
            bw += getLogicalSector(sector - 1);
            sector = dir.fat.at(sector);
        }

        return data;
    }

    std::map<std::string, std::string> getMetadata(const Path& path)
    {
        std::map<std::string, std::string> attributes;

		BrotherDirectory dir(this);
        auto dirent = dir.findFile(path);
        attributes[FILENAME] = dirent->filename;
        attributes[LENGTH] = fmt::format("{}", dirent->length);
        attributes[FILE_TYPE] = "file";
        attributes[MODE] = dirent->mode;
        attributes["brother120.inode"] = fmt::format("{}", dirent->inode);
        attributes["brother120.start_sector"] =
            fmt::format("{}", dirent->start_sector);
        attributes["brother120.type"] = fmt::format("{}", dirent->brother_type);

        return attributes;
    }

private:
    const Brother120FsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createBrother120Filesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Brother120Filesystem>(config.brother120(), sectors);
}
