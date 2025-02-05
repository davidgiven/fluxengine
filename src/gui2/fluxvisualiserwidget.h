#pragma once

#include <QGraphicsView>

class FluxVisualiserWidget : public QWidget
{
    W_OBJECT(FluxVisualiserWidget)

public:
    virtual void clearData() = 0;
    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;

    virtual void resetView() = 0;

public:
    static FluxVisualiserWidget* create();
};
