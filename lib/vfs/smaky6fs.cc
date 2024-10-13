#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"

/* A directory entry looks like:
 *
 * 00-09: ten byte filename FFFFFFFF.EE
 * 0a-0b: word: start sector
 * 0c-17: unknown
 */

class Smaky6Filesystem : public Filesystem
{
    class Entry
    {
    public:
        Entry(const Bytes& bytes)
        {
            ByteReader br(bytes);
            br.seek(10);
            startSector = br.read_le16();
            endSector = br.read_le16();
        }

    public:
        std::string filename;
        std::string mode;
        uint16_t startSector;
        uint16_t endSector;
    };

    class SmakyDirent : public Dirent
    {
    public:
        SmakyDirent(const Bytes& dbuf)
        {
            {
                std::stringstream ss;

                for (int i = 0; i <= 7; i++)
                {
                    uint8_t c = dbuf[i] & 0x7f;
                    if (c == ' ')
                        break;
                    ss << (char)c;
                }
                for (int i = 8; i <= 9; i++)
                {
                    uint8_t c = dbuf[i] & 0x7f;
                    if (c == ' ')
                        break;
                    if (i == 8)
                        ss << '.';
                    ss << (char)c;
                }
                filename = ss.str();
            }

            std::string metadataBytes;
            {
                std::stringstream ss;

                for (int i = 10; i < 0x18; i++)
                    ss << fmt::format("{:02x} ", (uint8_t)dbuf[i]);

                metadataBytes = ss.str();
            }

            ByteReader br(dbuf);

            br.skip(10); /* filename */
            startSector = br.read_le16();
            endSector = br.read_le16();
            br.skip(2); /* unknown */
            lastSectorLength = br.read_le16();

            file_type = TYPE_FILE;
            length = (endSector - startSector - 1) * 256 + lastSectorLength;

            path = {filename};
            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = "";
            attributes["smaky6.start_sector"] = std::to_string(startSector);
            attributes["smaky6.end_sector"] = std::to_string(endSector);
            attributes["smaky6.sectors"] =
                std::to_string(endSector - startSector);
            attributes["smaky6.metadata_bytes"] = metadataBytes;
        }

    public:
        unsigned startSector;
        unsigned endSector;
        unsigned lastSectorLength;
    };

    friend class Directory;
    class Directory
    {
    public:
        Directory(Smaky6Filesystem* fs)
        {
            /* Read the directory. */

            auto bytes = fs->getLogicalSector(0, 3);
            ByteReader br(bytes);

            for (int i = 0; i < 32; i++)
            {
                auto dbuf = bytes.slice(i * 0x18, 0x18);
                if (dbuf[0])
                {
                    auto de = std::make_shared<SmakyDirent>(dbuf);
                    dirents.push_back(de);
                }
            }
        }

        std::shared_ptr<SmakyDirent> findFile(const std::string& filename)
        {
            for (auto& de : dirents)
                if (de->filename == filename)
                    return de;

            throw FileNotFoundException();
        }

    public:
        std::vector<std::shared_ptr<SmakyDirent>> dirents;
    };

public:
    Smaky6Filesystem(
        const Smaky6FsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_LIST | OP_GETFILE | OP_GETFSDATA | OP_GETDIRENT;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        Directory dir(this);
        unsigned usedBlocks = 3;
        for (auto& de : dir.dirents)
            usedBlocks += (de->endSector - de->startSector);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(getLogicalSectorCount());
        attributes[USED_BLOCKS] = std::to_string(usedBlocks);
        attributes[BLOCK_SIZE] = std::to_string(getLogicalSectorSize(0, 0));
        return attributes;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        if (!path.empty())
            throw FileNotFoundException();

        Directory dir(this);
        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& de : dir.dirents)
            result.push_back(de);
        return result;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        Directory dir(this);
        if (path.size() != 1)
            throw BadPathException();

        return dir.findFile(path[0]);
    }

    Bytes getFile(const Path& path) override
    {
        if (path.size() != 1)
            throw BadPathException(path);

        Directory dir(this);
        auto de = dir.findFile(path[0]);

        Bytes data =
            getLogicalSector(de->startSector, de->endSector - de->startSector);
        data = data.slice(0, de->length);
        return data;
    }

private:
    const Smaky6FsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createSmaky6Filesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Smaky6Filesystem>(config.smaky6(), sectors);
}
