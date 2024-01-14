#pragma once

#include "lib/globals.h"
#include "userinterface.h"
#include "wobjectdefs.h"
#include "wobjectimpl.h"
#include <QtWidgets>
#include <QThreadPool>
#include <QtConcurrent>

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
extern void postToUiThread(std::function<void()> callback);
extern void runOnUiThread(std::function<void()> callback);

template <typename F>
static auto runOnWorkerThread(F function)
{
    return QtConcurrent::run(&workerThreadPool,
        [=]()
        {
            return function();
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
