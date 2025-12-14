#ifndef FLUXVIEWERCONTROL_H
#define FLUXVIEWERCONTROL_H

#include "lib/core/globals.h"

class TrackFlux;
class wxScrollBar;
class wxScrollEvent;
class Sector;
class Record;
class TrackInfo;

class FluxViewerControl : public wxWindow
{
public:
    FluxViewerControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);
    virtual ~FluxViewerControl() {}

public:
    void SetScrollbar(wxScrollBar* scrollbar);
    void SetFlux(std::shared_ptr<const TrackFlux> flux);

private:
    void UpdateScale();
    void ShowSectorMenu(std::shared_ptr<const Sector> sector);
    void ShowRecordMenu(std::shared_ptr<const TrackInfo>& layout,
        std::shared_ptr<const Record> record);
    void DisplayDecodedData(std::shared_ptr<const Sector> sector);
    void DisplaySectorSummary(std::shared_ptr<const Sector> sector);
    void DisplayRawData(std::shared_ptr<const Sector> sector);
    void DisplayRawData(std::shared_ptr<const TrackInfo>& layout,
        std::shared_ptr<const Record> record);

private:
    void OnPaint(wxPaintEvent&);
    void OnMouseWheel(wxMouseEvent&);
    void OnScrollbarChanged(wxScrollEvent&);
    void OnMouseMotion(wxMouseEvent&);
    void OnContextMenu(wxContextMenuEvent&);

private:
    wxScrollBar* _scrollbar;
    std::shared_ptr<const TrackFlux> _flux;
    nanoseconds_t _scrollPosition = 0;
    nanoseconds_t _totalDuration = 0;
    double _nanosecondsPerPixel = 0;
    std::set<nanoseconds_t> _events;
    std::vector<float> _densityMap;
    int _dragStartX = -1;
    nanoseconds_t _dragStartPosition = -1;
    int _mouseX = -1;
    int _mouseY = -1;
    bool _rightClicked = false;
    wxDECLARE_EVENT_TABLE();
};

#endif
