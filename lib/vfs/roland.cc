#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"
#include <regex>

static std::string unmangleFilename(const std::string& mangled)
{
    std::string extension = mangled.substr(10);
    extension.erase(extension.find_last_not_of("_") + 1);

    std::string root = mangled.substr(0, 10);
    root.erase(root.find_last_not_of("_") + 1);

    if (!extension.empty())
        return root + "." + extension;
    return root;
}

static std::string mangleFilename(const std::string& human)
{
    int dot = human.rfind('.');
    std::string extension =
        (dot == std::string::npos) ? "" : human.substr(dot + 1);
    std::string root =
        (dot == std::string::npos) ? human : human.substr(0, dot);

    if (extension.empty())
        extension = "___";
    if (extension.size() > 3)
        throw BadPathException("Invalid filename: extension too long");
    if (root.size() > 10)
        throw BadPathException("Invalid filename: root too long");
    root = (root + std::string(10, '_')).substr(0, 10);
    std::string mangled = root + extension;

    static const std::regex checker("[A-Z0-9_$.]*");
    if (!std::regex_match(mangled, checker))
        throw BadPathException(
            "Invalid filename: unsupported characters (remember to use "
            "uppercase)");
    return mangled;
}

class RolandFsFilesystem : public Filesystem
{
private:
    class RolandDirent : public Dirent
    {
    public:
        RolandDirent(const std::string& filename)
        {
            file_type = TYPE_FILE;
            rename(filename);

            length = 0;
            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = "";
        }

        void rename(const std::string& name)
        {
            filename = name;
            path = {filename};
        }

        void putBlock(RolandFsFilesystem* fs, uint8_t offset, uint8_t block)
        {
            if (blocks.size() <= offset)
                blocks.resize(offset + 1);
            blocks[offset] = block;

            length = (offset + 1) * fs->_blockSectors * fs->_sectorSize;
            attributes[Filesystem::LENGTH] = std::to_string(length);
        }

        void putBlocks(RolandFsFilesystem* fs, uint8_t offset, Bytes& dirent)
        {
            for (int i = 0; i < 16; i++)
            {
                uint8_t blocknumber = dirent[16 + i];
                if (!blocknumber)
                    break;

                putBlock(fs, offset + i, blocknumber);
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
               OP_GETDIRENT | OP_DELETE | OP_MOVE;
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
        auto de = std::make_shared<RolandDirent>(
            RolandDirent(mangleFilename(path.front())));

        ByteReader br(bytes);
        int offset = 0;
        while (!br.eof())
        {
            Bytes data =
                br.read(_config.block_size()).slice(0, _config.block_size());
            int block = allocateBlock();
            de->putBlock(this, offset, block);
            putRolandBlock(block, data);
            offset++;
        }

        _dirents.push_back(de);
        rewriteDirectory();
    }

    void deleteFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        auto de = findFile(path.front());
        for (uint8_t b : de->blocks)
            freeBlock(b);

        for (auto it = _dirents.begin(); it != _dirents.end(); it++)
        {
            if (*it == de)
            {
                _dirents.erase(it);
                break;
            }
        }

        rewriteDirectory();
    }

    void moveFile(const Path& oldName, const Path& newName) override
    {
        mount();
        if ((oldName.size() != 1) || (newName.size() != 1))
            throw BadPathException();
        if (findFileOrReturnNull(newName.front()))
            throw BadPathException("File exists");

        auto de = findFile(oldName.front());
        de->rename(mangleFilename(newName.front()));
        rewriteDirectory();
    }

private:
    void init()
    {
        _directoryLba = getOffsetOfSector(_config.directory_track(), 0, 0);
        _sectorSize = getLogicalSectorSize(0, 0);

        _blockSectors = _config.block_size() / _sectorSize;

        _filesystemBlocks = getLogicalSectorCount() / _blockSectors;
        _midBlock = (getLogicalSectorCount() - _directoryLba) / _blockSectors;

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

                if ((blockIndex % 16) == 0)
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
                int extent = direntBytes[15];
                std::string filename =
                    unmangleFilename(direntBytes.slice(1, 13));

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

                de->putBlocks(this, extent * 16, direntBytes);
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

    void freeBlock(int block)
    {
        if (block >= _filesystemBlocks)
            throw BadFilesystemException();

        if (!_allocationBitmap[block])
            throw BadFilesystemException();
        _allocationBitmap[block] = false;
    }

    unsigned blockToLogicalSectorNumber(int block)
    {
        int track;
        if (block < _midBlock)
            track = _config.directory_track() + block;
        else
            track = _config.directory_track() - (1 + block - _midBlock);
        return track * _blockSectors;
    }

    Bytes getRolandBlock(int number)
    {
        return getLogicalSector(
            blockToLogicalSectorNumber(number), _blockSectors);
    }

    void putRolandBlock(int number, const Bytes& bytes)
    {
        assert(bytes.size() == _config.block_size());
        putLogicalSector(blockToLogicalSectorNumber(number), bytes);
    }

private:
    const RolandFsProto& _config;
    unsigned _sectorSize;
    unsigned _blockSectors;
    unsigned _midBlock;
    unsigned _directoryLba;
    unsigned _filesystemBlocks;
    std::vector<std::shared_ptr<RolandDirent>> _dirents;
    std::vector<bool> _allocationBitmap;
};

std::unique_ptr<Filesystem> Filesystem::createRolandFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<RolandFsFilesystem>(config.roland(), sectors);
}
