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
#include "textviewerwindow.h"
#include "fileviewerwindow.h"
#include "texteditorwindow.h"
#include "filesystemmodel.h"
#include "customstatusbar.h"
#include "lib/vfs/vfs.h"
#include <google/protobuf/text_format.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <deque>

extern const std::map<std::string, std::string> formats;

#define CONFIG_SELECTEDSOURCE "SelectedSource"
#define CONFIG_DEVICE "Device"
#define CONFIG_DRIVE "Drive"
#define CONFIG_HIGHDENSITY "HighDensity"
#define CONFIG_FORMAT "Format"
#define CONFIG_EXTRACONFIG "ExtraConfig"
#define CONFIG_FLUXIMAGE "FluxImage"
#define CONFIG_DISKIMAGE "DiskImage"

const std::string DEFAULT_EXTRA_CONFIGURATION =
    "# Place any extra configuration here.\n"
    "# Each line can contain a key=value pair to set a property,\n"
    "# or the name of a built-in configuration, or the filename\n"
    "# of a text proto file. Or a comment, of course.\n\n";

class MainWindow : public MainWindowGen
{
private:
    class FilesystemOperation;

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

        _logWindow.reset(
            TextViewerWindow::Create(this, "Log viewer", "", false));
        _configWindow.reset(
            TextViewerWindow::Create(this, "Configuration viewer", "", false));

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
            _formats.push_back(std::make_pair(it.first, std::move(config)));
            i++;
        }

        Bind(UPDATE_STATE_EVENT,
            [this](wxCommandEvent&)
            {
                UpdateState();
            });
        _exitTimer.Bind(wxEVT_TIMER,
            [this](auto&)
            {
                wxCommandEvent e;
                OnExit(e);
            });

        visualiser->Bind(
            TRACK_SELECTION_EVENT, &MainWindow::OnTrackSelection, this);

        CreateStatusBar();

        _statusBar->Bind(PROGRESSBAR_STOP_EVENT,
            [this](auto&)
            {
                emergencyStop = true;
            });

        _filesystemModel = FilesystemModel::Associate(browserTree);

        /* This is a bug workaround for an issue in wxformbuilder's generated
         * code; see https://github.com/wxFormBuilder/wxFormBuilder/pull/758.
         * The default handler for the submenu doesn't allow events to fire on
         * the button itself, so we have to override it with our own version. */

        browserToolbar->Connect(browserMoreMenuButton->GetId(),
            wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN,
            wxAuiToolBarEventHandler(MainWindow::OnBrowserMoreMenuButton),
            NULL,
            this);

        /* I have no idea why this is necessary, but on Windows things aren't
         * laid out correctly without it. */

        realDiskRadioButtonPanel->Hide();
        fluxImageRadioButtonPanel->Hide();
        diskImageRadioButtonPanel->Hide();

        LoadConfig();
        UpdateDevices();
    }

    void OnShowLogWindow(wxCommandEvent& event) override
    {
        _logWindow->Show();
    }

    void OnShowConfigWindow(wxCommandEvent& event) override
    {
        _configWindow->Show();
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

    void OnExit(wxCommandEvent& event)
    {
        if (wxGetApp().IsWorkerThreadRunning())
        {
            emergencyStop = true;

            /* We need to wait for the worker thread to exit before we can
             * continue to shut down. */

            _exitTimer.StartOnce(100);
        }
        else
            Destroy();
    }

    void OnClose(wxCloseEvent& event)
    {
        event.Veto();

        wxCommandEvent e;
        OnExit(e);
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
        auto configRadioButton = [&](wxRadioButton* button, wxPanel* panel)
        {
            panel->Show(button->GetValue());
        };
        configRadioButton(realDiskRadioButton, realDiskRadioButtonPanel);
        configRadioButton(fluxImageRadioButton, fluxImageRadioButtonPanel);
        configRadioButton(diskImageRadioButton, diskImageRadioButtonPanel);
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

    void OnControlsChanged(wxFileDirPickerEvent& event)
    {
        SaveConfig();
        UpdateState();
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

    /* --- Browser ---------------------------------------------------------- */

    void OnBrowseButton(wxCommandEvent& event) override
    {
        try
        {
            PrepareConfig();

            visualiser->Clear();
            _filesystemModel->Clear(Path());
            _currentDisk = nullptr;

            _state = STATE_BROWSING_WORKING;
            UpdateState();
            ShowConfig();

            QueueBrowserOperation(
                [this]()
                {
                    _filesystem = Filesystem::createFilesystemFromConfig();

                    runOnUiThread(
                        [&]()
                        {
                            RepopulateBrowser();
                        });
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            _state = STATE_IDLE;
        }
    }

    void OnBrowserMoreMenuButton(wxAuiToolBarEvent& event)
    {
        browserToolbar->SetToolSticky(event.GetId(), true);
        wxRect rect = browserToolbar->GetToolRect(event.GetId());
        wxPoint pt = browserToolbar->ClientToScreen(rect.GetBottomLeft());
        pt = ScreenToClient(pt);
        browserToolbar->PopupMenu(browserMoreMenu, pt);
        browserToolbar->SetToolSticky(event.GetId(), false);
    }

    void RepopulateBrowser(Path path = Path())
    {
        QueueBrowserOperation(
            [this, path]()
            {
                auto files = _filesystem->list(path);

                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Clear(path);
                        for (auto& f : files)
                            _filesystemModel->Add(f);

                        auto node = _filesystemModel->Find(path);
                        if (node)
                            browserTree->Expand(node->item);
                        UpdateDiskSpaceGauge();
                    });
            });
    }

    void UpdateDiskSpaceGauge()
    {
        QueueBrowserOperation(
            [this]()
            {
                auto metadata = _filesystem->getMetadata();

                runOnUiThread(
                    [&]()
                    {
                        try
                        {
                            uint32_t blockSize =
                                std::stoul(metadata.at(Filesystem::BLOCK_SIZE));
                            uint32_t totalBlocks = std::stoul(
                                metadata.at(Filesystem::TOTAL_BLOCKS));
                            uint32_t usedBlocks = std::stoul(
                                metadata.at(Filesystem::USED_BLOCKS));

                            diskSpaceGauge->Enable();
                            diskSpaceGauge->SetRange(totalBlocks * blockSize);
                            diskSpaceGauge->SetValue(usedBlocks * blockSize);
                        }
                        catch (const std::out_of_range& e)
                        {
                            diskSpaceGauge->Disable();
                        }
                    });
            });
    }

    void OnBrowserDirectoryExpanding(wxDataViewEvent& event) override
    {
        auto node = _filesystemModel->Find(event.GetItem());
        if (node && !node->populated && !node->populating)
        {
            node->populating = true;
            RepopulateBrowser(node->dirent->path);
        }
    }

    void OnBrowserInfoButton(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        std::stringstream ss;
        ss << "File attributes for " << node->dirent->path.to_str() << ":\n\n";
        for (const auto& e : node->dirent->attributes)
            ss << e.first << "=" << quote(e.second) << "\n";

        TextViewerWindow::Create(
            this, node->dirent->path.to_str(), ss.str(), true)
            ->Show();
    }

    void OnBrowserViewButton(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        QueueBrowserOperation(
            [this, node]()
            {
                auto bytes = _filesystem->getFile(node->dirent->path);

                runOnUiThread(
                    [&]()
                    {
                        (new FileViewerWindow(
                             this, node->dirent->path.to_str(), bytes))
                            ->Show();
                    });
            });
    }

    void OnBrowserSaveButton(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        GetfileDialog dialog(this, wxID_ANY);
        dialog.filenameText->SetValue(node->dirent->path.to_str());
        dialog.targetFilePicker->SetFileName(
            wxFileName(node->dirent->filename));
        if (dialog.ShowModal() != wxID_OK)
            return;

        auto localPath = dialog.targetFilePicker->GetPath().ToStdString();
        QueueBrowserOperation(
            [this, node, localPath]()
            {
                auto bytes = _filesystem->getFile(node->dirent->path);
                bytes.writeToFile(localPath);
            });
    }

    /* Called from worker thread only! */
    Path ResolveFileConflicts_WT(Path path)
    {
        do
        {
            try
            {
                _filesystem->getDirent(path);
            }
            catch (const FileNotFoundException& e)
            {
                break;
            }

            runOnUiThread(
                [&]()
                {
                    FileConflictDialog d(this, wxID_ANY);
                    d.oldNameText->SetValue(path.to_str());
                    d.newNameText->SetValue(path.to_str());
                    if (d.ShowModal() == wxID_OK)
                        path = Path(d.newNameText->GetValue().ToStdString());
                    else
                        path = Path("");
                });
        } while (!path.empty());
        return path;
    }

    void OnBrowserAddMenuItem(wxCommandEvent&) override
    {
        Path dirPath;
        auto item = browserTree->GetSelection();
        if (item.IsOk())
        {
            auto dirNode = _filesystemModel->Find(item);
            if (!dirNode)
                return;
            dirPath = dirNode->dirent->path;
        }

        auto dirNode = _filesystemModel->Find(dirPath);
        if (!dirNode)
            return;
        if (dirNode->dirent->file_type != TYPE_DIRECTORY)
        {
            dirPath = dirPath.parent();
            dirNode = _filesystemModel->Find(dirPath);
            if (!dirNode)
                return;
        }

        auto localPath = wxFileSelector("Choose the name of the file to add",
            /* default_path= */ wxEmptyString,
            /* default_filename= */ wxEmptyString,
            /* default_extension= */ wxEmptyString,
            /* wildcard= */ wxEmptyString,
            /* flags= */ wxFD_OPEN | wxFD_FILE_MUST_EXIST)
                             .ToStdString();
        if (localPath.empty())
            return;
        Path path = dirPath;
        path.push_back(wxFileName(localPath).GetFullName().ToStdString());

        QueueBrowserOperation(
            [this, path, localPath, item]() mutable
            {
                path = ResolveFileConflicts_WT(path);
				if (path.empty())
					return;

                auto bytes = Bytes::readFromFile(localPath);
                _filesystem->putFile(path, bytes);

                auto dirent = _filesystem->getDirent(path);

                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Add(dirent);
                    });
            });
    }

    void OnBrowserDeleteMenuItem(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        QueueBrowserOperation(
            [this, node]()
            {
                _filesystem->deleteFile(node->dirent->path);

                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Delete(node->dirent->path);
                    });
            });
    }

    void OnBrowserFormatButton(wxCommandEvent&) override
    {
        FormatDialog d(this, wxID_ANY);
        if (d.ShowModal() != wxID_OK)
            return;

        auto volumeName = d.volumeNameText->GetValue().ToStdString();
        auto quickFormat = d.quickFormatCheckBox->GetValue();
        QueueBrowserOperation(
            [this, volumeName, quickFormat]()
            {
                _filesystem->discardChanges();
                _filesystem->create(quickFormat, volumeName);

                runOnUiThread(
                    [&]()
                    {
                        RepopulateBrowser();
                    });
            });
    }

    void OnBrowserFilenameChanged(wxDataViewEvent& event)
    {
        if (!(_filesystem->capabilities() & Filesystem::OP_MOVE))
            return;

        auto node = _filesystemModel->Find(event.GetItem());
        if (!node)
            return;

        if (node->newname.empty())
            return;

        QueueBrowserOperation(
            [this, node]() mutable
            {
                auto oldPath = node->dirent->path;
                auto newPath = oldPath.parent();
                newPath.push_back(node->newname);

                newPath = ResolveFileConflicts_WT(newPath);
				if (newPath.empty())
					return;

                _filesystem->moveFile(oldPath, newPath);

                auto dirent = _filesystem->getDirent(newPath);
                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Delete(oldPath);
                        _filesystemModel->Add(dirent);
                    });
            });
    }

    void OnBrowserRenameMenuItem(wxCommandEvent& event)
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        FileRenameDialog d(this, wxID_ANY);
        d.oldNameText->SetValue(node->dirent->path.to_str());
        d.newNameText->SetValue(node->dirent->path.to_str());
        if (d.ShowModal() != wxID_OK)
            return;

        auto oldPath = node->dirent->path;
        auto newPath = Path(d.newNameText->GetValue().ToStdString());
        QueueBrowserOperation(
            [this, oldPath, newPath]() mutable
            {
                newPath = ResolveFileConflicts_WT(newPath);
				if (newPath.empty())
					return;

                _filesystem->moveFile(oldPath, newPath);

                auto dirent = _filesystem->getDirent(newPath);
                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Delete(oldPath);
                        _filesystemModel->Add(dirent);
                    });
            });
    }

    void OnBrowserNewDirectoryMenuItem(wxCommandEvent& event)
    {
        Path path;
        auto item = browserTree->GetSelection();
        if (item.IsOk())
        {
            auto node = _filesystemModel->Find(item);
            if (!node)
                return;
            path = node->dirent->path;
        }

        auto node = _filesystemModel->Find(path);
        if (!node)
            return;
        if (node->dirent->file_type != TYPE_DIRECTORY)
        {
            path = path.parent();
            node = _filesystemModel->Find(path);
            if (!node)
                return;
        }

        CreateDirectoryDialog d(this, wxID_ANY);
        d.newNameText->SetValue(path.to_str() + "/");
        if (d.ShowModal() != wxID_OK)
            return;

        auto newPath = Path(d.newNameText->GetValue().ToStdString());
        QueueBrowserOperation(
            [this, newPath]() mutable
            {
                newPath = ResolveFileConflicts_WT(newPath);
                _filesystem->createDirectory(newPath);

                auto dirent = _filesystem->getDirent(newPath);
                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Add(dirent);
                        UpdateDiskSpaceGauge();
                    });
            });
    }

    void OnBrowserCommitButton(wxCommandEvent&) override
    {
        QueueBrowserOperation(
            [this]()
            {
                _filesystem->flushChanges();
            });
    }

    void OnBrowserDiscardButton(wxCommandEvent&) override
    {
        QueueBrowserOperation(
            [this]()
            {
                _filesystem->discardChanges();

                runOnUiThread(
                    [&]()
                    {
                        RepopulateBrowser();
                    });
            });
    }

    void QueueBrowserOperation(std::function<void()> f)
    {
        _filesystemQueue.push_back(f);
        KickBrowserQueue();
    }

    void KickBrowserQueue()
    {
        bool running = wxGetApp().IsWorkerThreadRunning();
        if (!running)
        {
            _state = STATE_BROWSING_WORKING;
            _errorState = STATE_BROWSING_IDLE;
            UpdateState();
            runOnWorkerThread(
                [this]()
                {
                    for (;;)
                    {
                        std::function<void()> f;
                        runOnUiThread(
                            [&]()
                            {
                                UpdateState();
                                if (!_filesystemQueue.empty())
                                {
                                    f = _filesystemQueue.front();
                                    _filesystemQueue.pop_front();
                                }
                            });
                        if (!f)
                            break;

                        f();
                    }

                    runOnUiThread(
                        [&]()
                        {
                            _state = STATE_BROWSING_IDLE;
                            UpdateState();
                        });
                });
        }
    }

    void OnBrowserSelectionChanged(wxDataViewEvent& event) override
    {
        UpdateState();
    }

    /* --- Config management
     * ------------------------------------------------ */

    /* This sets the *global* config object. That's safe provided the worker
     * thread isn't running, otherwise you'll get a race. */
    void PrepareConfig()
    {
        assert(!wxGetApp().IsWorkerThreadRunning());

        auto formatSelection = formatChoice->GetSelection();
        if (formatSelection == wxNOT_FOUND)
            Error() << "no format selected";
        config = *_formats[formatChoice->GetSelection()].second;

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

        auto serial = deviceCombo->GetValue().ToStdString();
        if (!serial.empty() && (serial[0] == '/'))
            setProtoByString(&config, "usb.greaseweazle.port", serial);
        else
            setProtoByString(&config, "usb.serial", serial);

        _logWindow->GetTextControl()->Clear();

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

    void ShowConfig()
    {
        std::string s;
        google::protobuf::TextFormat::PrintToString(config, &s);
        _configWindow->GetTextControl()->Clear();
        _configWindow->GetTextControl()->AppendText(s);
    }

    void OnLogMessage(std::shared_ptr<const AnyLogMessage> message)
    {
        _logWindow->GetTextControl()->AppendText(Logger::toString(*message));

        std::visit(
            overloaded{
                /* Fallback --- do nothing */
                [&](const auto& m)
                {
                },

                /* We terminated due to the stop button. */
                [&](const EmergencyStopMessage& m)
                {
                    _statusBar->SetLeftLabel("Emergency stop!");
                    _statusBar->HideProgressBar();
                    _statusBar->SetRightLabel("");
                    _state = _errorState;
                    UpdateState();
                },

                /* A fatal error. */
                [&](const ErrorLogMessage& m)
                {
                    _statusBar->SetLeftLabel(m.message);
                    wxMessageBox(m.message, "Error", wxOK | wxICON_ERROR);
                    _statusBar->HideProgressBar();
                    _statusBar->SetRightLabel("");
                    _state = _errorState;
                    _filesystemQueue.clear();
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

        s = DEFAULT_EXTRA_CONFIGURATION;
        _config.Read(CONFIG_EXTRACONFIG, &s);
        _extraConfiguration = s;

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
        _config.Write(CONFIG_EXTRACONFIG, wxString(_extraConfiguration));
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

            bool selection = browserTree->GetSelection().IsOk();

            browserToolbar->EnableTool(
                browserBackTool->GetId(), _state == STATE_BROWSING_IDLE);

            uint32_t capabilities =
                _filesystem ? _filesystem->capabilities() : 0;

            browserToolbar->EnableTool(browserInfoTool->GetId(),
                (capabilities & Filesystem::OP_GETDIRENT) && selection);
            browserToolbar->EnableTool(browserViewTool->GetId(),
                (capabilities & Filesystem::OP_GETFILE) && selection);
            browserToolbar->EnableTool(browserSaveTool->GetId(),
                (capabilities & Filesystem::OP_GETFILE) && selection);
            browserMoreMenu->Enable(browserAddMenuItem->GetId(),
                capabilities & Filesystem::OP_PUTFILE);
            browserMoreMenu->Enable(browserNewDirectoryMenuItem->GetId(),
                capabilities & Filesystem::OP_CREATEDIR);
            browserMoreMenu->Enable(browserRenameMenuItem->GetId(),
                (capabilities & Filesystem::OP_MOVE) && selection);
            browserMoreMenu->Enable(browserDeleteMenuItem->GetId(),
                (capabilities & Filesystem::OP_DELETE) && selection);
            browserToolbar->EnableTool(browserFormatTool->GetId(),
                capabilities & Filesystem::OP_CREATE);

            bool needsFlushing =
                _filesystem ? _filesystem->needsFlushing() : false;
            browserDiscardButton->Enable(needsFlushing);
            browserCommitButton->Enable(needsFlushing);
        }

        Refresh();
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

    class FilesystemOperation
    {
    public:
        FilesystemOperation(
            int operation, const Path& path, const wxDataViewItem& item):
            operation(operation),
            path(path),
            item(item)
        {
        }

        FilesystemOperation(int operation, const Path& path):
            operation(operation),
            path(path)
        {
        }

        FilesystemOperation(
            int operation, const Path& path, const std::string& local):
            operation(operation),
            path(path),
            local(local)
        {
        }

        FilesystemOperation(int operation,
            const Path& path,
            const wxDataViewItem& item,
            wxString& local):
            operation(operation),
            path(path),
            item(item),
            local(local)
        {
        }

        FilesystemOperation(int operation, const Path& path, wxString local):
            operation(operation),
            path(path),
            local(local)
        {
        }

        FilesystemOperation(int operation): operation(operation) {}

        int operation;
        Path path;
        wxDataViewItem item;
        std::string local;

        std::string volumeName;
        bool quickFormat;
    };

    wxConfig _config;
    std::vector<std::pair<std::string, std::unique_ptr<const ConfigProto>>>
        _formats;
    std::vector<std::unique_ptr<const CandidateDevice>> _devices;
    int _state = STATE_IDLE;
    int _errorState;
    int _selectedSource;
    bool _dontSaveConfig = false;
    std::shared_ptr<const DiskFlux> _currentDisk;
    CustomStatusBar* _statusBar;
    wxTimer _exitTimer;
    std::unique_ptr<TextViewerWindow> _logWindow;
    std::unique_ptr<TextViewerWindow> _configWindow;
    std::string _extraConfiguration;
    std::unique_ptr<Filesystem> _filesystem;
    FilesystemModel* _filesystemModel;
    std::deque<std::function<void()>> _filesystemQueue;
};

wxWindow* FluxEngineApp::CreateMainWindow()
{
    return new MainWindow();
}
