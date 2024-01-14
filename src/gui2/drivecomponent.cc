#include "lib/globals.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "drivecomponent.h"
#include "mainwindow.h"
#include "driveConfigurationForm.h"
#include "fluxConfigurationForm.h"
#include <QStandardItemModel>

static const char* DRIVE = "drive/";
static const char* SELECTED_DRIVE = "drive/drive";

class DriveComponentImpl : public DriveComponent, public QObject
{
    W_OBJECT(DriveComponentImpl)

public:
    class ConfigurationForm : public QStandardItem, public QObject
    {
        W_OBJECT(ConfigurationForm)

    public:
        ConfigurationForm(
            DriveComponentImpl* dci, QIcon icon, const std::string text):
            QStandardItem(icon, QString::fromStdString(text)),
            _dci(dci)
        {
            _widget = new QWidget();
        }

    public:
        virtual std::string id() const = 0;

        virtual void updateSavedState() const = 0;
        W_SLOT(updateSavedState)

    public:
        QWidget* widget() const
        {
            return _widget;
        }

    protected:
        QString qid() const
        {
            return DRIVE + QString::fromStdString(id());
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

            connect(filenameEdit,
                &QLineEdit::editingFinished,
                this,
                &ConfigurationForm::updateSavedState);
        }

    public:
        std::string id() const override
        {
            return "flux";
        }

        void updateSavedState() const override
        {
            app->setValue(qid() + "/filename", filenameEdit->text());
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

            connect(portLineEdit,
                &QLineEdit::editingFinished,
                this,
                &ConfigurationForm::updateSavedState);
            connect(driveComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(driveTypeComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(highDensityToggle,
                &QCheckBox::stateChanged,
                this,
                &ConfigurationForm::updateSavedState);
        }

        void updateSavedState() const override
        {
            app->setValue(
                qid() + "/highDensity", highDensityToggle->isChecked());
            app->setValue(qid() + "/drive", driveComboBox->currentIndex());
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

    public:
        std::string id() const override
        {
            return "greaseweazle/manual";
        }

        void updateSavedState() const override
        {
            app->setValue(qid() + "/port", portLineEdit->text());
            DriveConfigurationForm::updateSavedState();
        }
    };

    class DetectedDriveConfigurationForm : public DriveConfigurationForm
    {
    public:
        DetectedDriveConfigurationForm(DriveComponentImpl* dci,
            const std::string& type,
            const std::string& id,
            std::shared_ptr<CandidateDevice>& device):
            DriveConfigurationForm(dci,
                QIcon(":/ui/extras/hardware.png"),
                fmt::format("{}\n{}", type, id)),
            _type(type),
            _id(id),
            _device(device)
        {
            portLineEdit->setEnabled(false);
            portLineEdit->setText(QString::fromStdString(_device->serialPort));
        }

    public:
        std::string id() const override
        {
            return fmt::format("{}:{}", _type, _id);
        }

    private:
        std::shared_ptr<CandidateDevice>& _device;
        std::string _type;
        std::string _id;
    };

public:
    DriveComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        setParent(mainWindow);
        _devicesModel.setColumnCount(1);
        _mainWindow->deviceSelectionComboBox->setModel(&_devicesModel);

        addForm(new FluxConfigurationForm(this));
        addForm(new ManualDriveConfigurationForm(this));

        auto devices = runOnWorkerThread(findUsbDevices).result();
        for (auto& it : devices)
            addForm(new DetectedDriveConfigurationForm(
                this, getDeviceName(it->type), it->serial, it));

        auto currentFormId =
            app->value(SELECTED_DRIVE).toString().toStdString();
        int currentFormIndex = findFormById(currentFormId, 0);
        _mainWindow->deviceSelectionComboBox->setCurrentIndex(currentFormIndex);
        changeSelectedDevice(currentFormIndex);

        _mainWindow->connect(_mainWindow->deviceSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &DriveComponentImpl::changeSelectedDevice);
        _mainWindow->connect(_mainWindow->deviceSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &DriveComponentImpl::updateSavedState);

        container()->updateGeometry();
    }

private:
    void addForm(ConfigurationForm* form)
    {
        _forms.append(form);
        _devicesModel.appendRow(form);

        container()->layout()->addWidget(form->widget());
        form->widget()->hide();
    }

    int findFormById(const std::string& id, int def)
    {
        for (int i = 0; i < _forms.size(); i++)
            if (_forms[i]->id() == id)
                return i;
        return def;
    }

public:
    void changeSelectedDevice(int index)
    {
        for (int i = 0; i < _forms.size(); i++)
            _forms[i]->widget()->setVisible(i == index);
    }
    W_SLOT(changeSelectedDevice)

    void updateSavedState()
    {
        int selectedForm = _mainWindow->deviceSelectionComboBox->currentIndex();
        ConfigurationForm* form = _forms[selectedForm];

        app->setValue(SELECTED_DRIVE, QString::fromStdString(form->id()));
    }
    W_SLOT(updateSavedState)

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
W_OBJECT_IMPL(DriveComponentImpl::ConfigurationForm)

DriveComponent* DriveComponent::create(MainWindow* mainWindow)
{
    return new DriveComponentImpl(mainWindow);
}
