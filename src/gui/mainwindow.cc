#include "globals.h"
#include "proto.h"
#include "gui.h"
#include "logger.h"
#include "reader.h"
#include "writer.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "lib/usb/usbfinder.h"
#include "fmt/format.h"
#include "mapper.h"
#include "utils.h"
#include "mainwindow.h"
#include <google/protobuf/text_format.h>

extern const std::map<std::string, std::string> formats;

MainWindow::MainWindow(): MainWindowGen(nullptr)
{
    Logger::setLogger(
        [&](std::shared_ptr<const AnyLogMessage> message)
        {
            runOnUiThread(
                [message, this]()
                {
                    OnLogMessage(message);
                });
        });

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
    readImageButton->Bind(wxEVT_BUTTON, &MainWindow::OnReadImageButton, this);
	writeFluxButton->Bind(wxEVT_BUTTON, &MainWindow::OnWriteFluxButton, this);
    writeImageButton->Bind(wxEVT_BUTTON, &MainWindow::OnWriteImageButton, this);
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
        PrepareConfig();

        FluxSource::updateConfigForFilename(config.mutable_flux_source(),
            fluxSourceSinkCombo->GetValue().ToStdString());
		visualiser->Clear();
		_currentDisk = nullptr;

		SetHighDensity();
		ShowConfig();
        runOnWorkerThread(
            [this]()
            {
                auto fluxSource = FluxSource::create(config.flux_source());
                auto decoder = AbstractDecoder::create(config.decoder());
                auto diskflux = readDiskCommand(*fluxSource, *decoder);

                runOnUiThread(
                    [&]()
                    {
                        visualiser->SetDiskData(diskflux);
                    });
            });
    }
    catch (const ErrorException& e)
    {
        wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
    }
}

void MainWindow::OnWriteFluxButton(wxCommandEvent&)
{
    try
    {
        PrepareConfig();

        FluxSink::updateConfigForFilename(config.mutable_flux_sink(),
            fluxSourceSinkCombo->GetValue().ToStdString());
        FluxSource::updateConfigForFilename(config.mutable_flux_source(),
            fluxSourceSinkCombo->GetValue().ToStdString());

		SetHighDensity();
		ShowConfig();
		auto image = _currentDisk->image;
        runOnWorkerThread(
            [image, this]()
            {
				auto encoder = AbstractEncoder::create(config.encoder());
				auto fluxSink = FluxSink::create(config.flux_sink());

				std::unique_ptr<AbstractDecoder> decoder;
				std::unique_ptr<FluxSource> fluxSource;
				if (config.has_decoder())
				{
					decoder = AbstractDecoder::create(config.decoder());
					fluxSource = FluxSource::create(config.flux_source());
				}
				writeDiskCommand(image, *encoder, *fluxSink, decoder.get(), fluxSource.get());
            });
    }
    catch (const ErrorException& e)
    {
        wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
    }
}

void MainWindow::OnReadImageButton(wxCommandEvent&)
{
    try
    {
        PrepareConfig();
        if (!config.has_image_reader())
            Error() << "This format is read-only.";

        auto filename = wxFileSelector(
			"Choose a image file to read",
			/* default_path= */ wxEmptyString,
			/* default_filename= */ config.image_reader().filename(),
			/* default_extension= */ wxEmptyString,
			/* wildcard= */ wxEmptyString,
			/* flags= */ wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (filename.empty())
            return;

        ImageReader::updateConfigForFilename(
            config.mutable_image_reader(), filename.ToStdString());
		visualiser->Clear();
		_currentDisk = nullptr;

		ShowConfig();
        runOnWorkerThread(
            [this]()
            {
                auto imageReader = ImageReader::create(config.image_reader());
                std::unique_ptr<const Image> image = imageReader->readImage();
                runOnUiThread(
                    [&]()
                    {
                        auto disk = std::make_shared<DiskFlux>();
                        disk = std::make_shared<DiskFlux>();
                        disk->image = std::move(image);
                        _currentDisk = disk;
                        visualiser->SetDiskData(_currentDisk);
                    });
            });
    }
    catch (const ErrorException& e)
    {
        wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
    }
}

void MainWindow::OnWriteImageButton(wxCommandEvent&)
{
    try
    {
        PrepareConfig();
        if (!config.has_image_writer())
            Error() << "This format is write-only.";

        auto filename = wxFileSelector(
			"Choose a image file to write",
			/* default_path= */ wxEmptyString,
			/* default_filename= */ config.image_writer().filename(),
			/* default_extension= */ wxEmptyString,
			/* wildcard= */ wxEmptyString,
			/* flags= */ wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (filename.empty())
            return;

        ImageWriter::updateConfigForFilename(
            config.mutable_image_writer(), filename.ToStdString());

		ShowConfig();
		auto image = _currentDisk->image;
        runOnWorkerThread(
            [image, this]()
            {
				auto imageWriter = ImageWriter::create(config.image_writer());
				imageWriter->writeImage(*image);
            });
    }
    catch (const ErrorException& e)
    {
        wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
    }
}

/* This sets the *global* config object. That's safe provided the worker thread
 * isn't running, otherwise you'll get a race. */
void MainWindow::PrepareConfig()
{
	assert(!wxGetApp().IsWorkerThreadRunning());

    auto formatSelection = formatChoice->GetSelection();
    if (formatSelection == wxNOT_FOUND)
        Error() << "no format selected";

	config = *_formats[formatChoice->GetSelection()];

    auto serial = deviceCombo->GetValue().ToStdString();
    if (!serial.empty() && (serial[0] == '/'))
        setProtoByString(&config, "usb.greaseweazle.port", serial);
    else
        setProtoByString(&config, "usb.serial", serial);

    ApplyCustomSettings();
    logEntry->Clear();
}

void MainWindow::SetHighDensity()
{
	bool hd = highDensityToggle->GetValue();
	config.mutable_drive()->set_high_density(hd);
}

void MainWindow::ShowConfig()
{
	std::string s;
	google::protobuf::TextFormat::PrintToString(config, &s);
	protoConfigEntry->Clear();
	protoConfigEntry->AppendText(s);
}

void MainWindow::ApplyCustomSettings()
{
    for (int i = 0; i < additionalSettingsEntry->GetNumberOfLines(); i++)
    {
        auto setting = additionalSettingsEntry->GetLineText(i).ToStdString();
		setting = trimWhitespace(setting);
        if (setting.size() == 0)
            continue;

        auto equals = setting.find('=');
        if (equals != std::string::npos)
        {
            auto key = setting.substr(0, equals);
            auto value = setting.substr(equals + 1);
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

            /* A fatal error. */
            [&](const ErrorLogMessage& m)
            {
                wxMessageBox(m.message, "Error", wxOK | wxICON_ERROR);
            },

            /* Indicates that we're starting a write operation. */
            [&](const BeginWriteOperationLogMessage& m)
            {
                visualiser->SetMode(m.track, m.head, VISMODE_WRITING);
            },

            [&](const EndWriteOperationLogMessage& m)
            {
                visualiser->SetMode(0, 0, VISMODE_NOTHING);
            },

            /* Indicates that we're starting a read operation. */
            [&](const BeginReadOperationLogMessage& m)
            {
                visualiser->SetMode(m.track, m.head, VISMODE_READING);
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
	bool running = wxGetApp().IsWorkerThreadRunning();

    writeImageButton->Enable(!running && !!_currentDisk);
    writeFluxButton->Enable(!running && !!_currentDisk);
    stopButton->Enable(running);
    readFluxButton->Enable(!running);
    readImageButton->Enable(!running);
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
