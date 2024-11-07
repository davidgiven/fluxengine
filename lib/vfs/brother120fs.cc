#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"

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
    Brother120Dirent(const Bytes& bytes)
    {
        ByteReader br(bytes);
        filename = br.read(8);

        for (int i = 0; filename.size(); i++)
        {
            if (filename[i] == ' ')
            {
                filename = filename.substr(0, i);
                break;
            }
            if ((filename[i] < 32) || (filename[i] > 126))
                throw BadFilesystemException();
        }

        path = {filename};

        brotherType = br.read_8();
        startSector = br.read_be16();
        sectorLength = br.read_8();
        length = sectorLength * SECTOR_SIZE;
        file_type = TYPE_FILE;
        mode = "";

        populateAttributes();
    }

    Brother120Dirent(const std::string& filename,
        int brotherType,
        uint16_t startSector,
        uint8_t sectorLength):
        brotherType(brotherType),
        startSector(startSector),
        sectorLength(sectorLength)
    {
        this->filename = filename;
        path = {filename};
        length = sectorLength * SECTOR_SIZE;
        file_type = TYPE_FILE;
        mode = "";

        populateAttributes();
    }

private:
    void populateAttributes()
    {
        attributes[Filesystem::FILENAME] = filename;
        attributes[Filesystem::LENGTH] = std::to_string(length);
        attributes[Filesystem::FILE_TYPE] = "file";
        attributes[Filesystem::MODE] = mode;
        attributes["brother120.start_sector"] = std::to_string(startSector);
        attributes["brother120.type"] = std::to_string(brotherType);
    }

public:
    int brotherType;
    uint32_t startSector;
    uint32_t sectorLength;
};

class BrotherDirectory
{
public:
    BrotherDirectory(Filesystem* fs)
    {
        /* Read directory. */

        for (int block = 0; block < DIRECTORY_SECTORS; block++)
        {
            auto bytes = fs->getLogicalSector(block);
            for (int d = 0; d < SECTOR_SIZE / 16; d++)
            {
                Bytes buffer = bytes.slice(d * 16, 16);
                if (buffer[0] & 0x80)
                    continue;

                auto de = std::make_shared<Brother120Dirent>(buffer);
                usedSectors += de->sectorLength;
                dirents.push_back(de);
            }
        }

        /* Read FAT. */

        Bytes bytes = fs->getLogicalSector(FAT_START_SECTOR, FAT_SECTORS);
        ByteReader br(bytes);

        fat.push_back(0xffff);
        for (int sector = 1; sector != SECTOR_COUNT; sector++)
            fat.push_back(br.read_be16());
    }

    void write(Filesystem* fs)
    {
        Bytes bytes;
        ByteWriter bw(bytes);

        /* Write the directory. */

        if (dirents.size() > DIRECTORY_SIZE)
            throw DiskFullException("directory full");
        int i = 0;
        for (const auto& de : dirents)
        {
            bw.append(Bytes(de->filename + "        ").slice(0, 8));
            bw.write_8(de->brotherType);
            bw.write_be16(de->startSector);
            bw.write_8(de->sectorLength);
            bw.write_le32(0);
            i++;
        }
        while (i < DIRECTORY_SIZE)
        {
            bw.write_8(0xf0);
            bw.append(Bytes(15));
            i++;
        }

        /* Write the FAT. */

        for (int i = 1; i != SECTOR_COUNT; i++)
            bw.write_be16(fat[i]);

        fs->putLogicalSector(0, bytes);
    }

    std::shared_ptr<Brother120Dirent> findFile(const Path& path)
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

    void deleteFile(const Path& path)
    {
        if (path.size() != 1)
            throw BadPathException();

        for (auto it = dirents.begin(); it != dirents.end(); it++)
        {
            auto& dirent = *it;
            if (dirent->filename == path[0])
            {
                int sector = dirent->startSector;

                while ((sector != 0) && (sector != 0xffff))
                {
                    int nextSector = fat.at(sector);
                    freeSector(sector);
                    sector = nextSector;
                }

                dirents.erase(it);
                return;
            }
        }

        throw FileNotFoundException();
    }

    uint16_t allocateSector()
    {
        for (int i = 0; i < fat.size(); i++)
            if (fat[i] == 0)
            {
                fat[i] = 0xffff;
                usedSectors++;
                return i;
            }
        throw DiskFullException();
    }

    void freeSector(uint16_t i)
    {
        if (fat[i])
        {
            fat[i] = 0;
            usedSectors--;
        }
    }

public:
    std::vector<uint16_t> fat;
    std::vector<std::shared_ptr<Brother120Dirent>> dirents;
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

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_PUTFILE | OP_GETDIRENT |
               OP_DELETE;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        BrotherDirectory dir(this);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(getLogicalSectorCount());
        attributes[USED_BLOCKS] = std::to_string(dir.usedSectors);
        attributes[BLOCK_SIZE] = std::to_string(SECTOR_SIZE);
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

        BrotherDirectory dir(this);

        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& dirent : dir.dirents)
            result.push_back(dirent);

        return result;
    }

    Bytes getFile(const Path& path) override
    {
        BrotherDirectory dir(this);
        auto dirent = dir.findFile(path);
        int sector = dirent->startSector;

        Bytes data;
        ByteWriter bw(data);
        while ((sector != 0) && (sector != 0xffff))
        {
            bw += getLogicalSector(sector - 1);
            sector = dir.fat.at(sector);
        }

        return data;
    }

    void putFile(const Path& path, const Bytes& data) override
    {
        BrotherDirectory dir(this);

        try
        {
            dir.findFile(path);
            throw CannotWriteException("file exists");
        }
        catch (const FileNotFoundException& e)
        {
        }

        auto& filename = path.back();
        if (filename.size() > 8)
            throw CannotWriteException(
                "filename too long (eight characters maximum)");

        int sectorLength = (data.size() + SECTOR_SIZE - 1) / SECTOR_SIZE;
        if (sectorLength > 0xff)
            throw CannotWriteException("file is too big (64kB is the maximum)");

        ByteReader br(data);

        int firstSector = 0xffff;
        int previousSector = 0;
        while (!br.eof())
        {
            int currentSector = dir.allocateSector();
            if (previousSector)
                dir.fat[previousSector] = currentSector;

            putLogicalSector(currentSector - 1, br.read(SECTOR_SIZE));
            if (firstSector == 0xffff)
                firstSector = currentSector;
            previousSector = currentSector;
        }

        auto dirent = std::make_shared<Brother120Dirent>(
            filename, 0, firstSector, sectorLength);
        dir.dirents.push_back(dirent);
        dir.write(this);
    }

    void deleteFile(const Path& path) override
    {
        BrotherDirectory dir(this);
        dir.deleteFile(path);
        dir.write(this);
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        BrotherDirectory dir(this);
        return dir.findFile(path);
    }

private:
    const Brother120FsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createBrother120Filesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Brother120Filesystem>(config.brother120(), sectors);
}
