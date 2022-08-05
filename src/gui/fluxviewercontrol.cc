#include "globals.h"
#include "gui.h"
#include "fluxviewercontrol.h"
#include "lib/flux.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"

DECLARE_COLOUR(BACKGROUND, 192, 192, 192);
DECLARE_COLOUR(READ_SEPARATOR, 255, 0, 0);
DECLARE_COLOUR(INDEX_SEPARATOR, 255, 255, 0);
DECLARE_COLOUR(FOREGROUND, 0, 0, 0);
DECLARE_COLOUR(FLUX, 64, 64, 255);
DECLARE_COLOUR(RECORD, 255, 255, 255);

const int BORDER = 10;

FluxViewerControl::FluxViewerControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent, id, pos, size, style, "FluxViewerControl")
{
	SetDoubleBuffered(true);
}

wxBEGIN_EVENT_TABLE(FluxViewerControl, wxPanel)
    EVT_PAINT(FluxViewerControl::OnPaint)
	EVT_MOUSEWHEEL(FluxViewerControl::OnMouseWheel)
wxEND_EVENT_TABLE()

void FluxViewerControl::SetScrollbar(wxScrollBar* scrollbar)
{
	_scrollbar = scrollbar;
	_scrollbar->Bind(wxEVT_SCROLL_THUMBTRACK, &FluxViewerControl::OnScrollbarChanged, this);
	_scrollbar->Bind(wxEVT_SCROLL_CHANGED, &FluxViewerControl::OnScrollbarChanged, this);
}
	
void FluxViewerControl::SetFlux(std::shared_ptr<const TrackFlux> flux)
{
	_flux = flux;

	_scrollPosition = 0;
	_totalDuration = 0;
	for (const auto& trackdata : _flux->trackDatas)
		_totalDuration += trackdata->fluxmap->duration();

	auto size = GetSize();
	_nanosecondsPerPixel = (double)_totalDuration / (double)size.GetWidth();

	ResizeScrollbar();
	Refresh();
}

void FluxViewerControl::ResizeScrollbar()
{
	auto size = GetSize();
	nanoseconds_t thumbSize = size.GetWidth() * _nanosecondsPerPixel;
	_scrollbar->SetScrollbar(_scrollPosition/1000, thumbSize/1000, _totalDuration/1000, thumbSize/2000);
}

void FluxViewerControl::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
	dc.SetBackground(BACKGROUND_BRUSH);
	dc.Clear();

	auto size = GetSize();
	int w = size.GetWidth();
	int h = size.GetHeight();
	int h2 = h / 2;
	int h4 = h / 4;
	int h8 = h / 8;

	int x = -_scrollPosition / _nanosecondsPerPixel;
	for (const auto& trackdata : _flux->trackDatas)
	{
		nanoseconds_t duration = trackdata->fluxmap->duration();
		int fw = duration / _nanosecondsPerPixel;

		if (((x+fw) > 0) && (x < w))
		{
			if (x != 0)
			{
				dc.SetPen(READ_SEPARATOR_PEN);
				dc.DrawLine({x, 0}, {x, h});
			}

			dc.SetPen(FOREGROUND_PEN);
			dc.DrawLine({x, h2}, {x+fw, h2});

			dc.SetPen(FLUX_PEN);
			dc.DrawLine({x, h2+h4}, {x+fw, h2+h4});

			for (const auto& record : trackdata->records)
			{
				int rp = record->startTime / _nanosecondsPerPixel;
				int rw = (record->endTime - record->startTime) / _nanosecondsPerPixel;

				dc.SetPen(FOREGROUND_PEN);
				dc.SetBrush(RECORD_BRUSH);
				dc.DrawRectangle({x+rp, h8}, {rw, h4});
			}

			FluxmapReader fmr(*trackdata->fluxmap);
			dc.SetPen(FLUX_PEN);
			while (!fmr.eof())
			{
				int event;
				unsigned ticks;
				fmr.getNextEvent(event, ticks);

				int fx = fmr.tell().ns() / _nanosecondsPerPixel;

				if (event & F_BIT_INDEX)
				{
					dc.SetPen(INDEX_SEPARATOR_PEN);
					dc.DrawLine({x+fx, 0}, {x+fx, h});
					dc.SetPen(FLUX_PEN);
				}

				if (event & F_BIT_PULSE)
					dc.DrawLine({x+fx, h2+h8}, {x+fx, h2+h4+h8});
			}
		}

		x += w;
	}
}

void FluxViewerControl::OnMouseWheel(wxMouseEvent& event)
{
    wxClientDC dc(this);
	int x = event.GetLogicalPosition(dc).x;
	
	_scrollPosition += x * _nanosecondsPerPixel;
	if (event.GetWheelRotation() > 0)
		_nanosecondsPerPixel /= 1.2;
	else
		_nanosecondsPerPixel *= 1.2;
	_scrollPosition -= x * _nanosecondsPerPixel;

	ResizeScrollbar();
	Refresh();
}

void FluxViewerControl::OnScrollbarChanged(wxScrollEvent& event)
{
	_scrollPosition = event.GetPosition() * 1000LL;
	Refresh();
}

