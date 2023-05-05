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
#include "iconbutton.h"
#include <wx/config.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include ".obj/extras/hardware.h"
#include ".obj/extras/fluxfile.h"
#include ".obj/extras/imagefile.h"

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

wxDEFINE_EVENT(PAGE_SELECTED_EVENT, wxCommandEvent);

static wxBitmap createBitmap(const uint8_t* data, size_t length)
{
    wxMemoryInputStream stream(data, length);
    wxImage image(stream, wxBITMAP_TYPE_PNG);
    return wxBitmap(image);
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
            if (!config->ParseFromString(it.second))
                continue;
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
            createBitmap(extras_hardware_png, sizeof(extras_hardware_png)));
        _imageList.Add(
            createBitmap(extras_fluxfile_png, sizeof(extras_fluxfile_png)));
        _imageList.Add(
            createBitmap(extras_imagefile_png, sizeof(extras_imagefile_png)));

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

        auto serial = _selectedDevice;
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
                config.mutable_drive()->set_high_density(_selectedHighDensity);

                if (_selectedFortyTrack)
                    FlagGroup::parseConfigFile("40track_drive", formats);

                std::string filename = _selectedDrive ? "drive:1" : "drive:0";
                FluxSink::updateConfigForFilename(
                    config.mutable_flux_sink(), filename);
                FluxSource::updateConfigForFilename(
                    config.mutable_flux_source(), filename);

                break;
            }

            case SELECTEDSOURCE_FLUX:
            {
                FluxSink::updateConfigForFilename(
                    config.mutable_flux_sink(), _selectedFluxfilename);
                FluxSource::updateConfigForFilename(
                    config.mutable_flux_source(), _selectedFluxfilename);
                break;
            }

            case SELECTEDSOURCE_IMAGE:
            {
                ImageReader::updateConfigForFilename(
                    config.mutable_image_reader(), _selectedImagefilename);
                ImageWriter::updateConfigForFilename(
                    config.mutable_image_writer(), _selectedImagefilename);
                break;
            }
        }
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

        s = "0";
        _config.Read(CONFIG_FORTYTRACK, &s);
        _selectedFortyTrack = wxAtoi(s);

        /* Flux image block. */

        s = "";
        _config.Read(CONFIG_FLUXIMAGE, &s);
        _selectedFluxfilename = s;

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
        _config.Write(
            CONFIG_FORTYTRACK, wxString(std::to_string(_selectedFortyTrack)));

        /* Flux image block. */

        _config.Write(CONFIG_FLUXIMAGE, wxString(_selectedFluxfilename));

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

                panel->fortyTrackDriveToggle->SetValue(_selectedFortyTrack);
                panel->fortyTrackDriveToggle->Bind(
                    wxEVT_COMMAND_CHECKBOX_CLICKED,
                    [=](wxCommandEvent& e)
                    {
                        _selectedFortyTrack =
                            panel->fortyTrackDriveToggle->GetValue();
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

            panel->fluxImagePicker->SetPath(_selectedFluxfilename);
            panel->fluxImagePicker->Bind(wxEVT_COMMAND_FILEPICKER_CHANGED,
                [=](wxFileDirPickerEvent& e)
                {
                    _selectedFluxfilename = e.GetPath();
                    OnControlsChanged(e);
                });

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

                for (auto& group : config.option_group())
                {
                    sizer->Add(new wxStaticText(formatOptionsContainer,
                        wxID_ANY,
                        group.comment() + ":"));

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
                                    _formatOptions.erase(std::make_pair(
                                        formatName, option.name()));
                                }

                                _formatOptions.insert(key);

                                OnControlsChanged(e);
                            });

                        if (_formatOptions.find(key) != _formatOptions.end())
                        {
                            rb->SetValue(true);
                            valueSet = true;
                        }

                        if (option.set_by_default() || !defaultButton)
                            defaultButton = rb;

                        first = false;
                    }

                    if (!valueSet && defaultButton)
                        defaultButton->SetValue(true);
                }

                /* Anything that's _not_ in a group gets a checkbox. */

                for (auto& option : config.option())
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

                if (config.option().empty() && config.option_group().empty())
                    sizer->Add(new wxStaticText(formatOptionsContainer,
                        wxID_ANY,
                        "(no options for this format)"));
            }

            formatOptionsContainer->SetSizerAndFit(sizer);
            Layout();
            SafeFit();
        }
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
    ConfigProto _configProto;
    std::vector<std::string> _formatNames;
    std::vector<std::shared_ptr<const CandidateDevice>> _devices;
    int _selectedSource;
    std::string _selectedDevice;
    int _selectedDrive;
    bool _selectedFortyTrack;
    bool _selectedHighDensity;
    std::string _selectedFluxfilename;
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
