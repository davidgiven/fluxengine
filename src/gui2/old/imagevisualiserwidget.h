#pragma once

#include <QGraphicsView>

class ImageVisualiserWidget : public QGraphicsView
{
    W_OBJECT(ImageVisualiserWidget)

public:
    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;
    virtual void setDiskData(std::shared_ptr<const DiskFlux> disk) = 0;

public:
    static ImageVisualiserWidget* create();
};
