#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"

/* A directory entry looks like:
 *
 * 00-07: name, space-padded ASCII (high bit may be set as attribute flag → mask with 0x7f)
 * 08-09: 2-char type code (SY, SM, ST, SR, LS, FH, BS, IM, KS …)
 * 0a-0b: uint16 LE start_sector  – first data sector (0-based absolute)
 * 0c-0d: uint16 LE end_sector    – first sector after the file (exclusive)
 * 0e-0f: uint16 LE flags         – purpose unknown
 * 10-11: uint16 LE last_bytes    – bytes used in last sector;
 *                                   0 means last sector is completely full
 *         exact_size = (size_sectors-1)*256 + last_bytes  if last_bytes > 0
 *                      size_sectors * 256                  if last_bytes == 0
 * 12-13: load address  – high byte then low byte (reliable for type SM)
 * 14-15: entry point   – same encoding
 * 16:    month BCD (0x07 = July);  0x00 or 0xFF = no date
 * 17:    year  BCD (0x82 = 1982);  0x00 or 0xFF = no date
 */

class Smaky6Filesystem : public Filesystem
{
    class Entry
    {
    public:
        Entry(const Bytes& bytes)
        {
            ByteReader br(bytes);
            br.seek(10);
            startSector = br.read_le16();
            endSector = br.read_le16();
        }

    public:
        std::string filename;
        std::string mode;
        uint16_t startSector;
        uint16_t endSector;
    };

    class SmakyDirent : public Dirent
    {
    public:
        SmakyDirent(const Bytes& dbuf)
        {
            {
                std::stringstream ss;

                for (int i = 0; i <= 7; i++)
                {
                    uint8_t c = dbuf[i] & 0x7f;
                    if (c == ' ')
                        break;
                    ss << (char)c;
                }
                for (int i = 8; i <= 9; i++)
                {
                    uint8_t c = dbuf[i] & 0x7f;
                    if (c == ' ')
                        break;
                    if (i == 8)
                        ss << '.';
                    ss << (char)c;
                }
                filename = ss.str();
            }

            ByteReader br(dbuf);
            br.skip(10); /* skip filename/type already parsed above */
            startSector = br.read_le16();
            endSector   = br.read_le16();
            uint16_t flags = br.read_le16(); /* purpose unknown */
            lastSectorLength = br.read_le16();
            uint8_t loadHi  = br.read_8();
            uint8_t loadLo  = br.read_8();
            uint8_t entryHi = br.read_8();
            uint8_t entryLo = br.read_8();
            uint8_t monthBcd = br.read_8();
            uint8_t yearBcd  = br.read_8();

            /* Decode BCD date; 0x00 and 0xFF both mean "no date" */
            auto bcdToInt = [](uint8_t b) -> int {
                return (b >> 4) * 10 + (b & 0x0f);
            };
            int month = (monthBcd && monthBcd != 0xff) ? bcdToInt(monthBcd) : 0;
            int year  = (yearBcd  && yearBcd  != 0xff) ? bcdToInt(yearBcd)  : 0;

            uint16_t loadAddr  = ((uint16_t)loadHi  << 8) | loadLo;
            uint16_t entryAddr = ((uint16_t)entryHi << 8) | entryLo;

            file_type = TYPE_FILE;
            /* When lastSectorLength == 0 the last sector is completely full;
             * FluxEngine's original formula subtracted 256 bytes in that case. */
            length = lastSectorLength
                ? (endSector - startSector - 1) * 256 + lastSectorLength
                : (endSector - startSector) * 256;

            path = {filename};
            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH]    = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE]      = "";
            attributes["smaky6.start_sector"] = std::to_string(startSector);
            attributes["smaky6.end_sector"]   = std::to_string(endSector);
            attributes["smaky6.sectors"]      = std::to_string(endSector - startSector);
            attributes["smaky6.flags"]        = fmt::format("0x{:04x}", flags);
            if (loadAddr)
                attributes["smaky6.load_addr"]  = fmt::format("0x{:04x}", loadAddr);
            if (entryAddr)
                attributes["smaky6.entry_addr"] = fmt::format("0x{:04x}", entryAddr);
            if (month && year)
            {
                static const char* months[] = {"","Jan","Feb","Mar","Apr","May","Jun",
                                                "Jul","Aug","Sep","Oct","Nov","Dec"};
                int yearFull = (year >= 78) ? 1900 + year : 2000 + year;
                attributes["smaky6.date"] =
                    fmt::format("{} {}",
                        (month >= 1 && month <= 12) ? months[month] : "?",
                        yearFull);
            }
        }

    public:
        unsigned startSector;
        unsigned endSector;
        unsigned lastSectorLength;
    };

    friend class Directory;
    class Directory
    {
    public:
        Directory(Smaky6Filesystem* fs)
        {
            /* Read the directory. */

            auto bytes = fs->getLogicalSector(0, 3);
            ByteReader br(bytes);

            for (int i = 0; i < 32; i++)
            {
                auto dbuf = bytes.slice(i * 0x18, 0x18);
                /* 0x00 = empty slot; 0xFF = deleted entry (used by system files) */
                if (dbuf[0] && dbuf[0] != 0xff)
                {
                    auto de = std::make_shared<SmakyDirent>(dbuf);
                    dirents.push_back(de);
                }
            }
        }

        std::shared_ptr<SmakyDirent> findFile(const std::string& filename)
        {
            for (auto& de : dirents)
                if (de->filename == filename)
                    return de;

            throw FileNotFoundException();
        }

    public:
        std::vector<std::shared_ptr<SmakyDirent>> dirents;
    };

public:
    Smaky6Filesystem(const Smaky6FsProto& config,
        const std::shared_ptr<const DiskLayout>& diskLayout,
        std::shared_ptr<SectorInterface> sectors):
        Filesystem(diskLayout, sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_LIST | OP_GETFILE | OP_GETFSDATA | OP_GETDIRENT;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        Directory dir(this);
        unsigned usedBlocks = 3;
        for (auto& de : dir.dirents)
            usedBlocks += (de->endSector - de->startSector);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(getLogicalSectorCount());
        attributes[USED_BLOCKS] = std::to_string(usedBlocks);
        attributes[BLOCK_SIZE] = std::to_string(getLogicalSectorSize(0, 0));
        return attributes;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        if (!path.empty())
            throw FileNotFoundException();

        Directory dir(this);
        std::vector<std::shared_ptr<Dirent>> result;
        for (auto& de : dir.dirents)
            result.push_back(de);
        return result;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        Directory dir(this);
        if (path.size() != 1)
            throw BadPathException();

        return dir.findFile(path[0]);
    }

    Bytes getFile(const Path& path) override
    {
        if (path.size() != 1)
            throw BadPathException(path);

        Directory dir(this);
        auto de = dir.findFile(path[0]);

        Bytes data =
            getLogicalSector(de->startSector, de->endSector - de->startSector);
        data = data.slice(0, de->length);
        return data;
    }

private:
    const Smaky6FsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createSmaky6Filesystem(
    const FilesystemProto& config,
    const std::shared_ptr<const DiskLayout>& diskLayout,
    std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Smaky6Filesystem>(
        config.smaky6(), diskLayout, sectors);
}
