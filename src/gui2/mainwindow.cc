#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "globals.h"
#include "mainwindow.h"

W_OBJECT_IMPL(MainWindow)

MainWindow::MainWindow()
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
