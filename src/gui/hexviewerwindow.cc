#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "hexviewerwindow.h"
#include "fmt/format.h"

HexViewerWindow::HexViewerWindow(wxWindow* parent,
		const std::string& title, const std::string& text):
	HexViewerWindowGen(parent)
{
	auto size = hexEntry->GetTextExtent("M");
	SetSize(size.Scale(85, 25));
	SetTitle(title);
	hexEntry->SetValue(text);
}

void HexViewerWindow::Create(wxWindow* parent, const std::string& title, const std::string& text)
{
	(new HexViewerWindow(parent, title, text))->Show(true);
}

void HexViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}


