#include "lib/globals.h"
#include "lib/config.h"
#include "lib/fluxmap.h"
#include "lib/flux.h"
#include "lib/layout.h"
#include "lib/decoders/fluxmapreader.h"
#include "globals.h"
#include "fluxvisualiserwidget.h"
#include "scene.h"
#include <QWheelEvent>
#include <QFrame>
#include <QConicalGradient>
#include <range/v3/all.hpp>

W_OBJECT_IMPL(FluxVisualiserWidget)

class DiskFlux;
class TrackFlux;

static constexpr int SIDES = 2;
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
    FluxVisualiserWidgetImpl(): _scene(new Scene())
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
        for (int i = 0; i < track->trackInfo->groupSize; i++)
        {
            key_t key = {track->trackInfo->physicalTrack + i,
                track->trackInfo->physicalSide};
            _tracks[key] = track;
            _numTracks = track->trackInfo->numPhysicalTracks;
            _viewData0.resize(_numTracks);
            _viewData1.resize(_numTracks);

            recompute(track->trackInfo->physicalSide,
                track->trackInfo->physicalTrack + i);
        }
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
        if (_numTracks == 0)
        {
            _scene->addAlignedText(x, y, "No flux loaded!");
            return;
        }

        auto& viewData = side ? _viewData1 : _viewData0;
        float step = (VOUTER_RADIUS - VINNER_RADIUS) / _numTracks;
        for (int track = 0; track < _numTracks; track++)
        {
            QConicalGradient gradient(x, y, 90.0);
            gradient.setStops(viewData.at(track).gradientStops);
            QBrush brush(gradient);
            QPen pen(brush, step * 1.15);

            float r = VOUTER_RADIUS - track * step;
            _scene->addEllipse(x - r, y - r, r * 2, r * 2, pen);
        }

        QFont font;
        font.setPointSizeF(VINNER_RADIUS / 3.0);
        _scene->addAlignedText(
            x, y, QString::fromStdString(fmt::format("Side {}", side)), font);
    }

    void recompute()
    {
        for (int side = 0; side < SIDES; side++)
            for (int track = 0; track < _numTracks; track++)
                recompute(side, track);
    }

    void recompute(int side, int track)
    {
        auto& viewData = side ? _viewData1 : _viewData0;
        VData& vdata = viewData.at(track);
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

    Scene* _scene;
    std::map<key_t, std::shared_ptr<const TrackFlux>> _tracks;
    int _viewMode = BOTH_SIDES;
    unsigned _numTracks = 0;
    std::vector<VData> _viewData0;
    std::vector<VData> _viewData1;
    float _gamma = 1.0;
};

FluxVisualiserWidget* FluxVisualiserWidget::create()
{
    return new FluxVisualiserWidgetImpl();
}
