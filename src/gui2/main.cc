#include "globals.h"
#include "mainwindow.h"

std::unique_ptr<Application> app;

class ApplicationImpl : public Application
{
public:
    ApplicationImpl(int& argc, char** argv):
        Application(argc, argv),
        _mainWindow(MainWindow::create())
    {
        _mainWindow->show();
    }

private:
    std::unique_ptr<MainWindow> _mainWindow;
};

int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(resources);
    app = std::make_unique<ApplicationImpl>(argc, argv);
    return app->exec();
}