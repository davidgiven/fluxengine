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

private:
    std::unique_ptr<DriveComponent> _driveComponent;
    std::unique_ptr<FormatComponent> _formatComponent;
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
