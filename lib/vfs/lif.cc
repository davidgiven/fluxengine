#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"
#include <iomanip>

/* See https://www.hp9845.net/9845/projects/hpdir/#lif_filesystem for
 * a description. */

static std::map<uint16_t, std::string> numberToFileType = {
    {0x0001, "TEXT"    },
    {0x00ff, "D-LEX"   },
    {0xe008, "BIN8x"   },
    {0xe010, "DTA8x"   },
    {0xe020, "BAS8x"   },
    {0xe030, "XM41"    },
    {0xe040, "ALL41"   },
    {0xe050, "KEY41"   },
    {0xe052, "TXT75"   },
    {0xe053, "APP75"   },
    {0xe058, "DAT75"   },
    {0xe060, "STA41"   },
    {0xe070, "X-M41"   },
    {0xe080, "PGM41"   },
    {0xe088, "BAS75"   },
    {0xe089, "LEX75"   },
    {0xe08a, "WKS75"   },
    {0xe08b, "ROM75"   },
    {0xe0d0, "SDATA"   },
    {0xe0d1, "TEXT_S"  },
    {0xe0f0, "DAT71"   },
    {0xe0f1, "DAT71_S" },
    {0xe204, "BIN71"   },
    {0xe205, "BIN71_S" },
    {0xe206, "BIN71_P" },
    {0xe207, "BIN71_SP"},
    {0xe208, "LEX71"   },
    {0xe209, "LEX71_S" },
    {0xe20a, "LEX71_P" },
    {0xe20b, "LEX71_SP"},
    {0xe20c, "KEY71"   },
    {0xe20d, "KEY71_S" },
    {0xe214, "BAS71"   },
    {0xe215, "BAS71_S" },
    {0xe216, "BAS71_P" },
    {0xe217, "BAS71_SP"},
    {0xe218, "FTH71"   },
    {0xe219, "FTH71_S" },
    {0xe21a, "FTH71_P" },
    {0xe21b, "FTH71_SP"},
    {0xe21c, "ROM71"   },
    {0xe222, "GRA71"   },
    {0xe224, "ADR71"   },
    {0xe22e, "SYM71"   },
    {0xe942, "SYS9k"   },
    {0xe946, "HP-UX"   },
    {0xe950, "BAS9k"   },
    {0xe961, "BDA9k"   },
    {0xe971, "BIN9k"   },
    {0xea0a, "DTA9k"   },
    {0xea32, "COD9k"   },
    {0xea3e, "TXT9k"   },
};

static std::map<std::string, uint16_t> fileTypeToNumber =
    reverseMap(numberToFileType);

static void trimZeros(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), 0), s.end());
}

class LifFilesystem : public Filesystem
{
    class LifDirent : public Dirent
    {
    public:
        LifDirent(const LifProto& config, Bytes& bytes)
        {
            file_type = TYPE_FILE;

            ByteReader br(bytes);
            filename = trimWhitespace(br.read(10));
            uint16_t type = br.read_be16();
            location = br.read_be32();
            length = br.read_be32() * config.block_size();
            int year = unbcd(br.read_8());
            int month = unbcd(br.read_8()) + 1;
            int day = unbcd(br.read_8());
            int hour = unbcd(br.read_8());
            int minute = unbcd(br.read_8());
            int second = unbcd(br.read_8());
            uint16_t volume = br.read_be16();
            uint16_t protection = br.read_be16();
            uint16_t recordSize = br.read_be16();

            if (year >= 70)
                year += 1900;
            else
                year += 2000;

            std::tm tm = {.tm_sec = second,
                .tm_min = minute,
                .tm_hour = hour,
                .tm_mday = day,
                .tm_mon = month,
                .tm_year = year - 1900,
                .tm_isdst = -1};
            std::stringstream ss;
            ss << std::put_time(&tm, "%FT%T%z");
            ctime = ss.str();

            auto it = numberToFileType.find(type);
            if (it != numberToFileType.end())
                mode = it->second;
            else
                mode = fmt::format("0x{:04x}", type);

            path = {filename};

            attributes[Filesystem::FILENAME] = filename;
            attributes[Filesystem::LENGTH] = std::to_string(length);
            attributes[Filesystem::FILE_TYPE] = "file";
            attributes[Filesystem::MODE] = mode;
            attributes["lif.ctime"] = ctime;
            attributes["lif.volume"] = std::to_string(volume & 0x7fff);
            attributes["lif.protection"] = fmt::format("0x{:x}", protection);
            attributes["lif.record_size"] = std::to_string(recordSize);
        }

    public:
        uint32_t location;
        std::string ctime;
    };

public:
    LifFilesystem(
        const LifProto& config, std::shared_ptr<SectorInterface> sectors):
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

        attributes[VOLUME_NAME] = _volumeLabel;
        attributes[TOTAL_BLOCKS] = std::to_string(_totalBlocks);
        attributes[USED_BLOCKS] = std::to_string(_usedBlocks);
        attributes[BLOCK_SIZE] = std::to_string(_config.block_size());
        attributes["lif.directory_block"] = std::to_string(_directoryBlock);
        attributes["lif.directory_size"] = std::to_string(_directorySize);
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

        return getLifBlock(
            dirent->location, dirent->length / _config.block_size());
    }

private:
    void mount()
    {
        _sectorSize = getLogicalSectorSize();
        _sectorsPerBlock = _sectorSize / _config.block_size();

        _rootBlock = getLifBlock(0);

        ByteReader rbr(_rootBlock);
        if (rbr.read_be16() != 0x8000)
            throw BadFilesystemException();
        _volumeLabel = trimWhitespace(rbr.read(6));
        _directoryBlock = rbr.read_be32();
        rbr.skip(4);
        _directorySize = rbr.read_be32();
        rbr.skip(4);
        unsigned tracks = rbr.read_be32();
        unsigned heads = rbr.read_be32();
        unsigned sectors = rbr.read_be32();
        _usedBlocks = 1 + _directorySize;

        Bytes directory = getLifBlock(_directoryBlock, _directorySize);

        _dirents.clear();
        ByteReader br(directory);
        while (!br.eof())
        {
            Bytes direntBytes = br.read(32);
            if (direntBytes[0] != 0xff)
            {
                auto dirent = std::make_unique<LifDirent>(_config, direntBytes);
                _usedBlocks += dirent->length / _config.block_size();
                _dirents.push_back(std::move(dirent));
            }
        }
        _totalBlocks = std::max(tracks * heads * sectors, _usedBlocks);
    }

    std::shared_ptr<LifDirent> findFile(const std::string filename)
    {
        for (const auto& dirent : _dirents)
        {
            if (dirent->filename == filename)
                return dirent;
        }

        throw FileNotFoundException();
    }

    Bytes getLifBlock(uint32_t number, uint32_t count)
    {
        Bytes b;
        ByteWriter bw(b);

        while (count)
        {
            bw += getLifBlock(number);
            number++;
            count--;
        }

        return b;
    }

    Bytes getLifBlock(uint32_t number)
    {
        /* LIF uses 256-byte blocks, but the underlying format can have much
         * bigger sectors. */

        Bytes sector = getLogicalSector(number / _sectorsPerBlock);
        unsigned offset = number % _sectorsPerBlock;
        return sector.slice(
            offset * _config.block_size(), _config.block_size());
    }

private:
    const LifProto& _config;
    int _sectorSize;
    int _sectorsPerBlock;
    Bytes _rootBlock;
    std::string _volumeLabel;
    unsigned _directoryBlock;
    unsigned _directorySize;
    unsigned _totalBlocks;
    unsigned _usedBlocks;
    std::vector<std::shared_ptr<LifDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createLifFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<LifFilesystem>(config.lif(), sectors);
}
