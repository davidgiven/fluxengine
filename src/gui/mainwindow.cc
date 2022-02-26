#include "globals.h"
#include "proto.h"
#include "gui.h"
#include "logger.h"
#include "reader.h"
#include "fluxsource/fluxsource.h"
#include "decoders/decoders.h"
#include "lib/usb/usbfinder.h"
#include "fmt/format.h"
#include "utils.h"
#include "mainwindow.h"
#include <google/protobuf/text_format.h>

extern const std::map<std::string, std::string> formats;

MainWindow::MainWindow(): MainWindowGen(nullptr)
{
	Logger::setLogger(
		[&](std::shared_ptr<const AnyLogMessage> message) {
			runOnUiThread(
				[message, this]() {
					OnLogMessage(message);
				}
			);
		}
	);

	for (const auto& it : formats)
	{
		auto config = std::make_unique<ConfigProto>();
		if (!config->ParseFromString(it.second))
			continue;
		if (config->is_extension())
			continue;

		formatChoice->Append(it.first);
		_formats.push_back(std::move(config));
	}

	UpdateDevices();
	if (deviceCombo->GetCount() > 0)
		deviceCombo->SetValue(deviceCombo->GetString(0));

	fluxSourceSinkCombo->SetValue(fluxSourceSinkCombo->GetString(0));

	readFluxButton->Bind(wxEVT_BUTTON, &MainWindow::OnReadFluxButton, this);
	stopButton->Bind(wxEVT_BUTTON, &MainWindow::OnStopButton, this);

	UpdateState();
}

void MainWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void MainWindow::OnStopButton(wxCommandEvent&)
{
	emergencyStop = true;
}

void MainWindow::OnReadFluxButton(wxCommandEvent&)
{
	try
	{
		auto formatSelection = formatChoice->GetSelection();
		if (formatSelection == wxNOT_FOUND)
			Error() << "no format selected";

		ConfigProto config = *_formats[formatChoice->GetSelection()];

		FluxSource::updateConfigForFilename(config.mutable_flux_source(),
			fluxSourceSinkCombo->GetValue().ToStdString());

		auto serial = deviceCombo->GetValue().ToStdString();
		if (!serial.empty() && (serial[0] == '/'))
			setProtoByString(&config, "usb.greaseweazle.port", serial);
		else
			setProtoByString(&config, "usb.serial", serial);

		ApplyCustomSettings(config);

		{
			std::string s;
			google::protobuf::TextFormat::PrintToString(config, &s);
			protoConfigEntry->Clear();
			protoConfigEntry->AppendText(s);
		}

		visualiser->Clear();
		logEntry->Clear();
		_currentDisk = nullptr;
		runOnWorkerThread(
			[config, this]() {
				::config = config;
				auto fluxSource = FluxSource::create(config.flux_source());
				auto decoder = AbstractDecoder::create(config.decoder());
				auto diskflux = readDiskCommand(*fluxSource, *decoder);
				runOnUiThread(
					[&]() {
						visualiser->SetDiskData(diskflux);
					}
				);
			}
		);
	}
	catch (const ErrorException& e)
	{
		wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
	}
}

void MainWindow::ApplyCustomSettings(ConfigProto& config)
{
	for (int i=0; i < additionalSettingsEntry->GetNumberOfLines(); i++)
	{
		auto setting = additionalSettingsEntry->GetLineText(i).ToStdString();
		trimWhitespace(setting);
		if (setting.size() == 0)
			continue;

		auto equals = setting.find('=');
		if (equals != std::string::npos)
		{
			auto key = setting.substr(0, equals);
			auto value = setting.substr(equals+1);
			setProtoByString(&config, key, value);
		}
		else
			FlagGroup::parseConfigFile(setting, formats);
	}
}

void MainWindow::OnLogMessage(std::shared_ptr<const AnyLogMessage> message)
{
	logEntry->AppendText(Logger::toString(*message));
	notebook->SetSelection(1);

    std::visit(
        overloaded{
            /* Fallback --- do nothing */
            [&](const auto& m)
            {
            },

            /* Indicates that we're starting a write operation. */
            [&](const BeginWriteOperationLogMessage& m)
            {
				visualiser->SetMode(m.cylinder, m.head, VISMODE_WRITING);
            },

            [&](const EndWriteOperationLogMessage& m)
            {
				visualiser->SetMode(0, 0, VISMODE_NOTHING);
            },

            /* Indicates that we're starting a read operation. */
            [&](const BeginReadOperationLogMessage& m)
            {
				visualiser->SetMode(m.cylinder, m.head, VISMODE_READING);
            },

            [&](const EndReadOperationLogMessage& m)
            {
				visualiser->SetMode(0, 0, VISMODE_NOTHING);
            },

            [&](const TrackReadLogMessage& m)
            {
				visualiser->SetTrackData(m.track);
            },

            [&](const DiskReadLogMessage& m)
            {
				_currentDisk = m.disk;
            },
        },
        *message);
}

void MainWindow::UpdateState()
{
	writeImageButton->Enable(!!_currentDisk);
	writeFluxButton->Enable(!!_currentDisk);
	stopButton->Enable(wxGetApp().IsWorkerThreadRunning());
	readFluxButton->Enable(!wxGetApp().IsWorkerThreadRunning());
	readImageButton->Enable(!wxGetApp().IsWorkerThreadRunning());
}

void MainWindow::UpdateDevices()
{
	auto candidates = findUsbDevices();

	deviceCombo->Clear();
	_devices.clear();
	for (auto& candidate : candidates)
	{
		deviceCombo->Append(candidate->serial);
		_devices.push_back(std::move(candidate));
	}
}

