#include "globals.h"
#include "bytes.h"
#include "fluxmap.h"
#include "sector.h"
#include "flux.h"
#include "fmt/format.h"
#include "logger.h"

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

static bool indented = false;

static void indent()
{
    if (!indented)
        std::cout << "      ";
    indented = false;
}

Logger& Logger::operator<<(std::shared_ptr<AnyLogMessage> message)
{
    std::visit(
        overloaded{
            /* Fallback --- do nothing */
            [](const auto& m)
            {
            },

            /* Indicates that we're working on a given cylinder and head */
            [](const DiskContextLogMessage& m)
            {
                std::cout << fmt::format("{:2}.{}: ", m.cylinder, m.head)
                          << std::flush;
                indented = true;
            },

            /* A single read has happened */
            [](const SingleReadLogMessage& m)
            {
				const auto& trackdataflux = m.trackDataFlux;

                indent();
                std::cout << fmt::format("{} records, {} sectors",
                    trackdataflux->records.size(),
                    trackdataflux->sectors.size());
                if (trackdataflux->sectors.size() > 0)
                {
                    nanoseconds_t clock =
                        (*trackdataflux->sectors.begin())->clock;
                    std::cout << fmt::format("; {:.2f}us clock ({:.0f}kHz)",
                        clock / 1000.0,
                        1000000.0 / clock);
                }

                std::cout << '\n';

				indent();
				std::cout << "sectors:";

				std::vector<std::shared_ptr<Sector>> sectors(m.sectors.begin(), m.sectors.end());
				std::sort(sectors.begin(), sectors.end(),
					[](const std::shared_ptr<Sector>& s1, const std::shared_ptr<Sector>&s2)
					{
						return s1->logicalSector < s2->logicalSector;
					}
				);

				for (const auto& sector : sectors)
					std::cout << fmt::format(" {}{}", sector->logicalSector, Sector::statusToChar(sector->status));

				std::cout << '\n';
            },

			/* We've finished reading a track */
			[](const TrackReadLogMessage& m)
			{
				int size = 0;
				std::set<std::pair<int, int>> track_ids;
				for (const auto& sector : m.track->sectors)
				{
					track_ids.insert(std::make_pair(sector->logicalTrack, sector->logicalSide));
					size += sector->data.size();
				}

				if (!track_ids.empty())
				{
					std::vector<std::string> ids;

					for (const auto& i : track_ids)
						ids.push_back(fmt::format("{}.{}", i.first, i.second));

					indent();
					std::cout << fmt::format("logical track {}\n", fmt::join(ids, "; "));
				}

				indent();
				std::cout << fmt::format("{} bytes decoded\n", size);
			},

            /* Generic text message */
            [](const std::string& s)
            {
                indent();
				std::cout << s << '\n';
            },
        },
        *message);
    return *this;
}
