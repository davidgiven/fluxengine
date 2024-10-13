#include "lib/core/globals.h"
#include "gui.h"
#include "lib/data/layout.h"
#include "texteditorwindow.h"

wxDEFINE_EVENT(EDITOR_SAVE_EVENT, EditorSaveEvent);

TextEditorWindow::TextEditorWindow(
    wxWindow* parent, const std::string& title, const std::string& text):
    TextEditorWindowGen(parent)
{
    auto size = textControl->GetTextExtent("M");
    SetSize(size.Scale(85, 25));
    SetTitle(title);
    textControl->SetValue(text);
}

TextEditorWindow* TextEditorWindow::Create(
    wxWindow* parent, const std::string& title, const std::string& text)
{
    return new TextEditorWindow(parent, title, text);
}

wxTextCtrl* TextEditorWindow::GetTextControl() const
{
    return textControl;
}

void TextEditorWindow::OnClose(wxCloseEvent& event)
{
    event.Veto();

    if (textControl->IsModified())
    {
        int res = wxMessageBox(
            "You have unsaved changes. Do you wish to discard them?",
            "Unsaved changes",
            wxOK | wxCANCEL | wxICON_WARNING);
        if (res != wxOK)
            return;
    }

    Destroy();
}

void TextEditorWindow::OnSave(wxCommandEvent&)
{
    EditorSaveEvent event(EDITOR_SAVE_EVENT, GetId());
    event.SetEventObject(this);
    event.text = textControl->GetValue();
    ProcessWindowEvent(event);

    Destroy();
}

void TextEditorWindow::OnCancel(wxCommandEvent&)
{
    Destroy();
}
