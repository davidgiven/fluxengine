#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "arch/arch.h"
#include "globals.h"
#include "mainwindow.h"
#include "filer.h"
#include "fluxvisualiserwidget.h"
#include "imagevisualiserwidget.h"

class MainWindowImpl : public MainWindow, protected Ui_Filer
{
    W_OBJECT(MainWindowImpl)

private:
    enum State
    {
        STATE_IDLE,
        STATE_READING,
        STATE_WRITING
    };

public:
    MainWindowImpl()
    {
        Ui_Filer::setupUi(container);

        setState(STATE_IDLE);
    }

protected:
    /* Runs on the UI thread. */
    void logMessage(const AnyLogMessage& message)
    {
        std::visit(overloaded{/* Fallback --- do nothing */
                       [&](const auto& m)
                       {
                       },

                       /* A track has been read. */
                       [&](std::shared_ptr<const TrackReadLogMessage> m)
                       {
                       },

                       /* A complete disk has been read. */
                       [&](std::shared_ptr<const DiskReadLogMessage> m)
                       {
                       },

                       /* Large-scale operation end. */
                       [&](std::shared_ptr<const EndOperationLogMessage> m)
                       {
                       }},
            message);

        MainWindow::logMessage(message);
    }

private:
    void updateState()
    {
        setState(_state);
    }

    void setState(int state)
    {
        settingsCanBeChanged(state == STATE_IDLE);

        _state = state;
    }
    W_SLOT(setState)

private:
    int _state;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
