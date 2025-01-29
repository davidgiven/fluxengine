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

        FluxmapReader fmr(*fluxmap);
        unsigned ticks;
        if (fmr.findEvent(F_BIT_INDEX, ticks))
            t.firstIndexTimestamp = NS_PER_TICK * ticks;
        else
            t.firstIndexTimestamp = 0;

        updateGradient(t);
    }

    void clear() override
    {
        _trackData.clear();
    }

    void setScale(nanoseconds_t nanosecondsPerPixel) override
    {
        _nanosecondsPerPixel = nanosecondsPerPixel;
        for (auto& t : _trackData)
            updateGradient(t.second);
    }

    void updateGradient(track_t& t)
    {
        if (!t.fluxmap)
            return;

        int numPixels = t.fluxmap->duration() / _nanosecondsPerPixel;
        t.gradient.clear();
        t.gradient.resize(numPixels + 1, 0);

        FluxmapReader fmr(*t.fluxmap);
        while (!fmr.eof())
        {
            unsigned ticks;
            if (!fmr.findEvent(F_BIT_PULSE, ticks))
                break;
            int pixel = fmr.tell().ns() / _nanosecondsPerPixel;
            t.gradient[pixel]++;
        }
        int max = *ranges::max_element(t.gradient);

        for (float& f : t.gradient)
            f /= max;
    }

    void redraw(QPainter& painter,
        nanoseconds_t startPos,
        nanoseconds_t endPos,
        int track)
    {
        auto& t = _trackData[track];
        if (!t.fluxmap)
            return;

        painter.setBrush(Qt::NoBrush);

        int start = std::max(0, (int)(startPos / _nanosecondsPerPixel));
        int end = std::min(
            (int)t.gradient.size(), (int)(endPos / _nanosecondsPerPixel));

        for (int pixel = start; pixel < end; pixel++)
        {
            int c = t.gradient[pixel] * 255;
            painter.setPen(QPen(QColor(0, c, c), 1.5));
            painter.drawLine(pixel, 0, pixel, 5);
        }
    }

private:
    struct track_t
    {
        std::shared_ptr<const Fluxmap> fluxmap;
        nanoseconds_t firstIndexTimestamp = 0;
        std::vector<float> gradient;
    };

    std::map<int, track_t> _trackData;
    nanoseconds_t _nanosecondsPerPixel = 1e6;
};

std::unique_ptr<FluxView> FluxView::create()
{
    return std::make_unique<FluxViewImpl>();
}
