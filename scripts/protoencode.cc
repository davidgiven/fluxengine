#include <stdio.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fstream>
#include "fmt/format.h"
#include "lib/config.pb.h"

#define STRINGIFY(s) #s

int main(int argc, const char* argv[])
{
	ConfigProto message;

	std::ifstream input(argv[1]);
	if (!input)
	{
		perror("couldn't open input file");
		exit(1);
	}

    google::protobuf::io::IstreamInputStream istream(&input);
    if (!google::protobuf::TextFormat::Parse(&istream, &message))
    {
        fprintf(stderr, "cannot parse text proto");
        exit(1);
    }

	std::ofstream output(argv[2]);
	if (!output)
	{
		perror("couldn't open output file");
		exit(1);
	}

    auto data = message.SerializeAsString();

	output << "#include <string>\n"
	       << "static const unsigned char data[] = {\n";

	int count = 0;
	for (char c : data)
	{
        if (count == 0)
            output << "\n\t";
        else
            output << ' ';
        output << fmt::format("0x{:02x},", (unsigned char)c);

        count = (count+1) & 7;
	}

	output << "};\n"
		   << fmt::format("extern std::string {}();\n", argv[3])
		   << fmt::format("std::string {}()\n", argv[3])
		   << "{ return std::string((const char*)data, sizeof(data)); }\n";

    return 0;
}
