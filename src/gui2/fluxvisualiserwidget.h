#pragma once

#include <QGraphicsView>

class FluxVisualiserWidget : public QGraphicsView
{
    W_OBJECT(FluxVisualiserWidget)

public:
    virtual void refresh() = 0;
    W_SLOT(refresh)

    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;
    virtual void setDiskData(std::shared_ptr<const DiskFlux> disk) = 0;

public:
    static FluxVisualiserWidget* create();
};
