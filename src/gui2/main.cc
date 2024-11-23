#include "lib/core/globals.h"
#include "lib/core/logger.h"
#include "globals.h"
#include "mainwindow.h"
#include <QtConcurrent>

Application* app;
QThreadPool workerThreadPool;

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

        Logger::setLogger(
            [&](const AnyLogMessage& message)
            {
                if (isGuiThread())
                    _mainWindow->logMessage(message);
                else
                    postToUiThread(
                        [message, this]()
                        {
                            _mainWindow->logMessage(message);
                        });
            });
    }

public:
    void sendToUiThread(std::function<void()> callback) override {}

private:
    std::unique_ptr<MainWindow> _mainWindow;
};

bool isGuiThread()
{
    return (QThread::currentThread() == QCoreApplication::instance()->thread());
}

bool isWorkerThreadRunning()
{
    return workerThreadPool.activeThreadCount() != 0;
}

void postToUiThread(std::function<void()> callback)
{
    QMetaObject::invokeMethod((QApplication*)app, callback);
}

void runOnUiThread(std::function<void()> callback)
{
    QMetaObject::invokeMethod((QApplication*)app, callback);
}

QFuture<void> safeRunOnWorkerThread(std::function<void()> callback)
{
    return QtConcurrent::run(&workerThreadPool,
        [=]()
        {
            try
            {
                callback();
            }
            catch (const ErrorException& e)
            {
                log("Fatal error: {}", e.message);
            }
            catch (const std::exception& e)
            {
                log("Fatal error: {}", e.what());
            }
            catch (...)
            {
                log("Uncaught exception!");
            }
        });
}

Application::Application(int& argc, char** argv):
    QApplication(argc, argv),
    QSettings("Cowlark Technologies", "FluxEngine")
{
}

int main(int argc, char** argv)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    qRegisterMetaType<const ConfigProto*>();
    Q_INIT_RESOURCE(resources);
    workerThreadPool.setMaxThreadCount(1);

    ApplicationImpl impl(argc, argv);
    return app->exec();
}