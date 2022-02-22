#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <memory>
#include <wx/control.h>

class DiskFlux;

enum {
	VISMODE_NOTHING,
	VISMODE_READING,
	VISMODE_WRITING
};

class VisualisationControl : public wxWindow
{
public:
    VisualisationControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);

public:
	void SetMode(int head, int cylinder, int mode);
	void SetDiskFlux(std::shared_ptr<DiskFlux>& disk);

private:
	void OnPaint(wxPaintEvent & evt);

private:
	int _head;
	int _cylinder;
	int _mode = VISMODE_NOTHING;
	std::shared_ptr<DiskFlux> _disk;
    wxDECLARE_EVENT_TABLE();
};

#endif
