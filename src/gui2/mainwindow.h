#pragma once

#include "lib/logger.h"
#include "globals.h"

class MainWindow : public QMainWindow, public Ui_MainWindow
{
public:
    static std::unique_ptr<MainWindow> create();

public:
    virtual void logMessage(std::shared_ptr<const AnyLogMessage> message) = 0;
    virtual void setProgressBar(int progress) = 0;
    virtual void finishedWithProgressBar() = 0;
    virtual void collectConfig() = 0;
};
