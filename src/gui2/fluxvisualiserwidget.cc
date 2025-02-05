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
static constexpr double HMARGIN_SIZE = 40;
static constexpr int MINIMUM_TICK_DISTANCE = 10;

static constexpr QRect CENTRED(
    -(INT_MAX / 2), -(INT_MAX / 2), INT_MAX, INT_MAX);

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
        double topTrack = world.top() / VSCALE_TRACK_SIZE;
        double bottomTrack = world.bottom() / VSCALE_TRACK_SIZE;
        nanoseconds_t left = world.left() * FLUXVIEWER_NS_PER_UNIT;
        nanoseconds_t right = world.right() * FLUXVIEWER_NS_PER_UNIT;
        painter.setPen(palette().color(QPalette::Text));
        painter.setBrush(Qt::NoBrush);

        for (int i = 0; i < _numTracks; i++)
        {
            painter.save();
            painter.translate(0, VSCALE_TRACK_SIZE * i);
            painter.setPen(palette().color(QPalette::Text));
            _fluxView->redraw(painter, left, right, i, VSCALE_TRACK_SIZE);

            painter.setPen(palette().color(QPalette::Base));
            painter.drawLine(left / FLUXVIEWER_NS_PER_UNIT,
                0,
                right / FLUXVIEWER_NS_PER_UNIT,
                0);
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

        {
            double nanosecondsPerPixel = (right - left) / rect().width();

            int yy = VMARGIN_SIZE * 4 / 5;
            uint64_t tickStep = 1000;
            while ((tickStep / nanosecondsPerPixel) < MINIMUM_TICK_DISTANCE)
                tickStep *= 10;

            painter.setPen(QPen(palette().color(QPalette::Text), 0));
            painter.setBrush(Qt::NoBrush);

            nanoseconds_t dataLeft = std::max(left, 0.0);
            nanoseconds_t dataRight = std::min(right, _totalDuration);

            painter.drawLine((dataLeft - left) / nanosecondsPerPixel,
                yy,
                (dataRight - left) / nanosecondsPerPixel,
                yy);
            uint64_t tick =
                std::max(floor(dataLeft / tickStep) * tickStep, 0.0);
            while (tick < dataRight)
            {
                int xx = (tick - left) / nanosecondsPerPixel;
                int ts = VMARGIN_SIZE / 5;
                if ((tick % (10 * tickStep)) == 0)
                    ts = VMARGIN_SIZE / 4;
                if ((tick % (100 * tickStep)) == 0)
                    ts = VMARGIN_SIZE / 3;

                if ((tick % (10 * tickStep)) == 0)
                {
                    painter.drawText(xx - 512,
                        0,
                        1024,
                        VMARGIN_SIZE / 2,
                        Qt::AlignCenter,
                        QString::fromStdString(
                            fmt::format("{:.3f}ms", tick / 1e6)));
                }

                painter.drawLine(xx, yy, xx, yy - ts);

                tick += tickStep;
            }
        }

        /* Draw the vertical scale. */

        {
            painter.save();
            painter.setPen(QPen(palette().color(QPalette::Text), 0));
            painter.setBrush(Qt::NoBrush);

            double xx = HMARGIN_SIZE * 4 / 5;
            double ys = rect().height() / (bottomTrack - topTrack);
            int t = std::max(0.0, floor(topTrack));
            int bottom = std::min(82.0, ceil(bottomTrack));
            double w = std::clamp(xx * 4 / 3 - ys / 2, xx / 2, xx);

            QFont font;
            font.setPixelSize(std::clamp(ys * 2 / 3, 2.0, HMARGIN_SIZE/3));
            painter.setFont(font);

            while (t < bottom)
            {
                double yy = (t - topTrack) * ys;
                double yt = yy + ys * 0.5 / VSCALE_TRACK_SIZE;
                double yb = yy + ys * 9.5 / VSCALE_TRACK_SIZE;
                painter.drawLine(xx, yt, w, yt);
                painter.drawLine(xx, yb, w, yb);
                painter.drawLine(xx, yt, xx, yb);

                painter.save();
                painter.setTransform(
                    QTransform().translate(xx / 2, yy + ys * .5));

                painter.drawText(CENTRED,
                    Qt::AlignCenter,
                    QString::fromStdString(fmt::format("{}.{}", 0, t)));
                painter.restore();

                t++;
            }
            painter.restore();
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
