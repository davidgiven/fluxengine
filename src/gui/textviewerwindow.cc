#include "globals.h"
#include "gui.h"
#include "layout.h"
#include "textviewerwindow.h"
#include "fmt/format.h"

// clang-format off
wxBEGIN_EVENT_TABLE(TextViewerWindow, wxFrame)
	EVT_CLOSE(TextViewerWindow::OnClose)
wxEND_EVENT_TABLE();
// clang-format on

TextViewerWindow::TextViewerWindow(wxWindow* parent,
    const std::string& title,
    const std::string& text,
    bool autodestroy):
    TextViewerWindowGen(parent),
    _autodestroy(autodestroy)
{
    auto size = textControl->GetTextExtent("M");
    SetSize(size.Scale(85, 25));
    SetTitle(title);
    textControl->SetValue(text);
}

TextViewerWindow* TextViewerWindow::Create(wxWindow* parent,
    const std::string& title,
    const std::string& text,
    bool autodestroy)
{
    return new TextViewerWindow(parent, title, text, autodestroy);
}

wxTextCtrl* TextViewerWindow::GetTextControl() const
{
    return textControl;
}

void TextViewerWindow::OnClose(wxCloseEvent& event)
{
    if (_autodestroy)
        Destroy();
    else
        Hide();
}

void TextViewerWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}
