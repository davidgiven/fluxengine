#include "lib/core/globals.h"
#include "fmt/format.h"
#include <fstream>

static std::fstream inputFile;

void syntax()
{
    std::cout << "Syntax: brother240tool <image>\n"
                 "The disk image will be flipped from Brother to DOS format "
                 "and back\n"
                 "again.\n";
    exit(0);
}

uint8_t getbyte(uint32_t offset)
{
    inputFile.seekg(offset, std::ifstream::beg);
    return inputFile.get();
}

void putbyte(uint32_t offset, uint8_t value)
{
    inputFile.seekp(offset, std::ifstream::beg);
    inputFile.put(value);
}

int main(int argc, const char* argv[])
{
    try
    {
        if (argc < 2)
            syntax();

        inputFile.open(
            argv[1], std::ios::in | std::ios::out | std::ios::binary);
        if (!inputFile.is_open())
            error("cannot open input file '{}'", argv[1]);

        uint8_t b1 = getbyte(0x015);
        uint8_t b2 = getbyte(0x100);
        if ((b1 == 0x58) && (b2 == 0x58))
        {
            std::cerr << "Flipping from Brother to DOS.\n";
            putbyte(0x015, 0xf0);
            putbyte(0x100, 0xf0);
        }
        else if ((b1 == 0xf0) && (b2 == 0xf0))
        {
            std::cerr << "Flipping from DOS to Brother.\n";
            putbyte(0x015, 0x58);
            putbyte(0x100, 0x58);
        }
        else
            error("Unknown image format.");

        inputFile.close();
        return 0;
    }
    catch (const ErrorException& e)
    {
        e.print();
        exit(1);
    }
}
