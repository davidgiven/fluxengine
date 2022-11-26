#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <wx/wx.h>

class ExecEvent;
class MainWindow;

extern void postToUiThread(std::function<void()> callback);
extern void runOnUiThread(std::function<void()> callback);
extern void runOnWorkerThread(std::function<void()> callback);

wxDECLARE_EVENT(UPDATE_STATE_EVENT, wxCommandEvent);

template <typename R>
static inline R runOnUiThread(std::function<R()> callback)
{
    R retvar;
    runOnUiThread(
        [&]()
        {
            retvar = callback();
        });
    return retvar;
}

class FluxEngineApp : public wxApp, public wxThreadHelper
{
public:
    virtual bool OnInit();
    void RunOnWorkerThread(std::function<void()> callback);

private:
    void OnExec(const ExecEvent& event);

public:
    bool IsWorkerThreadRunning() const;

protected:
    virtual wxThread::ExitCode Entry();

private:
    static wxWindow* CreateMainWindow();
    void SendUpdateEvent();

private:
    std::function<void()> _callback;
    wxWindow* _mainWindow;
};
wxDECLARE_APP(FluxEngineApp);

#define DECLARE_COLOUR(name, red, green, blue)             \
    static const wxColour name##_COLOUR(red, green, blue); \
    static const wxBrush name##_BRUSH(name##_COLOUR);      \
    static const wxPen name##_PEN(name##_COLOUR)

#endif
