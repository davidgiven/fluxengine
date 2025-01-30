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

class FluxVisualiserWidgetImpl : public FluxVisualiserWidget
{
private:
    struct track_t;

public:
    FluxVisualiserWidgetImpl():
        _nanosecondsPerPixel(100000),
        _viewNavigator(ViewNavigator::create(this))
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
            _fluxView->setTrackData(
                key.first, track->trackDatas.front()->fluxmap);

        repaint();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
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
    nanoseconds_t _nanosecondsPerPixel;
    nanoseconds_t _totalDuration;
    std::unique_ptr<FluxView> _fluxView;
    std::unique_ptr<ViewNavigator> _viewNavigator;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
