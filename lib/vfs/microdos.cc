#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"
#include "fmt/format.h"

/* See https://www.hp9845.net/9845/projects/hpdir/#lif_filesystem for
 * a description. */

static void trimZeros(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), 0), s.end());
}

class MicrodosFilesystem : public Filesystem
{
    struct SDW
    {
        unsigned start;
        unsigned length;
    };

    class MicrodosDirent : public Dirent
    {
    public:
        MicrodosDirent(MicrodosFilesystem& fs, Bytes& bytes)
        {
            file_type = TYPE_FILE;

            ByteReader br(bytes);
            auto stem = trimWhitespace(br.read(6));
            auto ext = trimWhitespace(br.read(3));
            filename = fmt::format("{}.{}", stem, ext);

            br.skip(1);
            ssn = br.read_be16();
            attr = br.read_8();

            Bytes rib = fs.getLogicalSector(ssn);
            ByteReader rbr(rib);
            for (int i = 0; i < 57; i++)
            {
                unsigned w = rbr.read_be16();
                if (w & 0x8000)
                {
                    /* Last. */
                    sectors = w & 0x7fff;
                    break;
                }
                else
                {
                    /* Each record except the last is 24 bits long. */
                    w = (w << 8) | rbr.read_8();
                    sdws.emplace_back(SDW{w & 0xffff, (w >> 16) + 1});
                }
            }
            rbr.seek(500);
            lastSectorBytes = rbr.read_be16();
            loadSectors = rbr.read_be16();
            loadAddress = rbr.read_be16();
            startAddress = rbr.read_be16();

            length = sectors * 512;

            mode = "";
            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
            attributes["microdos.ssn"] = std::to_string(ssn);
            attributes["microdos.attr"] = fmt::format("0x{:x}", attr);
            attributes["microdos.sdw_count"] = std::to_string(sdws.size());
            attributes["microdos.total_sectors"] = std::to_string(sectors);
            attributes["microdos.lastSectorBytes"] =
                std::to_string(lastSectorBytes);
            attributes["microdos.loadSectors"] = std::to_string(loadSectors);
            attributes["microdos.loadAddress"] =
                fmt::format("0x{:x}", loadAddress);
            attributes["microdos.startAddress"] =
                fmt::format("0x{:x}", startAddress);
        }

    public:
        unsigned ssn;
        unsigned attr;
        std::vector<SDW> sdws;
        unsigned sectors;
        unsigned lastSectorBytes;
        unsigned loadSectors;
        unsigned loadAddress;
        unsigned startAddress;
    };

public:
    MicrodosFilesystem(
        const MicrodosProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_GETDIRENT;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        std::map<std::string, std::string> attributes;

        attributes[VOLUME_NAME] = _volumeLabel;
        attributes[TOTAL_BLOCKS] = std::to_string(_totalBlocks);
        attributes[USED_BLOCKS] = std::to_string(_usedBlocks);
        attributes[BLOCK_SIZE] = "512";
        return attributes;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        return findFile(path.front());
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        mount();
        if (!path.empty())
            throw FileNotFoundException();

        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& de : _dirents)
            result.push_back(de);
        return result;
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        auto dirent = findFile(path.front());

        Bytes data;
        ByteWriter bw(data);
        for (const auto& sdw : dirent->sdws)
            bw += getLogicalSector(sdw.start, sdw.length);

        return data.slice(512);
    }

private:
    void mount()
    {
        _rootBlock = getLogicalSector(0);
        _catBlock = getLogicalSector(9);
        Bytes directory = getLogicalSector(1, 8);

        ByteReader rbr(_rootBlock);
        rbr.seek(20);
        _volumeLabel = trimWhitespace(rbr.read(44));

        _dirents.clear();
        ByteReader dbr(directory);
        while (!dbr.eof())
        {
            Bytes direntBytes = dbr.read(16);
            if ((direntBytes[0] != 0) && (direntBytes[0] != 0xff))
            {
                auto dirent =
                    std::make_unique<MicrodosDirent>(*this, direntBytes);
                _dirents.push_back(std::move(dirent));
            }
        }

        ByteReader cbr(_catBlock);
        _totalBlocks = 630;
        _usedBlocks = 0;
        for (int i = 0; i < _totalBlocks / 8; i++)
        {
            uint8_t b = cbr.read_8();
            _usedBlocks += countSetBits(b);
        }
    }

    std::shared_ptr<MicrodosDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }

private:
    const MicrodosProto& _config;
    Bytes _rootBlock;
    Bytes _catBlock;
    std::string _volumeLabel;
    unsigned _totalBlocks;
    unsigned _usedBlocks;
    std::vector<std::shared_ptr<MicrodosDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createMicrodosFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<MicrodosFilesystem>(config.microdos(), sectors);
}
