#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/core/utils.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "arch/arch.h"
#include "globals.h"
#include "mainwindow.h"
#include "imager.h"

class MainWindowImpl : public MainWindow
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
        ui.setupUi(container);
        // _driveComponent = DriveComponent::create(this);
        // _formatComponent = FormatComponent::create(this);
        // _fluxComponent = FluxComponent::create(this);
        // _imageComponent = ImageComponent::create(this);

        // setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        // setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        // connect(readDiskButton,
        //     &QAbstractButton::clicked,
        //     this,
        //     &MainWindowImpl::readDisk);

        setState(STATE_IDLE);
    }

public:

    void collectConfig() override
    {
        try
        {
            globalConfig().clear();
            // _formatComponent->collectConfig();
            // _driveComponent->collectConfig();
        }
        catch (const ErrorException& e)
        {
            log("Fatal error: {}", e.message);
        }
        catch (...)
        {
            log("Mysterious uncaught exception!");
        }
    }

private:
    // void readDisk()
    // {
    //     if (isWorkerThreadRunning())
    //         return;

    //     collectConfig();
    //     ::emergencyStop = false;
    //     setState(STATE_READING);

    //     runThen(
    //         [this]()
    //         {
    //             auto fluxSource = FluxSource::create(globalConfig());
    //             auto decoder = Arch::createDecoder(globalConfig());
    //             auto diskflux = readDiskCommand(*fluxSource, *decoder);
    //         },
    //         [this]()
    //         {
    //             setState(STATE_IDLE);
    //         });
    // }
    // W_SLOT(readDisk)

private:
    void setState(int state)
    {
        _stopWidget->setEnabled(state != STATE_IDLE);
        _progressWidget->setEnabled(state != STATE_IDLE);

        _state = state;
    }
    W_SLOT(setState)

private:
    Ui_Imager ui;
    std::shared_ptr<const DiskFlux> _currentDisk;
    int _state;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
