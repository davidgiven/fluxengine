#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "hexviewerwindow.h"
#include "fmt/format.h"

HexViewerWindow::HexViewerWindow(wxWindow* parent, const std::string& text):
	HexViewerWindowGen(parent)
{
	auto size = hexEntry->GetTextExtent("M");
	SetSize(size.Scale(85, 25));
	hexEntry->SetValue(text);
}

void HexViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


