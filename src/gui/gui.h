#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <wx/wx.h>

class ExecEvent;
class DiskFlux;
class TrackFlux;

extern void postToUiThread(std::function<void()> callback);
extern void runOnUiThread(std::function<void()> callback);
extern void runOnWorkerThread(std::function<void()> callback);
extern bool isWorkerThread();

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
    bool IsWorkerThread();
    bool IsWorkerThreadRunning();

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

class CancelException
{
};

class MainWindow
{
public:
    enum
    {
        PAGE_IDLE,
        PAGE_IMAGER,
        PAGE_BROWSER,
        PAGE_EXPLORER,
    };

    void GoIdle()
    {
        SetPage(PAGE_IDLE);
    }
    virtual void SetPage(int page) = 0;
    virtual void PrepareConfig() = 0;
    virtual void ClearLog() = 0;
};

class PanelComponent
{
public:
    virtual void SwitchTo(){};
    virtual void SwitchFrom(){};
};

class IdlePanel : public PanelComponent
{
public:
    virtual void Start() = 0;

    virtual void PrepareConfig() = 0;
};

class ImagerPanel : public PanelComponent
{
public:
    enum
    {
        STATE_READING,
        STATE_READING_SUCCEEDED,
        STATE_WRITING,
        STATE_WRITING_SUCCEEDED,
    };

public:
    virtual void StartReading() = 0;
    virtual void StartWriting() = 0;

    virtual void SetVisualiserMode(int head, int track, int mode) = 0;
    virtual void SetVisualiserTrackData(
        std::shared_ptr<const TrackFlux> track) = 0;
    virtual void SetDisk(std::shared_ptr<const DiskFlux> disk) = 0;
};

class BrowserPanel : public PanelComponent
{
public:
    virtual void StartBrowsing() = 0;
    virtual void StartFormatting() = 0;
};

class ExplorerPanel : public PanelComponent
{
public:
    virtual void Start() = 0;
};

#endif
