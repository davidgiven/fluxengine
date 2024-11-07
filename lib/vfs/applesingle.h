#ifndef APPLESINGLE_H
#define APPLESINGLE_H

class AppleSingleException
{
};

class InvalidFileException : public AppleSingleException
{
};

class AppleSingle
{
public:
    static constexpr uint32_t OVERHEAD = 0x5e;

public:
    void parse(const Bytes& combined);
    Bytes render();

public:
    Bytes data;
    Bytes rsrc;
    Bytes creator;
    Bytes type;
};

#endif
