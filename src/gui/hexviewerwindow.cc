#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "hexviewerwindow.h"

HexViewerWindow::HexViewerWindow(const std::string& text):
	HexViewerWindowGen(nullptr)
{
}

void HexViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


