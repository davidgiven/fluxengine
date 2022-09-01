#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "layout.h"
#include "logger.h"
#include <wx/config.h>

class CandidateDevice;
class ConfigProto;
class DiskFlux;

class MainWindow : public MainWindowGen
{
public:
    MainWindow();

private:
    void OnExit(wxCommandEvent& event);
	void OnStopButton(wxCommandEvent&);
	void OnConfigRadioButtonClicked(wxCommandEvent&);
	void OnReadFluxButton(wxCommandEvent&);
	void OnReadImageButton(wxCommandEvent&);
	void OnWriteFluxButton(wxCommandEvent&);
	void OnWriteImageButton(wxCommandEvent&);
	void OnLogMessage(std::shared_ptr<const AnyLogMessage> message);
	void OnTrackSelection(TrackSelectionEvent&);
	void OnControlsChanged(wxCommandEvent&);

public:
	void UpdateState();
	void UpdateDevices();
	void PrepareConfig();
	void ShowConfig();
	void ApplyCustomSettings();

private:
	void SetHighDensity();

private:
	wxConfig _config;
	std::vector<std::unique_ptr<const ConfigProto>> _formats;
	std::vector<std::unique_ptr<const CandidateDevice>> _devices;
	std::shared_ptr<const DiskFlux> _currentDisk;
};

#endif

