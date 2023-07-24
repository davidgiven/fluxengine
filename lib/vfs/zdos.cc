#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"

class ZDosFilesystem : public Filesystem
{
    #if 0
    class ZDosDirent : public Dirent
    {
    public:
        ZDosDirent(const ZDosProto& config, Bytes& bytes)
        {
            file_type = TYPE_FILE;

            ByteReader br(bytes);
            filename = trimWhitespace(br.read(10));
            uint16_t type = br.read_be16();
            location = br.read_be32();
            length = br.read_be32() * config.block_size();
            int year = unbcd(br.read_8());
            int month = unbcd(br.read_8()) + 1;
            int day = unbcd(br.read_8());
            int hour = unbcd(br.read_8());
            int minute = unbcd(br.read_8());
            int second = unbcd(br.read_8());
            uint16_t volume = br.read_be16();
            uint16_t protection = br.read_be16();
            uint16_t recordSize = br.read_be16();

            if (year >= 70)
                year += 1900;
            else
                year += 2000;

            std::tm tm = {.tm_sec = second,
                .tm_min = minute,
                .tm_hour = hour,
                .tm_mday = day,
                .tm_mon = month,
                .tm_year = year - 1900,
                .tm_isdst = -1};
            std::stringstream ss;
            ss << std::put_time(&tm, "%FT%T%z");
            ctime = ss.str();

            auto it = numberToFileType.find(type);
            if (it != numberToFileType.end())
                mode = it->second;
            else
                mode = fmt::format("0x{:04x}", type);

            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
            attributes["lif.ctime"] = ctime;
            attributes["lif.volume"] = std::to_string(volume & 0x7fff);
            attributes["lif.protection"] = fmt::format("0x{:x}", protection);
            attributes["lif.record_size"] = std::to_string(recordSize);
        }

    public:
        uint32_t location;
        std::string ctime;
    };
    #endif

public:
    ZDosFilesystem(
        const ZDosProto& config, std::shared_ptr<SectorInterface> sectors):
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

    #if 0
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

        return getZDosBlock(
            dirent->location, dirent->length / _config.block_size());
    }
    #endif

private:
    void mount()
    {
    #if 0
        _sectorSize = getLogicalSectorSize();
        _sectorsPerBlock = _sectorSize / _config.block_size();

        _rootBlock = getZDosBlock(0);

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

        Bytes directory = getZDosBlock(_directoryBlock, _directorySize);

        _dirents.clear();
        ByteReader br(directory);
        while (!br.eof())
        {
            Bytes direntBytes = br.read(32);
            if (direntBytes[0] != 0xff)
            {
                auto dirent = std::make_unique<ZDosDirent>(_config, direntBytes);
                _usedBlocks += dirent->length / _config.block_size();
                _dirents.push_back(std::move(dirent));
            }
        }
        _totalBlocks = std::max(tracks * heads * sectors, _usedBlocks);
        #endif
    }

    #if 0
    std::shared_ptr<ZDosDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }
    #endif

private:
    const ZDosProto& _config;
    int _sectorSize;
    int _sectorsPerBlock;
    Bytes _rootBlock;
    std::string _volumeLabel;
    unsigned _directoryBlock;
    unsigned _directorySize;
    unsigned _totalBlocks;
    unsigned _usedBlocks;
    //std::vector<std::shared_ptr<ZDosDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createZDosFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<ZDosFilesystem>(config.zdos(), sectors);
}

