#include "lib/globals.h"
#include "lib/config.h"
#include "lib/readerwriter.h"
#include "lib/utils.h"
#include "globals.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"
#include "fluxcomponent.h"
#include "imagecomponent.h"

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
    MainWindowImpl()
    {
        setupUi(this);
        _driveComponent = DriveComponent::create(this);
        _formatComponent = FormatComponent::create(this);
        _fluxComponent = FluxComponent::create(this);
        _imageComponent = ImageComponent::create(this);

        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        _progressWidget = new QProgressBar();
        _progressWidget->setMinimum(0);
        _progressWidget->setMaximum(100);
        _progressWidget->setEnabled(false);
        _progressWidget->setAlignment(Qt::AlignRight);
        statusbar->addPermanentWidget(_progressWidget);

        _stopWidget = new QToolButton();
        _stopWidget->setText("Stop");
        statusbar->addPermanentWidget(_stopWidget);

        connect(readDiskButton,
            &QAbstractButton::clicked,
            this,
            &MainWindowImpl::readDisk);
        connect(_stopWidget,
            &QPushButton::clicked,
            this,
            &MainWindowImpl::emergencyStop);

        setState(STATE_IDLE);
    }

public:
    void logMessage(std::shared_ptr<const AnyLogMessage> message) override
    {
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

        logViewerEdit->appendPlainText(
            QString::fromStdString(Logger::toString(*message)));
        logViewerEdit->ensureCursorVisible();
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
                auto& fluxSource = globalConfig().getFluxSource();
                auto& decoder = globalConfig().getDecoder();
                auto diskflux = readDiskCommand(*fluxSource, *decoder);
            },
            [this]()
            {
                setState(STATE_IDLE);
            });
    }
    W_SLOT(readDisk)

private:
    void emergencyStop()
    {
        ::emergencyStop = true;
    }
    W_SLOT(emergencyStop)

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
    void runThen(
        std::function<void()> workCb, std::function<void()> completionCb)
    {
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        watcher->setFuture(safeRunOnWorkerThread(workCb));
        connect(watcher, &QFutureWatcher<void>::finished, completionCb);
        connect(watcher,
            &QFutureWatcher<void>::finished,
            watcher,
            &QFutureWatcher<void>::deleteLater);
    }

private:
    DriveComponent* _driveComponent;
    FormatComponent* _formatComponent;
    FluxComponent* _fluxComponent;
    ImageComponent* _imageComponent;
    QAbstractButton* _stopWidget;
    QProgressBar* _progressWidget;
    std::shared_ptr<const DiskFlux> _currentDisk;
    int _state;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
