#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/config/config.h"
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

static void test_option_validity()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

        option {
            name: "option1"

            prerequisite: {
                key: "drive.drive_type"
                value: "DRIVETYPE_80TRACK"
            }
        }

        option {
            name: "option2"

            prerequisite: {
                key: "drive.drive_type"
                value: "DRIVETYPE_40TRACK"
            }
        }

        option {
            name: "option3"

            prerequisite: {
                key: "drive.drive_type"
                value: ["DRIVETYPE_UNKNOWN", "DRIVETYPE_80TRACK"]
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
