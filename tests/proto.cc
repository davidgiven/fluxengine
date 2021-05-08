#include "globals.h"
#include "bytes.h"
#include "lib/config.pb.h"
#include "proto.h"
#include <google/protobuf/text_format.h>
#include <assert.h>
#include <regex>

static void test_proto(void)
{
	Config config;
	setProtoByString(&config, "file.filename", "foo");

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	s = std::regex_replace(s, std::regex("[ \t\n]+"), " ");
	assert(s == "file { filename: \"foo\" } ");
}

int main(int argc, const char* argv[])
{
	test_proto();
    return 0;
}


