#include "lib/globals.h"
#include "lib/config.h"
#include "lib/fluxmap.h"
#include "lib/flux.h"
#include "lib/layout.h"
#include "globals.h"
#include "fluxvisualiserwidget.h"
#include <QWheelEvent>
#include <QFrame>

W_OBJECT_IMPL(FluxVisualiserWidget)

class DiskFlux;
class TrackFlux;

static const int TRACKS = 82;

static const float VBORDER = 1.0;
static const float VOUTER_RADIUS = 50.0;
static const float VINNER_RADIUS = 10.0;

class FluxVisualiserWidgetImpl : public FluxVisualiserWidget
{
public:
    FluxVisualiserWidgetImpl(): _scene(new QGraphicsScene())
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setScene(_scene);
    }

public:
    void resizeEvent(QResizeEvent* event) override
    {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }

public:
    void setTrackData(std::shared_ptr<const TrackFlux> track)
    {
        key_t key = {
            track->trackInfo->physicalTrack, track->trackInfo->physicalSide};
        _tracks[key] = track;
    }

    void setDiskData(std::shared_ptr<const DiskFlux> disk)
    {
        for (const auto& track : disk->tracks)
        {
            key_t key = {track->trackInfo->physicalTrack,
                track->trackInfo->physicalSide};
            _tracks[key] = track;
        }
    }

public:
    void refresh() override
    {
        _scene->clear();

        drawSide(VOUTER_RADIUS, VOUTER_RADIUS);
        drawSide(VOUTER_RADIUS, VOUTER_RADIUS*3 + VBORDER);

        _scene->setSceneRect(0.0, 0.0, VOUTER_RADIUS*2, VOUTER_RADIUS*4 + VBORDER);
    }

private:
    void drawSide(float x, float y)
    {
        QPen black(QColorConstants::Black);
        black.setWidth(0);

        float step = (VOUTER_RADIUS - VINNER_RADIUS) / TRACKS;
        for (int track = 0; track<82; track++)
        {
            float r = VOUTER_RADIUS - track * step;
            _scene->addEllipse(x-r, y-r, r*2, r*2, black);
        }
    }

private:
    typedef std::pair<unsigned, unsigned> key_t;

    QGraphicsScene* _scene;
    std::map<key_t, std::shared_ptr<const TrackFlux>> _tracks;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
