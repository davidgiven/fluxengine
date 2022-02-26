#include "globals.h"
#include "gui.h"
#include "mainwindow.h"
#include "utils.h"

class FluxEngineApp;
class ExecEvent;

static wxSemaphore execSemaphore(0);

wxDEFINE_EVENT(EXEC_EVENT_TYPE, ExecEvent);
class ExecEvent : public wxThreadEvent
{
public:
    ExecEvent(wxEventType commandType = EXEC_EVENT_TYPE, int id = 0):
        wxThreadEvent(commandType, id)
    {
    }

    ExecEvent(const ExecEvent& event):
        wxThreadEvent(event),
        _callback(event._callback)
    {
    }

    wxEvent* Clone() const
    {
        return new ExecEvent(*this);
    }

	void SetCallback(const std::function<void()> callback)
	{
		_callback = callback;
	}

	void RunCallback() const
	{
		_callback();
	}

private:
    std::function<void()> _callback;
};

bool FluxEngineApp::OnInit()
{
	Bind(EXEC_EVENT_TYPE, &FluxEngineApp::OnExec, this);
    _mainWindow = new MainWindow();
    _mainWindow->Show(true);
    return true;
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
		Logger() << ErrorLogMessage { e.message+'\n' };
	}
	catch (const EmergencyStopException& e)
	{
		Logger() << "Emergency stop!\n";
	}

	runOnUiThread(
		[&] {
			_callback = nullptr;
			_mainWindow->UpdateState();
		}
	);
	return 0;
}

void FluxEngineApp::RunOnWorkerThread(std::function<void()> callback)
{
	if (_callback)
		std::cerr << "Cannot start new worker task as one is already running\n";
	_callback = callback;

	if (GetThread())
		GetThread()->Wait();

	emergencyStop = false;
	CreateThread(wxTHREAD_JOINABLE);
	GetThread()->Run();
	_mainWindow->UpdateState();
}

void runOnWorkerThread(std::function<void()> callback)
{
	wxGetApp().RunOnWorkerThread(callback);
}

bool FluxEngineApp::IsWorkerThreadRunning() const
{
	return !!_callback;
}

void FluxEngineApp::OnExec(const ExecEvent& event)
{
	event.RunCallback();
	execSemaphore.Post();
}

void runOnUiThread(std::function<void()> callback)
{
	ExecEvent* event = new ExecEvent();
	event->SetCallback(callback);
	wxGetApp().QueueEvent(event);
	execSemaphore.Wait();
}

wxIMPLEMENT_APP(FluxEngineApp);
