#include <stdio.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fstream>
#include "fmt/format.h"
#include "tests/testproto.pb.h"
#include "lib/config/config.pb.h"
#include <sstream>
#include <locale>

#define STRINGIFY(s) #s

static uint32_t readu8(std::string::iterator& it, std::string::iterator end)
{
    int len;
    uint32_t c = *it++;
    if (c < 0x80)
    {
        /* Do nothing! */
        len = 0;
    }
    else if (c < 0xc0)
    {
        /* Invalid character */
        c = -1;
        len = 0;
    }
    else if (c < 0xe0)
    {
        /* One trailing byte */
        c &= 0x1f;
        len = 1;
    }
    else if (c < 0xf0)
    {
        /* Two trailing bytes */
        c &= 0x0f;
        len = 2;
    }
    else if (c < 0xf8)
    {
        /* Three trailing bytes */
        c &= 0x07;
        len = 3;
    }
    else if (c < 0xfc)
    {
        /* Four trailing bytes */
        c &= 0x03;
        len = 4;
    }
    else
    {
        /* Five trailing bytes */
        c &= 0x01;
        len = 5;
    }

    while (len)
    {
        if (it == end)
            break;

        uint8_t d = *it++;
        c <<= 6;
        c += d & 0x3f;
    }
    return c;
}

int main(int argc, const char* argv[])
{
    PROTO message;

    std::ifstream input(argv[1]);
    if (!input)
    {
        perror("couldn't open input file");
        exit(1);
    }

    std::stringstream ss;
    std::string s;
    while (std::getline(input, s, '\n'))
    {
        if (s == "<<<")
        {
            while (std::getline(input, s, '\n'))
            {
                if (s == ">>>")
                    break;

                ss << '"';
                auto it = s.begin();
                for (;;)
                {
                    uint32_t u = readu8(it, s.end());
                    if (!u)
                        break;

                    ss << fmt::format("\\u{:04x}", u);
                }
                ss << "\\n\"\n";
            }
        }
        else
            ss << s << '\n';
    }

    if (!google::protobuf::TextFormat::ParseFromString(ss.str(), &message))
    {
        fprintf(stderr, "cannot parse text proto: %s\n", argv[1]);
        exit(1);
    }

    std::ofstream output(argv[2]);
    if (!output)
    {
        perror("couldn't open output file");
        exit(1);
    }

    auto data = message.SerializeAsString();
    auto name = argv[3];

    output << "#include \"lib/core/globals.h\"\n"
           << "#include \"lib/config/proto.h\"\n"
           << "#include <string_view>\n"
           << "static const uint8_t " << name << "_rawData[] = {";

    int count = 0;
    for (char c : data)
    {
        if (count == 0)
            output << "\n\t";
        else
            output << ' ';
        output << fmt::format("0x{:02x},", (unsigned char)c);

        count = (count + 1) & 7;
    }

    output << "\n};\n";
    output << "extern const std::string_view " << name << "_data;\n";
    output << "const std::string_view " << name
           << "_data = std::string_view((const char*)" << name << "_rawData, " << data.size()
           << ");\n";
    output << "extern const ConfigProto " << name << ";\n";
    output << "const ConfigProto " << name << " = parseConfigBytes("
           << argv[3] << "_data);\n";

    return 0;
}
