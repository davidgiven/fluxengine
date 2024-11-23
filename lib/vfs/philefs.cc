#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"

/* Root block:
 *
 * 00-0b	volume name
 * 0c		01
 * 0d		2a
 * 0e		79
 * 0f		6d
 * 10		07 0x07c10c19, creation timestamp; year
 * 11		c1 ^
 * 12		0c month
 * 13		19 day
 * 14		2f time? minutes
 * 15		00
 * 16		00 hours
 * 17		18 seconds
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

            {
                std::stringstream ss;
                ss << 'R';
                if (filedes[0] & 0x40)
                    ss << 'S';
                mode = ss.str();
            }

            this->filename = filename;
            path = {filename};

            attributes[Filesystem::CTIME] =
                fmt::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                    filedes.reader().seek(4).read_be16(),
                    filedes[6],
                    filedes[7] + 1,
                    filedes[10] & 0x1f,
                    filedes[8],
                    filedes[11]);

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;

            int spans = br.read_be16();
            for (int i = 0; i < spans; i++)
            {
                Span span;
                span.startBlock = br.read_be16();
                span.blockCount = br.read_be16();
                _spans.push_back(span);
            }

            attributes["phile.spans"] = std::to_string(spans);
        }

        const std::vector<Span>& spans() const
        {
            return _spans;
        }

    private:
        int _fileno;
        std::vector<Span> _spans;
    };

public:
    PhileFilesystem(
        const PhileProto& config, std::shared_ptr<SectorInterface> sectors):
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

        std::string volumename = _rootBlock.reader().read(0x0c);
        volumename = trimWhitespace(volumename);

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = volumename;
        attributes[TOTAL_BLOCKS] = std::to_string(_totalBlocks);
        attributes[USED_BLOCKS] = attributes[TOTAL_BLOCKS];
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
        _filedesLength = _rootBlock.reader().seek(0x20).read_be16() -
                         _filedesBlockNumber + 1;
        _totalBlocks = _rootBlock.reader().seek(0x18).read_be16();

        Bytes directoryBlock = getPsosBlock(3, 1);
        Bytes filedesBlock = getPsosBlock(_filedesBlockNumber, _filedesLength);

        _dirents.clear();
        ByteReader br(directoryBlock);
        ByteReader fr(filedesBlock);
        while (!br.eof())
        {
            uint16_t fileno = br.read_be16();
            std::string filename = br.read(14);
            filename = trimWhitespace(filename);

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
    int _totalBlocks;
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
