#ifndef HEXVIEWERWINDOW_H
#define HEXVIEWERWINDOW_H

#include "layout.h"

class HexViewerWindow : public HexViewerWindowGen
{
public:
    HexViewerWindow(wxWindow* parent, const std::string& text);

private:
    void OnExit(wxCommandEvent& event);
};

#endif

