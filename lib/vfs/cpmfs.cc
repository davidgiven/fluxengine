#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "fmt/format.h"
#include <regex>

class CpmFsFilesystem : public Filesystem, public HasBitmap, public HasMount
{
    class Entry
    {
    public:
        Entry(const Bytes& bytes, int map_entry_size, unsigned index):
            index(index)
        {
            if (bytes[0] == 0xe5)
                deleted = true;

            user = bytes[0] & 0x0f;

            {
                std::stringstream ss;

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

        Bytes toBytes(int map_entry_size) const
        {
            Bytes bytes(32);
            ByteWriter bw(bytes);

            if (deleted)
            {
                for (int i = 0; i < 32; i++)
                    bw.write_8(0xe5);
            }
            else
            {
                bw.write_8(user);

                /* Encode the filename. */

                for (int i = 1; i < 12; i++)
                    bytes[i] = 0x20;
                for (char c : filename)
                {
                    if (islower(c))
                        throw BadPathException();
                    if (c == '.')
                    {
                        if (bw.pos >= 9)
                            throw BadPathException();
                        bw.seek(9);
                        continue;
                    }
                    if ((bw.pos == 9) || (bw.pos == 12))
                        throw BadPathException();
                    bw.write_8(c);
                }

                /* Set the mode. */

                if (mode.find('R') != std::string::npos)
                    bytes[9] |= 0x80;
                if (mode.find('S') != std::string::npos)
                    bytes[10] |= 0x80;
                if (mode.find('A') != std::string::npos)
                    bytes[11] |= 0x80;

                /* EX, S1, S2, RC */

                bw.seek(12);
                bw.write_8(extent & 0x1f); /* EX */
                bw.write_8(0);             /* S1 */
                bw.write_8(extent >> 5);   /* S2 */
                bw.write_8(records);       /* RC */

                /* Allocation map. */

                switch (map_entry_size)
                {
                    case 1:
                        for (int i = 0; i < 16; i++)
                            bw.write_8(allocation_map[i]);
                        break;

                    case 2:
                        for (int i = 0; i < 8; i++)
                            bw.write_le16(allocation_map[i]);
                        break;
                }
            }

            return bytes;
        }

        void changeFilename(const std::string& filename)
        {
            static std::regex FORMATTER("(?:(1?[0-9]):)?([^ .]+)\\.?([^ .]*)");
            std::smatch results;
            bool matched = std::regex_match(filename, results, FORMATTER);
            if (!matched)
                throw BadPathException();

            std::string user = results[1];
            std::string stem = results[2];
            std::string ext = results[3];

            if (stem.size() > 8)
                throw BadPathException();
            if (ext.size() > 3)
                throw BadPathException();

            this->user = std::stoi(user);
            if (this->user > 15)
                throw BadPathException();

            if (ext.empty())
                this->filename = stem;
            else
                this->filename = fmt::format("{}.{}", stem, ext);
        }

        std::string combinedFilename() const
        {
            return fmt::format("{}:{}", user, filename);
        }

    public:
        unsigned index;
        std::string filename;
        std::string mode;
        unsigned user;
        unsigned extent;
        unsigned records;
        std::vector<unsigned> allocation_map;
        bool deleted = false;
    };

public:
    CpmFsFilesystem(
        const CpmFsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_PUTFILE | OP_DELETE |
               OP_GETDIRENT | OP_CREATE | OP_MOVE | OP_PUTATTRS;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        unsigned usedBlocks = _dirBlocks;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (entry->deleted)
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

    void create(bool, const std::string&) override
    {
        auto& start = _config.filesystem_start();
        _filesystemStart =
            getOffsetOfSector(start.track(), start.side(), start.sector());
        _sectorSize = getLogicalSectorSize(start.track(), start.side());

        _directory = Bytes{0xe5} * (_config.dir_entries() * 32);
        putCpmBlock(0, _directory);
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
            if (entry->deleted)
                continue;

            auto& dirent = map[entry->combinedFilename()];
            if (!dirent)
            {
                dirent = std::make_unique<Dirent>();
                dirent->filename = entry->combinedFilename();
                dirent->path = {dirent->filename};
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

    void putMetadata(const Path& path,
        const std::map<std::string, std::string>& metadata) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        /* Only updating MODE is supported. */

        if (metadata.empty())
            return;
        if ((metadata.size() != 1) || (metadata.begin()->first != MODE))
            throw UnimplementedFilesystemException();
        auto mode = metadata.begin()->second;

        /* Update all dirents corresponding to this file. */

        bool found = false;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            std::unique_ptr<Entry> entry = getEntry(d);
            if (entry->deleted)
                continue;
            if (path[0] == entry->combinedFilename())
            {
                entry->mode = mode;
                putEntry(entry);
                found = true;
            }
        }
        if (!found)
            throw FileNotFoundException();

        unmount();
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
                if (entry->deleted)
                    continue;
                if (path[0] != entry->combinedFilename())
                    continue;
                if (entry->extent < logicalExtent)
                    continue;
                if ((entry->extent & ~_logicalExtentMask) ==
                    (logicalExtent & ~_logicalExtentMask))
                    break;
            }

            if (entry->deleted)
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

public:
    void putFile(const Path& path, const Bytes& bytes) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        /* Test to see if the file already exists. */

        for (int d = 0; d < _config.dir_entries(); d++)
        {
            std::unique_ptr<Entry> entry = getEntry(d);
            if (entry->deleted)
                continue;
            if (path[0] == entry->combinedFilename())
                throw CannotWriteException();
        }

        /* Write blocks, one at a time. */

        std::unique_ptr<Entry> entry;
        ByteReader br(bytes);
        while (!br.eof())
        {
            unsigned extent = br.pos / 0x4000;
            Bytes block = br.read(_config.block_size());

            /* Allocate a block and write it. */

            auto bit = std::find(_bitmap.begin(), _bitmap.end(), false);
            if (bit == _bitmap.end())
                throw DiskFullException();
            *bit = true;
            unsigned blocknum = bit - _bitmap.begin();
            putCpmBlock(blocknum, block);

            /* Do we need a new directory entry? */

            if (!entry ||
                entry->allocation_map[std::size(entry->allocation_map) - 1])
            {
                if (entry)
                {
                    entry->records = 0x80;
                    putEntry(entry);
                }

                entry.reset();
                for (int d = 0; d < _config.dir_entries(); d++)
                {
                    entry = getEntry(d);
                    if (entry->deleted)
                        break;
                    entry.reset();
                }

                if (!entry)
                    throw DiskFullException();
                entry->deleted = false;
                entry->changeFilename(path[0]);
                entry->extent = extent;
                entry->mode = "";
                std::fill(entry->allocation_map.begin(),
                    entry->allocation_map.end(),
                    0);
            }

            /* Hook up the block in the allocation map. */

            auto mit = std::find(
                entry->allocation_map.begin(), entry->allocation_map.end(), 0);
            *mit = blocknum;
        }
        if (entry)
        {
            entry->records = ((bytes.size() & 0x3fff) + 127) / 128;
            putEntry(entry);
        }

        unmount();
    }

    void moveFile(const Path& oldPath, const Path& newPath) override
    {
        mount();
        if ((oldPath.size() != 1) || (newPath.size() != 1))
            throw BadPathException();

        /* Check to make sure that the file exists, and that the new filename
         * does not. */

        bool found = false;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (entry->deleted)
                continue;

            auto filename = entry->combinedFilename();
            if (filename == oldPath[0])
                found = true;
            if (filename == newPath[0])
                throw CannotWriteException();
        }
        if (!found)
            throw FileNotFoundException();

        /* Now do the rename. */

        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (entry->deleted)
                continue;

            auto filename = entry->combinedFilename();
            if (filename == oldPath[0])
            {
                entry->changeFilename(newPath[0]);
                putEntry(entry);
            }
        }

        unmount();
    }

    void deleteFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        /* Remove all dirents for this file. */

        bool found = false;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            auto entry = getEntry(d);
            if (entry->deleted)
                continue;
            if (path[0] != entry->combinedFilename())
                continue;
            entry->deleted = true;
            putEntry(entry);
            found = true;
        }

        if (!found)
            throw FileNotFoundException();
        unmount();
    }

public:
    void mount() override
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

        /* Create the allocation bitmap. */

        _bitmap.clear();
        _bitmap.resize(_filesystemBlocks);
        for (int d = 0; d < _dirBlocks; d++)
            _bitmap[d] = true;
        for (int d = 0; d < _config.dir_entries(); d++)
        {
            std::unique_ptr<Entry> entry = getEntry(d);
            if (entry->deleted)
                continue;
            for (unsigned block : entry->allocation_map)
            {
                if (block >= _filesystemBlocks)
                    throw BadFilesystemException();
                if (block)
                    _bitmap[block] = true;
            }
        }
    }

    void unmount()
    {
        putCpmBlock(0, _directory);
    }

private:
    std::unique_ptr<Entry> getEntry(unsigned d)
    {
        auto bytes = _directory.slice(d * 32, 32);
        return std::make_unique<Entry>(bytes, _allocationMapSize, d);
    }

    void putEntry(std::unique_ptr<Entry>& entry)
    {
        ByteWriter bw(_directory);
        bw.seek(entry->index * 32);
        bw.append(entry->toBytes(_allocationMapSize));
    }

    unsigned computeSector(uint32_t block) const
    {
        unsigned sector = block * _blockSectors;
        if (_config.has_padding())
            sector += (sector / _config.padding().every()) *
                      _config.padding().amount();
        return sector;
    }

    Bytes getCpmBlock(uint32_t block, uint32_t count = 1)
    {
        return getLogicalSector(
            computeSector(block) + _filesystemStart, _blockSectors * count);
    }

    void putCpmBlock(uint32_t block, const Bytes& bytes)
    {
        putLogicalSector(computeSector(block) + _filesystemStart, bytes);
    }

public:
    std::vector<bool> getBitmapForDebugging() override
    {
        return _bitmap;
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
    std::vector<bool> _bitmap;
};

std::unique_ptr<Filesystem> Filesystem::createCpmFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<CpmFsFilesystem>(config.cpmfs(), sectors);
}
