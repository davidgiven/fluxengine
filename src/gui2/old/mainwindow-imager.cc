#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "arch/arch.h"
#include "globals.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"
#include "fluxcomponent.h"
#include "imagecomponent.h"

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

class MainWindowImpl : public MainWindow
{
    W_OBJECT(MainWindowImpl)

private:
    enum State
    {
        STATE_IDLE,
        STATE_READING,
        STATE_WRITING
    };

public:
    MainWindowImpl():
        _logStreamBuf(
            [this](const std::string& s)
            {
                logViewerEdit->appendPlainText(QString::fromStdString(s));
                logViewerEdit->ensureCursorVisible();
            }),
        _logStream(&_logStreamBuf),
        _logRenderer(LogRenderer::create(_logStream))
    {
        _driveComponent = DriveComponent::create(this);
        _formatComponent = FormatComponent::create(this);
        _fluxComponent = FluxComponent::create(this);
        _imageComponent = ImageComponent::create(this);

        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        connect(readDiskButton,
            &QAbstractButton::clicked,
            this,
            &MainWindowImpl::readDisk);

        setState(STATE_IDLE);
    }

public:
    void logMessage(const AnyLogMessage& message) override
    {
#if 0
        std::visit(overloaded{/* Fallback --- do nothing */
                       [this](const auto& m)
                       {
                       },

                       /* A track has been read. */
                       [&](const TrackReadLogMessage& m)
                       {
                           _fluxComponent->setTrackData(m.track);
                           _imageComponent->setTrackData(m.track);
                       },

                       /* A complete disk has been read. */
                       [&](const DiskReadLogMessage& m)
                       {
                           _imageComponent->setDiskData(m.disk);
                           _currentDisk = m.disk;
                       },

                       /* Large-scale operation start. */
                       [this](const BeginOperationLogMessage& m)
                       {
                           _progressWidget->setValue(0);
                       },

                       /* Large-scale operation end. */
                       [this](const EndOperationLogMessage& m)
                       {
                       },

                       /* Large-scale operation progress. */
                       [this](const OperationProgressLogMessage& m)
                       {
                           _progressWidget->setValue(m.progress);
                       }},
            *message);
#endif

        _logRenderer->add(message);
        _logStream.flush();
    }

    void collectConfig() override
    {
        try
        {
            globalConfig().clear();
            _formatComponent->collectConfig();
            _driveComponent->collectConfig();
        }
        catch (const ErrorException& e)
        {
            log("Fatal error: {}", e.message);
        }
        catch (...)
        {
            log("Mysterious uncaught exception!");
        }
    }

private:
    void readDisk()
    {
        if (isWorkerThreadRunning())
            return;

        collectConfig();
        ::emergencyStop = false;
        setState(STATE_READING);

        runThen(
            [this]()
            {
                auto fluxSource = FluxSource::create(globalConfig());
                auto decoder = Arch::createDecoder(globalConfig());
                auto diskflux = readDiskCommand(*fluxSource, *decoder);
            },
            [this]()
            {
                setState(STATE_IDLE);
            });
    }
    W_SLOT(readDisk)

private:
    void setState(int state)
    {
        _stopWidget->setEnabled(state != STATE_IDLE);
        _progressWidget->setEnabled(state != STATE_IDLE);
        diskOperationsGroup->setEnabled(state == STATE_IDLE);
        memoryOperationsGroup->setEnabled(state == STATE_IDLE);
        driveWindow->setEnabled(state == STATE_IDLE);
        formatWindow->setEnabled(state == STATE_IDLE);

        _state = state;
    }
    W_SLOT(setState)

private:
    std::ostream _logStream;
    CallbackOstream _logStreamBuf;
    std::unique_ptr<LogRenderer> _logRenderer;
    DriveComponent* _driveComponent;
    FormatComponent* _formatComponent;
    FluxComponent* _fluxComponent;
    ImageComponent* _imageComponent;
    std::shared_ptr<const DiskFlux> _currentDisk;
    int _state;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
