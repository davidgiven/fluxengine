#include "lib/globals.h"
#include "lib/config.h"
#include "lib/fluxmap.h"
#include "lib/flux.h"
#include "lib/layout.h"
#include "lib/decoders/fluxmapreader.h"
#include "globals.h"
#include "fluxvisualiserwidget.h"
#include <QWheelEvent>
#include <QFrame>
#include <QConicalGradient>
#include <range/v3/all.hpp>

W_OBJECT_IMPL(FluxVisualiserWidget)

class DiskFlux;
class TrackFlux;

static constexpr int SIDES = 2;
static constexpr int TRACKS = 82;
static constexpr int SLOTS = 300;

static constexpr float VBORDER = 1.0;
static constexpr float VOUTER_RADIUS = 50.0;
static constexpr float VINNER_RADIUS = 10.0;

class FluxVisualiserWidgetImpl : public FluxVisualiserWidget
{
private:
    enum
    {
        SIDE_0,
        SIDE_1,
        BOTH_SIDES
    };

public:
    FluxVisualiserWidgetImpl(): _scene(new QGraphicsScene())
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setScene(_scene);

        recompute();
    }

public:
    void resizeEvent(QResizeEvent* event) override
    {
        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }

public:
    void setVisibleSide(int mode) override
    {
        _viewMode = mode;
        redraw();
    }

    void setGamma(float gamma) override
    {
        _gamma = gamma;
        recompute();
    }

    void setTrackData(std::shared_ptr<const TrackFlux> track)
    {
        key_t key = {
            track->trackInfo->physicalTrack, track->trackInfo->physicalSide};
        _tracks[key] = track;

        recompute(
            track->trackInfo->physicalSide, track->trackInfo->physicalTrack);
    }

    void setDiskData(std::shared_ptr<const DiskFlux> disk)
    {
        _tracks.clear();
        for (const auto& track : disk->tracks)
        {
            key_t key = {track->trackInfo->physicalTrack,
                track->trackInfo->physicalSide};
            _tracks[key] = track;
        }

        recompute();
    }

private:
    void redraw()
    {
        _scene->clear();

        switch (_viewMode)
        {
            case SIDE_0:
                drawSide(VOUTER_RADIUS, VOUTER_RADIUS, 0);
                _scene->setSceneRect(
                    0.0, 0.0, VOUTER_RADIUS * 2, VOUTER_RADIUS * 2);
                break;

            case SIDE_1:
                drawSide(VOUTER_RADIUS, VOUTER_RADIUS, 1);
                _scene->setSceneRect(
                    0.0, 0.0, VOUTER_RADIUS * 2, VOUTER_RADIUS * 2);
                break;

            case BOTH_SIDES:
                drawSide(VOUTER_RADIUS, VOUTER_RADIUS, 0);
                drawSide(VOUTER_RADIUS, VOUTER_RADIUS * 3 + VBORDER, 1);

                _scene->setSceneRect(
                    0.0, 0.0, VOUTER_RADIUS * 2, VOUTER_RADIUS * 4 + VBORDER);
                break;
        }

        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }

    void drawSide(float x, float y, int side)
    {
        float step = (VOUTER_RADIUS - VINNER_RADIUS) / TRACKS;
        for (int track = 0; track < 82; track++)
        {
            QConicalGradient gradient(x, y, 90.0);
            gradient.setStops(_viewData[side][track].gradientStops);
            QBrush brush(gradient);
            QPen pen(brush, step * 1.15);

            float r = VOUTER_RADIUS - track * step;
            _scene->addEllipse(x - r, y - r, r * 2, r * 2, pen);
        }

        QFont font;
        font.setPointSizeF(VINNER_RADIUS / 3.0);
        QGraphicsSimpleTextItem* label = _scene->addSimpleText(
            QString::fromStdString(fmt::format("Side {}", side)));
        label->setFont(font);
        QRectF bounds = label->boundingRect();
        label->setPos(x - bounds.width() / 2, y - bounds.height() / 2);
    }

    void recompute()
    {
        for (int side = 0; side < SIDES; side++)
            for (int track = 0; track < TRACKS; track++)
                recompute(side, track);
    }

    void recompute(int side, int track)
    {
        VData& vdata = _viewData[side][track];
        QGradientStops& stops = vdata.gradientStops;
        stops.clear();
        ranges::fill(vdata.slot, VSlot());

        auto it = _tracks.find(key_t(track, side));
        if (it != _tracks.end())
        {
            const TrackFlux& trackFlux = *it->second;
            for (auto& trackDataFlux : trackFlux.trackDatas)
            {
                nanoseconds_t rotationalPeriod =
                    trackDataFlux->rotationalPeriod;
                if (rotationalPeriod == 0.0)
                    rotationalPeriod = 200e6;
                const Fluxmap& fm = *trackDataFlux->fluxmap;
                FluxmapReader fmr(fm);

                fmr.seekToIndexMark();
                nanoseconds_t indexTimeNs = fmr.tell().ns();
                fmr.rewind();

                int slotIndex = -1;
                for (;;)
                {
                    int event;
                    unsigned ticks;
                    fmr.getNextEvent(event, ticks);

                    if (event & F_BIT_PULSE)
                    {
                        nanoseconds_t ns = fmr.tell().ns();
                        while (ns < indexTimeNs)
                            ns += rotationalPeriod;
                        ns = fmod(ns - indexTimeNs, rotationalPeriod);

                        int newIndex = (ns / rotationalPeriod) * SLOTS;
                        if (slotIndex != -1)
                        {
                            while (
                                (slotIndex < newIndex) && (slotIndex < SLOTS))
                            {
                                vdata.slot[slotIndex].count++;
                                slotIndex++;
                            }
                        }

                        vdata.slot[newIndex].pulses++;
                        slotIndex = newIndex;
                    }

                    if (event & F_EOF)
                        break;
                }

                if (slotIndex != -1)
                    vdata.slot[slotIndex].count++;
            }

            for (int i = 0; i < SLOTS; i++)
            {
                VSlot& slot = vdata.slot[i];
                if (slot.count == 0)
                    stops.append(
                        QPair((float)i / SLOTS, QColorConstants::LightGray));
                else
                {
                    float factor = (float)slot.pulses / (float)slot.count / 300;
                    int c = powf(factor, _gamma) * 255.0;
                    stops.append(
                        QPair(1.0 - ((float)i / SLOTS), QColor(0, c, c)));
                }
            }
        }

        if (stops.empty())
        {
            stops.append(QPair(0.0, QColorConstants::LightGray));
            stops.append(QPair(1.0, QColorConstants::LightGray));
        }

        redraw();
    }

private:
    typedef std::pair<unsigned, unsigned> key_t;

    struct VSlot
    {
        int count;
        int pulses;
    };

    struct VData
    {
        QGradientStops gradientStops;
        VSlot slot[SLOTS];
    };

    QGraphicsScene* _scene;
    std::map<key_t, std::shared_ptr<const TrackFlux>> _tracks;
    int _viewMode = BOTH_SIDES;
    VData _viewData[SIDES][TRACKS];
    float _gamma = 1.0;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
