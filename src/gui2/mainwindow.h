#pragma once

#include "lib/core/logger.h"
#include "globals.h"

class CallbackOstream : public std::streambuf
{
public:
    CallbackOstream(std::function<void(const std::string&)> cb): _cb(cb) {}

public:
    std::streamsize xsputn(const char* p, std::streamsize n) override
    {
        _cb(std::string(p, n));
        return n;
    }

    int_type overflow(int_type v) override
    {
        char c = v;
        _cb(std::string(&c, 1));
        return 1;
    }

private:
    std::function<void(const std::string&)> _cb;
};

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    W_OBJECT(MainWindow)

public:
    static std::unique_ptr<MainWindow> create();

public:
    MainWindow();

public:
    virtual void logMessage(const AnyLogMessage& message);
    virtual void collectConfig() = 0;

protected:
    void runThen(
        std::function<void()> workCb, std::function<void()> completionCb);

protected:
    QAbstractButton* _stopWidget;
    QProgressBar* _progressWidget;
    std::ostream _logStream;
    CallbackOstream _logStreamBuf;
    std::unique_ptr<LogRenderer> _logRenderer;
};
