#include "globals.h"
#include "bytes.h"
#include "fluxmap.h"
#include "sector.h"
#include "flux.h"
#include "logger.h"

static bool indented = false;

static std::function<void(std::shared_ptr<const AnyLogMessage>)> loggerImpl =
    [](auto message)
{
    std::cout << Logger::toString(*message) << std::flush;
};

void log(std::shared_ptr<const AnyLogMessage> message)
{
    loggerImpl(message);
}

void Logger::setLogger(
    std::function<void(std::shared_ptr<const AnyLogMessage>)> cb)
{
    loggerImpl = cb;
}

std::string Logger::toString(const AnyLogMessage& message)
{
    std::stringstream stream;

    auto indent = [&]()
    {
        if (!indented)
            stream << "      ";
        indented = false;
    };

    std::visit(
        overloaded{
            /* Fallback --- do nothing */
            [&](const auto& m)
            {
            },

            /* Start measuring the rotational speed */
            [&](const BeginSpeedOperationLogMessage& m)
            {
                stream << "Measuring rotational speed... ";
            },

            /* Finish measuring the rotational speed */
            [&](const EndSpeedOperationLogMessage& m)
            {
                stream << fmt::format("{:.1f}ms ({:.1f}rpm)\n",
                    m.rotationalPeriod / 1e6,
                    60e9 / m.rotationalPeriod);
            },

            /* Indicates that we're starting a write operation. */
            [&](const BeginWriteOperationLogMessage& m)
            {
                stream << fmt::format("{:2}.{}: ", m.track, m.head);
                indented = true;
            },

            /* Indicates that we're starting a read operation. */
            [&](const BeginReadOperationLogMessage& m)
            {
                stream << fmt::format("{:2}.{}: ", m.track, m.head);
                indented = true;
            },

            /* We've just read a track (we might reread it if there are errors)
             */
            [&](const TrackReadLogMessage& m)
            {
                const auto& track = *m.track;

                std::set<std::shared_ptr<const Sector>> rawSectors;
                std::set<std::shared_ptr<const Record>> rawRecords;
                for (const auto& trackDataFlux : track.trackDatas)
                {
                    rawSectors.insert(trackDataFlux->sectors.begin(),
                        trackDataFlux->sectors.end());
                    rawRecords.insert(trackDataFlux->records.begin(),
                        trackDataFlux->records.end());
                }

                nanoseconds_t clock = 0;
                for (const auto& sector : rawSectors)
                    clock += sector->clock;
                if (!rawSectors.empty())
                    clock /= rawSectors.size();

                indent();
                stream << fmt::format("{} raw records, {} raw sectors",
                    rawRecords.size(),
                    rawSectors.size());
                if (clock != 0)
                {
                    stream << fmt::format("; {:.2f}us clock ({:.0f}kHz)",
                        clock / 1000.0,
                        1000000.0 / clock);
                }

                stream << '\n';

                indent();
                stream << "sectors:";

                std::vector<std::shared_ptr<const Sector>> sectors(
                    track.sectors.begin(), track.sectors.end());
                std::sort(
                    sectors.begin(), sectors.end(), sectorPointerSortPredicate);

                for (const auto& sector : sectors)
                    stream << fmt::format(" {}.{}.{}{}",
                        sector->logicalTrack,
                        sector->logicalSide,
                        sector->logicalSector,
                        Sector::statusToChar(sector->status));

                stream << '\n';

                int size = 0;
                std::set<std::pair<int, int>> track_ids;
                for (const auto& sector : m.track->sectors)
                {
                    track_ids.insert(std::make_pair(
                        sector->logicalTrack, sector->logicalSide));
                    size += sector->data.size();
                }

                indent();
                stream << fmt::format("{} bytes decoded\n", size);
            },

            /* Large-scale operation start. */
            [&](const BeginOperationLogMessage& m)
            {
            },

            /* Large-scale operation end. */
            [&](const EndOperationLogMessage& m)
            {
            },

            /* Large-scale operation progress. */
            [&](const OperationProgressLogMessage& m)
            {
            },

            /* Generic text message */
            [&](const std::string& s)
            {
                indent();
                stream << s << '\n';
            },
        },
        message);
    return stream.str();
}
