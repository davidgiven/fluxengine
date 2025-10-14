#include "lib/core/globals.h"
#include "lib/data/disk.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/data/sector.h"
#include "protocol.h"

namespace
{
    struct pair_to_range_t
    {
        template <typename I>
        friend constexpr auto operator|(
            std::pair<I, I> const& pr, pair_to_range_t)
        {
            return std::ranges::subrange(pr.first, pr.second);
        }
    };
    inline constexpr pair_to_range_t pair_to_range{};
}

Disk::Disk() {}

Disk::Disk(
    const std::shared_ptr<const Image>& image, const DiskLayout& diskLayout):
    image(image)
{
    std::multimap<CylinderHead, std::shared_ptr<const Sector>>
        sectorsGroupedByTrack;
    for (const auto& sector : *image)
        sectorsGroupedByTrack.insert(
            std::make_pair(sector->physicalLocation.value(), sector));

    for (const auto& physicalLocation :
        std::views::keys(sectorsGroupedByTrack) | std::ranges::to<std::set>())
    {
        const auto& ptl =
            diskLayout.layoutByPhysicalLocation.at(physicalLocation);
        const auto& ltl = ptl->logicalTrackLayout;

        auto decodedTrack = std::make_shared<Track>();
        decodedTrack->ltl = ltl;
        decodedTrack->ptl = ptl;
        tracksByPhysicalLocation.insert(
            std::make_pair(physicalLocation, decodedTrack));

        for (auto& [ch, sector] :
            sectorsGroupedByTrack.equal_range(physicalLocation) | pair_to_range)
        {
            decodedTrack->sectors.push_back(sector);
            sectorsByPhysicalLocation.insert(
                std::make_pair(physicalLocation, sector));
        }
    }
}
