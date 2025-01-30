#pragma once

static constexpr double FLUXVIEWER_NS_PER_UNIT = 100000.0;
static constexpr double FLUXVIEWER_GRADIENT_ADJUST = 0.001;
static constexpr double FLUXVIEWER_LINE_WIDTH = 1.1;

class FluxView
{
public:
    static std::unique_ptr<FluxView> create();

public:
    virtual void setTrackData(
        int track, std::shared_ptr<const Fluxmap>& fluxmap) = 0;
    virtual void clear() = 0;

    virtual void redraw(QPainter& painter,
        nanoseconds_t startPos,
        nanoseconds_t endPos,
        int track) = 0;
};
