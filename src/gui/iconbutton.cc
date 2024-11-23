#include "lib/core/globals.h"
#include "gui.h"
#include "iconbutton.h"

IconButton::IconButton(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name):
    wxPanel(parent, id, pos, size, style, name)
{
    _sizer = new wxFlexGridSizer(1, 0, 0);
    SetSizer(_sizer);

    _bitmap = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    _sizer->Add(_bitmap, 0, wxALL | wxEXPAND, 0, nullptr);

    _text = new wxStaticText(this,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxALIGN_CENTRE_HORIZONTAL);
    _sizer->Add(_text, 0, wxALL | wxEXPAND, 0, nullptr);

    _text->SetFont(_text->GetFont().MakeSmaller().MakeSmaller().MakeSmaller());

    Bind(wxEVT_LEFT_DOWN, &IconButton::OnMouseClick, this);
    _bitmap->Bind(wxEVT_LEFT_DOWN, &IconButton::OnMouseClick, this);
    _text->Bind(wxEVT_LEFT_DOWN, &IconButton::OnMouseClick, this);
}

void IconButton::SetSelected(bool selected)
{
    _selected = selected;
    wxColor bg = wxSystemSettings::GetColour(
        _selected ? wxSYS_COLOUR_HIGHLIGHT : wxSYS_COLOUR_FRAMEBK);
    SetBackgroundColour(bg);
    Refresh();
}

void IconButton::OnMouseClick(wxMouseEvent&)
{
    auto* event = new wxCommandEvent(wxEVT_BUTTON, 0);
    wxQueueEvent(this, event);
}

void IconButton::SetBitmapAndLabel(
    const wxBitmap bitmap, const std::string text)
{
    _bitmap->SetBitmap(bitmap);
    _text->SetLabel(text);
}
