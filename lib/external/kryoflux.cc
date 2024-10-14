#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/kryoflux.h"
#include "protocol.h"
#include <fstream>
#include <sys/types.h>
#include <dirent.h>
#include <filesystem>

#define MCLK_HZ (((18432000.0 * 73.0) / 14.0) / 2.0)
#define SCLK_HZ (MCLK_HZ / 2)
#define ICLK_HZ (MCLK_HZ / 16)

#define TICKS_PER_SCLK (TICK_FREQUENCY / SCLK_HZ)

static bool has_suffix(const std::string& haystack, const std::string& needle)
{
    if (needle.length() > haystack.length())
        return false;

    return haystack.compare(haystack.length() - needle.length(),
               needle.length(),
               needle) == 0;
}

std::unique_ptr<Fluxmap> readStream(
    std::string dir, unsigned track, unsigned side)
{
    std::string suffix = fmt::format("{:02}.{}.raw", track, side);

    FILE* fp = fopen(dir.c_str(), "r");
    if (fp)
    {
        fclose(fp);
        int i = dir.find_last_of("/\\");
        dir = dir.substr(0, i);
    }

    DIR* dirp = opendir(dir.c_str());
    if (!dirp)
        error("cannot access path '{}'", dir);

    std::string filename;
    for (;;)
    {
        struct dirent* de = readdir(dirp);
        if (!de)
            break;

        if (has_suffix(de->d_name, suffix))
        {
            if (!filename.empty())
                error("data is ambiguous --- multiple files end in {}", suffix);
            filename = fmt::format("{}/{}", dir, de->d_name);
        }
    }
    closedir(dirp);

    if (filename.empty())
        error("failed to find track {} side {} in {}", track, side, dir);

    return readStream(filename);
}

std::unique_ptr<Fluxmap> readStream(const std::string& filename)
{
    std::ifstream f(filename, std::ios::in | std::ios::binary);
    if (!f.is_open())
        error("cannot open input file '{}'", filename);

    Bytes bytes;
    ByteWriter bw(bytes);
    bw.append(f);

    return readStream(bytes);
}

std::unique_ptr<Fluxmap> readStream(const Bytes& bytes)
{
    ByteReader br(bytes);

    /* Pass 1: scan the stream looking for index marks. */

    std::set<uint32_t> indexmarks;
    br.seek(0);
    while (!br.eof())
    {
        uint8_t b = br.read_8();
        unsigned len = 0;
        switch (b)
        {
            case 0x0d: /* OOB block */
            {
                int blocktype = br.read_8();
                len = br.read_le16();
                if (br.eof())
                    goto finished_pass_1;

                if (blocktype == 0x02)
                {
                    /* index data, sent asynchronously */
                    uint32_t streampos = br.read_le32();
                    indexmarks.insert(streampos);
                    len -= 4;
                }
                break;
            }

            default:
            {
                if ((b >= 0x00) && (b <= 0x07))
                {
                    /* Flux2: double byte value */
                    len = 1;
                }
                else if (b == 0x08)
                {
                    /* Nop1: do nothing */
                    len = 0;
                }
                else if (b == 0x09)
                {
                    /* Nop2: skip one byte */
                    len = 1;
                }
                else if (b == 0x0a)
                {
                    /* Nop3: skip two bytes */
                    len = 2;
                }
                else if (b == 0x0b)
                {
                    /* Ovl16: the next block is 0x10000 sclks longer than
                     * normal. */
                    len = 0;
                }
                else if (b == 0x0c)
                {
                    /* Flux3: triple byte value */
                    len = 2;
                }
                else if ((b >= 0x0e) && (b <= 0xff))
                {
                    /* Flux1: single byte value */
                    len = 0;
                }
                else
                    error("unknown stream block byte 0x{:02x} at 0x{:08x}",
                        b,
                        (uint64_t)br.pos - 1);
            }
        }

        br.skip(len);
    }
finished_pass_1:

    /* Pass 2: actually read the data. */

    std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
    int streamdelta = 0;
    auto writeFlux = [&](uint32_t sclk)
    {
        const auto& nextindex = indexmarks.begin();
        if (nextindex != indexmarks.end())
        {
            uint32_t nextindexpos = *nextindex + streamdelta;
            if (br.pos >= nextindexpos)
            {
                fluxmap->appendIndex();
                indexmarks.erase(nextindex);
            }
        }
        int ticks = (double)sclk * TICKS_PER_SCLK;
        fluxmap->appendInterval(ticks);
        fluxmap->appendPulse();
    };

    uint32_t extrasclks = 0;
    br.seek(0);
    while (!br.eof())
    {
        unsigned b = br.read_8();
        switch (b)
        {
            case 0x0d: /* OOB block */
            {
                int blocktype = br.read_8();
                uint16_t blocklen = br.read_le16();
                if (br.eof())
                    goto finished_pass_2;

                switch (blocktype)
                {
                    case 0x01: /* streaminfo */
                    {
                        uint32_t blockpos = br.pos - 3;
                        streamdelta = blockpos - br.read_le32();
                        blocklen -= 4;
                        break;
                    }
                }

                br.skip(blocklen);
                break;
            }

            default:
            {
                if ((b >= 0x00) && (b <= 0x07))
                {
                    /* Flux2: double byte value */
                    b = (b << 8) | br.read_8();
                    writeFlux(extrasclks + b);
                    extrasclks = 0;
                }
                else if (b == 0x08)
                {
                    /* Nop1: do nothing */
                }
                else if (b == 0x09)
                {
                    /* Nop2: skip one byte */
                    br.skip(1);
                }
                else if (b == 0x0a)
                {
                    /* Nop3: skip two bytes */
                    br.skip(2);
                }
                else if (b == 0x0b)
                {
                    /* Ovl16: the next block is 0x10000 sclks longer than
                     * normal. */
                    extrasclks += 0x10000;
                }
                else if (b == 0x0c)
                {
                    /* Flux3: triple byte value */
                    int ticks = br.read_be16(); /* yes, really big-endian */
                    writeFlux(extrasclks + ticks);
                    extrasclks = 0;
                }
                else if ((b >= 0x0e) && (b <= 0xff))
                {
                    /* Flux1: single byte value */
                    writeFlux(extrasclks + b);
                    extrasclks = 0;
                }
                else
                    error("unknown stream block byte 0x{:02x} at 0x{:08x}",
                        b,
                        (uint64_t)br.pos - 1);
            }
        }
    }

finished_pass_2:
    if (!br.eof())
        error("I/O error reading stream");
    return fluxmap;
}
