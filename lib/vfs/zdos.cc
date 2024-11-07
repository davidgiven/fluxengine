#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/data/layout.h"
#include <iomanip>

/* See
 * https://oldcomputers.dyndns.org/public/pub/rechner/zilog/zds/manuals/z80-rio_os_userman.pdf,
 * page 116. */

enum
{
    ZDOS_TYPE_DATA = 0x10,
    ZDOS_TYPE_ASCII = 0x20,
    ZDOS_TYPE_DIRECTORY = 0x40,
    ZDOS_TYPE_PROCEDURE = 0x80
};

enum
{
    ZDOS_MODE_FORCE = 1 << 2,
    ZDOS_MODE_RANDOM = 1 << 3,
    ZDOS_MODE_SECRET = 1 << 4,
    ZDOS_MODE_LOCKED = 1 << 5,
    ZDOS_MODE_ERASEPROTECT = 1 << 6,
    ZDOS_MODE_WRITEPROTECT = 1 << 7
};

static const std::map<uint8_t, std::string> fileTypeMap = {
    {0,                   "INVALID"  },
    {ZDOS_TYPE_DATA,      "DATA"     },
    {ZDOS_TYPE_ASCII,     "ASCII"    },
    {ZDOS_TYPE_DIRECTORY, "DIRECTORY"},
    {ZDOS_TYPE_PROCEDURE, "PROCEDURE"}
};

static std::string convertTime(std::string zdosTime)
{
    /* Due to a bug in std::get_time, we can't parse the string directly --- a
     * pattern of %y%m%d causes the first four digits of the string to become
     * the year. So we need to reform the string. */

    zdosTime = fmt::format("{}-{}-{}",
        zdosTime.substr(0, 2),
        zdosTime.substr(2, 2),
        zdosTime.substr(4, 2));

    std::tm tm = {};
    std::stringstream(zdosTime) >> std::get_time(&tm, "%y-%m-%d");

    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T%z");
    return ss.str();
}

class ZDosFilesystem : public Filesystem
{
    class ZDosDescriptor
    {
    public:
        ZDosDescriptor(ZDosFilesystem* zfs, int block): zfs(zfs)
        {
            Bytes bytes = zfs->getLogicalSector(block);
            ByteReader br(bytes);
            br.seek(8);
            firstRecord = zfs->readBlockNumber(br);
            br.seek(12);
            type = br.read_8();
            recordCount = br.read_le16();
            recordSize = br.read_le16();
            br.seek(19);
            properties = br.read_8();
            startAddress = br.read_le16();
            lastRecordSize = br.read_le16();
            br.seek(24);
            ctime = br.read(8);
            mtime = br.read(8);

            rewind();
        }

        void rewind()
        {
            currentRecord = firstRecord;
            eof = false;
        }

        Bytes readRecord()
        {
            assert(!eof);
            int count = recordSize / 0x80;

            Bytes result;
            ByteWriter bw(result);

            while (count--)
            {
                Bytes sector = zfs->getLogicalSector(currentRecord);
                ByteReader br(sector);

                bw += br.read(0x80);
                br.skip(2);
                int sectorId = br.read_8();
                int track = br.read_8();
                currentRecord = zfs->toBlockNumber(sectorId, track);
                if (sectorId == 0xff)
                    eof = true;
            }

            return result;
        }

    public:
        ZDosFilesystem* zfs;
        uint16_t firstRecord;
        uint8_t type;
        uint16_t recordCount;
        uint16_t recordSize;
        uint8_t properties;
        uint16_t startAddress;
        uint16_t lastRecordSize;
        std::string ctime;
        std::string mtime;

        uint16_t currentRecord;
        bool eof;
    };

    class ZDosDirent : public Dirent
    {
    public:
        ZDosDirent(ZDosFilesystem* zfs,
            const std::string& filename,
            int descriptorBlock):
            zfs(zfs),
            descriptorBlock(descriptorBlock),
            zd(zfs, descriptorBlock)
        {
            file_type = TYPE_FILE;
            this->filename = filename;

            length = (zd.recordCount - 1) * zd.recordSize + zd.lastRecordSize;

            mode = "";
            if (zd.properties & ZDOS_MODE_FORCE)
                mode += 'F';
            if (zd.properties & ZDOS_MODE_RANDOM)
                mode += 'R';
            if (zd.properties & ZDOS_MODE_SECRET)
                mode += 'S';
            if (zd.properties & ZDOS_MODE_LOCKED)
                mode += 'L';
            if (zd.properties & ZDOS_MODE_ERASEPROTECT)
                mode += 'E';
            if (zd.properties & ZDOS_MODE_WRITEPROTECT)
                mode += 'W';

            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
            attributes["zdos.descriptor_record"] =
                std::to_string(descriptorBlock);
            attributes["zdos.first_record"] = std::to_string(zd.firstRecord);
            attributes["zdos.record_size"] = std::to_string(zd.recordSize);
            attributes["zdos.record_count"] = std::to_string(zd.recordCount);
            attributes["zdos.last_record_size"] =
                std::to_string(zd.lastRecordSize);
            attributes["zdos.start_address"] =
                fmt::format("0x{:04x}", zd.startAddress);
            attributes["zdos.type"] = fileTypeMap.at(zd.type & 0xf0);
            attributes["zdos.ctime"] = convertTime(zd.ctime);
            attributes["zdos.mtime"] = convertTime(zd.mtime);
        }

    public:
        ZDosFilesystem* zfs;
        int descriptorBlock;
        ZDosDescriptor zd;
    };

public:
    ZDosFilesystem(
        const ZDosProto& config, std::shared_ptr<SectorInterface> sectors):
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

        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(_totalBlocks);
        attributes[USED_BLOCKS] = std::to_string(_usedBlocks);
        attributes[BLOCK_SIZE] = "128";
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
        dirent->zd.rewind();

        Bytes bytes;
        ByteWriter bw(bytes);
        while (!dirent->zd.eof)
        {
            bw += dirent->zd.readRecord();
        }

        return bytes.slice(0, dirent->length);
    }

private:
    void mount()
    {
        _sectorsPerTrack = Layout::getLayoutOfTrack(0, 0)->numSectors;

        int rootBlock = toBlockNumber(_config.filesystem_start().sector(),
            _config.filesystem_start().track());
        ZDosDescriptor zd(this, rootBlock);
        if (zd.type != ZDOS_TYPE_DIRECTORY)
            throw BadFilesystemException();

        _totalBlocks = getLogicalSectorCount();
        _usedBlocks = (zd.recordCount * zd.recordSize) / 0x80 + 1;
        while (!zd.eof)
        {
            Bytes bytes = zd.readRecord();
            ByteReader br(bytes);
            for (;;)
            {
                int len = br.read_8();
                if (len == 0xff)
                    break;

                std::string filename = br.read(len & 0x7f);
                int descriptorBlock = readBlockNumber(br);

                auto dirent = std::make_unique<ZDosDirent>(
                    this, filename, descriptorBlock);
                _usedBlocks +=
                    (dirent->zd.recordCount * dirent->zd.recordSize) / 0x80 + 1;
                _dirents.push_back(std::move(dirent));
            }
        }
    }

    std::shared_ptr<ZDosDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }

    int toBlockNumber(int sectorId, int track)
    {
        return track * _sectorsPerTrack + sectorId;
    }

    int readBlockNumber(ByteReader& br)
    {
        int sectorId = br.read_8();
        int track = br.read_8();
        return toBlockNumber(sectorId, track);
    }

private:
    const ZDosProto& _config;
    unsigned _sectorsPerTrack;
    unsigned _totalBlocks;
    unsigned _usedBlocks;
    std::vector<std::shared_ptr<ZDosDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createZDosFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<ZDosFilesystem>(config.zdos(), sectors);
}
