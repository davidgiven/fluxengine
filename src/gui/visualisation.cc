#include "globals.h"
#include "visualisation.h"
#include <wx/wx.h>

#define BORDER 20
#define TICK 10
#define TRACKS 82

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
	dc.DrawLine({ w2-TICK, scalebottom }, { w2+TICK, scalebottom });

	for (int track = 0; track < TRACKS; track++)
	{
		int y = (double)track * trackstep;
		dc.DrawLine({ w2-TICK/2, scaletop+y }, { w2+TICK/2, scaletop+y });
	}
}
