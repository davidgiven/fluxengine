#include "lib/core/globals.h"
#include "gui.h"
#include "lib/data/layout.h"
#include "textviewerwindow.h"

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

std::streamsize TextViewerWindow::xsputn(const char* s, std::streamsize n)
{
    textControl->AppendText(std::string(s, n));
    return n;
}

int TextViewerWindow::overflow(int c)
{
    char b = c;
    return xsputn(&b, 1);
}

void TextViewerWindow::OnClose(wxCloseEvent& event)
{
    if (_autodestroy)
        Destroy();
    else
        Hide();
}
