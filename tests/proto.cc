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
	setProtoByString(&config, "firstoption.s", "1");
	setProtoByString(&config, "secondoption.s", "2");

	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	s = cleanup(s);
	AssertThat(s, Equals("i64: -1 i32: -2 u64: 3 u32: 4 d: 5.5 m { s: \"string\" } r { s: \"val2\" } secondoption { s: \"2\" }"));
}

static void test_config(void)
{
	ConfigProto config;

	const std::string text = R"M(
		input {
			file {
				filename: "filename"
			}
		}

		output {
			disk {
				drive { }
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
	extern std::string testproto_pb();

	TestProto proto;
	bool r = proto.ParseFromString(testproto_pb());

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
		RangeProto r;
		r.set_start(0);
		r.set_end(3);
		r.add_also(5);

		AssertThat(iterate(r), Equals(std::set<unsigned>{0, 1, 2, 3, 5}));
	}

	{
		RangeProto r;
		r.set_start(1);
		r.set_end(1);
		r.add_also(5);

		AssertThat(iterate(r), Equals(std::set<unsigned>{1, 5}));
	}
}

static void test_fields(void)
{
	TestProto proto;
	auto fields = findAllProtoFields(&proto);
	AssertThat(fields, HasLength(13));
}

static void test_options(void)
{
	TestProto proto;
	const auto* descriptor = proto.descriptor();
	const auto* field = descriptor->FindFieldByName("i64");
	const auto& options = field->options();
	const std::string s = options.GetExtension(help);
	AssertThat(s, Equals("i64"));
}

int main(int argc, const char* argv[])
{
	test_setting();
	test_config();
	test_load();
	test_range();
	test_fields();
	test_options();
    return 0;
}


