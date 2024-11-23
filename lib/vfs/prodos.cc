#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"

/* This is described here:
 * http://fileformats.archiveteam.org/wiki/ProDOS_file_system
 */

class ProdosFilesystem : public Filesystem
{
    static constexpr int BLOCK_SECTORS = 2;
    static constexpr int ROOT_DIRECTORY_BLOCK = 2;

    enum
    {
        STORAGETYPE_VOLUME = 0xf0,
        STORAGETYPE_DIRBLOCK = 0xe0,
        STORAGETYPE_SUBDIR = 0xd0,
        STORAGETYPE_TREE = 0x30,
        STORAGETYPE_SAPLING = 0x20,
        STORAGETYPE_SEEDLING = 0x10
    };

    class ProdosDirent : public Dirent
    {
    public:
        ProdosDirent(const Bytes& de)
        {
            ByteReader br(de);
            storageType = br.read_8();
            filename = br.read(15).slice(0, storageType & 0x0f);
            storageType &= 0xf0;
            prodosType = br.read_8();
            keyBlock = br.read_le16();
            blocksUsed = br.read_le16();
            length = br.read_le24();
            ctime = br.read_le32();
            version = br.read_8();
            minVersion = br.read_8();
            access = br.read_8();
            auxType = br.read_8();
            mtime = br.read_le32();

            file_type = (storageType == STORAGETYPE_SUBDIR) ? TYPE_DIRECTORY
                                                            : TYPE_FILE;

            attributes[FILENAME] = filename;
            attributes[LENGTH] = std::to_string(length);
            attributes[FILE_TYPE] =
                (file_type == TYPE_DIRECTORY) ? "dir" : "file";
            attributes[MODE] = "";
            attributes["prodos.storage_type"] =
                fmt::format("0x{:x}", storageType);
            attributes["prodos.prodos_type"] = std::to_string(prodosType);
            attributes["prodos.key_block"] = std::to_string(keyBlock);
            attributes["prodos.blocks_used"] = std::to_string(blocksUsed);
            attributes["prodos.version"] = std::to_string(version);
            attributes["prodos.min_version"] = std::to_string(minVersion);
            attributes["prodos.access"] = std::to_string(access);
            attributes["prodos.aux_type"] = std::to_string(auxType);
        }

        uint8_t storageType;
        uint8_t prodosType;
        uint16_t keyBlock;
        uint16_t blocksUsed;
        uint32_t ctime;
        uint8_t version;
        uint8_t minVersion;
        uint8_t access;
        uint8_t auxType;
        uint32_t mtime;
    };

    class Directory
    {
    public:
        Directory(ProdosFilesystem* fs, uint16_t block): _fs(fs), _block(block)
        {
            for (;;)
            {
                block = readDirectoryBlock(block);
                if (!block)
                    break;
            }
        }

    public:
        std::shared_ptr<ProdosDirent> find(const std::string& filename)
        {
            for (auto& dirent : dirents)
                if (dirent->filename == filename)
                    return dirent;

            throw FileNotFoundException();
        }

    private:
        uint16_t readDirectoryBlock(uint16_t block)
        {
            auto bytes = _fs->getLogicalBlock(block);
            ByteReader br(bytes);
            br.seek(2);
            uint16_t nextBlock = br.read_le16();

            int here;
            if ((br.read_8() & 0xf0) >= 0xe0)
            {
                /* First block of a directory. */
                br.seek(0x2b);
            }
            else
                br.seek(4);

            while (br.pos < 473)
            {
                Bytes de = br.read(0x27);
                if ((de[0] & 0xf0) == 0)
                    continue;

                dirents.push_back(std::make_shared<ProdosDirent>(de));
            }

            return nextBlock;
        }

    private:
        ProdosFilesystem* _fs;
        uint16_t _block;

    public:
        std::vector<std::shared_ptr<ProdosDirent>> dirents;
    };

public:
    ProdosFilesystem(
        const ProdosProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_LIST | OP_GETDIRENT | OP_GETFILE | OP_GETFSDATA;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        auto block = getLogicalBlock(ROOT_DIRECTORY_BLOCK);

        uint8_t flen = block[4] & 0x0f;
        std::string volumename = block.slice(5, flen);

        uint16_t usedBlocks = 0;
        for (bool bit : _allocationBitmap)
            if (!bit)
                usedBlocks++;

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = volumename;
        attributes[TOTAL_BLOCKS] =
            std::to_string(block.reader().seek(0x29).read_le16());
        attributes[USED_BLOCKS] = std::to_string(usedBlocks);
        attributes[BLOCK_SIZE] = "512";
        return attributes;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        mount();

        auto dir = chdir(path);
        std::vector<std::shared_ptr<Dirent>> results;
        for (auto& de : dir->dirents)
            results.push_back(de);

        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        mount();

        auto dir = chdir(path.parent());
        return dir->find(path.back());
    }

    Bytes getFile(const Path& path) override
    {
        mount();

        auto dir = chdir(path.parent());
        auto dirent = dir->find(path.back());

        Bytes bytes;
        switch (dirent->storageType)
        {
            case STORAGETYPE_SUBDIR:
                throw BadPathException("tried to use a directory like a file");

            case STORAGETYPE_SEEDLING:
                bytes = getLogicalBlock(dirent->keyBlock);
                break;

            case STORAGETYPE_SAPLING:
            {
                auto keyBytes = getLogicalBlock(dirent->keyBlock);
                ByteWriter bw(bytes);
                readIndexBlock(keyBytes, bw);
                break;
            }

            case STORAGETYPE_TREE:
            {
                auto masterKeyBytes = getLogicalBlock(dirent->keyBlock);
                ByteWriter bw(bytes);
                ByteReader br(masterKeyBytes);

                /* This always appends 16MB of data, the maximum amount
                 * for a tree file, which is wasteful but simple.
                 */

                while (!br.eof())
                {
                    uint16_t indexBlock = br.read_le16();
                    if (indexBlock)
                        readIndexBlock(getLogicalBlock(indexBlock), bw);
                    else
                        bw += Bytes(128 * 1024);
                }
                break;
            }

            default:
                throw UnimplementedFilesystemException(
                    fmt::format("storage type 0x{:x} isn't supported yet",
                        dirent->storageType));
        }

        return bytes.slice(0, dirent->length);
    }

private:
    void mount()
    {
        auto rootVolume = getLogicalBlock(ROOT_DIRECTORY_BLOCK);
        if ((rootVolume[4] & 0xf0) != 0xf0)
            throw BadFilesystemException();

        ByteReader br(rootVolume);
        br.seek(0x27);
        _allocationBitmapLocation = br.read_le16();
        uint16_t totalBlocks = br.read_le16();

        uint16_t bitmapBlocks = (totalBlocks + 4095) / 4096;
        _allocationBitmap =
            getLogicalBlock(_allocationBitmapLocation, bitmapBlocks).toBits();
        _allocationBitmap.resize(totalBlocks);
    }

    std::unique_ptr<Directory> chdir(const Path& path)
    {
        std::unique_ptr<Directory> dir =
            std::make_unique<Directory>(this, ROOT_DIRECTORY_BLOCK);
        for (const auto& element : path)
        {
            auto entry = dir->find(element);
            if (entry->file_type == TYPE_FILE)
                throw BadPathException("tried to use a file like a directory");

            dir = std::make_unique<Directory>(this, entry->keyBlock);
        }
        return dir;
    }

    /* Always appends 128kB of data. */
    void readIndexBlock(const Bytes& indexBlock, ByteWriter& bw)
    {
        ByteReader br(indexBlock);
        while (!br.eof())
        {
            uint16_t block = br.read_le16();
            if (block)
                bw += getLogicalBlock(block);
            else
                bw += Bytes(512);
        }
    }

    Bytes getLogicalBlock(uint16_t block, unsigned count = 1)
    {
        return getLogicalSector(block * BLOCK_SECTORS, count * BLOCK_SECTORS);
    }

private:
    const ProdosProto& _config;
    uint16_t _allocationBitmapLocation;
    std::vector<bool> _allocationBitmap;
};

std::unique_ptr<Filesystem> Filesystem::createProdosFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<ProdosFilesystem>(config.prodos(), sectors);
}
