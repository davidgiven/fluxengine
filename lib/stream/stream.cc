#include "globals.h"
#include "fluxmap.h"
#include "stream.h"
#include "protocol.h"
#include "fmt/format.h"
#include <fstream>
#include <glob.h>

#define SCLK_HZ 24027428.57142857
#define TICKS_PER_SCLK (TICK_FREQUENCY / SCLK_HZ)

std::unique_ptr<Fluxmap> readStream(const std::string& path, unsigned track, unsigned side)
{
    std::string suffix = fmt::format("{:02}.{}.raw", track, side);
    std::string pattern = fmt::format("{}*{}", path, suffix);
    glob_t globdata;
    if (glob(pattern.c_str(), GLOB_NOSORT, NULL, &globdata))
        Error() << fmt::format("cannot access path '{}'", path);
    if (globdata.gl_pathc != 1)
        Error() << fmt::format("data is ambiguous --- multiple files end in {}", suffix);
    std::string filename = globdata.gl_pathv[0];
    globfree(&globdata);

    std::ifstream f(filename, std::ios::in | std::ios::binary);
    if (!f.is_open())
		Error() << fmt::format("cannot open input file '{}'", filename);

    std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
    auto writeFlux = [&](uint32_t sclk)
    {
        int ticks = (double)sclk * TICKS_PER_SCLK;
        while (ticks >= 0x100)
        {
            fluxmap->appendInterval(0);
            ticks -= 0x100;
        }
        fluxmap->appendInterval((uint8_t)ticks);
    };

    for (;;)
    {
        int b = f.get(); /* returns -1 or UNSIGNED char */
        if (b == -1)
            break;
        uint64_t here = f.tellg();

        switch (b)
        {
            case 0x0d: /* OOB block */
            {
                int blocktype = f.get();
                int blocklen = f.get() | (f.get()<<8);
                if (f.fail() || f.eof())
                    goto finished;

                f.seekg(here + blocklen, std::ios_base::beg);
                break;
            }

            default:
            {
                if ((b >= 0x00) && (b <= 0x07))
                {
                    /* Flux2: double byte value */
                    b = (b<<8) | f.get();
                    writeFlux(b);
                }
                else if (b == 0x08)
                {
                    /* Nop1: do nothing */
                }
                else if (b == 0x09)
                {
                    /* Nop2: skip one byte */
                    f.seekg(1, std::ios_base::cur);
                }
                else if (b == 0x0a)
                {
                    /* Nop3: skip two bytes */
                    f.seekg(2, std::ios_base::cur);
                }
                else if (b == 0x0b)
                {   /* Ovl16: the next block is 0x10000 sclks longer than normal.
                     * FluxEngine can't handle long transitions, and implementing
                     * this is complicated, so we just bodge it.
                     */
                    writeFlux(0x10000);
                }
                else if (b == 0x0c)
                {
                    /* Flux3: triple byte value */
                    int ticks = f.get() << 8;
                    ticks |= f.get();
                    writeFlux(ticks);
                }
                else if ((b >= 0x0e) && (b <= 0xff))
                {
                    /* Flux1: single byte value */
                    writeFlux(b);
                }
                else
                    Error() << fmt::format(
                        "unknown stream block byte 0x{:02x} at 0x{:08x}", b, here);
            }
        }
    }

finished:
    if (!f.eof())
        Error() << fmt::format("I/O error reading '{}'", filename);
    return fluxmap;
}
