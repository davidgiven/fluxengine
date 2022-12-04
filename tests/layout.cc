#include "lib/globals.h"
#include "lib/bytes.h"
#include "lib/config.pb.h"
#include "lib/proto.h"
#include "lib/layout.h"
#include "snowhouse/snowhouse.h"
#include <google/protobuf/text_format.h>
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
    config.Clear();
    if (!google::protobuf::TextFormat::MergeFromString(cleanup(s), &config))
        Error() << "couldn't load test config";
}

static void test_physical_sectors()
{
    load_config(R"M(
		layout {
			tracks: 78
			sides: 2
			layoutdata {
				sector_size: 256
				physical {
					sector: 0
					sector: 2
					sector: 1
					sector: 3
				}
			}
		}
	)M");

    auto layout = Layout::getLayoutOfTrack(0, 0);
    AssertThat(
        layout->naturalSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
    AssertThat(
        layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 2, 1, 3}));
    AssertThat(layout->filesystemSectorOrder,
        Equals(std::vector<unsigned>{0, 1, 2, 3}));
}

static void test_logical_sectors()
{
    load_config(R"M(
		layout {
			tracks: 78
			sides: 2
			layoutdata {
				sector_size: 256
				physical {
					sector: 0
					sector: 1
					sector: 2
					sector: 3
				}
				filesystem {
					sector: 0
					sector: 2
					sector: 1
					sector: 3
				}
			}
		}
	)M");

    auto layout = Layout::getLayoutOfTrack(0, 0);
    AssertThat(
        layout->naturalSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
    AssertThat(
        layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
    AssertThat(layout->filesystemSectorOrder,
        Equals(std::vector<unsigned>{0, 2, 1, 3}));
}

static void test_both_sectors()
{
    load_config(R"M(
		layout {
			tracks: 78
			sides: 2
			layoutdata {
				sector_size: 256
				physical {
					sector: 3
					sector: 2
					sector: 1
					sector: 0
				}
				filesystem {
					sector: 0
					sector: 2
					sector: 1
					sector: 3
				}
			}
		}
	)M");

    auto layout = Layout::getLayoutOfTrack(0, 0);
    AssertThat(
        layout->naturalSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
    AssertThat(
        layout->diskSectorOrder, Equals(std::vector<unsigned>{3, 2, 1, 0}));
    AssertThat(layout->filesystemSectorOrder,
        Equals(std::vector<unsigned>{0, 2, 1, 3}));
}

int main(int argc, const char* argv[])
{
    test_physical_sectors();
    test_logical_sectors();
    test_both_sectors();
}
