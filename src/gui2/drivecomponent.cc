#include "lib/globals.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "drivecomponent.h"
#include "mainwindow.h"
#include "driveConfigurationForm.h"
#include "fluxConfigurationForm.h"
#include <QStandardItemModel>

class DriveComponentImpl : public DriveComponent, public QObject
{
    W_OBJECT(DriveComponentImpl)

public:
    class ConfigurationForm : public QStandardItem
    {
    public:
        ConfigurationForm(
            DriveComponentImpl* dci, QIcon icon, const std::string text):
            QStandardItem(icon, QString::fromStdString(text)),
            _dci(dci)
        {
            _widget = new QWidget();
        }

    public:
        QWidget* widget() const
        {
            return _widget;
        }

    protected:
        DriveComponentImpl* _dci;
        QWidget* _widget;
    };

    class FluxConfigurationForm :
        public ConfigurationForm,
        public Ui_fluxConfigurationForm
    {
    public:
        FluxConfigurationForm(DriveComponentImpl* dci):
            ConfigurationForm(
                dci, QIcon(":/ui/extras/fluxfile.png"), "Flux file")
        {
            setupUi(_widget);
        }
    };

    class DriveConfigurationForm :
        public ConfigurationForm,
        public Ui_driveConfigurationForm
    {
    public:
        DriveConfigurationForm(
            DriveComponentImpl* dci, QIcon icon, const std::string& label):
            ConfigurationForm(dci, icon, label)
        {
            setupUi(_widget);
        }
    };

    class ManualDriveConfigurationForm : public DriveConfigurationForm
    {
    public:
        ManualDriveConfigurationForm(DriveComponentImpl* dci):
            DriveConfigurationForm(dci,
                QIcon(":/ui/extras/hardware.png"),
                "Greaseweazle\n(configured manually)")
        {
        }
    };

    class DetectedDriveConfigurationForm : public DriveConfigurationForm
    {
    public:
        DetectedDriveConfigurationForm(DriveComponentImpl* dci,
            const std::string& type,
            const std::string& id, std::shared_ptr<CandidateDevice>& device):
            DriveConfigurationForm(dci,
                QIcon(":/ui/extras/hardware.png"),
                fmt::format("{}\n{}", type, id)),
                _device(device)
        {
            portLineEdit->setEnabled(false);
            portLineEdit->setText(QString::fromStdString(_device->serialPort));
        }

    private:
        std::shared_ptr<CandidateDevice>& _device;
    };

public:
    DriveComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        _mainWindow->connect(_mainWindow->deviceSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &DriveComponentImpl::onDeviceIndexChanged);

        _devicesModel.setColumnCount(1);
        _mainWindow->deviceSelectionComboBox->setModel(&_devicesModel);

        addForm(new FluxConfigurationForm(this));
        addForm(new ManualDriveConfigurationForm(this));

        auto devices = runOnWorkerThread(findUsbDevices).result();
        for (auto& it : devices)
            addForm(new DetectedDriveConfigurationForm(
                this, getDeviceName(it->type), it->serial, it));

        onDeviceIndexChanged(0);
    }

private:
    void addForm(ConfigurationForm* form)
    {
        _forms.append(form);
        _devicesModel.appendRow(form);

        container()->layout()->addWidget(form->widget());
        form->widget()->hide();
    }

public:
    void onDeviceIndexChanged(int index)
    {
        for (int i = 0; i < _forms.size(); i++)
            _forms[i]->widget()->setVisible(i == index);
    }
    W_SLOT(onDeviceIndexChanged)

public:
    QWidget* container() const
    {
        return _mainWindow->driveConfigurationContainer;
    }

private:
    MainWindow* _mainWindow;
    QStandardItemModel _devicesModel;
    QList<ConfigurationForm*> _forms;
};

W_OBJECT_IMPL(DriveComponentImpl)

std::unique_ptr<DriveComponent> DriveComponent::create(MainWindow* mainWindow)
{
    return std::make_unique<DriveComponentImpl>(mainWindow);
}
