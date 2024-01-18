#pragma once

#include <QGraphicsView>

class FluxVisualiserWidget : public QGraphicsView
{
    W_OBJECT(FluxVisualiserWidget)

public:
    virtual void setVisibleSide(int mode) = 0;
    W_SLOT(setVisibleSide)

    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;
    virtual void setDiskData(std::shared_ptr<const DiskFlux> disk) = 0;

public:
    static FluxVisualiserWidget* create();
};
