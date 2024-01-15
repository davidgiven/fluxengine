#include "lib/globals.h"
#include "lib/config.h"
#include "lib/readerwriter.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"

class MainWindowImpl : public MainWindow
{
    W_OBJECT(MainWindowImpl)

public:
    MainWindowImpl()
    {
        setupUi(this);
        _driveComponent = DriveComponent::create(this);
        _formatComponent = FormatComponent::create(this);

        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        connect(readDiskButton,
            &QAbstractButton::clicked,
            this,
            &MainWindowImpl::readDisk);

        connect(revolutionsSlider,
            &QSlider::valueChanged,
            revolutionsSpinBox,
            &QSpinBox::setValue);
        connect(revolutionsSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            revolutionsSlider,
            &QSlider::setValue);
    }

public:
    void logMessage(std::shared_ptr<const AnyLogMessage> message) override
    {
        logViewerEdit->appendPlainText(
            QString::fromStdString(Logger::toString(*message)));
        logViewerEdit->ensureCursorVisible();
    }

    void collectConfig() override
    {
        try
        {
            globalConfig().clear();
            _formatComponent->collectConfig();
            _driveComponent->collectConfig();
        }
        catch (const ErrorException& e)
        {
            log("Fatal error: {}", e.message);
        }
        catch (...)
        {
            std::exception_ptr p = std::current_exception();
            std::clog << (p ? p.__cxa_exception_type()->name() : "null")
                      << std::endl;
        }
    }

private:
    void readDisk()
    {
        if (isWorkerThreadRunning())
            return;

        collectConfig();
        safeRunOnWorkerThread(
            [this]()
            {
                auto& fluxSource = globalConfig().getFluxSource();
                auto& decoder = globalConfig().getDecoder();
                auto diskflux = readDiskCommand(*fluxSource, *decoder);
            });
    }
    W_SLOT(readDisk)

private:
    DriveComponent* _driveComponent;
    FormatComponent* _formatComponent;
};
W_OBJECT_IMPL(MainWindowImpl)

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
