#include "globals.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "visualiser.h"
#include "fmt/format.h"
#include <iostream>
#include <fstream>

static const int SIZE = 480;
static const int BORDER = 10;
static const int RADIUS = (SIZE/2) - (BORDER/2);
static const int CORE = 50;
static const int TRACKS = 83;
static const double TRACK_SPACING = double(RADIUS-CORE) / TRACKS;
static const nanoseconds_t PERIOD = 200e6;
static const double RADIANS_PER_NS = 2.0*M_PI / (double)PERIOD;

void visualiseSectorsToFile(const SectorSet& sectors, const std::string& filename)
{
    std::cout << "writing visualisation\n";
    std::ofstream f(filename, std::ios::out);
    if (!f.is_open())
        Error() << "cannot open visualisation file";

    f << fmt::format("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"{0} {1} {2} {3}\">",
        0, 0, SIZE*2, SIZE);

    auto drawSide = [&](int side)
    {
        f << fmt::format("<g transform='matrix(1 0 0 -1 {} {})'>", SIZE/2 + (side*SIZE), SIZE/2);
        f << fmt::format("<circle cx='0' cy='0' r='{}' stroke='none' fill='#ccc'/>", RADIUS);

        for (int physicalTrack = 0; physicalTrack < TRACKS; physicalTrack++)
        {
            double radius = CORE + physicalTrack*TRACK_SPACING;
            f << fmt::format("<circle cx='0' cy='0' r='{}' stroke='#888' stroke-width='0.5' fill='none'/>", radius);

            auto drawArc = [&](const std::unique_ptr<Sector>& sector, nanoseconds_t start, nanoseconds_t end, const std::string& colour)
            {
                start %= PERIOD;
                end %= PERIOD;
                if (end < start)
                    end += PERIOD;
                
                double theta1 = start * RADIANS_PER_NS;
                double theta2 = end * RADIANS_PER_NS;
                int large = (theta2 - theta1) >= M_PI;

                f << fmt::format("\n<!-- {} {} = {} {} -->", start, end, theta1, theta2);
                f << fmt::format("<path fill='none' stroke='{}' stroke-width='1.5' d='", colour);
                f << fmt::format("M {} {} ", cos(theta1)*radius, sin(theta1)*radius);
                f << fmt::format("A {0} {0} 0 {3} 1 {1} {2}", radius, cos(theta2)*radius, sin(theta2)*radius, large);
                f << fmt::format("'><title>Track {} Head {} Sector {}; {}ms to {}ms</title></path>",
                    sector->logicalTrack, sector->logicalSide, sector->logicalSector,
                    start/1e6, end/1e6);
            };

            /* Sadly, SectorSets aren't indexable by physical track. */
            for (const auto& e : sectors.get())
            {
                const auto& sector = e.second;
                if ((sector->physicalSide == side) && (sector->physicalTrack == physicalTrack))
                {
                    const char* colour = "#f00";
                    if (sector->status == Sector::OK)
                        colour = "#00f";
                    if (sector->headerStartTime && sector->headerEndTime)
                        drawArc(sector, sector->headerStartTime, sector->headerEndTime, "#0ff");
                    if (sector->dataStartTime && sector->dataEndTime)
                        drawArc(sector, sector->dataStartTime, sector->dataEndTime, colour);
                }
            }
        }

        f << "</g>";
    };

    f << fmt::format("<rect x='0' y='0' width='{}' height='{}' stroke='none' fill='#fff'/>", SIZE*2, SIZE);

    drawSide(0);
    drawSide(1);

    f << "</svg>";
}
