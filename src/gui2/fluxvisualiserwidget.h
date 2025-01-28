#pragma once

#include <QGraphicsView>

class FluxVisualiserWidget : public QWidget
{
    W_OBJECT(FluxVisualiserWidget)

public:
    virtual void setVisibleSide(int mode) = 0;
    W_SLOT(setVisibleSide)

    virtual void setGamma(float gamma) = 0;
    W_SLOT(setGamma)

    virtual void clearData() = 0;
    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;

public:
    static FluxVisualiserWidget* create();
};
