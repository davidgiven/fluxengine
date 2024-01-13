#pragma once

#include "lib/globals.h"
#include "userinterface.h"
#include <QThreadPool>
#include <QtConcurrent>

extern QThreadPool workerThreadPool;

class UserInterface : public Ui_MainWindow
{
};

class Application : public QApplication
{
public:
    Application(int& argc, char** argv): QApplication(argc, argv) {}
    virtual ~Application() {}

    virtual void sendToUiThread(std::function<void()> callback) = 0;

public:
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
