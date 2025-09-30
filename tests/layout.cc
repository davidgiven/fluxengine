#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/config/config.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include "lib/data/locations.h"
#include "snowhouse/snowhouse.h"
#include <google/protobuf/text_format.h>
#include <regex>

using namespace snowhouse;

template <typename F, typename S>
struct snowhouse::Stringizer<std::pair<F, S>>
{
    static std::string ToString(const std::pair<F, S>& a)
    {
        std::stringstream stream;
        stream << '(' << a.first << ", " << a.second << ')';
        return stream.str();
    }
};

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

    {
        auto layout = Layout::getLayoutOfTrack(0, 0);
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 2, 1, 3}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
    }

    {
        auto diskLayout = createDiskLayout();
        auto physicalLayout = diskLayout->layoutByPhysicalLocation.at({0, 0});
        auto layout = physicalLayout->logicalTrackLayout;
        AssertThat(
            diskLayout->layoutByLogicalLocation.at({0, 0}), Equals(layout));
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 2, 1, 3}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
    }
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

    {
        auto layout = Layout::getLayoutOfTrack(0, 0);
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 2, 1, 3}));
    }

    {
        auto diskLayout = createDiskLayout();
        auto physicalLayout = diskLayout->layoutByPhysicalLocation.at({0, 0});
        auto layout = physicalLayout->logicalTrackLayout;
        AssertThat(
            diskLayout->layoutByLogicalLocation.at({0, 0}), Equals(layout));
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 2, 1, 3}));
    }
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

    {
        auto layout = Layout::getLayoutOfTrack(0, 0);
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{3, 2, 1, 0}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 2, 1, 3}));
    }

    {
        auto diskLayout = createDiskLayout();
        auto physicalLayout = diskLayout->layoutByPhysicalLocation.at({0, 0});
        auto layout = physicalLayout->logicalTrackLayout;
        AssertThat(
            diskLayout->layoutByLogicalLocation.at({0, 0}), Equals(layout));
        AssertThat(layout->naturalSectorOrder,
            Equals(std::vector<unsigned>{0, 1, 2, 3}));
        AssertThat(
            layout->diskSectorOrder, Equals(std::vector<unsigned>{3, 2, 1, 0}));
        AssertThat(layout->filesystemSectorOrder,
            Equals(std::vector<unsigned>{0, 2, 1, 3}));
    }
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

    {
        auto layout = Layout::getLayoutOfTrack(0, 0);
        AssertThat(layout->naturalSectorOrder,
            Equals(
                std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
        AssertThat(layout->diskSectorOrder,
            Equals(
                std::vector<unsigned>{0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11}));
    }

    {
        auto diskLayout = createDiskLayout();
        auto physicalLayout = diskLayout->layoutByPhysicalLocation.at({0, 0});
        auto layout = physicalLayout->logicalTrackLayout;
        AssertThat(layout->naturalSectorOrder,
            Equals(
                std::vector<unsigned>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
        AssertThat(layout->diskSectorOrder,
            Equals(
                std::vector<unsigned>{0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11}));
    }
}

static void test_bounds()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_40TRACK
			tracks: 2
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

    auto diskLayout = createDiskLayout();
    AssertThat(diskLayout->groupSize, Equals(2));
    AssertThat(diskLayout->getLogicalBounds(),
        Equals(Layout::LayoutBounds{0, 1, 0, 1}));
    AssertThat(diskLayout->getPhysicalBounds(),
        Equals(Layout::LayoutBounds{0, 3, 0, 1}));
}

template <typename K, typename V>
static std::vector<std::pair<K, V>> toVector(const std::map<K, V>& map)
{
    return std::vector<std::pair<K, V>>(map.begin(), map.end());
}

static void test_sectoroffsets()
{
    globalConfig().clear();
    globalConfig().readBaseConfig(R"M(
		drive {
			drive_type: DRIVETYPE_80TRACK
		}

		layout {
			format_type: FORMATTYPE_80TRACK
			tracks: 2
			sides: 2
			layoutdata {
				sector_size: 256
                physical {
					start_sector: 0
					count: 4
                }
				filesystem {
					start_sector: 0
					count: 4
					skew: 2
				}
			}
		}
	)M");

    auto diskLayout = createDiskLayout();
    AssertThat(diskLayout->groupSize, Equals(1));
    AssertThat(diskLayout->logicalLocationsBySectorOffset,
        EqualsContainer(decltype(diskLayout->logicalLocationsBySectorOffset){
            {0,    {0, 0, 0}},
            {256,  {0, 0, 2}},
            {512,  {0, 0, 1}},
            {768,  {0, 0, 3}},
            {1024, {0, 1, 0}},
            {1280, {0, 1, 2}},
            {1536, {0, 1, 1}},
            {1792, {0, 1, 3}},
            {2048, {1, 0, 0}},
            {2304, {1, 0, 2}},
            {2560, {1, 0, 1}},
            {2816, {1, 0, 3}},
            {3072, {1, 1, 0}},
            {3328, {1, 1, 2}},
            {3584, {1, 1, 1}},
            {3840, {1, 1, 3}}
    }));
    AssertThat(diskLayout->sectorOffsetByLogicalLocation,
        EqualsContainer(decltype(diskLayout->sectorOffsetByLogicalLocation){
            {{0, 0, 0}, 0   },
            {{0, 0, 1}, 512 },
            {{0, 0, 2}, 256 },
            {{0, 0, 3}, 768 },
            {{0, 1, 0}, 1024},
            {{0, 1, 1}, 1536},
            {{0, 1, 2}, 1280},
            {{0, 1, 3}, 1792},
            {{1, 0, 0}, 2048},
            {{1, 0, 1}, 2560},
            {{1, 0, 2}, 2304},
            {{1, 0, 3}, 2816},
            {{1, 1, 0}, 3072},
            {{1, 1, 1}, 3584},
            {{1, 1, 2}, 3328},
            {{1, 1, 3}, 3840}
    }));
}

int main(int argc, const char* argv[])
{
    test_physical_sectors();
    test_logical_sectors();
    test_both_sectors();
    test_skew();
    test_bounds();
    test_sectoroffsets();
}
