#ifndef TEXTVIEWERWINDOW_H
#define TEXTVIEWERWINDOW_H

#include "layout.h"

class TextViewerWindow : public TextViewerWindowGen, public std::streambuf
{
public:
    TextViewerWindow(wxWindow* parent,
        const std::string& title,
        const std::string& text,
        bool autodestroy);

    static TextViewerWindow* Create(wxWindow* parent,
        const std::string& title,
        const std::string& text,
        bool autodestroy = true);

public:
    wxTextCtrl* GetTextControl() const;

public:
    std::streamsize xsputn(const char* s, std::streamsize n) override;
    int overflow(int c) override;

private:
    void OnClose(wxCloseEvent& event) override;

private:
    bool _autodestroy;
};

#endif
