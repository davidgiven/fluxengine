#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/proto.h"
#include "gui.h"
#include "lib/core/logger.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/encoders/encoders.h"
#include "lib/decoders/decoders.h"
#include "lib/core/utils.h"
#include "fluxviewerwindow.h"
#include "textviewerwindow.h"
#include "fileviewerwindow.h"
#include "texteditorwindow.h"
#include "filesystemmodel.h"
#include "customstatusbar.h"
#include "context.h"
#include "lib/vfs/vfs.h"
#include "lib/data/layout.h"
#include <google/protobuf/text_format.h>
#include <wx/aboutdlg.h>
#include <deque>

class CallbackOstream : public std::streambuf
{
public:
    CallbackOstream(std::function<void(const std::string&)> cb): _cb(cb) {}

public:
    std::streamsize xsputn(const char* p, std::streamsize n) override
    {
        _cb(std::string(p, n));
        return n;
    }

    int_type overflow(int_type v) override
    {
        char c = v;
        _cb(std::string(&c, 1));
        return 1;
    }

private:
    std::function<void(const std::string&)> _cb;
};

class MainWindowImpl : public MainWindowGen, public MainWindow
{
private:
    class FilesystemOperation;

public:
    MainWindowImpl():
        MainWindowGen(nullptr),
        _logStreamBuf(
            [this](const std::string& s)
            {
                if (_logWindow)
                    _logWindow->GetTextControl()->AppendText(s);
            }),
        _logStream(&_logStreamBuf),
        _logRenderer(LogRenderer::create(_logStream))
    {
        Logger::setLogger(
            [&](const AnyLogMessage& message)
            {
                if (isWorkerThread())
                {
                    runOnUiThread(
                        [message, this]()
                        {
                            OnLogMessage(message);
                        });
                }
                else
                    OnLogMessage(message);
            });

        _logWindow.reset(
            TextViewerWindow::Create(this, "Log viewer", "", false));
        _configWindow.reset(
            TextViewerWindow::Create(this, "Configuration viewer", "", false));

        _exitTimer.Bind(wxEVT_TIMER,
            [this](auto&)
            {
                wxCommandEvent e;
                OnExit(e);
            });

        CreateStatusBar();

        _statusBar->Bind(PROGRESSBAR_STOP_EVENT,
            [this](auto&)
            {
                emergencyStop = true;
            });

        _panelComponents[0] = _idlePanel =
            IdlePanel::Create(this, dataNotebook);
        _panelComponents[1] = _imagerPanel =
            ImagerPanel::Create(this, dataNotebook);
        _panelComponents[2] = _browserPanel =
            BrowserPanel::Create(this, dataNotebook);
        _panelComponents[3] = _explorerPanel =
            ExplorerPanel::Create(this, dataNotebook);

        wxIcon icon;
        icon.CopyFromBitmap(_idlePanel->GetBitmap());
        SetIcon(icon);

        StartIdle();
        Layout();
        Fit();
    }

    void SafeFit() override
    {
        auto minSize = GetMinClientSize();

        auto clientSize = GetClientSize();
        SetMinClientSize(clientSize);
        Fit();

        SetMinClientSize(minSize);
    }

    void StartIdle() override
    {
        _idlePanel->Start();
    }

    void StartReading() override
    {
        _imagerPanel->StartReading();
    }

    void StartWriting() override
    {
        _imagerPanel->StartWriting();
    }

    void StartBrowsing() override
    {
        _browserPanel->StartBrowsing();
    }

    void StartFormatting() override
    {
        _browserPanel->StartFormatting();
    }

    void StartExploring() override
    {
        _explorerPanel->Start();
    }

    void OnShowLogWindow(wxCommandEvent& event) override
    {
        _logWindow->Show();
    }

    void OnShowConfigWindow(wxCommandEvent& event) override
    {
        _configWindow->Show();
    }

    void ClearLog() override
    {
        _logWindow->GetTextControl()->Clear();
    }

    void OnExit(wxCommandEvent& event) override
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

    void OnClose(wxCloseEvent& event) override
    {
        event.Veto();

        wxCommandEvent e;
        OnExit(e);
    }

    void OnAboutMenuItem(wxCommandEvent& event) override
    {
        wxAboutDialogInfo aboutInfo;
        aboutInfo.SetName("FluxEngine");
        aboutInfo.SetDescription("Flux-level floppy disk management");
        aboutInfo.SetWebSite("http://cowlark.com/fluxengine");
        aboutInfo.SetCopyright("Mostly (C) 2018-2022 David Given");

        wxAboutBox(aboutInfo);
    }

    /* --- Config management ----------------------------------------------- */

    /* This sets the *global* config object. That's safe provided the worker
     * thread isn't running, otherwise you'll get a race. */
public:
    Context& GetContext()
    {
        assert(_context);
        return *_context;
    }

    void ShowConfig()
    {
        std::string s;
        google::protobuf::TextFormat::PrintToString(globalConfig(), &s);
        _configWindow->GetTextControl()->Clear();
        _configWindow->GetTextControl()->AppendText(s);
    }

    void PrepareConfig() override
    {
        _context = Context::Create();
        _idlePanel->PrepareConfig();
        ShowConfig();
    }

    void OnLogMessage(const AnyLogMessage& message)
    {
        _logRenderer->add(message);
        _logStream.flush();

        std::visit(
            overloaded{
                /* Fallback --- do nothing */
                [&](const auto& m)
                {
                },

                /* We terminated due to the stop button. */
                [&](std::shared_ptr<const EmergencyStopMessage> m)
                {
                    _statusBar->SetLeftLabel("Emergency stop!");
                    _statusBar->HideProgressBar();
                    _statusBar->SetRightLabel("");
                },

                /* A fatal error. */
                [&](std::shared_ptr<const ErrorLogMessage> m)
                {
                    _statusBar->SetLeftLabel(m->message);
                    wxMessageBox(m->message, "Error", wxOK | wxICON_ERROR);
                    _statusBar->HideProgressBar();
                    _statusBar->SetRightLabel("");
                },

                /* Indicates that we're starting a write operation. */
                [&](std::shared_ptr<const BeginWriteOperationLogMessage> m)
                {
                    _statusBar->SetRightLabel(
                        fmt::format("W {}.{}", m->track, m->head));
                    _imagerPanel->SetVisualiserMode(
                        m->track, m->head, VISMODE_WRITING);
                },

                [&](std::shared_ptr<const EndWriteOperationLogMessage> m)
                {
                    _statusBar->SetRightLabel("");
                    _imagerPanel->SetVisualiserMode(0, 0, VISMODE_NOTHING);
                },

                /* Indicates that we're starting a read operation. */
                [&](std::shared_ptr<const BeginReadOperationLogMessage> m)
                {
                    _statusBar->SetRightLabel(
                        fmt::format("R {}.{}", m->track, m->head));
                    _imagerPanel->SetVisualiserMode(
                        m->track, m->head, VISMODE_READING);
                },

                [&](std::shared_ptr<const EndReadOperationLogMessage> m)
                {
                    _statusBar->SetRightLabel("");
                    _imagerPanel->SetVisualiserMode(0, 0, VISMODE_NOTHING);
                },

                [&](std::shared_ptr<const TrackReadLogMessage> m)
                {
                    _imagerPanel->SetVisualiserTrackData(m->track);
                },

                [&](std::shared_ptr<const DiskReadLogMessage> m)
                {
                    _imagerPanel->SetDisk(m->disk);
                },

                /* Large-scale operation start. */
                [&](std::shared_ptr<const BeginOperationLogMessage> m)
                {
                    _statusBar->SetLeftLabel(m->message);
                    _statusBar->ShowProgressBar();
                },

                /* Large-scale operation end. */
                [&](std::shared_ptr<const EndOperationLogMessage> m)
                {
                    _statusBar->SetLeftLabel(m->message);
                    _statusBar->HideProgressBar();
                },

                /* Large-scale operation progress. */
                [&](std::shared_ptr<const OperationProgressLogMessage> m)
                {
                    _statusBar->SetProgress(m->progress);
                },

            },
            message);
    }

    void SetPage(int page) override
    {
        if (page != _page)
        {
            _panelComponents[_page]->SwitchFrom();
            _page = page;
            dataNotebook->SetSelection(_page);
            _panelComponents[_page]->SwitchTo();
        }
    }

    wxStatusBar* OnCreateStatusBar(
        int number, long style, wxWindowID id, const wxString& name) override
    {
        _statusBar = new CustomStatusBar(this, id);
        return _statusBar;
    }

private:
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

    PanelComponent* _panelComponents[4];
    IdlePanel* _idlePanel;
    ImagerPanel* _imagerPanel;
    BrowserPanel* _browserPanel;
    ExplorerPanel* _explorerPanel;
    int _page = PAGE_IDLE;
    int _state = 0;
    CustomStatusBar* _statusBar;
    wxTimer _exitTimer;
    std::unique_ptr<TextViewerWindow> _logWindow;
    CallbackOstream _logStreamBuf;
    std::ostream _logStream;
    std::unique_ptr<TextViewerWindow> _configWindow;
    std::unique_ptr<Context> _context;
    std::unique_ptr<LogRenderer> _logRenderer;
};

wxWindow* FluxEngineApp::CreateMainWindow()
{
    return new MainWindowImpl();
}
