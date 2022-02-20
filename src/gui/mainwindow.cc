#include "globals.h"
#include "proto.h"
#include "gui.h"
#include "fmt/format.h"
#include <wx/wx.h>
#include "mainwindow.h"

extern const std::map<std::string, std::string> formats;

MainWindow::MainWindow(): MainWindowGen(nullptr)
{
	for (const auto& it : formats)
	{
		auto config = std::make_unique<ConfigProto>();
		if (!config->ParseFromString(it.second))
			continue;
		if (config->is_extension())
			continue;

		formatChoice->Append(it.first);
		_formats[it.first] = std::move(config);
	}

	UpdateDevices();
}

void MainWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void MainWindow::UpdateDevices()
{
	//auto candidates = findUsbDevices({FLUXENGINE_ID, GREASEWEAZLE_ID});
}

