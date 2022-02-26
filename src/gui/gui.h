#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

#include <wx/wx.h>

class ExecEvent;
class MainWindow;

extern void runOnUiThread(std::function<void()> callback);
extern void runOnWorkerThread(std::function<void()> callback);

template <typename R>
static inline R runOnUiThread(std::function<R()> callback)
{
	R retvar;
	runOnUiThread(
		[&]() {
			retvar = callback();
		}
	);
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
	std::function<void()> _callback;
	MainWindow* _mainWindow;
};
wxDECLARE_APP(FluxEngineApp);

#endif

