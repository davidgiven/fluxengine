#include "lib/globals.h"
#include "lib/config.h"
#include "lib/flux.h"
#include "lib/layout.h"
#include "lib/image.h"
#include "lib/sector.h"
#include "lib/decoders/fluxmapreader.h"
#include "globals.h"
#include "imagevisualiserwidget.h"
#include "scene.h"
#include <QWheelEvent>
#include <QFrame>
#include <QConicalGradient>
#include <range/v3/all.hpp>

W_OBJECT_IMPL(ImageVisualiserWidget)

class DiskFlux;
class TrackFlux;

static constexpr int WIDTH = 300;
static constexpr int HEIGHT = 500;

class ImageVisualiserWidgetImpl : public ImageVisualiserWidget
{
private:
    class SectorGraphicsItem : public QGraphicsRectItem
    {
    public:
        SectorGraphicsItem(std::shared_ptr<const Sector> sector,
            qreal x,
            qreal y,
            qreal width,
            qreal height,
            const QBrush& brush):
            QGraphicsRectItem(x, y, width, height),
            _sector(sector)
        {
            setPen(QPen(Qt::NoPen));
            setBrush(brush);
            setAcceptHoverEvents(true);
        }

    protected:
        void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
        {
            if (scene() && !scene()->views().isEmpty() &&
                scene()->views().first() &&
                scene()->views().first()->viewport())
            {
                QGraphicsView* view = scene()->views().first();
                QPointF scenePos = mapToScene(boundingRect().bottomRight());
                QPoint viewportPos = view->mapFromScene(scenePos);
                QPoint tooltipPos = view->viewport()->mapToGlobal(viewportPos);

                QToolTip::showText(tooltipPos,
                    QString::fromStdString(
                        fmt::format("Track: {} Head: {} Sector: {}\nStatus: {}",
                            _sector->logicalTrack,
                            _sector->logicalSide,
                            _sector->logicalSector,
                            Sector::statusToString(_sector->status))));
            }

            setPen(QPen(QColorConstants::Magenta));
        }

        void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override
        {
            QToolTip::hideText();
            setPen(QPen(Qt::NoPen));
        }

    private:
        std::shared_ptr<const Sector> _sector;
    };

public:
    ImageVisualiserWidgetImpl(): _scene(new Scene())
    {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setScene(_scene);

        redraw();
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
            track->trackInfo->logicalTrack, track->trackInfo->logicalSide};

        _sectors.erase(key);
        for (auto& sector : track->sectors)
            _sectors.insert({key, sector});
        _trackInfos.insert({key, track->trackInfo});
        _numTracks = track->trackInfo->numTracks;

        redraw();
    }

    void setDiskData(std::shared_ptr<const DiskFlux> disk) {}

private:
    void redraw()
    {
        _scene->clear();

        if (_numTracks != 0)
        {
            QPen black(QColorConstants::Black, 0);
            _scene->addLine(WIDTH / 2, 0, WIDTH / 2, HEIGHT, black);

            float step = (float)HEIGHT / (float)(_numTracks + 1);
            float border = step * 0.2;
            float size = step * 0.8;
            float top = step;

            size_t maxSectors = 0;
            for (int track = 0; track < _numTracks; track++)
            {
                auto drawSectors = [&](int side)
                {
                    key_t key = {track, side};
                    std::vector<std::shared_ptr<const Sector>> sectors;
                    for (auto it = _sectors.lower_bound(key);
                         it != _sectors.upper_bound(key);
                         it++)
                        sectors.push_back(it->second);
                    std::sort(sectors.begin(),
                        sectors.end(),
                        sectorPointerSortPredicate);
                    maxSectors = std::max(maxSectors, sectors.size());

                    float y = top + (float)track * step;
                    float x = WIDTH / 2;
                    if (side == 0)
                        x -= size * 2;
                    else
                        x += size;

                    for (int sectorId = 0; sectorId < sectors.size();
                         sectorId++)
                    {
                        std::shared_ptr<const Sector> sector =
                            sectors.at(sectorId);

                        QColor colour;
                        switch (sector->status)
                        {
                            case Sector::OK:
                                colour = QColorConstants::DarkGreen;
                                break;

                            case Sector::MISSING:
                            case Sector::DATA_MISSING:
                                colour = QColorConstants::Cyan;
                                break;

                            default:
                                colour = QColorConstants::Red;
                        }

                        QBrush brush(colour);
                        SectorGraphicsItem* item = new SectorGraphicsItem(
                            sector, x, y, size, size, brush);
                        _scene->addItem(item);

                        if (side == 0)
                            x -= step;
                        else
                            x += step;
                    }
                };

                drawSectors(0);
                drawSectors(1);
            }

            QFont font;
            font.setPixelSize(step);
            _scene->addAlignedText(WIDTH / 2 - step,
                step,
                "Side 0",
                font,
                Qt::AlignRight | Qt::AlignBottom);
            _scene->addAlignedText(WIDTH / 2 + step,
                step,
                "Side 1",
                font,
                Qt::AlignLeft | Qt::AlignBottom);

            float sideWidth = (maxSectors + 1) * step;
            setSceneRect(
                {WIDTH / 2 - sideWidth, 0, WIDTH / 2 + sideWidth, HEIGHT});
        }
        else
        {
            _scene->addAlignedText(WIDTH / 2, HEIGHT / 2, "No image loaded!");
            setSceneRect({0, 0, WIDTH, HEIGHT});
        }

        fitInView(sceneRect(), Qt::KeepAspectRatio);
    }

private:
    typedef std::pair<int, int> key_t;

    Scene* _scene;
    std::multimap<key_t, std::shared_ptr<const Sector>> _sectors;
    std::map<key_t, std::shared_ptr<const TrackInfo>> _trackInfos;
    unsigned _numTracks;
};

ImageVisualiserWidget* ImageVisualiserWidget::create()
{
    return new ImageVisualiserWidgetImpl();
}
