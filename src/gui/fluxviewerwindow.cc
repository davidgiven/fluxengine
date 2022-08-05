#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "fluxviewerwindow.h"
#include "fluxviewercontrol.h"

FluxViewerWindow::FluxViewerWindow(): FluxViewerWindowGen(nullptr)
{
}

void FluxViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


