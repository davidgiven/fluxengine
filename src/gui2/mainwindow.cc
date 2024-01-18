#include "lib/globals.h"
#include "lib/config.h"
#include "lib/readerwriter.h"
#include "globals.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"
#include "fluxcomponent.h"
#include "imagecomponent.h"

class MainWindowImpl : public MainWindow
{
    W_OBJECT(MainWindowImpl)

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

        auto* stopWidget = new QToolButton();
        stopWidget->setText("Stop");
        statusbar->addPermanentWidget(stopWidget);

        connect(readDiskButton,
            &QAbstractButton::clicked,
            this,
            &MainWindowImpl::readDisk);
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
                           _fluxComponent->setDiskData(m.disk);
                           _imageComponent->setDiskData(m.disk);
                       },

                       /* Large-scale operation start. */
                       [this](const BeginOperationLogMessage& m)
                       {
                           setProgressBar(0);
                       },

                       /* Large-scale operation end. */
                       [this](const EndOperationLogMessage& m)
                       {
                           finishedWithProgressBar();
                       },

                       /* Large-scale operation progress. */
                       [this](const OperationProgressLogMessage& m)
                       {
                           setProgressBar(m.progress);
                       }},
            *message);

        logViewerEdit->appendPlainText(
            QString::fromStdString(Logger::toString(*message)));
        logViewerEdit->ensureCursorVisible();
    }

    void setProgressBar(int progress) override
    {
        _progressWidget->setEnabled(true);
        _progressWidget->setValue(progress);
    }

    void finishedWithProgressBar() override
    {
        _progressWidget->setEnabled(false);
        _progressWidget->setValue(0);
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
            std::exception_ptr p = std::current_exception();
            std::clog << (p ? p.__cxa_exception_type()->name() : "null")
                      << std::endl;
        }
    }

private:
    void readDisk()
    {
        if (isWorkerThreadRunning())
            return;

        collectConfig();
        safeRunOnWorkerThread(
            [this]()
            {
                auto& fluxSource = globalConfig().getFluxSource();
                auto& decoder = globalConfig().getDecoder();
                auto diskflux = readDiskCommand(*fluxSource, *decoder);
            });
    }
    W_SLOT(readDisk)

private:
    DriveComponent* _driveComponent;
    FormatComponent* _formatComponent;
    FluxComponent* _fluxComponent;
    ImageComponent* _imageComponent;
    QProgressBar* _progressWidget;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
