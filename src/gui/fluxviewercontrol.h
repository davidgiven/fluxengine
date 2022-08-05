#ifndef FLUXVIEWERCONTROL_H
#define FLUXVIEWERCONTROL_H

class FluxViewerControl : public wxWindow
{
public:
    FluxViewerControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);
};

#endif

