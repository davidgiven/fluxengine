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
#include "customstatusbar.h"
#include <google/protobuf/text_format.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>

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
            _formats.push_back(std::make_pair(it.first, std::move(config)));
            i++;
        }

        UpdateDevices();

        Bind(UPDATE_STATE_EVENT,
            [this](wxCommandEvent&)
            {
                UpdateState();
            });

        menuBar->Bind(
            wxEVT_MENU, &MainWindow::OnAboutMenuItem, this, wxID_ABOUT);

        realDiskRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);
        fluxImageRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);
        diskImageRadioButton->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnConfigRadioButtonClicked, this);

        driveChoice->Bind(wxEVT_CHOICE, &MainWindow::OnControlsChanged, this);
        deviceCombo->Bind(wxEVT_TEXT, &MainWindow::OnControlsChanged, this);
        highDensityToggle->Bind(
            wxEVT_RADIOBUTTON, &MainWindow::OnControlsChanged, this);

        fluxImagePicker->Bind(
            wxEVT_FILEPICKER_CHANGED, &MainWindow::OnControlsChanged, this);

        diskImagePicker->Bind(
            wxEVT_FILEPICKER_CHANGED, &MainWindow::OnControlsChanged, this);

        formatChoice->Bind(wxEVT_CHOICE, &MainWindow::OnControlsChanged, this);

        readButton->Bind(wxEVT_BUTTON, &MainWindow::OnReadButton, this);
        writeButton->Bind(wxEVT_BUTTON, &MainWindow::OnWriteButton, this);
        //		browseButton->Bind(wxEVT_BUTTON,
        //&MainWindow::OnBrowseButtonPressed, this);

        imagerSaveImageButton->Bind(
            wxEVT_BUTTON, &MainWindow::OnSaveImageButton, this);
        imagerSaveFluxButton->Bind(
            wxEVT_BUTTON, &MainWindow::OnSaveFluxButton, this);
        imagerGoAgainButton->Bind(
            wxEVT_BUTTON, &MainWindow::OnImagerGoAgainButton, this);
        imagerToolbar->Bind(wxEVT_TOOL,
            &MainWindow::OnBackButton,
            this,
            imagerBackTool->GetId());
        browserToolbar->Bind(wxEVT_TOOL,
            &MainWindow::OnBackButton,
            this,
            imagerBackTool->GetId());

        visualiser->Bind(
            TRACK_SELECTION_EVENT, &MainWindow::OnTrackSelection, this);

        LoadConfig();
        UpdateState();

        CreateStatusBar();
    }

    void OnExit(wxCommandEvent& event)
    {
        Close(true);
    }

    void OnAboutMenuItem(wxCommandEvent& event)
    {
        wxAboutDialogInfo aboutInfo;
        aboutInfo.SetName("FluxEngine");
        aboutInfo.SetDescription("Flux-level floppy disk management");
        aboutInfo.SetWebSite("http://cowlark.com/fluxengine");
        aboutInfo.SetCopyright("Mostly (C) 2018-2022 David Given");

        wxAboutBox(aboutInfo);
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

    void OnReadButton(wxCommandEvent&)
    {
        try
        {
            PrepareConfig();

            visualiser->Clear();
            _currentDisk = nullptr;

            _state = STATE_READING_WORKING;
            UpdateState();
            ShowConfig();

            _errorState = STATE_READING_FAILED;
            runOnWorkerThread(
                [this]()
                {
                    auto fluxSource = FluxSource::create(config.flux_source());
                    auto decoder = AbstractDecoder::create(config.decoder());
                    auto diskflux = readDiskCommand(*fluxSource, *decoder);

                    runOnUiThread(
                        [&]()
                        {
                            _state = STATE_READING_SUCCEEDED;
                            UpdateState();
                            visualiser->SetDiskData(diskflux);
                        });
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            if (_state == STATE_READING_WORKING)
            {
                _state = STATE_READING_FAILED;
                UpdateState();
            }
        }
    }

    void OnWriteButton(wxCommandEvent&)
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

            _state = STATE_WRITING_WORKING;
            UpdateState();
            ShowConfig();

            _errorState = STATE_WRITING_FAILED;
            runOnWorkerThread(
                [this]()
                {
                    auto image =
                        ImageReader::create(config.image_reader())->readImage();
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

                    runOnUiThread(
                        [&]()
                        {
                            _state = STATE_WRITING_SUCCEEDED;
                            UpdateState();
                        });
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            if (_state == STATE_WRITING_WORKING)
            {
                _state = STATE_WRITING_FAILED;
                UpdateState();
            }
        }
    }

    void OnSaveImageButton(wxCommandEvent&)
    {
        try
        {
            if (!config.has_image_writer())
                Error() << "This format cannot be saved.";

            auto filename =
                wxFileSelector("Choose the name of the image file to write",
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

            _errorState = _state;
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

    void OnSaveFluxButton(wxCommandEvent&)
    {
        try
        {
            auto filename = wxFileSelector(
                "Choose the name of the flux file to write",
                /* default_path= */ wxEmptyString,
                /* default_filename= */
                formatChoice->GetString(formatChoice->GetSelection()) + ".flux",
                /* default_extension= */ wxEmptyString,
                /* wildcard= */ wxEmptyString,
                /* flags= */ wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (filename.empty())
                return;

            FluxSink::updateConfigForFilename(
                config.mutable_flux_sink(), filename.ToStdString());

            ShowConfig();

            _errorState = _state;
            runOnWorkerThread(
                [this]()
                {
                    auto fluxSource =
                        FluxSource::createMemoryFluxSource(*_currentDisk);
                    auto fluxSink = FluxSink::create(config.flux_sink());
                    writeRawDiskCommand(*fluxSource, *fluxSink);
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnImagerGoAgainButton(wxCommandEvent& event)
    {
        if (_state < STATE_READING__LAST)
            OnReadButton(event);
        else if (_state < STATE_WRITING__LAST)
            OnWriteButton(event);
    }

    void OnBackButton(wxCommandEvent&)
    {
        _state = STATE_IDLE;
        UpdateState();
    }

    /* This sets the *global* config object. That's safe provided the worker
     * thread isn't running, otherwise you'll get a race. */
    void PrepareConfig()
    {
        assert(!wxGetApp().IsWorkerThreadRunning());

        auto formatSelection = formatChoice->GetSelection();
        if (formatSelection == wxNOT_FOUND)
            Error() << "no format selected";

        config = *_formats[formatChoice->GetSelection()].second;

        auto serial = deviceCombo->GetValue().ToStdString();
        if (!serial.empty() && (serial[0] == '/'))
            setProtoByString(&config, "usb.greaseweazle.port", serial);
        else
            setProtoByString(&config, "usb.serial", serial);

        ApplyCustomSettings();
        logEntry->Clear();

        switch (_selectedSource)
        {
            case SELECTEDSOURCE_REAL:
            {
                bool hd = highDensityToggle->GetValue();
                config.mutable_drive()->set_high_density(hd);

                std::string filename =
                    driveChoice->GetSelection() ? "drive:1" : "drive:0";
                FluxSink::updateConfigForFilename(
                    config.mutable_flux_sink(), filename);
                FluxSource::updateConfigForFilename(
                    config.mutable_flux_source(), filename);

                break;
            }

            case SELECTEDSOURCE_FLUX:
            {
                auto filename = fluxImagePicker->GetPath().ToStdString();
                FluxSink::updateConfigForFilename(
                    config.mutable_flux_sink(), filename);
                FluxSource::updateConfigForFilename(
                    config.mutable_flux_source(), filename);
                break;
            }

            case SELECTEDSOURCE_IMAGE:
            {
                auto filename = diskImagePicker->GetPath().ToStdString();
                ImageReader::updateConfigForFilename(
                    config.mutable_image_reader(), filename);
                ImageWriter::updateConfigForFilename(
                    config.mutable_image_writer(), filename);
                break;
            }
        }
    }

    void SetHighDensity() {}

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
                    _statusBar->SetLeftLabel(m.message);
                    wxMessageBox(m.message, "Error", wxOK | wxICON_ERROR);
                    _state = _errorState;
                    UpdateState();
                },

                /* Indicates that we're starting a write operation. */
                [&](const BeginWriteOperationLogMessage& m)
                {
                    _statusBar->SetRightLabel(
                        fmt::format("W {}.{}", m.track, m.head));
                    visualiser->SetMode(m.track, m.head, VISMODE_WRITING);
                },

                [&](const EndWriteOperationLogMessage& m)
                {
                    _statusBar->SetRightLabel("");
                    visualiser->SetMode(0, 0, VISMODE_NOTHING);
                },

                /* Indicates that we're starting a read operation. */
                [&](const BeginReadOperationLogMessage& m)
                {
                    _statusBar->SetRightLabel(
                        fmt::format("R {}.{}", m.track, m.head));
                    visualiser->SetMode(m.track, m.head, VISMODE_READING);
                },

                [&](const EndReadOperationLogMessage& m)
                {
                    _statusBar->SetRightLabel("");
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

                /* Large-scale operation start. */
                [&](const BeginOperationLogMessage& m)
                {
                    _statusBar->SetLeftLabel(m.message);
                    _statusBar->ShowProgressBar();
                },

                /* Large-scale operation end. */
                [&](const EndOperationLogMessage& m)
                {
                    _statusBar->SetLeftLabel(m.message);
                    _statusBar->HideProgressBar();
                },

                /* Large-scale operation progress. */
                [&](const OperationProgressLogMessage& m)
                {
                    _statusBar->SetProgress(m.progress);
                },

            },
            *message);
    }

    void LoadConfig()
    {
        /* Prevent saving the config half-way through reloading it when the
         * widget states all change. */

        _dontSaveConfig = true;

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
        for (const auto& it : _formats)
        {
            if (it.first == s)
            {
                formatChoice->SetSelection(i);
                break;
            }
            i++;
        }

        /* Triggers SaveConfig */

        _dontSaveConfig = false;
        wxCommandEvent dummyEvent;
        OnConfigRadioButtonClicked(dummyEvent);
    }

    void SaveConfig()
    {
        if (_dontSaveConfig)
            return;

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

        if (_state < STATE_IDLE__LAST)
        {
            dataNotebook->SetSelection(0);

            readButton->Enable(_selectedSource != SELECTEDSOURCE_IMAGE);
            writeButton->Enable(_selectedSource == SELECTEDSOURCE_REAL);
        }
        else if (_state < STATE_READING__LAST)
        {
            dataNotebook->SetSelection(1);

            imagerSaveImageButton->Enable(_state == STATE_READING_SUCCEEDED);
            imagerSaveFluxButton->Enable(_state == STATE_READING_SUCCEEDED);
            imagerGoAgainButton->Enable(_state != STATE_READING_WORKING);

            imagerToolbar->EnableTool(
                imagerBackTool->GetId(), _state != STATE_READING_WORKING);
        }
        else if (_state < STATE_WRITING__LAST)
        {
            dataNotebook->SetSelection(1);

            imagerSaveImageButton->Enable(false);
            imagerSaveFluxButton->Enable(false);
            imagerGoAgainButton->Enable(_state != STATE_WRITING_WORKING);

            imagerToolbar->EnableTool(
                imagerBackTool->GetId(), _state != STATE_WRITING_WORKING);
        }
        else if (_state < STATE_BROWSING__LAST)
        {
            dataNotebook->SetSelection(2);
        }
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

    wxStatusBar* OnCreateStatusBar(
        int number, long style, wxWindowID id, const wxString& name) override
    {
        _statusBar = new CustomStatusBar(this, id);
        return _statusBar;
    }

private:
    enum
    {
        SELECTEDSOURCE_REAL,
        SELECTEDSOURCE_FLUX,
        SELECTEDSOURCE_IMAGE
    };

    enum
    {
        STATE_IDLE,
        STATE_IDLE__LAST,

        STATE_READING_WORKING,
        STATE_READING_FAILED,
        STATE_READING_SUCCEEDED,
        STATE_READING__LAST,

        STATE_WRITING_WORKING,
        STATE_WRITING_FAILED,
        STATE_WRITING_SUCCEEDED,
        STATE_WRITING__LAST,

        STATE_BROWSING_WORKING,
        STATE_BROWSING_IDLE,
        STATE_BROWSING__LAST
    };

    wxConfig _config;
    std::vector<std::pair<std::string, std::unique_ptr<const ConfigProto>>>
        _formats;
    std::vector<std::unique_ptr<const CandidateDevice>> _devices;
    int _state;
    int _errorState;
    int _selectedSource;
    bool _dontSaveConfig = false;
    std::shared_ptr<const DiskFlux> _currentDisk;
    CustomStatusBar* _statusBar;
};

wxWindow* FluxEngineApp::CreateMainWindow()
{
    return new MainWindow();
}
