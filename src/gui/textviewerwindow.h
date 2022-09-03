#ifndef HEXVIEWERWINDOW_H
#define HEXVIEWERWINDOW_H

#include "layout.h"

class TextViewerWindow : public TextViewerWindowGen
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

private:
    void OnExit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

private:
    bool _autodestroy;
};

#endif
