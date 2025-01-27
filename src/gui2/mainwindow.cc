#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "globals.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"

W_OBJECT_IMPL(MainWindow)

MainWindow::MainWindow():
    _logStreamBuf(
        [this](const std::string& s)
        {
            logViewerEdit->moveCursor(QTextCursor::End);
            logViewerEdit->insertPlainText(QString::fromStdString(s));
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

    _driveComponent = DriveComponent::create(this);
    _formatComponent = FormatComponent::create(this);
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
    std::visit(overloaded{/* Fallback --- do nothing */
                   [this](const auto& m)
                   {
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
        message);

    _logRenderer->add(message);
    _logStream.flush();
}

void MainWindow::collectConfig()
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

void MainWindow::settingsCanBeChanged(bool state)
{
    driveConfigurationBox->setEnabled(state);
    formatConfigurationBox->setEnabled(state);
}
