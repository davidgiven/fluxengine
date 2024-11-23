#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/config/config.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
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

static void test_physical_sectors()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_80TRACK
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
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_80TRACK
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
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_80TRACK
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

static void test_skew()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_80TRACK
			tracks: 78
			sides: 2
			layoutdata {
				sector_size: 256
				physical {
					start_sector: 0
					count: 12
					skew: 6
				}
			}
		}
	)M");

    auto layout = Layout::getLayoutOfTrack(0, 0);
    AssertThat(layout->naturalSectorOrder,
        Equals(std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
    AssertThat(layout->diskSectorOrder,
        Equals(std::vector<unsigned>{0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11}));
}

int main(int argc, const char* argv[])
{
    test_physical_sectors();
    test_logical_sectors();
    test_both_sectors();
    test_skew();
}
