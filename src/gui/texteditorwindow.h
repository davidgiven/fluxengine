#ifndef TEXTEDITORWINDOW_H
#define TEXTEDITORWINDOW_H

#include "layout.h"

class EditorSaveEvent : public wxEvent
{
public:
    EditorSaveEvent(wxEventType eventType, int winId): wxEvent(winId, eventType)
    {
    }

    wxEvent* Clone() const override
    {
        return new EditorSaveEvent(*this);
    }

    std::string text;
};

wxDECLARE_EVENT(EDITOR_SAVE_EVENT, EditorSaveEvent);

class TextEditorWindow : public TextEditorWindowGen
{
public:
    TextEditorWindow(
        wxWindow* parent, const std::string& title, const std::string& text);

    static TextEditorWindow* Create(
        wxWindow* parent, const std::string& title, const std::string& text);

public:
    wxTextCtrl* GetTextControl() const;

private:
    void OnClose(wxCloseEvent& event) override;
    void OnSave(wxCommandEvent& event) override;
    void OnCancel(wxCommandEvent& event) override;
};

#endif
