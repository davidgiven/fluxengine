#pragma once

#include "lib/core/logger.h"
#include "globals.h"

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    W_OBJECT(MainWindow)

public:
    static std::unique_ptr<MainWindow> create();

public:
    MainWindow();

public:
    virtual void logMessage(const AnyLogMessage& message) = 0;
    virtual void collectConfig() = 0;

protected:
    void runThen(
        std::function<void()> workCb, std::function<void()> completionCb);

protected:
    QAbstractButton* _stopWidget;
    QProgressBar* _progressWidget;
};
