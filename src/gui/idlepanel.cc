#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "gui.h"
#include "lib/data/fluxmap.h"
#include "lib/usb/usbfinder.h"
#include "lib/config/proto.h"
#include "lib/config/flags.h"
#include "lib/core/utils.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/layout.h"
#include "texteditorwindow.h"
#include "iconbutton.h"
#include "context.h"
#include <wx/config.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include "icons/hardware.h"
#include "icons/fluxfile.h"
#include "icons/imagefile.h"

#define CONFIG_SELECTEDSOURCE "SelectedSource"
#define CONFIG_DEVICE "Device"
#define CONFIG_DRIVE "Drive"
#define CONFIG_DRIVETYPE "DriveType"
#define CONFIG_HIGHDENSITY "HighDensity"
#define CONFIG_FORMAT "Format"
#define CONFIG_FORMATOPTIONS "FormatOptions"
#define CONFIG_EXTRACONFIG "ExtraConfig"
#define CONFIG_FLUXIMAGE "FluxImage"
#define CONFIG_FLUXFORMAT "FluxImageFormat"
#define CONFIG_DISKIMAGE "DiskImage"

const std::string DEFAULT_EXTRA_CONFIGURATION =
    "# Place any extra configuration here.\n"
    "# Each line can contain a key=value pair to set a property,\n"
    "# or the name of a built-in configuration, or the filename\n"
    "# of a text proto file. Or a comment, of course.\n\n";

wxDEFINE_EVENT(PAGE_SELECTED_EVENT, wxCommandEvent);

static wxBitmap createBitmap(const uint8_t* data, size_t length)
{
    wxMemoryInputStream stream(data, length);
    wxImage image(stream, wxBITMAP_TYPE_PNG);
    return wxBitmap(image);
}

static void ignoreInapplicableValueExceptions(std::function<void(void)> cb)
{
    try
    {
        cb();
    }
    catch (const InapplicableValueException* e)
    {
        /* swallow */
    }
}

class IdlePanelImpl : public IdlePanelGen, public IdlePanel
{
    enum
    {
        SELECTEDSOURCE_REAL,
        SELECTEDSOURCE_FLUX,
        SELECTEDSOURCE_IMAGE
    };

    enum
    {
        ICON_HARDWARE,
        ICON_FLUXFILE,
        ICON_IMAGEFILE
    };

public:
    IdlePanelImpl(MainWindow* mainWindow, wxSimplebook* parent):
        IdlePanelGen(parent),
        IdlePanel(mainWindow),
        _imageList(48, 48, true, 0),
        _config("FluxEngine")
    {
        int defaultFormat = 0;
        int i = 0;
        for (const auto& it : formats)
        {
            auto config = std::make_unique<ConfigProto>();
            *config = *it.second;
            if (config->is_extension())
                continue;

            formatChoice->Append(it.first);
            _formatNames.push_back(it.first);
            i++;
        }

        LoadConfig();
        UpdateDevices();
        UpdateFormatOptions();

        parent->AddPage(this, "idle");

        _imageList.Add(
            createBitmap(icon_hardware_png, sizeof(icon_hardware_png)));
        _imageList.Add(
            createBitmap(icon_fluxfile_png, sizeof(icon_fluxfile_png)));
        _imageList.Add(
            createBitmap(icon_imagefile_png, sizeof(icon_imagefile_png)));

        UpdateSources();
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

    void OnControlsChanged(wxCommandEvent& event) override
    {
        SaveConfig();
        UpdateState();
        UpdateFormatOptions();
    }

    void OnControlsChanged(wxFileDirPickerEvent& event)
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
            error("no format selected");

        ClearLog();
        globalConfig().clear();
        auto formatName = _formatNames[formatChoice->GetSelection()];
        globalConfig().readBaseConfigFile(formatName);

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
                globalConfig().set(key, value);
            }
            else
                globalConfig().readBaseConfigFile(setting);
        }

        /* Apply the source/destination. */

        switch (_selectedSource)
        {
            case SELECTEDSOURCE_REAL:
            {
                globalConfig().overrides()->mutable_drive()->set_high_density(
                    _selectedHighDensity);
                globalConfig().overrides()->MergeFrom(*_selectedDriveType);

                std::string filename = _selectedDrive ? "drive:1" : "drive:0";
                globalConfig().setFluxSink(filename);
                globalConfig().setFluxSource(filename);
                globalConfig().setVerificationFluxSource(filename);
                break;
            }

            case SELECTEDSOURCE_FLUX:
            {
                if (_selectedFluxFormat)
                {
                    if (_selectedFluxFormat->sink)
                        _selectedFluxFormat->sink(_selectedFluxFilename,
                            globalConfig().overrides()->mutable_flux_sink());
                    if (_selectedFluxFormat->source)
                        _selectedFluxFormat->source(_selectedFluxFilename,
                            globalConfig().overrides()->mutable_flux_source());
                }
                break;
            }

            case SELECTEDSOURCE_IMAGE:
            {
                ignoreInapplicableValueExceptions(
                    [&]()
                    {
                        globalConfig().setImageReader(_selectedImagefilename);
                    });
                ignoreInapplicableValueExceptions(
                    [&]()
                    {
                        globalConfig().setImageWriter(_selectedImagefilename);
                    });
                break;
            }
        }

        /* Apply any format options. */

        for (const auto& e : _formatOptions)
        {
            if (e.first == formatName)
            {
                try
                {
                    globalConfig().applyOption(e.second);
                }
                catch (const OptionException& e)
                {
                }
            }
        }

        /* Locate the device, if any. */

        auto serial = _selectedDevice;
        if (!serial.empty() && (serial[0] == '/'))
            globalConfig().set("usb.greaseweazle.port", serial);
        else
            globalConfig().set("usb.serial", serial);

        globalConfig().validateAndThrow();
    }

    const wxBitmap GetBitmap() override
    {
        return applicationBitmap->GetBitmap();
    }

private:
    void LoadConfig()
    {
        /* Prevent saving the config half-way through reloading it when the
         * widget states all change. */

        _dontSaveConfig = true;

        /* Radio button config. */

        wxString s = std::to_string(SELECTEDSOURCE_IMAGE);
        _config.Read(CONFIG_SELECTEDSOURCE, &s);
        _selectedSource = std::atoi(s.c_str());

        /* Real disk block. */

        s = "";
        _config.Read(CONFIG_DEVICE, &s);
        _selectedDevice = s;

        s = "0";
        _config.Read(CONFIG_DRIVE, &s);
        _selectedDrive = wxAtoi(s);

        s = "0";
        _config.Read(CONFIG_HIGHDENSITY, &s);
        _selectedHighDensity = wxAtoi(s);

        s = "";
        _config.Read(CONFIG_DRIVETYPE, &s);
        auto it = drivetypes.find(s.ToStdString());
        if (it != drivetypes.end())
            _selectedDriveType = it->second;
        else
            _selectedDriveType = drivetypes.begin()->second;

        /* Flux image block. */

        s = "";
        _config.Read(CONFIG_FLUXIMAGE, &s);
        _selectedFluxFilename = s;

        s = "";
        _config.Read(CONFIG_FLUXFORMAT, &s);
        _selectedFluxFormatName = s;

        /* Disk image block. */

        s = "";
        _config.Read(CONFIG_DISKIMAGE, &s);
        _selectedImagefilename = s;

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
    }

    void SaveConfig()
    {
        if (_dontSaveConfig)
            return;

        _config.Write(
            CONFIG_SELECTEDSOURCE, wxString(std::to_string(_selectedSource)));

        /* Real disk block. */

        _config.Write(CONFIG_DEVICE, wxString(_selectedDevice));
        _config.Write(CONFIG_DRIVE, wxString(std::to_string(_selectedDrive)));
        _config.Write(
            CONFIG_HIGHDENSITY, wxString(std::to_string(_selectedHighDensity)));
        for (auto it : drivetypes)
        {
            if (_selectedDriveType == it.second)
                _config.Write(CONFIG_DRIVETYPE, wxString(it.first));
        }

        /* Flux image block. */

        _config.Write(CONFIG_FLUXIMAGE, wxString(_selectedFluxFilename));
        _config.Write(CONFIG_FLUXFORMAT, wxString(_selectedFluxFormatName));

        /* Disk image block. */

        _config.Write(CONFIG_DISKIMAGE, wxString(_selectedImagefilename));

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

    void UpdateSources()
    {
        sourceBook->DeleteAllPages();
        sourceIconPanel->DestroyChildren();

        int page = 0;
        bool switched = false;
        for (auto& device : _devices)
        {
            for (int drive = 0; drive <= 1; drive++)
            {
                auto* panel = new HardwareSourcePanelGen(sourceBook);
                sourceBook->AddPage(panel, "");
                panel->label->SetLabel(
                    fmt::format("{}; serial number {}; drive:{}",
                        getDeviceName(device->type),
                        device->serial,
                        drive));

                auto* button = AddIcon(ICON_HARDWARE,
                    fmt::format(
                        "{}\ndrive:{}", device->serial.substr(0, 10), drive));
                button->Bind(wxEVT_BUTTON,
                    [=](auto& e)
                    {
                        SwitchToPage(page);

                        _selectedSource = SELECTEDSOURCE_REAL;
                        _selectedDevice = device->serial;
                        _selectedDrive = drive;
                        OnControlsChanged(e);
                    });

                panel->highDensityToggle->SetValue(_selectedHighDensity);
                panel->highDensityToggle->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [=](wxCommandEvent& e)
                    {
                        _selectedHighDensity =
                            panel->highDensityToggle->GetValue();
                        OnControlsChanged(e);
                    });

                int i = 0;
                for (auto& driveConfig : drivetypes)
                {
                    panel->driveTypeChoice->Append(
                        driveConfig.second->comment());
                    if (driveConfig.second == _selectedDriveType)
                        panel->driveTypeChoice->SetSelection(i);
                    i++;
                }

                panel->driveTypeChoice->Bind(wxEVT_CHOICE,
                    [=](wxCommandEvent& e)
                    {
                        auto it = drivetypes.begin();
                        std::advance(
                            it, panel->driveTypeChoice->GetSelection());

                        _selectedDriveType = it->second;
                        OnControlsChanged(e);
                    });

                if ((_selectedSource == SELECTEDSOURCE_REAL) &&
                    (_selectedDevice == device->serial) &&
                    (_selectedDrive == drive))
                {
                    SwitchToPage(page);
                    switched = true;
                }

                page++;
            }
        }

        {
            auto* panel = new FluxfileSourcePanelGen(sourceBook);
            sourceBook->AddPage(panel, "");

            auto* button = AddIcon(ICON_FLUXFILE, "Flux file");
            button->Bind(wxEVT_BUTTON,
                [=](auto& e)
                {
                    SwitchToPage(page);

                    _selectedSource = SELECTEDSOURCE_FLUX;
                    OnControlsChanged(e);
                });

            panel->fluxImagePicker->SetPath(_selectedFluxFilename);

            panel->fluxImageFormat->Clear();
            _selectedFluxFormat = &Config::getFluxFormats()[0];
            int choiceIndex = 0;
            for (int i = 0; i < Config::getFluxFormats().size(); i++)
            {
                const auto& format = Config::getFluxFormats()[i];
                if (!format.name.empty() && format.source)
                {
                    int index = panel->fluxImageFormat->Append(
                        format.name, (void*)&format);
                    if (format.name == _selectedFluxFormatName)
                    {
                        _selectedFluxFormat = &format;
                        choiceIndex = index;
                    }
                }
            }
            panel->fluxImageFormat->SetSelection(choiceIndex);

            auto onFormatChanged = [=](wxCommandEvent& e)
            {
                int i = panel->fluxImageFormat->GetSelection();
                _selectedFluxFormat =
                    (const FluxConstructor*)
                        panel->fluxImageFormat->GetClientData(i);
                _selectedFluxFormatName = _selectedFluxFormat->name;
                OnControlsChanged(e);
            };

            panel->fluxImagePicker->Bind(wxEVT_COMMAND_FILEPICKER_CHANGED,
                [=](wxFileDirPickerEvent& e)
                {
                    _selectedFluxFilename = e.GetPath();

                    for (int i = 0; i < Config::getFluxFormats().size(); i++)
                    {
                        const auto& format = Config::getFluxFormats()[i];
                        if (std::regex_match(
                                _selectedFluxFilename, format.pattern))
                        {
                            int i =
                                panel->fluxImageFormat->FindString(format.name);
                            if (i != wxNOT_FOUND)
                            {
                                panel->fluxImageFormat->SetSelection(i);

                                wxCommandEvent e;
                                onFormatChanged(e);
                            }
                        }
                    }

                    OnControlsChanged(e);
                });

            panel->fluxImageFormat->Bind(wxEVT_CHOICE, onFormatChanged);

            if (_selectedSource == SELECTEDSOURCE_FLUX)
            {
                SwitchToPage(page);
                switched = true;
            }

            page++;
        }

        {
            auto* panel = new ImagefileSourcePanelGen(sourceBook);
            sourceBook->AddPage(panel, "");

            auto* button = AddIcon(ICON_IMAGEFILE, "Disk image");
            button->Bind(wxEVT_BUTTON,
                [=](auto& e)
                {
                    SwitchToPage(page);

                    _selectedSource = SELECTEDSOURCE_IMAGE;
                    OnControlsChanged(e);
                });

            panel->diskImagePicker->SetPath(_selectedImagefilename);
            panel->diskImagePicker->Bind(wxEVT_COMMAND_FILEPICKER_CHANGED,
                [=](wxFileDirPickerEvent& e)
                {
                    _selectedImagefilename = e.GetPath();
                    OnControlsChanged(e);
                });

            if (_selectedSource == SELECTEDSOURCE_IMAGE)
            {
                SwitchToPage(page);
                switched = true;
            }

            page++;
        }

        Fit();
        Layout();
        if (!switched)
            SwitchToPage(0);
    }

    void OnFluxFormatChanged(wxCommandEvent&) {}

    IconButton* AddIcon(int bitmapIndex, const std::string text)
    {
        auto* button = new IconButton(sourceIconPanel, wxID_ANY);
        button->SetBitmapAndLabel(_imageList.GetBitmap(bitmapIndex), text);
        sourceIconPanel->GetSizer()->Add(
            button, 0, wxALL | wxEXPAND, 5, nullptr);
        return button;
    }

    void SwitchToPage(int page)
    {
        int i = 0;
        for (auto* window : sourceIconPanel->GetChildren())
        {
            IconButton* button = dynamic_cast<IconButton*>(window);
            if (button)
                button->SetSelected(i == page);
            i++;
        }

        sourceBook->ChangeSelection(page);
    }

    void UpdateFormatOptions()
    {
        int formatSelection = formatChoice->GetSelection();
        _currentlyDisplayedFormat = formatSelection;

        try
        {
            PrepareConfig();
            globalConfig().combined();
        }
        catch (const InapplicableOptionException& e)
        {
            /* The current set of options is invalid for some reason. Just
             * swallow the errors. */
        }
        catch (const ErrorException& e)
        {
            /* This really isn't supposed to happen, but sometimes does and
             * it crashes the whole program. */
            return;
        }

        assert(!wxGetApp().IsWorkerThreadRunning());

        formatOptionsContainer->DestroyChildren();
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        if (formatSelection == wxNOT_FOUND)
            sizer->Add(new wxStaticText(
                formatOptionsContainer, wxID_ANY, "(no format selected)"));
        else
        {
            std::string formatName = _formatNames[formatChoice->GetSelection()];

            for (auto& group : globalConfig()->option_group())
            {
                std::string groupName = group.comment();
                if (groupName == "$formats")
                    groupName = "Formats";
                sizer->Add(new wxStaticText(
                    formatOptionsContainer, wxID_ANY, groupName + ":"));

                bool first = true;
                bool valueSet = false;
                wxRadioButton* defaultButton = nullptr;
                for (auto& option : group.option())
                {
                    auto* rb = new wxRadioButton(formatOptionsContainer,
                        wxID_ANY,
                        option.comment(),
                        wxDefaultPosition,
                        wxDefaultSize,
                        first ? wxRB_GROUP : 0);
                    auto key = std::make_pair(formatName, option.name());
                    sizer->Add(rb);

                    rb->Bind(wxEVT_RADIOBUTTON,
                        [=](wxCommandEvent& e)
                        {
                            for (auto& option : group.option())
                            {
                                _formatOptions.erase(
                                    std::make_pair(formatName, option.name()));
                            }

                            _formatOptions.insert(key);

                            OnControlsChanged(e);
                        });

                    if (_formatOptions.find(key) != _formatOptions.end())
                    {
                        rb->SetValue(true);
                        valueSet = true;
                    }

                    rb->Enable(globalConfig().isOptionValid(option));
                    if (option.set_by_default() || !defaultButton)
                        defaultButton = rb;

                    first = false;
                }

                if (!valueSet && defaultButton)
                    defaultButton->SetValue(true);
            }

            /* Anything that's _not_ in a group gets a checkbox. */

            for (auto& option : globalConfig()->option())
            {
                auto* choice = new wxCheckBox(
                    formatOptionsContainer, wxID_ANY, option.comment());
                auto key = std::make_pair(formatName, option.name());
                sizer->Add(choice);

                choice->SetValue(
                    (_formatOptions.find(key) != _formatOptions.end()) ||
                    option.set_by_default());

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

            if (globalConfig()->option().empty() &&
                globalConfig()->option_group().empty())
                sizer->Add(new wxStaticText(formatOptionsContainer,
                    wxID_ANY,
                    "(no options for this format)"));
        }

        formatOptionsContainer->SetSizerAndFit(sizer);
        Layout();
        SafeFit();
    }

    void UpdateDevices()
    {
        auto candidates = findUsbDevices();
        for (auto& candidate : candidates)
            _devices.push_back(candidate);
    }

private:
    wxConfig _config;
    wxImageList _imageList;
    std::vector<std::string> _formatNames;
    std::vector<std::shared_ptr<const CandidateDevice>> _devices;
    int _selectedSource;
    std::string _selectedDevice;
    int _selectedDrive;
    const ConfigProto* _selectedDriveType;
    bool _selectedHighDensity;
    std::string _selectedFluxFilename;
    std::string _selectedFluxFormatName;
    const FluxConstructor* _selectedFluxFormat = nullptr;
    std::string _selectedImagefilename;
    bool _dontSaveConfig = false;
    std::string _extraConfiguration;
    std::set<std::pair<std::string, std::string>> _formatOptions;
    int _currentlyDisplayedFormat = wxNOT_FOUND - 1;
};

IdlePanel* IdlePanel::Create(MainWindow* mainWindow, wxSimplebook* parent)
{
    return new IdlePanelImpl(mainWindow, parent);
}
