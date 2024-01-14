#pragma once

#include "lib/globals.h"
#include "userinterface.h"
#include "wobjectdefs.h"
#include "wobjectimpl.h"
#include <QtWidgets>
#include <QThreadPool>
#include <QtConcurrent>

extern QThreadPool workerThreadPool;
Q_DECLARE_METATYPE(const ConfigProto*);

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
