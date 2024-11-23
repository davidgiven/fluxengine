#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "tests/testproto.pb.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
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
    setProtoByString(&config, "f", "6.7");
    setProtoByString(&config, "m.s", "string");
    setProtoByString(&config, "r.s", "val1");
    setProtoByString(&config, "r.s", "val2");
    setProtoByString(&config, "firstoption.s", "1");
    setProtoByString(&config, "secondoption.s", "2");
    setProtoByString(&config, "range", "1-3x2");

    std::string s;
    google::protobuf::TextFormat::PrintToString(config, &s);
    s = cleanup(s);
    AssertThat(s, Equals(cleanup(R"M(
		i64: -1
		i32: -2
		u64: 3
		u32: 4
		d: 5.5
		m {
			s: "string"
		}
		r {
			s: "val2"
		}
		secondoption {
			s: "2"
		}
		range {
			start: 1
			step: 2
			end: 3
		}
		f: 6.7
		)M")));
}

static void test_getting(void)
{
    std::string s = R"M(
		i64: -1
		i32: -2
		u64: 3
		u32: 4
		d: 5.5
		f: 6.7
		m {
			s: "string"
		}
		r {
			s: "val2"
		}
		secondoption {
			s: "2"
		}
		range {
			start: 1
			step: 2
			end: 3
		}
	)M";

    TestProto tp;
    if (!google::protobuf::TextFormat::MergeFromString(cleanup(s), &tp))
        error("couldn't load test proto");

    AssertThat(getProtoByString(&tp, "i64"), Equals("-1"));
    AssertThat(getProtoByString(&tp, "i32"), Equals("-2"));
    AssertThat(getProtoByString(&tp, "u64"), Equals("3"));
    AssertThat(getProtoByString(&tp, "u32"), Equals("4"));
    AssertThat(getProtoByString(&tp, "d"), Equals("5.5"));
    AssertThat(getProtoByString(&tp, "f"), Equals("6.7"));
    AssertThat(getProtoByString(&tp, "m.s"), Equals("string"));
    AssertThat(getProtoByString(&tp, "r.s"), Equals("val2"));
    AssertThrows(
        ProtoPathNotFoundException, getProtoByString(&tp, "firstoption.s"));
    AssertThat(getProtoByString(&tp, "secondoption.s"), Equals("2"));
    AssertThat(getProtoByString(&tp, "range"), Equals("1-3x2"));
}

static void test_config(void)
{
    ConfigProto config;

    const std::string text = R"M(
		image_reader {
			filename: "filename"
		}

		flux_sink {
			drive { }
		}
	)M";
    google::protobuf::TextFormat::MergeFromString(text, &config);

    std::string s;
    google::protobuf::TextFormat::PrintToString(config, &s);
    AssertThat(cleanup(s), Equals(cleanup(text)));
}

static void test_load(void)
{
    extern std::string_view testproto_pb_data;

    TestProto proto;
    bool r = proto.ParseFromArray(
        testproto_pb_data.begin(), testproto_pb_data.size());

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

        AssertThat(iterate(r), Equals(std::set<unsigned>{0, 1, 2, 3}));
    }

    {
        RangeProto r;
        r.set_start(0);
        r.set_end(3);
        r.set_step(2);

        AssertThat(iterate(r), Equals(std::set<unsigned>{0, 2}));
    }

    {
        RangeProto r;
        r.set_start(1);
        r.set_end(1);

        AssertThat(iterate(r), Equals(std::set<unsigned>{1}));
    }

    {
        RangeProto r;
        r.set_start(1);

        AssertThat(iterate(r), Equals(std::set<unsigned>{1}));
    }

    {
        RangeProto r;
        setRange(&r, "1-3");

        AssertThat(iterate(r), Equals(std::set<unsigned>{1, 2, 3}));
    }

    {
        RangeProto r;
        setRange(&r, "0-3x2");

        AssertThat(iterate(r), Equals(std::set<unsigned>{0, 2}));
    }

    {
        RangeProto r;
        setRange(&r, "0");

        AssertThat(iterate(r), Equals(std::set<unsigned>{0}));
    }

    {
        RangeProto r;
        setRange(&r, "7");

        AssertThat(iterate(r), Equals(std::set<unsigned>{7}));
    }
}

static void test_fields(void)
{
    TestProto proto;
    auto fields = findAllProtoFields(&proto);
    AssertThat(fields.size(), Equals(18));
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
    try
    {
        test_setting();
        test_getting();
        test_config();
        test_load();
        test_range();
        test_fields();
        test_options();
    }
    catch (const ErrorException& e)
    {
        fmt::print("uncaught error: {}\n", e.message);
        return 1;
    }
    return 0;
}
