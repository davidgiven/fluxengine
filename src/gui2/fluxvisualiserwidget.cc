#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/data/flux.h"
#include "lib/data/layout.h"
#include "lib/data/fluxmapreader.h"
#include "globals.h"
#include "fluxvisualiserwidget.h"
#include "fluxview.h"
#include "scene.h"
#include "viewnavigator.h"
#include <range/v3/all.hpp>

W_OBJECT_IMPL(FluxVisualiserWidget)

class DiskFlux;
class TrackFlux;

static constexpr double VSCALE_TRACK_SIZE = 10;
static constexpr double VMARGIN_SIZE = 30;
static constexpr double HMARGIN_SIZE = 30;
static constexpr int MINIMUM_TICK_DISTANCE = 10;

class FluxVisualiserWidgetImpl : public FluxVisualiserWidget
{
private:
    struct track_t;

public:
    FluxVisualiserWidgetImpl(): _viewNavigator(ViewNavigator::create(this))
    {
        clearData();
    }

public:
    void resizeEvent(QResizeEvent* event) override {}

public:
    void clearData() override
    {
        _tracks.clear();
        _numTracks = 0;
        _totalDuration = 0;
        _fluxView = FluxView::create();
        repaint();
    }

    void setTrackData(std::shared_ptr<const TrackFlux> track) override
    {
        key_t key = {
            track->trackInfo->physicalTrack, track->trackInfo->physicalSide};
        _numTracks = std::max(_numTracks, track->trackInfo->numPhysicalTracks);

        if (track->trackInfo->physicalSide == 0)
        {
            std::shared_ptr<const Fluxmap> data =
                track->trackDatas.front()->fluxmap;
            _fluxView->setTrackData(key.first, data);
            _totalDuration = std::max(_totalDuration, data->duration());
        }

        repaint();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        /* Draw the flux view. */

        painter.save();
        _viewNavigator->transform(painter);

        QRectF world =
            painter.worldTransform().inverted().mapRect(QRectF(event->rect()));
        nanoseconds_t left = world.left() * FLUXVIEWER_NS_PER_UNIT;
        nanoseconds_t right = world.right() * FLUXVIEWER_NS_PER_UNIT;
        painter.setPen(palette().color(QPalette::Text));
        painter.setBrush(Qt::NoBrush);

        for (int i = 0; i < _numTracks; i++)
        {
            painter.save();
            painter.translate(0, VSCALE_TRACK_SIZE * i);
            _fluxView->redraw(painter, left, right, i);
            painter.restore();
        }
        painter.restore();

        /* Draw the margins. */

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawRect(0, 0, rect().width(), VMARGIN_SIZE);
        painter.drawRect(
            0, VMARGIN_SIZE, HMARGIN_SIZE, rect().height() - VMARGIN_SIZE);

        /* Draw the horizontal scale. */

        double nanosecondsPerPixel = (right - left) / rect().width();

        int yy = VMARGIN_SIZE * 4 / 5;
        uint64_t tickStep = 1000;
        while ((tickStep / nanosecondsPerPixel) < MINIMUM_TICK_DISTANCE)
            tickStep *= 10;

        painter.setPen(palette().color(QPalette::Text));
        painter.setBrush(Qt::NoBrush);
        // static wxFont font(
        //     6, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        // dc.SetFont(font);
        // dc.SetBackgroundMode(wxTRANSPARENT);
        // dc.SetTextForeground(*wxBLACK);
        // dc.SetPen(FOREGROUND_PEN);

        nanoseconds_t dataLeft = std::max(left, 0.0);
        nanoseconds_t dataRight = std::min(right, _totalDuration);

        painter.drawLine((dataLeft-left) / nanosecondsPerPixel,
            yy,
            (dataRight-left) / nanosecondsPerPixel,
            yy);
        uint64_t tick = std::max(floor(dataLeft / tickStep) * tickStep, 0.0);
        while (tick < dataRight)
        {
            int xx = (tick - left) / nanosecondsPerPixel;
            int ts = VMARGIN_SIZE / 5;
            if ((tick % (10 * tickStep)) == 0)
                ts = VMARGIN_SIZE / 4;
            if ((tick % (100 * tickStep)) == 0)
                ts = VMARGIN_SIZE / 3;
#if 0
                dc.DrawLine({x + xx, t4y - ts}, {x + xx, t4y + ts});
                if ((tick % (10 * tickStep)) == 0)
                {
                    dc.DrawText(fmt::format("{:.3f}ms", tick / 1e6),
                        {x + xx, t4y - ch2});
                }
#endif
            painter.drawLine(xx, yy, xx, yy - ts);

            tick += tickStep;
        }
    }

private:
    typedef std::pair<unsigned, unsigned> key_t;

    struct track_t
    {
        std::shared_ptr<const TrackFlux> flux;
        std::vector<float> densityMap;
    };

    std::map<key_t, track_t> _tracks;
    unsigned _numTracks = 0;
    nanoseconds_t _totalDuration;
    std::unique_ptr<FluxView> _fluxView;
    std::unique_ptr<ViewNavigator> _viewNavigator;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
