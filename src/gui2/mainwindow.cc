#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "globals.h"
#include "mainwindow.h"

W_OBJECT_IMPL(MainWindow)

MainWindow::MainWindow():
        _logStreamBuf(
            [this](const std::string& s)
            {
                logViewerEdit->appendPlainText(QString::fromStdString(s));
                logViewerEdit->ensureCursorVisible();
            }),
        _logStream(&_logStreamBuf),
        _logRenderer(LogRenderer::create(_logStream))
{
    setupUi(this);

    _progressWidget = new QProgressBar();
    _progressWidget->setMinimum(0);
    _progressWidget->setMaximum(100);
    _progressWidget->setEnabled(false);
    _progressWidget->setAlignment(Qt::AlignRight);
    statusbar->addPermanentWidget(_progressWidget);

    _stopWidget = new QToolButton();
    _stopWidget->setText("Stop");
    statusbar->addPermanentWidget(_stopWidget);

    connect(_stopWidget,
        &QPushButton::clicked,
        this,
        [&]()
        {
            ::emergencyStop = true;
        });
}

void MainWindow::runThen(
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

    void MainWindow::logMessage(const AnyLogMessage& message)
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
