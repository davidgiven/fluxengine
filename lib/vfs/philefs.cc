#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/utils.h"
#include <fmt/format.h>

/* Root block:
 *
 * 00-0b	volume name
 * 0c		01
 * 0d		2a
 * 0e		79
 * 0f		6d
 * 10		07 0x07c10c19, creation timestamp
 * 11		c1 ^
 * 12		0c ^
 * 13		19 ^
 * 14		2f
 * 15		00
 * 16		00 0x0018, first file block?
 * 17		18 ^
 * 18		03 0x320, number of blocks on the disk
 * 19		20 ^
 * 1a		00 0x0010, first data block?
 * 1b		10 ^
 * 1c		00 0x0010, address of bitmap in HCS
 * 1d		10 ^
 * 1e		00 0x0011, address of FLIST in HCS
 * 1f		11 ^
 * 20		00 0x0017, address of last block of FLIST?
 * 21		17 ^
 * 22		00
 * 23		6b
 * 24		00
 * 25		20
 *
 * 14 files
 * file id 3 is not used
 * directory at 0xc00
 * 0x4000, block 0x10, volume bitmap
 * 0x4400, block 0x11, flist, 7 blocks long?
 * file descriptors seem to be 64 bytes
 *
 * File descriptor, 64 bytes:
 * 00		file type
 * 0e+04    length in bytes
 * 14...    spans
 *          word: start block
 *          word: number of blocks
 *
00000C00   00 01 42 49  54 4D 41 50  2E 53 59 53  00 00 00 00  ..BITMAP.SYS....

00008040   41 00 00 00  07 C1 0C 19  1E 00 00 18  00 02 00 00  A...............
00008050   04 00 00 01  00 10 00 01  00 00 00 00  00 00 00 00  ................
00008060   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00008070   00 00 00 00  00 00 00 00  00 00 00 01  00 01 01 00  ................

00000C10   00 02 46 4C  49 53 54 2E  53 59 53 00  00 00 00 00  ..FLIST.SYS.....

00008080   41 00 00 00  07 C1 0C 19  1E 00 00 18  00 02 00 00  A...............
00008090   1C 00 00 01  00 11 00 07  00 00 00 00  00 00 00 00  ................
000080A0   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
000080B0   00 00 00 00  00 00 00 00  00 00 00 07  00 07 01 00  ................

00000C20   00 04 53 4B  45 4C 00 00  00 00 00 00  00 00 00 00  ..SKEL..........

00008100   01 00 00 03  07 C1 0C 19  19 00 00 19  00 02 00 00  ................
00008110   55 00 00 01  00 20 00 16  00 00 00 00  00 00 00 00  U.... ..........
00008120   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00008130   00 00 00 00  00 00 00 00  00 00 00 16  00 16 01 00  ................


00000C30   00 05 43 4F  44 45 00 00  00 00 00 00  00 00 00 00  ..CODE..........

00004540   01 00 00 03  07 C1 0C 19  26 00 00 1F  00 02 00 08  ........&.......
00004550   10 00 00 01  00 36 02 04  00 00 00 00  00 00 00 00  .....6..........
00004560   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00  ................
00004570   00 00 00 00  00 00 00 00  00 00 02 04  02 04 01 00  ................

00000C40   00 06 53 43  53 49 00 00  00 00 00 00  00 00 00 00  ..SCSI..........
00000C50   00 07 41 4F  46 00 00 00  00 00 00 00  00 00 00 00  ..AOF...........
00000C60   00 08 4D 43  46 00 00 00  00 00 00 00  00 00 00 00  ..MCF...........
00000C70   00 09 53 59  53 54 45 4D  2E 53 43 46  00 00 00 00  ..SYSTEM.SCF....
00000C80   00 0A 53 59  53 54 45 4D  2E 50 44 46  00 00 00 00  ..SYSTEM.PDF....
00000C90   00 0B 43 4C  54 31 2E 43  4C 54 00 00  00 00 00 00  ..CLT1.CLT......
00000CA0   00 0C 43 4C  54 32 2E 43  4C 54 00 00  00 00 00 00  ..CLT2.CLT......
00000CB0   00 0D 43 4C  54 33 2E 43  4C 54 00 00  00 00 00 00  ..CLT3.CLT......
00000CC0   00 0E 43 4C  54 34 2E 43  4C 54 00 00  00 00 00 00  ..CLT4.CLT......
00000CD0   00 0F 47 52  45 59 2E 43  4C 54 00 00  00 00 00 00  ..GREY.CLT......
 */

class PhileFilesystem : public Filesystem
{
    struct Span
    {
        uint16_t startBlock;
        uint16_t blockCount;
    };

    class PhileDirent : public Dirent
    {
    public:
        PhileDirent(
            int fileno, const std::string& filename, const Bytes& filedes):
            _fileno(fileno)
        {
            file_type = TYPE_FILE;

            ByteReader br(filedes);
            br.seek(0x0e);
            length = br.read_be32();

            this->filename = filename;
            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = "";

            int spans = br.read_be16();
            for (int i = 0; i < spans; i++)
            {
                Span span;
                span.startBlock = br.read_be16();
                span.blockCount = br.read_be16();
                _spans.push_back(span);
            }

            attributes["smaky6.spans"] = std::to_string(spans);
        }

        const std::vector<Span>& spans() const
        {
            return _spans;
        }

    private:
        int _fileno;
        std::vector<Span> _spans;
    };

#if 0
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

    class PhileDirent : public Dirent
    {
    public:
        PhileDirent(const Bytes& dbuf, const Bytes& fbuf)
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

            std::string metadataBytes;
            {
                std::stringstream ss;

                for (int i = 10; i < 0x18; i++)
                    ss << fmt::format("{:02x} ", (uint8_t)dbuf[i]);

                metadataBytes = ss.str();
            }

            ByteReader br(dbuf);

            br.skip(10); /* filename */
            startSector = br.read_le16();
            endSector = br.read_le16();
            br.skip(2); /* unknown */
            lastSectorLength = br.read_le16();

            file_type = TYPE_FILE;
            length = (endSector - startSector - 1) * 256 + lastSectorLength;

            path = {filename};
            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = "";
            attributes["smaky6.start_sector"] = std::to_string(startSector);
            attributes["smaky6.end_sector"] = std::to_string(endSector);
            attributes["smaky6.sectors"] =
                std::to_string(endSector - startSector);
            attributes["smaky6.metadata_bytes"] = metadataBytes;
        }

    public:
        unsigned startSector;
        unsigned endSector;
        unsigned lastSectorLength;
    };
#endif

public:
    PhileFilesystem(
        const PhileProto& config, std::shared_ptr<SectorInterface> sectors):
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

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = "";
        attributes[USED_BLOCKS] = "";
        attributes[BLOCK_SIZE] = std::to_string(_config.block_size());
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
            result.push_back(de.second);
        return result;
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        auto dirent = findFile(path.front());

        Bytes data;
        ByteWriter bw(data);
        for (const auto& span : dirent->spans())
            bw += getPsosBlock(span.startBlock, span.blockCount);

        data.resize(dirent->length);
        return data;
    }

private:
    void mount()
    {
        _sectorSize = getLogicalSectorSize();
        _blockSectors = _config.block_size() / _sectorSize;

        _rootBlock = getPsosBlock(2, 1);
        _bitmapBlockNumber = _rootBlock.reader().seek(0x1c).read_be16();
        _filedesBlockNumber = _rootBlock.reader().seek(0x1e).read_be16();
        _filedesLength =
            _rootBlock.reader().seek(0x16).read_be16() - _filedesBlockNumber;

        Bytes directoryBlock = getPsosBlock(3, 1);
        Bytes filedesBlock = getPsosBlock(_filedesBlockNumber, _filedesLength);

        _dirents.clear();
        ByteReader br(directoryBlock);
        ByteReader fr(filedesBlock);
        while (!br.eof())
        {
            uint16_t fileno = br.read_be16();
            std::string filename = br.read(14);
            filename.erase(std::remove(filename.begin(), filename.end(), 0),
                filename.end());

            if (fileno)
            {
                fr.seek(fileno * 64);
                Bytes filedes = fr.read(64);
                auto dirent =
                    std::make_unique<PhileDirent>(fileno, filename, filedes);
                _dirents[fileno] = std::move(dirent);
            }
        }
    }

    std::shared_ptr<PhileDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent.second->filename == filename)
                return dirent.second;
        }

        throw FileNotFoundException();
    }

    Bytes getPsosBlock(uint32_t number, uint32_t count = 1)
    {
        unsigned sector = number * _blockSectors;
        return getLogicalSector(sector, _blockSectors * count);
    }

private:
    const PhileProto& _config;
    int _sectorSize;
    int _blockSectors;
    int _bitmapBlockNumber;
    int _filedesBlockNumber;
    int _filedesLength;
    Bytes _rootBlock;
    std::map<int, std::shared_ptr<PhileDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createPhileFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<PhileFilesystem>(config.phile(), sectors);
}
