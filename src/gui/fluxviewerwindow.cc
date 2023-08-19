#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "fluxviewerwindow.h"
#include "fluxviewercontrol.h"
#include "lib/flux.h"
#include "lib/layout.h"

FluxViewerWindow::FluxViewerWindow(
    wxWindow* parent, std::shared_ptr<const TrackFlux> flux):
    FluxViewerWindowGen(parent),
    _flux(flux)
{
    fluxviewer->SetScrollbar(scrollbar);
    fluxviewer->SetFlux(flux);
    SetTitle(fmt::format("Flux for c{} h{}",
        flux->trackInfo->physicalTrack,
        flux->trackInfo->physicalSide));
}

void FluxViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}
