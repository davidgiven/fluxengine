#include "lib/core/globals.h"
#include "protocol.h"
#include "lib/core/bytes.h"
#include "greaseweazle.h"

Bytes fluxEngineToGreaseweazle(const Bytes& fldata, nanoseconds_t clock)
{
    Bytes gwdata;
    ByteWriter bw(gwdata);
    ByteReader br(fldata);
    uint32_t ticks_fl = 0;
    uint32_t ticks_gw = 0;

    auto write_28 = [&](uint32_t val)
    {
        bw.write_8(1 | (val << 1) & 0xff);
        bw.write_8(1 | (val >> 6) & 0xff);
        bw.write_8(1 | (val >> 13) & 0xff);
        bw.write_8(1 | (val >> 20) & 0xff);
    };

    while (!br.eof())
    {
        uint8_t b = br.read_8();
        ticks_fl += b & 0x3f;
        if (b & F_BIT_PULSE)
        {
            uint32_t newticks_gw = ticks_fl * NS_PER_TICK / clock;
            uint32_t delta = newticks_gw - ticks_gw;
            if (delta < 250)
                bw.write_8(delta);
            else
            {
                int high = (delta - 250) / 255;
                if (high < 5)
                {
                    bw.write_8(250 + high);
                    bw.write_8(1 + (delta - 250) % 255);
                }
                else
                {
                    bw.write_8(255);
                    bw.write_8(FLUXOP_SPACE);
                    write_28(delta - 249);
                    bw.write_8(249);
                }
            }
            ticks_gw = newticks_gw;
        }
    }
    bw.write_8(0); /* end of stream */
    return gwdata;
}

Bytes greaseweazleToFluxEngine(const Bytes& gwdata, nanoseconds_t clock)
{
    Bytes fldata;
    ByteReader br(gwdata);
    ByteWriter bw(fldata);

    auto read_28 = [&]()
    {
        return ((br.read_8() & 0xfe) >> 1) | ((br.read_8() & 0xfe) << 6) |
               ((br.read_8() & 0xfe) << 13) | ((br.read_8() & 0xfe) << 20);
    };

    uint32_t ticks_gw = 0;
    uint32_t lastevent_fl = 0;
    uint32_t index_gw = ~0;

    while (!br.eof())
    {
        uint8_t b = br.read_8();
        if (!b)
            break;

        uint8_t event = 0;
        if (b == 255)
        {
            switch (br.read_8())
            {
                case FLUXOP_INDEX:
                    index_gw = ticks_gw + read_28();
                    break;

                case FLUXOP_SPACE:
                    ticks_gw += read_28();
                    break;

                default:
                    error("bad opcode in Greaseweazle stream");
            }
        }
        else
        {
            if (b < 250)
                ticks_gw += b;
            else
            {
                int delta = 250 + (b - 250) * 255 + br.read_8() - 1;
                ticks_gw += delta;
            }
            event = F_BIT_PULSE;
        }

        if (event)
        {
            uint32_t index_fl = round((index_gw * clock) / NS_PER_TICK);
            uint32_t ticks_fl = round((ticks_gw * clock) / NS_PER_TICK);
            if (index_gw != ~0)
            {
                if (index_fl < ticks_fl)
                {
                    uint32_t delta_fl = index_fl - lastevent_fl;
                    while (delta_fl > 0x3f)
                    {
                        bw.write_8(0x3f);
                        delta_fl -= 0x3f;
                    }
                    bw.write_8(delta_fl | F_BIT_INDEX);
                    lastevent_fl = index_fl;
                    index_gw = ~0;
                }
                else if (index_fl == ticks_fl)
                    event |= F_BIT_INDEX;
            }

            uint32_t delta_fl = ticks_fl - lastevent_fl;
            while (delta_fl > 0x3f)
            {
                bw.write_8(0x3f);
                delta_fl -= 0x3f;
            }
            bw.write_8(delta_fl | event);
            lastevent_fl = ticks_fl;
        }
    }

    return fldata;
}

/* Left-truncates at the first index mark, so the resulting data as aligned at
 * the index. */
Bytes stripPartialRotation(const Bytes& fldata)
{
    for (unsigned i = 0; i < fldata.size(); i++)
    {
        uint8_t b = fldata[i];
        if (b & F_BIT_INDEX)
            return fldata.slice(i);
    }
    return fldata;
}
