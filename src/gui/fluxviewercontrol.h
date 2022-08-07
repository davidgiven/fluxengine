#ifndef FLUXVIEWERCONTROL_H
#define FLUXVIEWERCONTROL_H

#include "globals.h"

class TrackFlux;
class wxScrollBar;
class wxScrollEvent;
class Sector;
class Record;
class Location;

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
	void UpdateScale();
	void ShowSectorMenu(std::shared_ptr<const Sector> sector);
	void ShowRecordMenu(const Location& location, std::shared_ptr<const Record> record);
	void DisplayDecodedData(std::shared_ptr<const Sector> sector);
	void DisplayRawData(std::shared_ptr<const Sector> sector);
	void DisplayRawData(const Location& location, std::shared_ptr<const Record> record);

private:
	void OnPaint(wxPaintEvent&);
	void OnMouseWheel(wxMouseEvent&);
	void OnScrollbarChanged(wxScrollEvent&);
	void OnMouseMotion(wxMouseEvent&);
	void OnContextMenu(wxContextMenuEvent&);

private:
	wxScrollBar* _scrollbar;
	std::shared_ptr<const TrackFlux> _flux;
	nanoseconds_t _scrollPosition;
	nanoseconds_t _totalDuration;
	double _nanosecondsPerPixel;
	std::vector<float> _densityMap;
	int _dragStartX;
	nanoseconds_t _dragStartPosition;
	int _mouseX;
	int _mouseY;
	bool _rightClicked;
    wxDECLARE_EVENT_TABLE();
};

#endif

