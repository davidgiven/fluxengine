#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/core/utils.h"

enum
{
    DEL,
    SEQ,
    PRG,
    USR,
    REL
};

static std::string fromPetscii(const Bytes& bytes)
{
    std::stringstream ss;

    for (uint8_t b : bytes)
    {
        if ((b >= 32) && (b <= 126))
            ss << (char)tolower(b);
        else
            ss << fmt::format("%{:2x}", b);
    }

    return ss.str();
}

static std::string toMode(uint8_t cbm_type)
{
    std::stringstream ss;
    if (cbm_type & 0x40)
        ss << 'L';
    if (cbm_type & 0x80)
        ss << 'S';
    return ss.str();
}

static std::string toFileType(uint8_t cbm_type)
{
    switch (cbm_type & 0x0f)
    {
        case DEL:
            return "DEL";
        case SEQ:
            return "SEQ";
        case PRG:
            return "PRG";
        case USR:
            return "USR";
        case REL:
            return "REL";
        default:
            return fmt::format("[bad type {:x}]", cbm_type & 0x0f);
    }
}

class CbmfsFilesystem : public Filesystem
{
    class CbmfsDirent : public Dirent
    {
    public:
        CbmfsDirent(const Bytes& dbuf)
        {
            ByteReader br(dbuf);

            br.skip(2); /* t/s field */
            cbm_type = br.read_8();
            start_track = br.read_8();
            start_sector = br.read_8();

            auto filenameBytes = br.read(16).split(0xa0)[0];
            filename = fromPetscii(filenameBytes);
            path = {filename};
            side_track = br.read_8();
            side_sector = br.read_8();
            recordlen = br.read_8();
            br.skip(6);
            sectors = br.read_le16();

            file_type = TYPE_FILE;
            length = sectors * 254;
            mode = "";

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
            attributes["cbmfs.type"] = toFileType(cbm_type);
            attributes["cbmfs.start_track"] = std::to_string(start_track);
            attributes["cbmfs.start_sector"] = std::to_string(start_sector);
            attributes["cbmfs.side_track"] = std::to_string(side_track);
            attributes["cbmfs.side_sector"] = std::to_string(side_sector);
            attributes["cbmfs.recordlen"] = std::to_string(recordlen);
            attributes["cbmfs.sectors"] = std::to_string(sectors);
        }

    public:
        unsigned cbm_type;
        unsigned start_track;
        unsigned start_sector;
        unsigned side_track;
        unsigned side_sector;
        unsigned recordlen;
        unsigned sectors;
    };

    friend class Directory;
    class Directory
    {
    public:
        Directory(CbmfsFilesystem* fs)
        {
            /* Read the BAM. */

            uint8_t t = fs->_config.directory_track();
            uint8_t s = 0;
            auto b = fs->getSector(t, 0, s);
            ByteReader br(b);
            br.skip(2);
            dosVersion = br.read_8();
            br.skip(1);

            bam.resize(fs->getLogicalSectorCount());
            usedBlocks = 0;
            unsigned block = 0;
            for (int track = 0; track < globalConfig()->layout().tracks();
                 track++)
            {
                uint8_t blocks = br.read_8();
                uint32_t bitmap = br.read_le24();
                for (int sector = 0; sector < blocks; sector++)
                {
                    if (bitmap & (1 << sector))
                    {
                        bam[block + sector] = bitmap & (1 << sector);
                        usedBlocks++;
                    }
                }
                block += blocks;
            }

            /* Read the volume name. */

            br.seek(0x90);
            auto nameBytes = br.read(16).split(0xa0)[0];
            volumeName = fromPetscii(nameBytes);

            /* Read the directory. */

            s = 1;
            while (t != 0xff)
            {
                auto b = fs->getSector(t, 0, s);

                for (int i = 0; i < 8; i++)
                {
                    auto dbuf = b.slice(i * 32, 32);
                    if (dbuf[2] == 0)
                        continue;

                    auto de = std::make_shared<CbmfsDirent>(dbuf);
                    dirents.push_back(de);
                }

                t = b[0] - 1;
                s = b[1];
            }
        }

        std::shared_ptr<CbmfsDirent> findFile(const std::string& filename)
        {
            for (auto& de : dirents)
                if (de->filename == filename)
                    return de;

            throw FileNotFoundException();
        }

    public:
        uint8_t dosVersion;
        std::string volumeName;
        unsigned usedBlocks;
        std::vector<bool> bam;
        std::vector<std::shared_ptr<CbmfsDirent>> dirents;
    };

public:
    CbmfsFilesystem(
        const CbmfsProto& config, std::shared_ptr<SectorInterface> sectors):
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
        Directory dir(this);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = dir.volumeName;
        attributes[USED_BLOCKS] = std::to_string(dir.usedBlocks);
        attributes[TOTAL_BLOCKS] = std::to_string(getLogicalSectorCount());
        attributes[BLOCK_SIZE] = std::to_string(getLogicalSectorSize());
        attributes["cbmfs.dos_type"] = std::to_string((char)dir.dosVersion);
        attributes["cbmfs.bam_size"] = std::to_string(dir.bam.size());
        return attributes;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        if (path.size() != 0)
            throw BadPathException();

        Directory dir(this);
        std::vector<std::shared_ptr<Dirent>> results;
        for (auto& de : dir.dirents)
            results.push_back(de);

        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        if (path.size() != 1)
            throw BadPathException();

        Directory dir(this);
        return dir.findFile(unhex(path[0]));
    }

    Bytes getFile(const Path& path) override
    {
        if (path.size() != 1)
            throw BadPathException();
        Directory dir(this);
        auto de = dir.findFile(unhex(path[0]));
        if (de->cbm_type == REL)
            throw UnimplementedFilesystemException("cannot read .REL files");

        Bytes bytes;
        ByteWriter bw(bytes);

        uint8_t t = de->start_track - 1;
        uint8_t s = de->start_sector;
        for (;;)
        {
            auto b = getSector(t, 0, s);

            if (b[0])
                bw += b.slice(2);
            else
            {
                bw += b.slice(2, b[1]);
                break;
            }

            t = b[0] - 1;
            s = b[1];
        }

        return bytes;
    }

private:
private:
    const CbmfsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createCbmfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<CbmfsFilesystem>(config.cbmfs(), sectors);
}
