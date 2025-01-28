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

static constexpr double HBORDER = 50;
static constexpr double VBORDER = 50;

static constexpr double HSCALE_NS_PER_PIXEL = 0.1;
static constexpr double VSCALE_TRACK_SIZE = 10;

class FluxVisualiserWidgetImpl : public FluxVisualiserWidget
{
private:
    struct track_t;

private:
    enum
    {
        SIDE_0,
        SIDE_1,
        BOTH_SIDES
    };

public:
    FluxVisualiserWidgetImpl():
        _nanosecondsPerPixel(100000),
        _viewNavigator(ViewNavigator::create(this))
    {
        clearData();
    }

public:
    void resizeEvent(QResizeEvent* event) override
    {
        // fitInView(sceneRect(), Qt::KeepAspectRatio);
    }

public:
    void setVisibleSide(int mode) override
    {
        _viewMode = mode;
    }

    void setGamma(float gamma) override
    {
        _gamma = gamma;
    }

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
        // const auto& it =
        //     _tracks.insert_or_assign(key, std::move(track_t())).first;
        // it->second.flux = track;
        _numTracks = std::max(_numTracks, track->trackInfo->numPhysicalTracks);

        _fluxView->setTrackData(key.first, track->trackDatas.front()->fluxmap);

        // if (!track->trackDatas.empty())
        // {
        //     _totalDuration = std::max(_totalDuration,
        //     track->trackDatas.front()->fluxmap->duration());
        // }
        repaint();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        _viewNavigator->transform(painter);

        painter.setPen(palette().color(QPalette::Text));
        painter.setBrush(Qt::NoBrush);

        for (int i = 0; i < _numTracks; i++)
        {
            painter.save();
            painter.translate(HBORDER, VBORDER + VSCALE_TRACK_SIZE * i);
            _fluxView->redraw(painter, 0, 100, i);
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

    // struct VSlot
    // {
    //     int count;
    //     int pulses;
    // };

    // struct VData
    // {
    //     QGradientStops gradientStops;
    //     VSlot slot[SLOTS];
    // };

    std::map<key_t, track_t> _tracks;
    int _viewMode = BOTH_SIDES;
    unsigned _numTracks = 0;
    // std::vector<VData> _viewData0;
    // std::vector<VData> _viewData1;
    float _gamma = 1.0;
    nanoseconds_t _nanosecondsPerPixel;
    nanoseconds_t _totalDuration;
    std::unique_ptr<FluxView> _fluxView;
    std::unique_ptr<ViewNavigator> _viewNavigator;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
