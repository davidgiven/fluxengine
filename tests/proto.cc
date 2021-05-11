#include "globals.h"
#include "bytes.h"
#include "tests/testproto.pb.h"
#include "lib/config.pb.h"
#include "proto.h"
#include "snowhouse/snowhouse.h"
#include <google/protobuf/text_format.h>
#include <assert.h>
#include <regex>

using namespace snowhouse;

static std::string cleanup(const std::string& s)
{
	auto outs = std::regex_replace(s, std::regex("[ \t\n]+"), " ");
	outs = std::regex_replace(outs, std::regex("^[ \t\n]+"), "");
	outs = std::regex_replace(outs, std::regex("[ \t\n]+$"), "");
	return outs;
}

static void test_setting(void)
{
	TestProto config;
	setProtoByString(&config, "i64", "-1");
	setProtoByString(&config, "i32", "-2");
	setProtoByString(&config, "u64", "3");
	setProtoByString(&config, "u32", "4");
	setProtoByString(&config, "d", "5.5");
	setProtoByString(&config, "m.s", "string");
	setProtoByString(&config, "r.s", "val1");
	setProtoByString(&config, "r.s", "val2");

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	s = cleanup(s);
	AssertThat(s, Equals("i64: -1 i32: -2 u64: 3 u32: 4 d: 5.5 m { s: \"string\" } r { s: \"val2\" }"));
}

static void test_config(void)
{
	Config config;

	const std::string text = R"M(
		input {
			file {
				filename: "filename"
			}
		}

		output {
			disk {
				drive: 0
				ibm {
				}
			}
		}
	)M";
	google::protobuf::TextFormat::MergeFromString(text, &config);

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	AssertThat(cleanup(s), Equals(cleanup(text)));
}

static void test_load(void)
{
	extern const char testproto_pb[];
	extern const int testproto_pb_size;

	TestProto proto;
	bool r = proto.ParseFromString(std::string(testproto_pb, testproto_pb_size));

	std::string s;
	google::protobuf::TextFormat::PrintToString(proto, &s);
	s = cleanup(s);
	AssertThat(s, Equals("u64: 42 r { } secondoption { }"));
	AssertThat(r, Equals(true));
	AssertThat(proto.has_secondoption(), Equals(true));
}

static void test_range(void)
{
	{
		Range r;
		r.set_start(0);
		r.set_end(3);
		r.add_also(5);

		AssertThat(iterate(r), Equals(std::set<int>{0, 1, 2, 3, 5}));
	}

	{
		Range r;
		r.set_start(1);
		r.set_end(1);
		r.add_also(5);

		AssertThat(iterate(r), Equals(std::set<int>{1, 5}));
	}
}

int main(int argc, const char* argv[])
{
	test_setting();
	test_config();
	test_load();
	test_range();
    return 0;
}


