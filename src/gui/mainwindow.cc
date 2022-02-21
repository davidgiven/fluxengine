#include "globals.h"
#include "proto.h"
#include "gui.h"
#include "logger.h"
#include "fluxsource/fluxsource.h"
#include "decoders/decoders.h"
#include "lib/usb/usbfinder.h"
#include "fmt/format.h"
#include <wx/wx.h>
#include "mainwindow.h"

extern const std::map<std::string, std::string> formats;

MainWindow::MainWindow(): MainWindowGen(nullptr)
{
	Logger::setLogger(
		[&](std::shared_ptr<const AnyLogMessage> message) {
			runOnUiThread(
				[message, this]() {
					std::cout << "UI thread got message "
						<< Logger::toString(*message)
						<< '\n'
						<< std::flush;
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

	readFluxButton->Bind(wxEVT_BUTTON, &MainWindow::OnReadFluxButton, this);
}

void MainWindow::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void MainWindow::OnReadFluxButton(wxCommandEvent&)
{
	ConfigProto config = *_formats[formatChoice->GetSelection()];

	FluxSource::updateConfigForFilename(config.mutable_flux_source(),
		fluxSourceSinkCombo->GetValue().ToStdString());

	auto serial = deviceCombo->GetValue().ToStdString();
	if (!serial.empty() && (serial[0] = '/'))
		setProtoByString(&config, "usb.greaseweazle.port", serial);
	else
		setProtoByString(&config, "usb.serial", serial);

	runOnWorkerThread(
		/* Must make another copy of all local parameters. */
		[=]() {
			auto fluxSource = FluxSource::create(config.flux_source());
			auto decoder = AbstractDecoder::create(config.decoder());
			printf("Worker thread!\n");
		}
	);
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

