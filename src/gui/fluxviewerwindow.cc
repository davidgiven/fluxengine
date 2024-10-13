#include "lib/core/globals.h"
#include "gui.h"
#include "lib/data/layout.h"
#include "fluxviewerwindow.h"
#include "fluxviewercontrol.h"
#include "lib/data/flux.h"
#include "lib/data/layout.h"

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
