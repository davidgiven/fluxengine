#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "layout.h"

class ConfigProto;
class MainWindow : public MainWindowGen
{
public:
    MainWindow();

private:
    void OnExit(wxCommandEvent& event);

	void UpdateDevices();

private:
	std::map<std::string, std::unique_ptr<ConfigProto>> _formats;
};

#endif

