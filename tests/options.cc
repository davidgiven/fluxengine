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

static void load_config(const std::string s)
{
    globalConfig().clear();
    if (!google::protobuf::TextFormat::MergeFromString(
            cleanup(s), globalConfig()))
        error("couldn't load test config");
}

static void test_option_validity()
{
    load_config(R"M(
		drive {
			tpi: 96
		}

        option {
            name: "option1"

            requires: {
                key: "drive.tpi"
                value: "96"
            }
        }

        option {
            name: "option2"

            requires: {
                key: "drive.tpi"
                value: "95"
            }
        }

        option {
            name: "option3"

            requires: {
                key: "drive.tpi"
                value: ["0", "96"]
            }
        }
	)M");

    AssertThat(
        globalConfig().isOptionValid(globalConfig().findOption("option1")),
        Equals(true));
    AssertThat(
        globalConfig().isOptionValid(globalConfig().findOption("option2")),
        Equals(false));
    AssertThat(
        globalConfig().isOptionValid(globalConfig().findOption("option3")),
        Equals(true));
}

int main(int argc, const char* argv[])
{
    test_option_validity();
    return 0;
}
