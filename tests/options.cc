#include "lib/globals.h"
#include "lib/bytes.h"
#include "tests/testproto.pb.h"
#include "lib/config.pb.h"
#include "lib/proto.h"
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

static void test_option_validity()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			tpi: 96
		}

        option {
            name: "option1"

            prerequisite: {
                key: "drive.tpi"
                value: "96"
            }
        }

        option {
            name: "option2"

            prerequisite: {
                key: "drive.tpi"
                value: "95"
            }
        }

        option {
            name: "option3"

            prerequisite: {
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
    try
    {
        test_option_validity();
        return 0;
    }
    catch (const ErrorException& e)
    {
        fmt::print(stderr, "Error: {}\n", e.message);
        return 1;
    }
}
