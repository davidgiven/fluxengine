#pragma once

class wxToggleButton;

class IconButton : public wxPanel
{
public:
    IconButton(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxPanelNameStr);

    void SetBitmapAndLabel(const wxBitmap bitmap, const std::string text);
    void SetSelected(bool selected);

private:
    void OnMouseClick(wxMouseEvent& e);

private:
    wxFlexGridSizer* _sizer;
    wxStaticBitmap* _bitmap;
    wxStaticText* _text;
    bool _selected;
};
