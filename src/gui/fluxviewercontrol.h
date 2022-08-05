#ifndef FLUXVIEWERCONTROL_H
#define FLUXVIEWERCONTROL_H

#include "globals.h"

class TrackFlux;
class wxScrollBar;
class wxScrollEvent;

class FluxViewerControl : public wxWindow
{
public:
    FluxViewerControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);

public:
	void SetScrollbar(wxScrollBar* scrollbar);
	void SetFlux(std::shared_ptr<const TrackFlux> flux);

private:
	void ResizeScrollbar();

private:
	void OnPaint(wxPaintEvent&);
	void OnMouseWheel(wxMouseEvent&);
	void OnScrollbarChanged(wxScrollEvent&);

private:
	wxScrollBar* _scrollbar;
	std::shared_ptr<const TrackFlux> _flux;
	nanoseconds_t _scrollPosition;
	nanoseconds_t _totalDuration;
	double _nanosecondsPerPixel;
    wxDECLARE_EVENT_TABLE();
};

#endif

