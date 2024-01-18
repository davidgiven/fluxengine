#pragma once

class MainWindow;

class FluxComponent
{
public:
    virtual void setTrackData(std::shared_ptr<const TrackFlux> track) = 0;
    virtual void setDiskData(std::shared_ptr<const DiskFlux> disk) = 0;

public:
    static FluxComponent* create(MainWindow* mainWindow);
};
