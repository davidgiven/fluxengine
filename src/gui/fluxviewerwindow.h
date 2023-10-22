#ifndef FLUXVIEWERWINDOW_H
#define FLUXVIEWERWINDOW_H

#include "layout.h"

class TrackFlux;

class FluxViewerWindow : public FluxViewerWindowGen
{
public:
    FluxViewerWindow(wxWindow* parent, std::shared_ptr<const TrackFlux> flux);

private:
    void OnExit(wxCommandEvent& event);

private:
    std::shared_ptr<const TrackFlux> _flux;
};

#endif
