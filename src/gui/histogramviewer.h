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
    void Redraw(const Fluxmap& fluxmap, nanoseconds_t clock=0);

    void Redraw(const Fluxmap* fluxmap, nanoseconds_t clock=0)
	{ Redraw(*fluxmap, clock); }

	nanoseconds_t GetMedian() const { return _data.median; }

private:
    void OnPaint(wxPaintEvent&);

private:
	bool _blank = true;
	Fluxmap::ClockData _data;
	wxFont _font;
	nanoseconds_t _clock;
    wxDECLARE_EVENT_TABLE();
};

