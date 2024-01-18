
#pragma once

class MainWindow;

class ImageComponent
{
public:
    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;
    virtual void setDiskData(std::shared_ptr<const DiskFlux> disk) = 0;

public:
    static ImageComponent* create(MainWindow* mainWindow);
};
