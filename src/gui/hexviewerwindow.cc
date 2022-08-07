#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "hexviewerwindow.h"

HexViewerWindow::HexViewerWindow(const std::string& text):
	HexViewerWindowGen(nullptr)
{
	auto size = hexEntry->GetFont().GetPixelSize();

	SetSize(size.Scale(85, 25));
	hexEntry->SetValue(text);
}

void HexViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


