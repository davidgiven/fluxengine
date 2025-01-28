#pragma once

class FluxView
{
public:
    static std::unique_ptr<FluxView> create();

public:
    virtual void setTrackData(
        int track, std::shared_ptr<const Fluxmap>& fluxmap) = 0;
    virtual void setScale(nanoseconds_t nanosecondsPerPixel) = 0;
    virtual void clear() = 0;

    virtual void redraw(QPainter& painter,
        nanoseconds_t startPos,
        nanoseconds_t endPos,
        int track) = 0;
};
