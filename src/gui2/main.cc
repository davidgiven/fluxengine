#include "globals.h"
#include "mainwindow.h"
#include <QtConcurrent>

Application* app;
QThreadPool workerThreadPool;

Application::Application(int& argc, char** argv):
    QApplication(argc, argv),
    QSettings("Cowlark Technologies", "FluxEngine")
{
}

Application::~Application() {}

class ApplicationImpl : public Application
{
public:
    ApplicationImpl(int& argc, char** argv): Application(argc, argv)
    {
        /* Must be set _before_ the main window is created. */

        app = this;

        _mainWindow = MainWindow::create();
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
    workerThreadPool.setMaxThreadCount(1);

    ApplicationImpl impl(argc, argv);
    return app->exec();
}