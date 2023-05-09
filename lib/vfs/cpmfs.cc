#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"

class CpmFsFilesystem : public Filesystem
{
    class Entry
    {
    public:
        Entry(const Bytes& bytes, int map_entry_size)
        {
            user = bytes[0] & 0x0f;

            {
                std::stringstream ss;
                ss << (char)(user + '0') << ':';

                for (int i = 1; i <= 8; i++)
                {
                    uint8_t c = bytes[i] & 0x7f;
                    if (c == ' ')
                        break;
                    ss << (char)c;
                }
                for (int i = 9; i <= 11; i++)
                {
                    uint8_t c = bytes[i] & 0x7f;
                    if (c == ' ')
                        break;
                    if (i == 9)
                        ss << '.';
                    ss << (char)c;
                }
                filename = ss.str();
            }

            {
                std::stringstream ss;
                if (bytes[9] & 0x80)
                    ss << 'R';
                if (bytes[10] & 0x80)
                    ss << 'S';
                if (bytes[11] & 0x80)
                    ss << 'A';
                mode = ss.str();
            }

            extent = bytes[12] | (bytes[14] << 5);
            records = bytes[15];

            ByteReader br(bytes);
            br.seek(16);
            switch (map_entry_size)
            {
                case 1:
                    for (int i = 0; i < 16; i++)
                        allocation_map.push_back(br.read_8());
                    break;

                case 2:
                    for (int i = 0; i < 8; i++)
                        allocation_map.push_back(br.read_le16());
                    break;
            }
        }

    public:
        std::string filename;
        std::string mode;
        unsigned user;
        unsigned extent;
        unsigned records;
        std::vector<unsigned> allocation_map;
    };

public:
    CpmFsFilesystem(
        const CpmFsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const
    {
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_GETDIRENT;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        unsigned usedBlocks = _dirBlocks;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (!entry)
                continue;

            for (unsigned block : entry->allocation_map)
            {
                if (block)
                    usedBlocks++;
            }
        }

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(_filesystemBlocks);
        attributes[USED_BLOCKS] = std::to_string(usedBlocks);
        attributes[BLOCK_SIZE] = std::to_string(_config.block_size());
        return attributes;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        mount();
        if (!path.empty())
            throw FileNotFoundException();

        std::map<std::string, std::shared_ptr<Dirent>> map;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (!entry)
                continue;

            auto& dirent = map[entry->filename];
            if (!dirent)
            {
                dirent = std::make_unique<Dirent>();
                dirent->path = {entry->filename};
                dirent->filename = entry->filename;
                dirent->mode = entry->mode;
                dirent->length = 0;
                dirent->file_type = TYPE_FILE;
            }

            dirent->length = std::max(
                dirent->length, entry->extent * 16384 + entry->records * 128);
        }

        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& e : map)
        {
            auto& de = e.second;
            de->attributes[FILENAME] = de->filename;
            de->attributes[LENGTH] = std::to_string(de->length);
            de->attributes[FILE_TYPE] = "file";
            de->attributes[MODE] = de->mode;
            result.push_back(std::move(de));
        }
        return result;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        for (const auto& dirent : list(Path()))
        {
            if (dirent->filename == path.front())
                return dirent;
        }

        throw FileNotFoundException();
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        Bytes data;
        ByteWriter bw(data);
        int logicalExtent = 0;
        for (;;)
        {
            /* Find a directory entry for this logical extent. */

            std::unique_ptr<Entry> entry;
            for (int d = 0; d < _config.dir_entries(); d++)
            {
                entry = getEntry(d);
                if (!entry)
                    continue;
                if (path[0] != entry->filename)
                    continue;
                if (entry->extent < logicalExtent)
                    continue;
                if ((entry->extent & ~_logicalExtentMask) ==
                    (logicalExtent & ~_logicalExtentMask))
                    break;
            }

            if (!entry)
            {
                if (logicalExtent == 0)
                    throw FileNotFoundException();
                break;
            }

            /* Copy the data out. */

            int i =
                (logicalExtent & _logicalExtentMask) * _blocksPerLogicalExtent;
            unsigned records =
                (entry->extent == logicalExtent) ? entry->records : 128;
            while (records != 0)
            {
                Bytes block;
                unsigned blockid = entry->allocation_map[i];
                if (blockid != 0)
                    block = getCpmBlock(entry->allocation_map[i]);
                else
                    block.resize(_config.block_size());

                unsigned r = std::min(records, _recordsPerBlock);
                bw += block.slice(0, r * 128);

                records -= r;
                i++;
            }

            logicalExtent++;
        }

        return data;
    }

private:
    void mount()
    {
        auto& start = _config.filesystem_start();
        _filesystemStart =
            getOffsetOfSector(start.track(), start.side(), start.sector());
        _sectorSize = getLogicalSectorSize(start.track(), start.side());

        _blockSectors = _config.block_size() / _sectorSize;
        _recordsPerBlock = _config.block_size() / 128;
        _dirBlocks = (_config.dir_entries() * 32) / _config.block_size();

        _filesystemBlocks =
            (getLogicalSectorCount() - _filesystemStart) / _blockSectors;
        _allocationMapSize = (_filesystemBlocks < 256) ? 1 : 2;

        int physicalExtentSize;
        if (_allocationMapSize == 1)
        {
            /* One byte allocation maps */
            physicalExtentSize = _config.block_size() * 16;
        }
        else
        {
            /* Two byte allocation maps */
            physicalExtentSize = _config.block_size() * 8;
        }
        _logicalExtentsPerEntry = physicalExtentSize / 16384;
        _logicalExtentMask = _logicalExtentsPerEntry - 1;
        _blocksPerLogicalExtent = 16384 / _config.block_size();

        _directory = getCpmBlock(0, _dirBlocks);
    }

    std::unique_ptr<Entry> getEntry(unsigned d)
    {
        auto bytes = _directory.slice(d * 32, 32);
        if (bytes[0] == 0xe5)
            return nullptr;

        return std::make_unique<Entry>(bytes, _allocationMapSize);
    }

    Bytes getCpmBlock(uint32_t number, uint32_t count = 1)
    {
        unsigned sector = number * _blockSectors;
        if (_config.has_padding())
            sector += (sector / _config.padding().every()) *
                      _config.padding().amount();

        return getLogicalSector(
            sector + _filesystemStart, _blockSectors * count);
    }

private:
    const CpmFsProto& _config;
    uint32_t _sectorSize;
    uint32_t _blockSectors;
    uint32_t _recordsPerBlock;
    uint32_t _dirBlocks;
    uint32_t _filesystemStart;
    uint32_t _filesystemBlocks;
    uint32_t _logicalExtentsPerEntry;
    uint32_t _logicalExtentMask;
    uint32_t _blocksPerLogicalExtent;
    int _allocationMapSize;
    Bytes _directory;
};

std::unique_ptr<Filesystem> Filesystem::createCpmFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<CpmFsFilesystem>(config.cpmfs(), sectors);
}
