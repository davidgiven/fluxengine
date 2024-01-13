#include "lib/globals.h"
#include "lib/proto.h"
#include "lib/usb/usbfinder.h"
#include "mainwindow.h"

class MainWindowImpl : public MainWindow
{
public:
    MainWindowImpl()
    {
        setupUi(this);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        setDriveConfigurationPane(noDriveConfigurationWidget);
        initialiseFormats();
        initialiseDevices();

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
    void setDriveConfigurationPane(QWidget* active)
    {
        for (auto* w : driveConfigurationContainer->findChildren<QWidget*>())
            w->setVisible(w == active);
    }

private:
    void initialiseFormats()
    {
        for (const auto& it : formats)
        {
            if (it.second->is_extension())
                continue;

            formatsList->addItem(QString::fromStdString(it.first));
        }
    }

    void initialiseDevices()
    {
        auto devices = runOnWorkerThread(findUsbDevices).result();

        for (const auto& it : devices) {}
        fmt::print("device count = {}\n", devices.size());
    }
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
