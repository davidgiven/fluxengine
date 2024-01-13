#include "lib/globals.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "drivecomponent.h"
#include "mainwindow.h"
#include "driveConfigurationForm.h"
#include "fluxConfigurationForm.h"
#include <QStandardItemModel>

class DriveComponentImpl : public DriveComponent
{
private:
    class ConfigurationForm : public QStandardItem
    {
    public:
        ConfigurationForm(QIcon icon, const std::string text):
            QStandardItem(icon, QString::fromStdString(text))
        {
        }
    };

    class FluxConfigurationForm :
        public ConfigurationForm,
        public Ui_fluxConfigurationForm
    {
    public:
        FluxConfigurationForm():
            ConfigurationForm(QIcon(":/ui/extras/fluxfile.png"), "Flux file")
        {
        }
    };

    class DetectedDriveConfigurationForm :
        public ConfigurationForm,
        public Ui_driveConfigurationForm
    {
    public:
        DetectedDriveConfigurationForm(
            const std::string& type, const std::string& id):
            ConfigurationForm(QIcon(":/ui/extras/hardware.png"),
                fmt::format("{}\n{}", type, id))
        {
        }
    };

public:
    DriveComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        _mainWindow->connect(_mainWindow->deviceSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            [this](int index)
            {
                onDeviceIndexChanged(index);
            });

        _devicesModel.setColumnCount(1);
        _mainWindow->deviceSelectionComboBox->setModel(&_devicesModel);

        _devicesModel.appendRow(new FluxConfigurationForm());
        _devicesModel.appendRow(
            new QStandardItem(QIcon(":/ui/extras/hardware.png"),
                "Greaseweazle\n(manually configured)"));

        auto devices = runOnWorkerThread(findUsbDevices).result();
        for (const auto& it : devices)
        {
            _devicesModel.appendRow(new DetectedDriveConfigurationForm(
                getDeviceName(it->type), it->serial));
        }
        fmt::print("device count = {}\n", devices.size());

        onDeviceIndexChanged(0);
    }

private:
    void onDeviceIndexChanged(int index)
    {
        fmt::print("value changed {}\n", index);
    }

public:
    void setDriveConfigurationPane(QWidget* active) override
    {
        for (auto* w :
            _mainWindow->driveConfigurationContainer->findChildren<QWidget*>())
            w->setVisible(w == active);
    }

private:
    MainWindow* _mainWindow;
    QStandardItemModel _devicesModel;
};

std::unique_ptr<DriveComponent> DriveComponent::create(MainWindow* mainWindow)
{
    return std::make_unique<DriveComponentImpl>(mainWindow);
}
