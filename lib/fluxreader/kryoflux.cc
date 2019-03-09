#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "protocol.h"
#include "fmt/format.h"
#include <fstream>
#include <glob.h>

#define SCLK_HZ 24027428.57142857
#define TICKS_PER_SCLK (TICK_FREQUENCY / SCLK_HZ)

std::unique_ptr<Fluxmap> readStream(const std::string& dir, unsigned track, unsigned side)
{
    std::string suffix = fmt::format("{:02}.{}.raw", track, side);
    std::string pattern = fmt::format("{}*{}", dir, suffix);
    glob_t globdata;
    if (glob(pattern.c_str(), GLOB_NOSORT, NULL, &globdata))
        Error() << fmt::format("cannot access path '{}'", dir);
    if (globdata.gl_pathc != 1)
        Error() << fmt::format("data is ambiguous --- multiple files end in {}", suffix);
    std::string filename = globdata.gl_pathv[0];
    globfree(&globdata);

    return readStream(filename);
}

std::unique_ptr<Fluxmap> readStream(const std::string& filename)
{
    std::ifstream f(filename, std::ios::in | std::ios::binary);
    if (!f.is_open())
		Error() << fmt::format("cannot open input file '{}'", filename);

    return readStream(f);
}

std::unique_ptr<Fluxmap> readStream(std::istream& f)
{
    std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
    auto writeFlux = [&](uint32_t sclk)
    {
        int ticks = (double)sclk * TICKS_PER_SCLK;
        fluxmap->appendInterval(ticks);
    };

    uint32_t extrasclks = 0;
    for (;;)
    {
        int b = f.get(); /* returns -1 or UNSIGNED char */
        if (b == -1)
            break;

        switch (b)
        {
            case 0x0d: /* OOB block */
            {
                int blocktype = f.get();
                (void) blocktype;
                uint16_t blocklen = f.get() | (f.get()<<8);
                if (f.fail() || f.eof())
                    goto finished;

                while (blocklen--)
                    f.get();
                break;
            }

            default:
            {
                if ((b >= 0x00) && (b <= 0x07))
                {
                    /* Flux2: double byte value */
                    b = (b<<8) | f.get();
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
                    f.get();
                }
                else if (b == 0x0a)
                {
                    /* Nop3: skip two bytes */
                    f.get();
                    f.get();
                }
                else if (b == 0x0b)
                {
                    /* Ovl16: the next block is 0x10000 sclks longer than normal. */
                    extrasclks += 0x10000;
                }
                else if (b == 0x0c)
                {
                    /* Flux3: triple byte value */
                    int ticks = f.get() << 8;
                    ticks |= f.get();
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
                    Error() << fmt::format(
                        "unknown stream block byte 0x{:02x} at 0x{:08x}", b, (uint64_t)f.tellg()-1);
            }
        }
    }

finished:
    if (!f.eof())
        Error() << "I/O error reading stream";
    return fluxmap;
}
