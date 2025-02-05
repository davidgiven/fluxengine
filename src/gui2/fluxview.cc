#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/data/flux.h"
#include "lib/data/layout.h"
#include "lib/data/fluxmapreader.h"
#include "globals.h"
#include "fluxview.h"
#include "scene.h"
#include <QConicalGradient>
#include <range/v3/all.hpp>

class FluxViewImpl : public FluxView
{
    struct track_t;

public:
    void setTrackData(
        int track, std::shared_ptr<const Fluxmap>& fluxmap) override
    {
        auto& t = _trackData[track];
        t.fluxmap = fluxmap;
        t.pulses.clear();
        t.indices.clear();

        FluxmapReader fmr(*fluxmap);
        t.firstIndexTimestamp = 0;
        while (!fmr.eof())
        {
            int type;
            unsigned ticks;
            fmr.getNextEvent(type, ticks);

            nanoseconds_t here = fmr.tell().ns();
            if (type & F_BIT_INDEX)
                t.indices.push_back(here);
            if (type & F_BIT_PULSE)
                t.pulses.push_back(here);
            if (type & F_EOF)
                break;
        }
    }

    void clear() override
    {
        _trackData.clear();
    }

    void redraw(QPainter& painter,
        nanoseconds_t startPos,
        nanoseconds_t endPos,
        int track,
        double height)
    {
        auto& t = _trackData[track];
        if (!t.fluxmap)
            return;

        /* Number of on-screen pixels per drawing unit. */
        double pixelsPerUnit = painter.worldTransform().m11();

        /* Width of an on-screen pixel in drawing units. */
        double granularity = 1.0 / pixelsPerUnit;

        /* Number of nanoseconds per on-screen pixel. */
        double nsPerPixel = granularity * FLUXVIEWER_NS_PER_UNIT;

        painter.setBrush(Qt::NoBrush);

        int pixel = 0;
        int count = 0;
        int i = 0;

        for (nanoseconds_t here : t.pulses)
        {
            if (here < startPos)
                continue;
            if (here > endPos)
                break;

            int thisPixel = here / nsPerPixel;
            if (thisPixel != pixel)
            {
                if (count != 0)
                {
                    double x = pixel * granularity;
                    double countPerNs = count / nsPerPixel;
                    double density = std::clamp(
                        countPerNs / FLUXVIEWER_GRADIENT_ADJUST, 0.0, 1.0);
                    int c = 255 - density * 255.0;
                    painter.setPen(QPen(
                        QColor(0, c, c), granularity * FLUXVIEWER_LINE_WIDTH));
                    painter.drawLine(QLineF(x, 0, x, height));
                    count = 0;
                }
                pixel = thisPixel;
            }

            count++;
        }
    }

private:
    struct track_t
    {
        std::shared_ptr<const Fluxmap> fluxmap;
        nanoseconds_t firstIndexTimestamp = 0;
        std::vector<nanoseconds_t> pulses;
        std::vector<nanoseconds_t> indices;
    };

    std::map<int, track_t> _trackData;
};

std::unique_ptr<FluxView> FluxView::create()
{
    return std::make_unique<FluxViewImpl>();
}
