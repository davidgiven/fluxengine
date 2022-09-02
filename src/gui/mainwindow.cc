#include "globals.h"
#include "proto.h"
#include "gui.h"
#include "logger.h"
#include "readerwriter.h"
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
#include "fluxviewerwindow.h"
#include <google/protobuf/text_format.h>
#include <wx/config.h>

extern const std::map<std::string, std::string> formats;

#define CONFIG_SELECTEDSOURCE "SelectedSource"
#define CONFIG_DEVICE "Device"
#define CONFIG_DRIVE "Drive"
#define CONFIG_HIGHDENSITY "HighDensity"
#define CONFIG_FORMAT "Format"
#define CONFIG_EXTRACONFIG "ExtraConfig"
#define CONFIG_FLUXIMAGE "FluxImage"
#define CONFIG_DISKIMAGE "DiskImage"

class MainWindow : public MainWindowGen
{
public:
    MainWindow(): MainWindowGen(nullptr), _config("FluxEngine")
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

        wxString defaultFormatName = "ibm";
        _config.Read(CONFIG_FORMAT, &defaultFormatName);

        int defaultFormat = 0;
        int i = 0;
        for (const auto& it : formats)
        {
            auto config = std::make_unique<ConfigProto>();
            if (!config->ParseFromString(it.second))
                continue;
            if (config->is_extension())
                continue;

            formatChoice->Append(it.first);
            if (it.first == defaultFormatName)
                defaultFormat = i;
            _formats.push_back(std::move(config));
            i++;
        }

        UpdateDevices();
        if (deviceCombo->GetCount() > 0)
            deviceCombo->SetValue(deviceCombo->GetString(0));

        if (MainWindow::formatChoice->GetCount() > 0)
            formatChoice->SetSelection(defaultFormat);

        // wxString defaultFluxSourceSink = sourceCombo->GetString(0);
        //_config.Read(CONFIG_FLUX, &defaultFluxSourceSink);
        // sourceCombo->SetValue(defaultFluxSourceSink);

        Bind(UPDATE_STATE_EVENT,
            [this](wxCommandEvent&)
            {
                UpdateState();
            });

        realDiskRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);
        fluxImageRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);
        diskImageRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);

        driveChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);
        deviceCombo->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);
        highDensityToggle->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);

        fluxImagePicker->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);

        diskImagePicker->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);

        formatChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED,
            &MainWindow::OnControlsChanged,
            this);
        // sourceCombo->Bind(wxEVT_TEXT, &MainWindow::OnControlsChanged, this);
        // readFluxButton->Bind(wxEVT_BUTTON, &MainWindow::OnReadFluxButton,
        // this); readImageButton->Bind(wxEVT_BUTTON,
        // &MainWindow::OnReadImageButton, this);
        // writeFluxButton->Bind(wxEVT_BUTTON, &MainWindow::OnWriteFluxButton,
        // this);
        // writeImageButton->Bind(wxEVT_BUTTON, &MainWindow::OnWriteImageButton,
        // this); stopTool->Bind(wxEVT_BUTTON, &MainWindow::OnStopButton, this);
        visualiser->Bind(
            TRACK_SELECTION_EVENT, &MainWindow::OnTrackSelection, this);

        LoadConfig();
        UpdateState();
    }

    void OnExit(wxCommandEvent& event)
    {
        Close(true);
    }

    void OnConfigRadioButtonClicked(wxCommandEvent& event)
    {
        auto configRadioButton = [&](wxRadioButton* button)
        {
            auto* following = button->GetNextSibling();
            if (button->GetValue())
                following->Show();
            else
                following->Hide();
        };
        configRadioButton(realDiskRadioButton);
        configRadioButton(fluxImageRadioButton);
        configRadioButton(diskImageRadioButton);
        idlePanel->Layout();

        if (realDiskRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_REAL;
        if (fluxImageRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_FLUX;
        if (diskImageRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_IMAGE;

        OnControlsChanged(event);
    }

    void OnControlsChanged(wxCommandEvent& event)
    {
        SaveConfig();
        UpdateState();
    }

    void OnStopButton(wxCommandEvent&)
    {
        emergencyStop = true;
    }

    void OnReadFluxButton(wxCommandEvent&)
    {
        try
        {
            PrepareConfig();

            // FluxSource::updateConfigForFilename(config.mutable_flux_source(),
            //     sourceCombo->GetValue().ToStdString());
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

    void OnWriteFluxButton(wxCommandEvent&)
    {
        try
        {
            PrepareConfig();

            // FluxSink::updateConfigForFilename(config.mutable_flux_sink(),
            //     sourceCombo->GetValue().ToStdString());
            // FluxSource::updateConfigForFilename(config.mutable_flux_source(),
            //     sourceCombo->GetValue().ToStdString());

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
                    writeDiskCommand(*image,
                        *encoder,
                        *fluxSink,
                        decoder.get(),
                        fluxSource.get());
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnReadImageButton(wxCommandEvent&)
    {
        try
        {
            PrepareConfig();
            if (!config.has_image_reader())
                Error() << "This format cannot be read from images.";

            auto filename = wxFileSelector("Choose a image file to read",
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
                    auto imageReader =
                        ImageReader::create(config.image_reader());
                    std::unique_ptr<const Image> image =
                        imageReader->readImage();
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

    void OnWriteImageButton(wxCommandEvent&)
    {
        try
        {
            PrepareConfig();
            if (!config.has_image_writer())
                Error() << "This format cannot be written to disks.";

            auto filename = wxFileSelector("Choose a image file to write",
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
                    auto imageWriter =
                        ImageWriter::create(config.image_writer());
                    imageWriter->writeImage(*image);
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
        }
    }

    /* This sets the *global* config object. That's safe provided the worker
     * thread isn't running, otherwise you'll get a race. */
    void PrepareConfig()
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

    void SetHighDensity()
    {
        bool hd = highDensityToggle->GetValue();
        config.mutable_drive()->set_high_density(hd);
    }

    void ShowConfig()
    {
        std::string s;
        google::protobuf::TextFormat::PrintToString(config, &s);
        protoConfigEntry->Clear();
        protoConfigEntry->AppendText(s);
    }

    void ApplyCustomSettings()
    {
        // for (int i = 0; i < additionalSettingsEntry->GetNumberOfLines(); i++)
        //{
        //     auto setting =
        //     additionalSettingsEntry->GetLineText(i).ToStdString();
        //	setting = trimWhitespace(setting);
        //    if (setting.size() == 0)
        //        continue;

        //    auto equals = setting.find('=');
        //    if (equals != std::string::npos)
        //    {
        //        auto key = setting.substr(0, equals);
        //        auto value = setting.substr(equals + 1);
        //        setProtoByString(&config, key, value);
        //    }
        //    else
        //        FlagGroup::parseConfigFile(setting, formats);
        //}
    }

    void OnLogMessage(std::shared_ptr<const AnyLogMessage> message)
    {
        logEntry->AppendText(Logger::toString(*message));
        // notebook->SetSelection(1);

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

    void LoadConfig()
    {
        /* Radio button config. */

        wxString s = std::to_string(SELECTEDSOURCE_REAL);
        _config.Read(CONFIG_SELECTEDSOURCE, &s);
        _selectedSource = std::atoi(s.c_str());

        switch (_selectedSource)
        {
            case SELECTEDSOURCE_REAL:
                realDiskRadioButton->SetValue(1);
                break;

            case SELECTEDSOURCE_FLUX:
                fluxImageRadioButton->SetValue(1);
                break;

            case SELECTEDSOURCE_IMAGE:
                diskImageRadioButton->SetValue(1);
                break;
        }

        wxCommandEvent dummyEvent;

        /* Real disk block. */

        s = "";
        _config.Read(CONFIG_DEVICE, &s);
        deviceCombo->SetValue(s);
        if (s.empty() && (deviceCombo->GetCount() > 0))
            deviceCombo->SetValue(deviceCombo->GetString(0));

        s = "0";
        _config.Read(CONFIG_DRIVE, &s);
        driveChoice->SetSelection(wxAtoi(s));

        s = "0";
        _config.Read(CONFIG_HIGHDENSITY, &s);
        highDensityToggle->SetValue(wxAtoi(s));

		/* Flux image block. */

		s = "";
		_config.Read(CONFIG_FLUXIMAGE, &s);
		fluxImagePicker->SetPath(s);

		/* Disk image block. */

		s = "";
		_config.Read(CONFIG_DISKIMAGE, &s);
		diskImagePicker->SetPath(s);

		/* Format block. */

        s = "ibm";
        _config.Read(CONFIG_FORMAT, &s);

        int defaultFormat = 0;
        int i = 0;
        for (const auto& it : formats)
        {
			if (it.first == s)
			{
				formatChoice->SetSelection(i);
				break;
			}
			i++;
		}

        /* Triggers SaveConfig */

        OnConfigRadioButtonClicked(dummyEvent);
    }

    void SaveConfig()
    {
        _config.Write(
            CONFIG_SELECTEDSOURCE, wxString(std::to_string(_selectedSource)));

        /* Real disk block. */

        _config.Write(CONFIG_DEVICE, deviceCombo->GetValue());
        _config.Write(CONFIG_DRIVE,
            wxString(std::to_string(driveChoice->GetSelection())));
        _config.Write(CONFIG_HIGHDENSITY,
            wxString(std::to_string(highDensityToggle->GetValue())));

		/* Flux image block. */

		_config.Write(CONFIG_FLUXIMAGE, fluxImagePicker->GetPath());

		/* Disk image block. */

		_config.Write(CONFIG_DISKIMAGE, diskImagePicker->GetPath());

		/* Format block. */

        _config.Write(CONFIG_FORMAT,
            formatChoice->GetString(formatChoice->GetSelection()));
    }

    void UpdateState()
    {
        bool running = wxGetApp().IsWorkerThreadRunning();

        // writeImageButton->Enable(!running && !!_currentDisk);
        // writeFluxButton->Enable(!running && !!_currentDisk);
        // stopTool->Enable(running);
        // readFluxButton->Enable(!running);
        // readImageButton->Enable(!running);
    }

    void UpdateDevices()
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

    void OnTrackSelection(TrackSelectionEvent& event)
    {
        (new FluxViewerWindow(this, event.trackFlux))->Show(true);
    }

private:
    enum
    {
        SELECTEDSOURCE_REAL,
        SELECTEDSOURCE_FLUX,
        SELECTEDSOURCE_IMAGE
    };

    wxConfig _config;
    std::vector<std::unique_ptr<const ConfigProto>> _formats;
    std::vector<std::unique_ptr<const CandidateDevice>> _devices;
    int _selectedSource;
    std::shared_ptr<const DiskFlux> _currentDisk;
};

wxWindow* FluxEngineApp::CreateMainWindow()
{
    return new MainWindow();
}
