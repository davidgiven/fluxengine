#include "lib/core/globals.h"
#include "lib/core/logger.h"
#include "gui.h"
#include "lib/core/utils.h"

class FluxEngineApp;
class ExecEvent;

static wxSemaphore execSemaphore(0);

wxDEFINE_EVENT(UPDATE_STATE_EVENT, wxCommandEvent);

wxDEFINE_EVENT(EXEC_EVENT_TYPE, ExecEvent);
class ExecEvent : public wxThreadEvent
{
public:
    ExecEvent(bool synchronous = true):
        wxThreadEvent(EXEC_EVENT_TYPE, 0),
        _synchronous(synchronous)
    {
    }

    ExecEvent(const ExecEvent& event):
        wxThreadEvent(event),
        _callback(event._callback)
    {
    }

    wxEvent* Clone() const override
    {
        return new ExecEvent(*this);
    }

    void SetCallback(const std::function<void()> callback)
    {
        _callback = callback;
    }

    bool IsSynchronous() const
    {
        return _synchronous;
    }

    void RunCallback() const
    {
        _callback();
    }

private:
    std::function<void()> _callback;
    bool _synchronous;
};

bool FluxEngineApp::OnInit()
{
    try
    {
        wxImage::AddHandler(new wxPNGHandler());
        Bind(EXEC_EVENT_TYPE, &FluxEngineApp::OnExec, this);
        _mainWindow = CreateMainWindow();
        _mainWindow->Show(true);
        return true;
    }
    catch (const ErrorException* e)
    {
        fmt::print(stderr, "Exception on startup: {}\n", e->message);
        exit(1);
    }
}

wxThread::ExitCode FluxEngineApp::Entry()
{
    try
    {
        if (_callback)
            _callback();
    }
    catch (const ErrorException& e)
    {
        log(ErrorLogMessage{e.message + '\n'});
    }
    catch (const EmergencyStopException& e)
    {
        log(EmergencyStopMessage());
    }

    postToUiThread(
        [&]
        {
            GetThread()->Wait();

            _callback = nullptr;
            SendUpdateEvent();
        });
    return 0;
}

void FluxEngineApp::RunOnWorkerThread(std::function<void()> callback)
{
    if (_callback)
        std::cerr << "Cannot start new worker task as one is already running\n";
    _callback = callback;

    if (GetThread() && GetThread()->IsRunning())
        GetThread()->Wait();

    emergencyStop = false;
    CreateThread(wxTHREAD_JOINABLE);
    GetThread()->Run();

    SendUpdateEvent();
}

void FluxEngineApp::SendUpdateEvent()
{
    auto* event = new wxCommandEvent(UPDATE_STATE_EVENT, 0);
    event->SetEventObject(_mainWindow);
    QueueEvent(event);
}

void runOnWorkerThread(std::function<void()> callback)
{
    wxGetApp().RunOnWorkerThread(callback);
}

bool isWorkerThread()
{
    return wxGetApp().IsWorkerThread();
}

bool FluxEngineApp::IsWorkerThread()
{
    return wxThread::GetCurrentId() != wxThread::GetMainId();
}

bool FluxEngineApp::IsWorkerThreadRunning()
{
    return !!_callback;
}

void FluxEngineApp::OnExec(const ExecEvent& event)
{
    try
    {
        event.RunCallback();
        if (event.IsSynchronous())
            execSemaphore.Post();
    }
    catch (std::exception& e)
    {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
    }
}

void runOnUiThread(std::function<void()> callback)
{
    ExecEvent* event = new ExecEvent();
    event->SetCallback(callback);
    wxGetApp().QueueEvent(event);
    execSemaphore.Wait();
}

void postToUiThread(std::function<void()> callback)
{
    ExecEvent* event = new ExecEvent(false);
    event->SetCallback(callback);
    wxGetApp().QueueEvent(event);
}

wxIMPLEMENT_APP(FluxEngineApp);
