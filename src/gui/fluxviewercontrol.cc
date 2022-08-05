#include "globals.h"
#include "gui.h"
#include "fluxviewercontrol.h"

FluxViewerControl::FluxViewerControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent, id, pos, size, style, "FluxViewerControl")
{
	SetDoubleBuffered(true);
}


