#pragma once

#include "lib/globals.h"
#include "lib/logger.h"
#include "userinterface.h"
#include "wobjectdefs.h"
#include "wobjectimpl.h"
#include <QtWidgets>
#include <QThreadPool>
#include <QtConcurrent>

class TrackFlux;
class DiskFlux;

Q_DECLARE_METATYPE(const ConfigProto*)
W_REGISTER_ARGTYPE(std::shared_ptr<const TrackFlux>)
W_REGISTER_ARGTYPE(std::shared_ptr<const DiskFlux>)

extern QThreadPool workerThreadPool;

class UserInterface : public Ui_MainWindow
{
};

class Application : public QApplication, public QSettings
{
public:
    Application(int& argc, char** argv);
    virtual ~Application();

public:
    virtual void sendToUiThread(std::function<void()> callback) = 0;
};

extern bool isGuiThread();
extern bool isWorkerThreadRunning();
extern void postToUiThread(std::function<void()> callback);
extern void runOnUiThread(std::function<void()> callback);
extern QFuture<void> safeRunOnWorkerThread(std::function<void()> callback);

extern const std::map<std::string, const ConfigProto*> drivetypes;

template <typename F>
static inline auto runOnWorkerThread(F callback)
{
    return QtConcurrent::run(&workerThreadPool,
        [=]()
        {
            return callback();
        });
}

template <typename R>
static inline R runOnUiThread(std::function<R()> callback)
{
    R retvar;
    runOnUiThread(
        [&]()
        {
            retvar = callback();
        });
    return retvar;
}

extern Application* app;
