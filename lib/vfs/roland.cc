#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/utils.h"

class RolandFsFilesystem : public Filesystem
{
private:
    class RolandDirent : public Dirent
    {
    public:
        RolandDirent(const std::string& filename)
        {
            file_type = TYPE_FILE;

            this->filename = filename;
            path = {filename};

            length = 0;
            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = "";
        }

        void addBlock(RolandFsFilesystem* fs, uint8_t block)
        {
            blocks.push_back(block);

            length += fs->_blockSectors * fs->_sectorSize;
            attributes[Filesystem::LENGTH] = std::to_string(length);
        }

        void addBlocks(RolandFsFilesystem* fs, Bytes& dirent)
        {
            for (int i = 0; i < 16; i++)
            {
                uint8_t blocknumber = dirent[16 + i];
                if (!blocknumber)
                    break;

                addBlock(fs, blocknumber);
            }
        }

    public:
        std::vector<int> blocks;
    };

public:
    RolandFsFilesystem(
        const RolandFsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_CREATE | OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_PUTFILE |
               OP_GETDIRENT;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        int usedBlocks = 0;
        for (bool b : _allocationBitmap)
            usedBlocks += b;

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

    void create(bool quick, const std::string& volumeName) override
    {
        if (!quick)
            eraseEverythingOnDisk();

        init();
        _allocationBitmap[0] = true;
        rewriteDirectory();
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

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        return findFile(path.front());
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        Bytes data;
        ByteWriter bw(data);
        auto f = findFile(path.front());
        for (uint8_t b : f->blocks)
            bw += getRolandBlock(b);

        return data;
    }

    void putFile(const Path& path, const Bytes& bytes) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();
        if (findFileOrReturnNull(path.front()))
            throw BadPathException("File exists");

        int blocks = bytes.size() / _config.block_size();
        auto de = std::make_shared<RolandDirent>(RolandDirent(path.front()));

        ByteReader br(bytes);
        while (!br.eof())
        {
            Bytes data =
                br.read(_config.block_size()).slice(0, _config.block_size());
            int block = allocateBlock();
            de->addBlock(this, block);
            putRolandBlock(block, data);
        }

        _dirents.push_back(de);
        rewriteDirectory();
    }

private:
    void init()
    {
        _filesystemStart = getOffsetOfSector(_config.directory_track(), 0, 0);
        _sectorSize = getLogicalSectorSize(0, 0);

        _blockSectors = _config.block_size() / _sectorSize;

        _filesystemBlocks =
            (getLogicalSectorCount() - _filesystemStart) / _blockSectors;

        _dirents.clear();
        _allocationBitmap.clear();
        _allocationBitmap.resize(_filesystemBlocks);
    }

    void rewriteDirectory()
    {
        Bytes directory;
        ByteWriter bw(directory);

        bw.write_8(0);
        bw.append("ROLAND-GCRDOS");
        bw.write_le16(0x4e);
        bw.pad(16);

        for (auto& de : _dirents)
        {
            int blockIndex = 0;
            for (;;)
            {
                if (bw.pos == 0xa00)
                    throw DiskFullException();

                if (blockIndex == 0)
                {
                    bw.write_8(0);
                    int len = de->filename.size();
                    bw.append(de->filename);
                    bw.pad(13 - len, '_');
                    bw.write_8(0);
                    bw.write_8(blockIndex / 16);
                }

                if (blockIndex == de->blocks.size())
                    break;

                bw.write_8(de->blocks[blockIndex]);
                blockIndex++;
            }

            bw.pad(16 - (blockIndex % 16), 0);
        }

        while (bw.pos != 0xa00)
        {
            bw.write_8(0xe5);
            bw.pad(13, ' ');
            bw.write_le16(0);
            bw.pad(16);
        }

        for (bool b : _allocationBitmap)
            bw.write_8(b ? 0xff : 0x00);

        putRolandBlock(0, directory.slice(0, _config.block_size()));
    }

    void mount()
    {
        init();

        Bytes directory = getRolandBlock(0);
        ByteReader br(directory);
        br.seek(1);
        if (br.read(13) != "ROLAND-GCRDOS")
            throw BadFilesystemException();
        br.seek(32);

        std::map<std::string, std::shared_ptr<RolandDirent>> files;
        for (int i = 0; i < _config.directory_entries(); i++)
        {
            Bytes direntBytes = br.read(32);
            if (direntBytes[0] == 0)
            {
                std::string filename = direntBytes.slice(1, 13);

                std::shared_ptr<RolandDirent> de;
                auto it = files.find(filename);
                if (it == files.end())
                {
                    files[filename] = de =
                        std::make_shared<RolandDirent>(filename);
                    _dirents.push_back(de);
                }
                else
                    de = it->second;

                de->addBlocks(this, direntBytes);
            }
        }

        br.seek(0xa00);
        for (int i = 0; i < _filesystemBlocks; i++)
            _allocationBitmap[i] = br.read_8();
    }

    std::shared_ptr<RolandDirent> findFileOrReturnNull(
        const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        return nullptr;
    }

    std::shared_ptr<RolandDirent> findFile(const std::string filename)
    {
        std::shared_ptr<RolandDirent> de = findFileOrReturnNull(filename);
        if (!de)
            throw FileNotFoundException();
        return de;
    }

    int allocateBlock()
    {
        for (int i = 0; i < _filesystemBlocks; i++)
            if (!_allocationBitmap[i])
            {
                _allocationBitmap[i] = true;
                return i;
            }

        throw DiskFullException();
    }

    Bytes getRolandBlock(int number)
    {
        int sector = number * _blockSectors;
        return getLogicalSector(sector + _filesystemStart, _blockSectors);
    }

    void putRolandBlock(int number, const Bytes& bytes)
    {
        assert(bytes.size() == _config.block_size());
        int sector = number * _blockSectors;
        putLogicalSector(sector + _filesystemStart, bytes);
    }

private:
    const RolandFsProto& _config;
    uint32_t _sectorSize;
    uint32_t _blockSectors;
    uint32_t _filesystemStart;
    uint32_t _filesystemBlocks;
    std::vector<std::shared_ptr<RolandDirent>> _dirents;
    std::vector<bool> _allocationBitmap;
};

std::unique_ptr<Filesystem> Filesystem::createRolandFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<RolandFsFilesystem>(config.roland(), sectors);
}
