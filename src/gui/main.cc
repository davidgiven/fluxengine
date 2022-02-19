#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include "layout.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

class MainWindow : public MainWindowGen
{
public:
	MainWindow():
		MainWindowGen(nullptr)
	{}

public:
	void OnExit(wxCommandEvent& event)
	{
		Close(true);
	}
};

bool MyApp::OnInit()
{
    MainWindow *frame = new MainWindow();
    frame->Show(true);
    return true;
	}

wxIMPLEMENT_APP(MyApp);
