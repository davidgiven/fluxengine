#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/utils.h"
#include <fmt/format.h>

/* See https://www.hp9845.net/9845/projects/hpdir/#lif_filesystem for
 * a description. */

static void trimZeros(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), 0), s.end());
}

class LifFilesystem : public Filesystem
{
    class LifDirent : public Dirent
    {
    public:
        LifDirent(const LifProto& config, Bytes& bytes)
        {
            file_type = TYPE_FILE;

            ByteReader br(bytes);
            filename = trimWhitespace(br.read(10));
            uint16_t type = br.read_be16();
            location = br.read_be32();
            length = br.read_be32() * config.block_size();

            mode = fmt::format("{:04x}", type);
            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
        }

    public:
        uint32_t location;
    };

public:
    LifFilesystem(
        const LifProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const
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
        attributes[BLOCK_SIZE] = std::to_string(_config.block_size());
        attributes["lif.directory_block"] = std::to_string(_directoryBlock);
        attributes["lif.directory_size"] = std::to_string(_directorySize);
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

        return getLifBlock(
            dirent->location, dirent->length / _config.block_size());
    }

private:
    void mount()
    {
        _sectorSize = getLogicalSectorSize();
        _sectorsPerBlock = _sectorSize / _config.block_size();

        _rootBlock = getLifBlock(0);

        ByteReader rbr(_rootBlock);
        if (rbr.read_be16() != 0x8000)
            throw BadFilesystemException();
        _volumeLabel = trimWhitespace(rbr.read(6));
        _directoryBlock = rbr.read_be32();
        rbr.skip(4);
        _directorySize = rbr.read_be32();
        unsigned tracks = rbr.read_be32();
        unsigned heads = rbr.read_be32();
        unsigned sectors = rbr.read_be32();
        _usedBlocks = 1 + _directorySize;

        Bytes directory = getLifBlock(_directoryBlock, _directorySize);

        _dirents.clear();
        ByteReader br(directory);
        while (!br.eof())
        {
            Bytes direntBytes = br.read(32);
            if (direntBytes[0] != 0xff)
            {
                auto dirent = std::make_unique<LifDirent>(_config, direntBytes);
                _usedBlocks += dirent->length / _config.block_size();
                _dirents.push_back(std::move(dirent));
            }
        }
        _totalBlocks = std::max(tracks * heads * sectors, _usedBlocks);
    }

    std::shared_ptr<LifDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }

    Bytes getLifBlock(uint32_t number, uint32_t count)
    {
        Bytes b;
        ByteWriter bw(b);

        while (count)
        {
            bw += getLifBlock(number);
            number++;
            count--;
        }

        return b;
    }

    Bytes getLifBlock(uint32_t number)
    {
        /* LIF uses 256-byte blocks, but the underlying format can have much
         * bigger sectors. */

        Bytes sector = getLogicalSector(number / _sectorsPerBlock);
        unsigned offset = number % _sectorsPerBlock;
        return sector.slice(
            offset * _config.block_size(), _config.block_size());
    }

private:
    const LifProto& _config;
    int _sectorSize;
    int _sectorsPerBlock;
    Bytes _rootBlock;
    std::string _volumeLabel;
    unsigned _directoryBlock;
    unsigned _directorySize;
    unsigned _totalBlocks;
    unsigned _usedBlocks;
    std::vector<std::shared_ptr<LifDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createLifFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<LifFilesystem>(config.lif(), sectors);
}
