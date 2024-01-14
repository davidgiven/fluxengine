#include "lib/globals.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include "formatcomponent.h"

class MainWindowImpl : public MainWindow
{
public:
    MainWindowImpl()
    {
        setupUi(this);
        _driveComponent = DriveComponent::create(this);
        _formatComponent = FormatComponent::create(this);

        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

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

private:
    DriveComponent* _driveComponent;
    FormatComponent* _formatComponent;
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
