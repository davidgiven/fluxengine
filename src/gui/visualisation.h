#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <wx/control.h>

class VisualisationControl : public wxWindow
{
public:
    VisualisationControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);

private:
	void OnPaint(wxPaintEvent & evt);

private:
    wxDECLARE_EVENT_TABLE();
};

#endif
