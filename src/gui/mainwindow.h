#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "layout.h"

class CandidateDevice;
class ConfigProto;

class MainWindow : public MainWindowGen
{
public:
    MainWindow();

private:
    void OnExit(wxCommandEvent& event);
	void OnReadFluxButton(wxCommandEvent&);

	void UpdateDevices();

private:
	std::vector<std::unique_ptr<ConfigProto>> _formats;
	std::vector<std::unique_ptr<CandidateDevice>> _devices;
};

#endif

