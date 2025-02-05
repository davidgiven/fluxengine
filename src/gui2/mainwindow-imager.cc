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
#include "fluxvisualiserwidget.h"
#include "imagevisualiserwidget.h"

class MainWindowImpl : public MainWindow, protected Ui_Imager
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
        Ui_Imager::setupUi(container);

        /* Flux visualiser */

        _fluxVisualiserWidget = FluxVisualiserWidget::create();
        fluxViewContainer->layout()->addWidget(_fluxVisualiserWidget);

        // connect(fluxSideComboBox,
        //     QOverload<int>::of(&QComboBox::activated),
        //     _fluxVisualiserWidget,
        //     &FluxVisualiserWidget::setVisibleSide);
        // connect(fluxContrastSlider,
        //     &QAbstractSlider::valueChanged,
        //     [this](int value)
        //     {
        //         _fluxVisualiserWidget->setGamma(value / 100.0);
        //     });
        // fluxContrastSlider->setValue(500);

        /* Image visualiser */

        _imageVisualiserWidget = ImageVisualiserWidget::create();
        imageViewContainer->layout()->addWidget(_imageVisualiserWidget);

        /* Controls */

        connect(readDiskButton,
            &QAbstractButton::clicked,
            this,
            &MainWindowImpl::readDisk);

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
                           _fluxVisualiserWidget->setTrackData(m->track);
                           _imageVisualiserWidget->setTrackData(m->track);
                       },

                       /* A complete disk has been read. */
                       [&](std::shared_ptr<const DiskReadLogMessage> m)
                       {
                           _currentDisk = m->disk;
                           updateState();
                       },

                       /* Large-scale operation end. */
                       [&](std::shared_ptr<const EndOperationLogMessage> m)
                       {
                       }},
            message);

        MainWindow::logMessage(message);
    }

public:
private:
    void readDisk()
    {
        if (isWorkerThreadRunning())
            return;

        collectConfig();
        ::emergencyStop = false;
        setState(STATE_READING);
        _fluxVisualiserWidget->resetView();

        runThen(
            [this]()
            {
                auto fluxSource = FluxSource::create(globalConfig());
                auto decoder = Arch::createDecoder(globalConfig());
                auto diskflux = readDiskCommand(*fluxSource, *decoder);
            },
            [this]()
            {
                setState(STATE_IDLE);
            });
    }
    W_SLOT(readDisk)

private:
    void updateState()
    {
        setState(_state);
    }

    void setState(int state)
    {
        settingsCanBeChanged(state == STATE_IDLE);
        commandButtonFrame->setEnabled(state == STATE_IDLE);

        _stopWidget->setEnabled(state != STATE_IDLE);
        _progressWidget->setEnabled(state != STATE_IDLE);

        saveImageButton->setEnabled((bool)_currentDisk);
        writeDiskButton->setEnabled((bool)_currentDisk);

        _state = state;
    }
    W_SLOT(setState)

private:
    std::shared_ptr<const DiskFlux> _currentDisk;
    FluxVisualiserWidget* _fluxVisualiserWidget;
    ImageVisualiserWidget* _imageVisualiserWidget;
    int _state;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
