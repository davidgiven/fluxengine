#pragma once

#include "lib/globals.h"
#include "lib/fluxmap.h"

class HistogramViewer : public wxWindow
{
public:
    HistogramViewer(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);
    virtual ~HistogramViewer() {}

public:
    void Redraw(const Fluxmap& fluxmap, nanoseconds_t clock);
	nanoseconds_t GetMedian() const { return _data.median; }

private:
    void OnPaint(wxPaintEvent&);

private:
	Fluxmap::ClockData _data;
	wxFont _font;
	nanoseconds_t _clock;
    wxDECLARE_EVENT_TABLE();
};

