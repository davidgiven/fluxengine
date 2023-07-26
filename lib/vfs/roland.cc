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

        void addBlocks(RolandFsFilesystem* fs, Bytes& dirent)
        {
            int extent = dirent[15];
            for (int i=0; i<16; i++)
            {
                uint8_t blocknumber = dirent[16+i];
                if (!blocknumber)
                    break;

                blocks[i + extent*16] = blocknumber;
                length += fs->_blockSectors * fs->_sectorSize;
            }

            attributes[Filesystem::LENGTH] = std::to_string(length);
        }

    public:
        std::map<int, int> blocks;
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
        return OP_GETFSDATA | OP_LIST | OP_GETFILE | OP_GETDIRENT;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = _volumeName;
        attributes[TOTAL_BLOCKS] = std::to_string(_filesystemBlocks);
        attributes[USED_BLOCKS] = std::to_string(_usedBlocks);
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
        for (auto& e : f->blocks)
        {
            uint8_t b = e.second;
            if (!b)
                break;
            bw += getRolandBlock(b);
        }

        return data;
    }

private:
    void mount()
    {
        _filesystemStart = getOffsetOfSector(_config.directory_track(), 0, 0);
        _sectorSize = getLogicalSectorSize(0, 0);

        _blockSectors = _config.block_size() / _sectorSize;

        _filesystemBlocks =
            (getLogicalSectorCount() - _filesystemStart) / _blockSectors;

        Bytes directory = getRolandBlock(0);
        ByteReader br(directory);
        br.seek(1);
        _volumeName = br.read(14);
        br.seek(32);

        _dirents.clear();
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
                    files[filename] = de = std::make_shared<RolandDirent>(filename);
                    _dirents.push_back(de);
                }
                else
                    de = it->second;

                de->addBlocks(this, direntBytes);
            }
        }

        _usedBlocks = 0;
        br.seek(0xa00);
        for (int i = 0; i < 512; i++)
        {
            bool used = br.read_8();
            if (used)
                _usedBlocks++;
        }
    }

    std::shared_ptr<RolandDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }

    Bytes getRolandBlock(int number)
    {
        int sector = number * _blockSectors;
        return getLogicalSector(sector + _filesystemStart, _blockSectors);
    }

private:
    const RolandFsProto& _config;
    uint32_t _sectorSize;
    uint32_t _blockSectors;
    uint32_t _filesystemStart;
    uint32_t _filesystemBlocks;
    uint32_t _usedBlocks;
    std::string _volumeName;
    std::vector<std::shared_ptr<RolandDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createRolandFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<RolandFsFilesystem>(config.roland(), sectors);
}
