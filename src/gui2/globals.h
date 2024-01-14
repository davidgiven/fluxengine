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

template <typename F>
auto runOnWorkerThread(F function)
{
    return QtConcurrent::run(&workerThreadPool,
        [=]()
        {
            return function();
        });
}

extern Application* app;
