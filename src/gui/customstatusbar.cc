#include "lib/core/globals.h"
#include "gui.h"
#include "customstatusbar.h"
#include <wx/artprov.h>

// clang-format off
BEGIN_EVENT_TABLE(CustomStatusBar, wxStatusBar)
	EVT_SIZE(CustomStatusBar::OnSize)
END_EVENT_TABLE()
// clang-format on

wxDEFINE_EVENT(PROGRESSBAR_STOP_EVENT, wxCommandEvent);

CustomStatusBar::CustomStatusBar(wxWindow* parent, wxWindowID id):
    wxStatusBar(parent, id)
{
    SetFieldsCount(4);

    static const int widths[] = {-1, 200, 100, 20};
    SetStatusWidths(4, widths);

    static const int styles[] = {wxSB_FLAT, wxSB_FLAT, wxSB_FLAT, wxSB_FLAT};
    SetStatusStyles(4, styles);

    _progressBar.reset(new wxGauge(this,
        wxID_ANY,
        100,
        wxDefaultPosition,
        wxDefaultSize,
        wxGA_HORIZONTAL | wxGA_SMOOTH));

    _stopButton.reset(new wxButton(this,
        wxID_ANY,
        "Stop",
        wxDefaultPosition,
        wxDefaultSize,
        wxBU_EXACTFIT));
    //_stopButton->SetBitmap(wxArtProvider::GetBitmap(wxART_ERROR,
    // wxART_BUTTON));
    _stopButton->Bind(wxEVT_BUTTON,
        [this](auto&)
        {
            auto* event = new wxCommandEvent(PROGRESSBAR_STOP_EVENT, 0);
            event->SetEventObject(this);
            QueueEvent(event);
        });

    _rightLabel.reset(new wxStaticText(this,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxALIGN_RIGHT | wxST_NO_AUTORESIZE));

    HideProgressBar();
    Layout();
}

void CustomStatusBar::OnSize(wxSizeEvent& event)
{
    auto buttonSize = _stopButton->GetEffectiveMinSize();

    wxRect r;
    GetFieldRect(1, r);
    int x = r.GetLeft();
    int y = r.GetTop();
    int w = r.GetWidth();
    int h = r.GetHeight();
    constexpr int b = 5;

    _stopButton->SetPosition({x + w - buttonSize.GetWidth(), y});
    _stopButton->SetSize(buttonSize.GetWidth(), h);

    _progressBar->SetPosition({x, y});
    _progressBar->SetSize(w - buttonSize.GetWidth() - b, h);

    GetFieldRect(2, r);
    _rightLabel->SetPosition(r.GetTopLeft());
    _rightLabel->SetSize(r);
}

void CustomStatusBar::ShowProgressBar()
{
    _progressBar->Show();
    _stopButton->Show();
}

void CustomStatusBar::HideProgressBar()
{
    _progressBar->Hide();
    _stopButton->Hide();
}

void CustomStatusBar::SetProgress(int amount)
{
    _progressBar->SetValue(amount);
}

void CustomStatusBar::SetLeftLabel(const std::string& text)
{
    SetStatusText(text, 0);
}

void CustomStatusBar::SetRightLabel(const std::string& text)
{
    _rightLabel->SetLabel(text);
}
