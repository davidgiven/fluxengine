#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/vfs/vfs.h"
#include "lib/core/utils.h"
#include "gui.h"
#include "lib/data/layout.h"
#include "filesystemmodel.h"
#include "fileviewerwindow.h"
#include "textviewerwindow.h"
#include "jobqueue.h"

const std::string DND_TYPE = "fluxengine.files";

class BrowserPanelImpl : public BrowserPanelGen, public BrowserPanel, JobQueue
{
    enum
    {
        STATE_DEAD,
        STATE_WORKING,
        STATE_IDLE,
    };

public:
    BrowserPanelImpl(MainWindow* mainWindow, wxSimplebook* parent):
        BrowserPanelGen(parent),
        BrowserPanel(mainWindow),
        /* This is wrong. Apparently the wxDataViewCtrl doesn't work properly
         * with DnD unless the format is wxDF_UNICODETEXT. It should be a custom
         * value. */
        _dndFormat(wxDF_UNICODETEXT)
    {
        _filesystemModel = FilesystemModel::Associate(browserTree);

        /* This is a bug workaround for an issue in wxformbuilder's generated
         * code; see https://github.com/wxFormBuilder/wxFormBuilder/pull/758.
         * The default handler for the submenu doesn't allow events to fire on
         * the button itself, so we have to override it with our own version. */

        browserToolbar->Connect(browserMoreMenuButton->GetId(),
            wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN,
            wxAuiToolBarEventHandler(BrowserPanelImpl::OnBrowserMoreMenuButton),
            NULL,
            this);

        /* This is a bug workaround for an issue where the calculation of the
         * item being dropped on is wrong due to the header not being taken into
         * account. See https://forums.wxwidgets.org/viewtopic.php?t=44752. */

        browserTree->EnableDragSource(_dndFormat);
        browserTree->EnableDropTarget(_dndFormat);

        parent->AddPage(this, "browser");
    }

    void OnBackButton(wxCommandEvent&) override
    {
        StartIdle();
    }

private:
    void SetState(int state)
    {
        _state = state;
        CallAfter(
            [&]()
            {
                UpdateState();
            });
    }

    void SwitchFrom() override
    {
        SetState(STATE_DEAD);
    }

public:
    void StartBrowsing() override
    {
        try
        {
            SetPage(MainWindow::PAGE_BROWSER);
            PrepareConfig();

            _filesystemModel->Clear(Path());
            _filesystemCapabilities = 0;
            _filesystemIsReadOnly = true;
            _filesystemNeedsFlushing = false;

            SetState(STATE_WORKING);

            QueueJob(
                [this]()
                {
                    _filesystem = Filesystem::createFilesystemFromConfig();
                    _filesystemCapabilities = _filesystem->capabilities();
                    _filesystemIsReadOnly = _filesystem->isReadOnly();

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
            StartIdle();
        }
    }

    void StartFormatting() override
    {
        try
        {
            SetPage(MainWindow::PAGE_BROWSER);
            PrepareConfig();

            _filesystemModel->Clear(Path());
            _filesystemCapabilities = 0;
            _filesystemIsReadOnly = true;
            _filesystemNeedsFlushing = false;

            SetState(STATE_WORKING);

            QueueJob(
                [this]()
                {
                    _filesystem = Filesystem::createFilesystemFromConfig();
                    _filesystemCapabilities = _filesystem->capabilities();
                    _filesystemIsReadOnly = _filesystem->isReadOnly();

                    runOnUiThread(
                        [&]()
                        {
                            wxCommandEvent e;
                            OnBrowserFormatButton(e);
                        });
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            StartIdle();
        }
    }

    void OnQueueEmpty() override
    {
        SetState(STATE_IDLE);
    }

    void QueueJob(std::function<void(void)> f)
    {
        SetState(STATE_WORKING);
        JobQueue::QueueJob(f);
    }

private:
    void UpdateState()
    {
        bool running = !IsQueueEmpty();
        bool selection = browserTree->GetSelection().IsOk();

        browserToolbar->EnableTool(
            browserBackTool->GetId(), _state == STATE_IDLE);

        uint32_t c = _filesystemCapabilities;
        bool ro = _filesystemIsReadOnly;
        bool needsFlushing = _filesystemNeedsFlushing;

        browserToolbar->EnableTool(browserInfoTool->GetId(),
            !running && (c & Filesystem::OP_GETDIRENT) && selection);
        browserToolbar->EnableTool(browserViewTool->GetId(),
            !running && (c & Filesystem::OP_GETFILE) && selection);
        browserToolbar->EnableTool(browserSaveTool->GetId(),
            !running && (c & Filesystem::OP_GETFILE) && selection);
        browserMoreMenu->Enable(browserAddMenuItem->GetId(),
            !running && !ro && (c & Filesystem::OP_PUTFILE));
        browserMoreMenu->Enable(browserNewDirectoryMenuItem->GetId(),
            !running && !ro && (c & Filesystem::OP_CREATEDIR));
        browserMoreMenu->Enable(browserRenameMenuItem->GetId(),
            !running && !ro && (c & Filesystem::OP_MOVE) && selection);
        browserMoreMenu->Enable(browserDeleteMenuItem->GetId(),
            !running && !ro && (c & Filesystem::OP_DELETE) && selection);
        browserToolbar->EnableTool(browserFormatTool->GetId(),
            !running && !ro && (c & Filesystem::OP_CREATE));

        browserDiscardButton->Enable(!running && needsFlushing);
        browserCommitButton->Enable(!running && needsFlushing);

        browserToolbar->Refresh();
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
        QueueJob(
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
                        UpdateFilesystemData();
                    });
            });
    }

    void UpdateFilesystemData()
    {
        QueueJob(
            [this]()
            {
                auto metadata = _filesystem->getMetadata();
                _filesystemNeedsFlushing = _filesystem->needsFlushing();

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

                            if (!totalBlocks)
                                throw std::out_of_range("no disk usage data");

                            diskSpaceGauge->Enable();
                            diskSpaceGauge->SetRange(totalBlocks);
                            diskSpaceGauge->SetValue(usedBlocks);
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

        QueueJob(
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

        GetfileDialog d(this, wxID_ANY);
        d.filenameText->SetValue(node->dirent->path.to_str());
        d.targetFilePicker->SetFileName(wxFileName(node->dirent->filename));
        d.targetFilePicker->SetFocus();
        d.buttons_OK->SetDefault();
        if (d.ShowModal() != wxID_OK)
            return;

        auto localPath = d.targetFilePicker->GetPath().ToStdString();
        QueueJob(
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
                    d.newNameText->SetFocus();
                    d.buttons_OK->SetDefault();
                    if (d.ShowModal() == wxID_OK)
                        path = Path(d.newNameText->GetValue().ToStdString());
                    else
                        path = Path("");
                });
        } while (!path.empty());
        return path;
    }

    std::shared_ptr<FilesystemNode> GetTargetDirectoryNode(wxDataViewItem& item)
    {
        Path path;
        if (item.IsOk())
        {
            auto node = _filesystemModel->Find(item);
            if (!node)
                return nullptr;
            path = node->dirent->path;
        }

        auto node = _filesystemModel->Find(path);
        if (!node)
            return nullptr;
        if (node->dirent->file_type != TYPE_DIRECTORY)
            return _filesystemModel->Find(path.parent());
        return node;
    }

    void OnBrowserAddMenuItem(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto dirNode = GetTargetDirectoryNode(item);
        if (!dirNode)
            return;

        auto localPath = wxFileSelector("Choose the name of the file to add",
            /* default_path= */ wxEmptyString,
            /* default_filename= */ wxEmptyString,
            /* default_extension= */ wxEmptyString,
            /* wildcard= */ wxEmptyString,
            /* flags= */ wxFD_OPEN | wxFD_FILE_MUST_EXIST)
                             .ToStdString();
        if (localPath.empty())
            return;
        auto path = dirNode->dirent->path.concat(
            wxFileName(localPath).GetFullName().ToStdString());

        QueueJob(
            [this, path, localPath]() mutable
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
                        UpdateFilesystemData();
                    });
            });
    }

    void OnBrowserDeleteMenuItem(wxCommandEvent&) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);
        if (!node)
            return;

        QueueJob(
            [this, node]()
            {
                _filesystem->deleteFile(node->dirent->path);

                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Delete(node->dirent->path);
                        UpdateFilesystemData();
                    });
            });
    }

    void OnBrowserFormatButton(wxCommandEvent&) override
    {
        FormatDialog d(this, wxID_ANY);
        d.volumeNameText->SetFocus();
        d.buttons_OK->SetDefault();
        if (d.ShowModal() != wxID_OK)
            return;

        auto volumeName = d.volumeNameText->GetValue().ToStdString();
        auto quickFormat = d.quickFormatCheckBox->GetValue();
        QueueJob(
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

    void OnBrowserFilenameChanged(wxDataViewEvent& event) override
    {
        if (!(_filesystem->capabilities() & Filesystem::OP_MOVE))
            return;

        auto node = _filesystemModel->Find(event.GetItem());
        if (!node)
            return;

        if (node->newname.empty())
            return;
        if (node->newname == node->dirent->filename)
            return;

        QueueJob(
            [this, node]() mutable
            {
                auto oldPath = node->dirent->path;
                auto newPath = oldPath.parent().concat(node->newname);

                newPath = ResolveFileConflicts_WT(newPath);
                if (newPath.empty())
                    return;

                _filesystem->moveFile(oldPath, newPath);
                node->newname = "";

                auto dirent = _filesystem->getDirent(newPath);
                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Delete(oldPath);
                        _filesystemModel->Add(dirent);
                        UpdateFilesystemData();
                    });
            });
    }

    void OnBrowserRenameMenuItem(wxCommandEvent& event) override
    {
        auto item = browserTree->GetSelection();
        auto node = _filesystemModel->Find(item);

        FileRenameDialog d(this, wxID_ANY);
        d.oldNameText->SetValue(node->dirent->path.to_str());
        d.newNameText->SetValue(node->dirent->path.to_str());
        d.newNameText->SetFocus();
        d.buttons_OK->SetDefault();
        if (d.ShowModal() != wxID_OK)
            return;

        ActuallyMoveFile(
            node->dirent->path, Path(d.newNameText->GetValue().ToStdString()));
    }

    void ActuallyMoveFile(const Path& oldPath, Path newPath)
    {
        QueueJob(
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
                        UpdateFilesystemData();
                    });
            });
    }

    void OnBrowserNewDirectoryMenuItem(wxCommandEvent& event) override
    {
        auto item = browserTree->GetSelection();
        auto node = GetTargetDirectoryNode(item);
        if (!node)
            return;
        auto path = node->dirent->path;

        CreateDirectoryDialog d(this, wxID_ANY);
        d.newNameText->SetValue(path.to_str() + "/");
        d.newNameText->SetFocus();
        d.buttons_OK->SetDefault();
        if (d.ShowModal() != wxID_OK)
            return;

        auto newPath = Path(d.newNameText->GetValue().ToStdString());
        QueueJob(
            [this, newPath]() mutable
            {
                newPath = ResolveFileConflicts_WT(newPath);
                _filesystem->createDirectory(newPath);

                auto dirent = _filesystem->getDirent(newPath);
                runOnUiThread(
                    [&]()
                    {
                        _filesystemModel->Add(dirent);
                        UpdateFilesystemData();
                    });
            });
    }

    void OnBrowserBeginDrag(wxDataViewEvent& event) override
    {
        auto item = browserTree->GetSelection();
        if (!item.IsOk())
        {
            event.Veto();
            return;
        }

        auto node = _filesystemModel->Find(item);
        if (!node)
        {
            event.Veto();
            return;
        }

        wxTextDataObject* obj = new wxTextDataObject();
        obj->SetText(node->dirent->path.to_str());
        event.SetDataObject(obj);
        event.SetDataFormat(_dndFormat);
    }

    void OnBrowserDropPossible(wxDataViewEvent& event) override
    {
        if (event.GetDataFormat() != _dndFormat)
        {
            event.Veto();
            return;
        }
    }

    void OnBrowserDrop(wxDataViewEvent& event) override
    {
        try
        {
            if (event.GetDataFormat() != _dndFormat)
                throw CancelException();

#if defined __WXGTK__
            /* wxWidgets 3.0 data view DnD on GTK is borked. See
             * https://forums.wxwidgets.org/viewtopic.php?t=44752. The hit
             * detection is done against the wrong object, resulting in the
             * header size not being taken into account, so we have to
             * manually do hit detection correctly. */

            auto* window = browserTree->GetMainWindow();
            auto screenPos = wxGetMousePosition();
            auto relPos = screenPos - window->GetScreenPosition();

            wxDataViewItem item;
            wxDataViewColumn* column;
            browserTree->HitTest(relPos, item, column);
            if (!item.IsOk())
                throw CancelException();
#else
            auto item = event.GetItem();
#endif

            auto destDirNode = GetTargetDirectoryNode(item);
            if (!destDirNode)
                throw CancelException();
            auto destDirPath = destDirNode->dirent->path;

            wxTextDataObject obj;
            obj.SetData(_dndFormat, event.GetDataSize(), event.GetDataBuffer());
            auto srcPath = Path(obj.GetText().ToStdString());
            if (srcPath.empty())
                throw CancelException();

            ActuallyMoveFile(srcPath, destDirPath.concat(srcPath.back()));
        }
        catch (const CancelException& e)
        {
            event.Veto();
        }
    }

    void OnBrowserCommitButton(wxCommandEvent&) override
    {
        QueueJob(
            [this]()
            {
                _filesystem->flushChanges();
                UpdateFilesystemData();
            });
    }

    void OnBrowserDiscardButton(wxCommandEvent&) override
    {
        QueueJob(
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

    void OnBrowserSelectionChanged(wxDataViewEvent& event) override
    {
        UpdateState();
    }

private:
    int _state = STATE_DEAD;
    std::unique_ptr<Filesystem> _filesystem;
    uint32_t _filesystemCapabilities;
    bool _filesystemIsReadOnly;
    bool _filesystemNeedsFlushing;
    FilesystemModel* _filesystemModel;
    wxDataFormat _dndFormat;
};

BrowserPanel* BrowserPanel::Create(MainWindow* mainWindow, wxSimplebook* parent)
{
    return new BrowserPanelImpl(mainWindow, parent);
}
