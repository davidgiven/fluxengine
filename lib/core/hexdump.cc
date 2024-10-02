#include "lib/core/globals.h"
#include "lib/core/bytes.h"

void hexdump(std::ostream& stream, const Bytes& buffer)
{
    size_t pos = 0;

    while (pos < buffer.size())
    {
        stream << fmt::format("{:05x} : ", pos);
        for (int i = 0; i < 16; i++)
        {
            if ((pos + i) < buffer.size())
                stream << fmt::format("{:02x} ", buffer[pos + i]);
            else
                stream << "-- ";
        }
        stream << " : ";
        for (int i = 0; i < 16; i++)
        {
            if ((pos + i) >= buffer.size())
                break;

            uint8_t c = buffer[pos + i];
            if ((c >= 32) && (c <= 126))
                stream << (char)c;
            else
                stream << '.';
        }
        stream << std::endl;

        pos += 16;
    }
}

void hexdumpForSrp16(std::ostream& stream, const Bytes& buffer)
{
    for (uint8_t byte : buffer)
        stream << fmt::format("{:02x}", byte);
    stream << std::endl;
}
