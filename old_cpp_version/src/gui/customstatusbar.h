#ifndef CUSTOMSTATUSBAR_H
#define CUSTOMSTATUSBAR_H

class wxGauge;
class wxButton;

wxDECLARE_EVENT(PROGRESSBAR_STOP_EVENT, wxCommandEvent);

class CustomStatusBar : public wxStatusBar
{
public:
    CustomStatusBar(wxWindow* parent, wxWindowID id);

public:
    void ShowProgressBar();
    void HideProgressBar();
    void SetProgress(int amount);
    void SetLeftLabel(const std::string& text);
    void SetRightLabel(const std::string& text);

private:
    void OnSize(wxSizeEvent& event);

private:
    std::unique_ptr<wxGauge> _progressBar;
    std::unique_ptr<wxButton> _stopButton;
    std::unique_ptr<wxStaticText> _rightLabel;

    DECLARE_EVENT_TABLE();
};

#endif
