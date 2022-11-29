#ifndef FLX_H
#define FLX_H

#define FLX_TICK_NS 40 /* ns per tick */

/* Special FLX opcodes */

enum
{
    FLX_INDEX = 0x08,
    FLX_STOP = 0x0d
};

extern std::unique_ptr<Fluxmap> readFlxBytes(const Bytes& bytes);

#endif
