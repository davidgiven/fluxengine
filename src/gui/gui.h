#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <wx/wx.h>

class ConfigProto;
class ExecEvent;
class DiskFlux;
class TrackFlux;
class wxSimplebook;
class Context;

extern void postToUiThread(std::function<void()> callback);
extern void runOnUiThread(std::function<void()> callback);
extern void runOnWorkerThread(std::function<void()> callback);
extern bool isWorkerThread();

extern const std::map<std::string, const ConfigProto*> drivetypes;

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
    virtual bool OnInit() override;
    void RunOnWorkerThread(std::function<void()> callback);

private:
    void OnExec(const ExecEvent& event);

public:
    bool IsWorkerThread();
    bool IsWorkerThreadRunning();

protected:
    virtual wxThread::ExitCode Entry() override;

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

    virtual void StartIdle() = 0;
    virtual void StartReading() = 0;
    virtual void StartWriting() = 0;
    virtual void StartBrowsing() = 0;
    virtual void StartFormatting() = 0;
    virtual void StartExploring() = 0;

    virtual void SafeFit() = 0;
    virtual void SetPage(int page) = 0;
    virtual void PrepareConfig() = 0;
    virtual void ClearLog() = 0;
    virtual Context& GetContext() = 0;
};

class PanelComponent
{
public:
    PanelComponent(MainWindow* mainWindow): _mainWindow(mainWindow) {}

    virtual void SwitchTo(){};
    virtual void SwitchFrom(){};

    void PrepareConfig()
    {
        _mainWindow->PrepareConfig();
    }

    void SetPage(int page)
    {
        _mainWindow->SetPage(page);
    }

    void ClearLog()
    {
        _mainWindow->ClearLog();
    }

    void SafeFit()
    {
        _mainWindow->SafeFit();
    }

    void StartIdle()
    {
        _mainWindow->StartIdle();
    }

    void StartReading()
    {
        _mainWindow->StartReading();
    }

    void StartWriting()
    {
        _mainWindow->StartWriting();
    }

    void StartBrowsing()
    {
        _mainWindow->StartBrowsing();
    }

    void StartFormatting()
    {
        _mainWindow->StartFormatting();
    }

    void StartExploring()
    {
        _mainWindow->StartExploring();
    }

    Context& GetContext()
    {
        return _mainWindow->GetContext();
    }

private:
    MainWindow* _mainWindow;
};

class IdlePanel : public PanelComponent
{
public:
    static IdlePanel* Create(MainWindow* mainWindow, wxSimplebook* parent);
    IdlePanel(MainWindow* mainWindow): PanelComponent(mainWindow) {}

public:
    virtual void Start() = 0;

    virtual void PrepareConfig() = 0;
    virtual const wxBitmap GetBitmap() = 0;
};

class ImagerPanel : public PanelComponent
{
public:
    static ImagerPanel* Create(MainWindow* mainWindow, wxSimplebook* parent);
    ImagerPanel(MainWindow* mainWindow): PanelComponent(mainWindow) {}

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
    static BrowserPanel* Create(MainWindow* mainWindow, wxSimplebook* parent);
    BrowserPanel(MainWindow* mainWindow): PanelComponent(mainWindow) {}

public:
    virtual void StartBrowsing() = 0;
    virtual void StartFormatting() = 0;
};

class ExplorerPanel : public PanelComponent
{
public:
    static ExplorerPanel* Create(MainWindow* mainWindow, wxSimplebook* parent);
    ExplorerPanel(MainWindow* mainWindow): PanelComponent(mainWindow) {}

public:
    virtual void Start() = 0;
};

#endif
