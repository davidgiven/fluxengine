#include "globals.h"
#include "mainwindow.h"
#include <QtConcurrent>

Application* app;
QThreadPool workerThreadPool;

/* This has to go first due to C++ compiler limitations (has to be defined
 * before use). */
class ApplicationImpl : public Application
{
public:
    ApplicationImpl(int& argc, char** argv):
        Application(argc, argv),
        _mainWindow(MainWindow::create())
    {
        _mainWindow->show();
    }

public:
    void sendToUiThread(std::function<void()> callback) override {}

private:
    std::unique_ptr<MainWindow> _mainWindow;
};

int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(resources);
    qRegisterMetaType<const ConfigProto*>("const ConfigProto*");
    workerThreadPool.setMaxThreadCount(1);

    ApplicationImpl impl(argc, argv);
    app = &impl;
    return app->exec();
}