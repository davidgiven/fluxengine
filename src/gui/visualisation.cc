#include "globals.h"
#include "visualisation.h"
#include "fmt/format.h"
#include <wx/wx.h>

#define BORDER 20
#define TICK 10
#define TRACKS 82

#define SECTORSIZE 4

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

	int scaleheight = h - BORDER*4;
	int centrey = h * 1.5;
	int outerradius = centrey - BORDER;
	int innerradius = centrey - h + BORDER;
	int scaletop = BORDER*2;
	int scalebottom = h - BORDER*2;
	double trackstep = (double)scaleheight / TRACKS;

	wxPaintDC dc(this);
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxLIGHT_GREY_BRUSH);
	dc.DrawCircle({ w2, centrey }, outerradius);
	dc.SetBrush(dc.GetBackground());
	dc.DrawCircle({ w2, centrey }, innerradius);

	dc.DrawLine({ w2, scaletop }, { w2, scalebottom });
	dc.DrawLine({ w2-TICK, scaletop }, { w2+TICK, scaletop });
	dc.DrawLine(
		{ w2-TICK, int(scaletop + 80*trackstep) },
		{ w2+TICK, int(scaletop + 80*trackstep) });

	if (_mode != VISMODE_NOTHING)
	{
		if (_mode == VISMODE_READING)
			dc.SetBrush(*wxGREEN_BRUSH);
		else if (_mode == VISMODE_WRITING)
			dc.SetBrush(*wxRED_BRUSH);

		int factor = (_head == 0) ? -1 : 1;

		dc.SetPen(*wxTRANSPARENT_PEN);
		int y = scaletop + (_cylinder * trackstep) - trackstep / 2;
		dc.DrawRectangle(
			{ w2 + factor*SECTORSIZE*2, y },
			{ factor*SECTORSIZE*82, (int)trackstep }
		);
	}

	dc.SetPen(*wxBLACK_PEN);
	for (int track = 0; track <= TRACKS; track++)
	{
		int y = (double)track * trackstep;
		dc.DrawLine({ w2-TICK/2, scaletop+y }, { w2+TICK/2, scaletop+y });
	}
}

void VisualisationControl::SetMode(int cylinder, int head, int mode)
{
	_cylinder = cylinder;
	_head = head;
	_mode = mode;
	Refresh();
}

void VisualisationControl::SetDiskFlux(std::shared_ptr<DiskFlux>& disk)
{
	_disk = disk;
	Refresh();
}

