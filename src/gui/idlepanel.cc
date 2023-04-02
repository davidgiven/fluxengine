#include "lib/globals.h"
#include "gui.h"
#include "lib/fluxmap.h"
#include "lib/usb/usbfinder.h"
#include "lib/proto.h"
#include "lib/flags.h"
#include "lib/utils.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "layout.h"
#include "texteditorwindow.h"
#include <wx/config.h>

extern const std::map<std::string, std::string> formats;

#define CONFIG_SELECTEDSOURCE "SelectedSource"
#define CONFIG_DEVICE "Device"
#define CONFIG_DRIVE "Drive"
#define CONFIG_FORTYTRACK "FortyTrack"
#define CONFIG_HIGHDENSITY "HighDensity"
#define CONFIG_FORMAT "Format"
#define CONFIG_FORMATOPTIONS "FormatOptions"
#define CONFIG_EXTRACONFIG "ExtraConfig"
#define CONFIG_FLUXIMAGE "FluxImage"
#define CONFIG_DISKIMAGE "DiskImage"

const std::string DEFAULT_EXTRA_CONFIGURATION =
    "# Place any extra configuration here.\n"
    "# Each line can contain a key=value pair to set a property,\n"
    "# or the name of a built-in configuration, or the filename\n"
    "# of a text proto file. Or a comment, of course.\n\n";

class IdlePanelImpl : public IdlePanelGen, public IdlePanel
{
    enum
    {
        SELECTEDSOURCE_REAL,
        SELECTEDSOURCE_FLUX,
        SELECTEDSOURCE_IMAGE
    };

public:
    IdlePanelImpl(MainWindow* mainWindow, wxSimplebook* parent):
        IdlePanelGen(parent),
        IdlePanel(mainWindow),
        _config("FluxEngine")
    {
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
            _formatNames.push_back(it.first);
            i++;
        }

        /* I have no idea why this is necessary, but on Windows things aren't
         * laid out correctly without it. */

        realDiskRadioButtonPanel->Hide();
        fluxImageRadioButtonPanel->Hide();
        diskImageRadioButtonPanel->Hide();

        LoadConfig();
        UpdateDevices();
        UpdateFormatOptions();

        parent->AddPage(this, "idle");
    }

public:
    void Start() override
    {
        SetPage(MainWindow::PAGE_IDLE);
    }

    void UpdateState()
    {
        bool hasFormat = formatChoice->GetSelection() != wxNOT_FOUND;

        readButton->Enable(
            (_selectedSource != SELECTEDSOURCE_IMAGE) && hasFormat);
        writeButton->Enable(
            (_selectedSource == SELECTEDSOURCE_REAL) && hasFormat);
        browseButton->Enable(hasFormat);
        formatButton->Enable(hasFormat);
        exploreButton->Enable(_selectedSource != SELECTEDSOURCE_IMAGE);

        UpdateFormatOptions();
    }

public:
    void OnReadButton(wxCommandEvent&) override
    {
        StartReading();
    }

    void OnWriteButton(wxCommandEvent&) override
    {
        StartWriting();
    }

    void OnBrowseButton(wxCommandEvent&) override
    {
        StartBrowsing();
    }

    void OnFormatButton(wxCommandEvent&) override
    {
        StartFormatting();
    }

    void OnExploreButton(wxCommandEvent&) override
    {
        StartExploring();
    }

    void OnConfigRadioButtonClicked(wxCommandEvent& event) override
    {
        auto configRadioButton = [&](wxRadioButton* button, wxPanel* panel)
        {
            panel->Show(button->GetValue());
        };
        configRadioButton(realDiskRadioButton, realDiskRadioButtonPanel);
        configRadioButton(fluxImageRadioButton, fluxImageRadioButtonPanel);
        configRadioButton(diskImageRadioButton, diskImageRadioButtonPanel);
        Layout();

        if (realDiskRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_REAL;
        if (fluxImageRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_FLUX;
        if (diskImageRadioButton->GetValue())
            _selectedSource = SELECTEDSOURCE_IMAGE;

        OnControlsChanged(event);
    }

    void OnControlsChanged(wxCommandEvent& event) override
    {
        SaveConfig();
        UpdateState();
        UpdateFormatOptions();
    }

    void OnControlsChanged(wxFileDirPickerEvent& event) override
    {
        wxCommandEvent e;
        OnControlsChanged(e);
    }

    void OnCustomConfigurationButton(wxCommandEvent& event) override
    {
        auto* editor = TextEditorWindow::Create(
            this, "Configuration editor", _extraConfiguration);
        editor->Bind(EDITOR_SAVE_EVENT,
            [this](auto& event)
            {
                _extraConfiguration = event.text;
                SaveConfig();
            });
        editor->Show();
    }

    void PrepareConfig() override
    {
        assert(!wxGetApp().IsWorkerThreadRunning());

        auto formatSelection = formatChoice->GetSelection();
        if (formatSelection == wxNOT_FOUND)
            Error() << "no format selected";

        config.Clear();
        auto formatName = _formatNames[formatChoice->GetSelection()];
        FlagGroup::parseConfigFile(formatName, formats);

        /* Apply any format options. */

        for (const auto& e : _formatOptions)
        {
            if (e.first == formatName)
                FlagGroup::applyOption(e.second);
        }

        /* Merge in any custom config. */

        for (auto setting : split(_extraConfiguration, '\n'))
        {
            setting = trimWhitespace(setting);
            if (setting.size() == 0)
                continue;
            if (setting[0] == '#')
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

        /* Locate the device, if any. */

        auto serial = deviceCombo->GetValue().ToStdString();
        if (!serial.empty() && (serial[0] == '/'))
            setProtoByString(&config, "usb.greaseweazle.port", serial);
        else
            setProtoByString(&config, "usb.serial", serial);

        ClearLog();

        /* Apply the source/destination. */

        switch (_selectedSource)
        {
            case SELECTEDSOURCE_REAL:
            {
                bool hd = highDensityToggle->GetValue();
                config.mutable_drive()->set_high_density(hd);

                if (fortyTrackDriveToggle->GetValue())
                    FlagGroup::parseConfigFile("40track_drive", formats);

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

private:
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

        s = "0";
        _config.Read(CONFIG_FORTYTRACK, &s);
        fortyTrackDriveToggle->SetValue(wxAtoi(s));

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
        for (const auto& it : _formatNames)
        {
            if (it == s)
            {
                formatChoice->SetSelection(i);
                break;
            }
            i++;
        }

        s = DEFAULT_EXTRA_CONFIGURATION;
        _config.Read(CONFIG_EXTRACONFIG, &s);
        _extraConfiguration = s;

        /* Format options. */

        _formatOptions.clear();
        s = "";
        _config.Read(CONFIG_FORMATOPTIONS, &s);
        for (auto combined : split(std::string(s), ','))
        {
            auto pair = split(combined, ':');
            try
            {
                auto key = std::make_pair(pair.at(0), pair.at(1));
                _formatOptions.insert(key);
            }
            catch (std::exception&)
            {
            }
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
        _config.Write(CONFIG_FORTYTRACK,
            wxString(std::to_string(fortyTrackDriveToggle->GetValue())));

        /* Flux image block. */

        _config.Write(CONFIG_FLUXIMAGE, fluxImagePicker->GetPath());

        /* Disk image block. */

        _config.Write(CONFIG_DISKIMAGE, diskImagePicker->GetPath());

        /* Format block. */

        _config.Write(CONFIG_FORMAT,
            formatChoice->GetString(formatChoice->GetSelection()));
        _config.Write(CONFIG_EXTRACONFIG, wxString(_extraConfiguration));

        /* Format options. */

        {
            std::vector<std::string> options;
            for (auto& e : _formatOptions)
                options.push_back(fmt::format("{}:{}", e.first, e.second));

            _config.Write(CONFIG_FORMATOPTIONS, wxString(join(options, ",")));
        }
    }

    void UpdateFormatOptions()
    {
        assert(!wxGetApp().IsWorkerThreadRunning());

        int formatSelection = formatChoice->GetSelection();
        if (formatSelection != _currentlyDisplayedFormat)
        {
            _currentlyDisplayedFormat = formatSelection;
            formatOptionsContainer->DestroyChildren();
            auto* sizer = new wxBoxSizer(wxVERTICAL);

            if (formatSelection == wxNOT_FOUND)
                sizer->Add(new wxStaticText(
                    formatOptionsContainer, wxID_ANY, "(no format selected)"));
            else
            {
                config.Clear();
                std::string formatName =
                    _formatNames[formatChoice->GetSelection()];
                FlagGroup::parseConfigFile(formatName, formats);

                std::set<std::string> exclusivityGroups;
                for (auto& option : config.option())
                {
                    if (option.has_exclusivity_group())
                        exclusivityGroups.insert(option.exclusivity_group());
                }

                if (config.option().empty())
                    sizer->Add(new wxStaticText(formatOptionsContainer,
                        wxID_ANY,
                        "(no options for this format)"));
                else
                {
                    /* Add grouped radiobuttons for anything in an exclusivity
                     * group. */

                    for (auto& group : exclusivityGroups)
                    {
                        bool first = true;
                        for (auto& option : config.option())
                        {
                            if (option.exclusivity_group() != group)
                                continue;

                            auto* rb = new wxRadioButton(formatOptionsContainer,
                                wxID_ANY,
                                option.comment());
                            auto key =
                                std::make_pair(formatName, option.name());
                            sizer->Add(rb);

                            rb->Bind(wxEVT_RADIOBUTTON,
                                [=](wxCommandEvent& e)
                                {
                                    for (auto& option : config.option())
                                    {
                                        if (option.exclusivity_group() == group)
                                            _formatOptions.erase(std::make_pair(
                                                formatName, option.name()));
                                    }

                                    _formatOptions.insert(key);

                                    OnControlsChanged(e);
                                });

                            if (_formatOptions.find(key) !=
                                _formatOptions.end())
                                rb->SetValue(true);

                            if (first)
                                rb->SetExtraStyle(wxRB_GROUP);
                            first = false;
                        }
                    }

                    /* Anything that's _not_ in a group gets a checkbox. */

                    for (auto& option : config.option())
                    {
                        if (option.has_exclusivity_group())
                            continue;

                        auto* choice = new wxCheckBox(
                            formatOptionsContainer, wxID_ANY, option.comment());
                        auto key = std::make_pair(formatName, option.name());
                        sizer->Add(choice);

                        if (_formatOptions.find(key) != _formatOptions.end())
                            choice->SetValue(true);

                        choice->Bind(wxEVT_CHECKBOX,
                            [=](wxCommandEvent& e)
                            {
                                if (choice->GetValue())
                                    _formatOptions.insert(key);
                                else
                                    _formatOptions.erase(key);

                                OnControlsChanged(e);
                            });
                    }
                }
            }

            formatOptionsContainer->SetSizerAndFit(sizer);
            Layout();
        }
    }

    void UpdateDevices()
    {
        auto candidates = findUsbDevices();

        auto device = deviceCombo->GetValue();
        deviceCombo->Clear();
        deviceCombo->SetValue(device);

        _devices.clear();
        for (auto& candidate : candidates)
        {
            deviceCombo->Append(candidate->serial);
            _devices.push_back(std::move(candidate));
        }
    }

private:
    wxConfig _config;
    std::vector<std::string> _formatNames;
    std::vector<std::unique_ptr<const CandidateDevice>> _devices;
    int _selectedSource;
    bool _dontSaveConfig = false;
    std::string _extraConfiguration;
    std::set<std::pair<std::string, std::string>> _formatOptions;
    int _currentlyDisplayedFormat = wxNOT_FOUND - 1;
};

IdlePanel* IdlePanel::Create(MainWindow* mainWindow, wxSimplebook* parent)
{
    return new IdlePanelImpl(mainWindow, parent);
}
