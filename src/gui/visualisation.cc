#include "globals.h"
#include "visualisation.h"
#include "fmt/format.h"
#include <wx/wx.h>

#define BORDER 20
#define TICK 3
#define TRACKS 82

#define SECTORSIZE 5

static const wxColour DARK_GREY(0x80, 0x80, 0x80);
static const wxBrush DARK_GREY_BRUSH(DARK_GREY);
static const wxPen DARK_GREY_PEN(DARK_GREY);

VisualisationControl::VisualisationControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
	 	wxWindow(parent, id, pos, size, style, "VisualisationControl")
{
}

wxBEGIN_EVENT_TABLE(VisualisationControl, wxPanel)
	EVT_PAINT(VisualisationControl::OnPaint)
wxEND_EVENT_TABLE()

void VisualisationControl::OnPaint(wxPaintEvent&)
{
	auto size = GetSize();
	int w = size.GetWidth();
	int w2 = w/2;
	int h = size.GetHeight();

	int centrey = h * 1.5;
	int outerradius = centrey - BORDER;
	int innerradius = centrey - h + BORDER;
	int scalesize = TRACKS*SECTORSIZE;
	int scaletop = h/2 - scalesize/2;
	int scalebottom = scaletop + scalesize;

	wxPaintDC dc(this);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxLIGHT_GREY_BRUSH);
	dc.DrawCircle({ w2, centrey }, outerradius);
	dc.SetBrush(dc.GetBackground());
	dc.DrawCircle({ w2, centrey }, innerradius);

	dc.SetPen(DARK_GREY_PEN);
	dc.DrawLine({ w2, scaletop }, { w2, scalebottom });

	if (_mode != VISMODE_NOTHING)
	{
		if (_mode == VISMODE_READING)
			dc.SetBrush(*wxGREEN_BRUSH);
		else if (_mode == VISMODE_WRITING)
			dc.SetBrush(*wxRED_BRUSH);

		int factor = (_head == 0) ? -1 : 1;

		dc.SetPen(*wxTRANSPARENT_PEN);
		int y = scaletop + _cylinder * SECTORSIZE;
		dc.DrawRectangle(
			{ w2 + factor*SECTORSIZE, y },
			{ factor*SECTORSIZE*82, SECTORSIZE-1 }
		);
	}

	dc.SetBrush(DARK_GREY_BRUSH);
	dc.SetPen(DARK_GREY_PEN);
	for (int track = 0; track <= TRACKS; track++)
	{
		int y = scaletop + track*SECTORSIZE;
		dc.DrawRectangle(
			{ w2-TICK/2, y },
			{ TICK, SECTORSIZE-1 }
		);
	}
}

void VisualisationControl::SetMode(int cylinder, int head, int mode)
{
	_cylinder = cylinder;
	_head = head;
	_mode = mode;
	Refresh();
}

void VisualisationControl::SetDiskFlux(std::shared_ptr<const DiskFlux>& disk)
{
	_disk = disk;
	Refresh();
}

