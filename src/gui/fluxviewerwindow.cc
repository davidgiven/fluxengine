#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "fluxviewerwindow.h"
#include "fluxviewercontrol.h"

FluxViewerWindow::FluxViewerWindow(wxWindow* parent, std::shared_ptr<const TrackFlux> flux):
	FluxViewerWindowGen(parent),
	_flux(flux)
{
	fluxviewer->SetScrollbar(scrollbar);
	fluxviewer->SetFlux(flux);
}

void FluxViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


