#include "globals.h"
#include <wx/wx.h>
#include "layout.h"
#include "gui.h"

class MyApp;
class ExecEvent;

static wxSemaphore execSemaphore(0);

class MyApp : public wxApp, public wxThreadHelper
{
public:
    virtual bool OnInit();
	void RunOnWorkerThread(std::function<void()> callback);

private:
	void OnExec(const ExecEvent& event);

protected:
	virtual wxThread::ExitCode Entry();

private:
	std::function<void()> _callback;
};
wxDECLARE_APP(MyApp);

class MainWindow : public MainWindowGen
{
public:
    MainWindow(): MainWindowGen(nullptr) {}

public:
    void OnExit(wxCommandEvent& event)
    {
        Close(true);
    }
};

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

bool MyApp::OnInit()
{
	Bind(EXEC_EVENT_TYPE, &MyApp::OnExec, this);
    MainWindow* frame = new MainWindow();
    frame->Show(true);

	runOnWorkerThread(
		[] {
			printf("I'm a worker thread!\n");
		}
	);
    return true;
}

wxThread::ExitCode MyApp::Entry()
{
	if (_callback)
		_callback();
	runOnUiThread(
		[&] {
			_callback = nullptr;
		}
	);
	return 0;
}

void MyApp::RunOnWorkerThread(std::function<void()> callback)
{
	if (_callback)
		std::cerr << "Cannot start new worker task as one is already running\n";
	_callback = callback;

	if (GetThread())
		GetThread()->Wait();

	CreateThread(wxTHREAD_JOINABLE);
	GetThread()->Run();
}

void runOnWorkerThread(std::function<void()> callback)
{
	wxGetApp().RunOnWorkerThread(callback);
}

void MyApp::OnExec(const ExecEvent& event)
{
	printf("exec handler\n");
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

wxIMPLEMENT_APP(MyApp);
