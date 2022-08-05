#ifndef FLUXVIEWERWINDOW_H
#define FLUXVIEWERWINDOW_H

#include "layout.h"

class FluxViewerWindow : public FluxViewerWindowGen
{
public:
    FluxViewerWindow();

private:
    void OnExit(wxCommandEvent& event);

public:
};

#endif

