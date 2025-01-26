#include "lib/core/globals.h"
#include "lib/usb/usbfinder.h"
#include "lib/config/config.h"
#include "globals.h"
#include "drivecomponent.h"
#include "mainwindow.h"
#include <QStandardItemModel>
#include <range/v3/all.hpp>

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
        ConfigurationForm(QWidget* widget,
            DriveComponentImpl* dci,
            QIcon icon,
            const std::string text):
            QStandardItem(icon, QString::fromStdString(text)),
            _widget(widget),
            _dci(dci),
            _mw(dci->_mainWindow)
        {
        }

    public:
        virtual std::string id() const = 0;
        virtual void collectConfig() const = 0;

        virtual void loadSavedState() = 0;
        virtual void connectAll() = 0;
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
        QWidget* _widget;
        DriveComponentImpl* _dci;
        MainWindow* _mw;
    };

    class FluxConfigurationForm : public ConfigurationForm
    {
    public:
        FluxConfigurationForm(DriveComponentImpl* dci):
            ConfigurationForm(dci->_mainWindow->fluxDevicePage,
                dci,
                QIcon(":/ui/extras/fluxfile.png"),
                "Flux file")
        {
        }

    public:
        std::string id() const override
        {
            return "flux";
        }

        void collectConfig() const override
        {
            auto fluxFile = _mw->filenameEdit->text().toStdString();

            const FluxConstructor* fc = &Config::getFluxFormats()[0];
            if (fc->sink)
                fc->sink(
                    fluxFile, globalConfig().overrides()->mutable_flux_sink());
            if (fc->source)
                fc->source(fluxFile,
                    globalConfig().overrides()->mutable_flux_source());

            QString rpmOverride =
                _mw->rpmOverrideComboBox->currentData().toString();
            if (rpmOverride == "300")
                globalConfig()
                    .overrides()
                    ->mutable_drive()
                    ->set_rotational_period_ms(200);
            else if (rpmOverride == "360")
                globalConfig()
                    .overrides()
                    ->mutable_drive()
                    ->set_rotational_period_ms(166);
            else if (rpmOverride == "override")
            {
                float value = _mw->rpmOverrideValue->value();
                globalConfig()
                    .overrides()
                    ->mutable_drive()
                    ->set_rotational_period_ms(60e3 / value);
            }

            QString driveTypeOverride =
                _mw->driveTypeOverrideComboBox->currentData().toString();
            if (driveTypeOverride != "default")
                globalConfig().overrides()->MergeFrom(
                    *drivetypes.at(driveTypeOverride.toStdString()));
        }

        void loadSavedState() override
        {
            _mw->filenameEdit->setText(
                app->value(qid() + "/filename").toString());

            _mw->rpmOverrideComboBox->addItem(
                "Use value in file", QVariant("default"));
            _mw->rpmOverrideComboBox->addItem(
                "300 rpm / 200 ms", QVariant("300"));
            _mw->rpmOverrideComboBox->addItem(
                "360 rpm / 166 ms", QVariant("360"));
            _mw->rpmOverrideComboBox->addItem(
                "Custom value", QVariant("custom"));
            QString rpmType = app->value(qid() + "/rpmType").toString();
            setByString(_mw->rpmOverrideComboBox, rpmType);
            _mw->rpmOverrideValue->setEnabled(rpmType == "custom");

            _mw->driveTypeOverrideComboBox->addItem(
                "Use value in file", QVariant("default"));
            for (auto& d : drivetypes)
            {
                _mw->driveTypeOverrideComboBox->addItem(
                    QString::fromStdString(d.second->comment()),
                    QVariant(QString::fromStdString(d.first)));
            }
            setByString(_mw->driveTypeOverrideComboBox,
                app->value(qid() + "/driveType").toString());

            _mw->rpmOverrideValue->setValue(
                app->value(qid() + "/customRpm").toInt());
        }

        void connectAll() override
        {
            connect(_mw->filenameEdit,
                &QLineEdit::editingFinished,
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->driveTypeOverrideComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->rpmOverrideComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->rpmOverrideValue,
                QOverload<int>::of(&QSpinBox::valueChanged),
                this,
                &ConfigurationForm::updateSavedState);

            connect(_mw->openButton,
                &QPushButton::clicked,
                [this]()
                {
                    std::string formats = Config::getFluxFormats() |
                                          ranges::views::transform(
                                              [](const FluxConstructor& f)
                                              {
                                                  return f.glob;
                                              }) |
                                          ranges::views::filter(
                                              [](const std::string& s)
                                              {
                                                  return !s.empty();
                                              }) |
                                          ranges::views::intersperse(" ") |
                                          ranges::views::join |
                                          ranges::to<std::string>();
                    QFileDialog dialogue(_mw);
                    dialogue.setFileMode(QFileDialog::ExistingFile);
                    dialogue.setNameFilter(
                        QString::fromStdString("Flux files (" + formats + ")"));

                    QStringList fileNames;
                    if (dialogue.exec())
                        _mw->filenameEdit->setText(
                            dialogue.selectedFiles().first());
                });
        }

        void updateSavedState() const override
        {
            app->setValue(qid() + "/filename", _mw->filenameEdit->text());

            QString rpmType =
                _mw->rpmOverrideComboBox->currentData().toString();
            app->setValue(qid() + "/rpmType", rpmType);
            _mw->rpmOverrideValue->setEnabled(rpmType == "custom");

            app->setValue(qid() + "/customRpm", _mw->rpmOverrideValue->value());

            app->setValue(qid() + "/driveType",
                _mw->driveTypeOverrideComboBox->currentData().toString());
        }
    };

    class DriveConfigurationForm : public ConfigurationForm
    {
    public:
        DriveConfigurationForm(
            DriveComponentImpl* dci, QIcon icon, const std::string& label):
            ConfigurationForm(
                dci->_mainWindow->hardwareDevicePage, dci, icon, label)
        {
        }

        void collectConfig() const override
        {
            globalConfig().overrides()->mutable_drive()->set_high_density(
                _mw->highDensityToggle->isChecked());

            auto driveTypeId =
                _mw->driveTypeComboBox->currentData().toString().toStdString();
            globalConfig().overrides()->MergeFrom(*drivetypes.at(driveTypeId));

            auto filename =
                fmt::format("drive:{}", _mw->driveComboBox->currentIndex());
            globalConfig().setFluxSink(filename);
            globalConfig().setFluxSource(filename);
            globalConfig().setVerificationFluxSource(filename);
        }

        void loadSavedState() override
        {
            _mw->driveComboBox->setCurrentIndex(
                app->value(qid() + "/drive").toInt());

            QString driveType = app->value(qid() + "/driveType").toString();
            for (auto& d : drivetypes)
            {
                _mw->driveTypeComboBox->addItem(
                    QString::fromStdString(d.second->comment()),
                    QVariant(QString::fromStdString(d.first)));
                if (QString::fromStdString(d.first) == driveType)
                    _mw->driveTypeComboBox->setCurrentIndex(
                        _mw->driveTypeComboBox->count() - 1);
            }

            _mw->highDensityToggle->setCheckState(
                app->value(qid() + "/highDensity").toBool() ? Qt::Checked
                                                            : Qt::Unchecked);
        }

        void connectAll() override
        {
            connect(_mw->portLineEdit,
                &QLineEdit::editingFinished,
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->driveComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->driveTypeComboBox,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &ConfigurationForm::updateSavedState);
            connect(_mw->highDensityToggle,
                &QCheckBox::stateChanged,
                this,
                &ConfigurationForm::updateSavedState);
        }

        void updateSavedState() const override
        {
            app->setValue(qid() + "/drive", _mw->driveComboBox->currentIndex());

            app->setValue(qid() + "/driveType",
                _mw->driveTypeComboBox->currentData().toString());

            app->setValue(
                qid() + "/highDensity", _mw->highDensityToggle->isChecked());
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
            return "device/manual";
        }

        void collectConfig() const override
        {
            DriveConfigurationForm::collectConfig();
            globalConfig()
                .overrides()
                ->mutable_usb()
                ->mutable_greaseweazle()
                ->set_port(_mw->portLineEdit->text().toStdString());
        }

        void loadSavedState() override
        {
            DriveConfigurationForm::loadSavedState();
            _mw->portLineEdit->setText(app->value(qid() + "/port").toString());
        }

        void updateSavedState() const override
        {
            app->setValue(qid() + "/port", _mw->portLineEdit->text());
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
            _mw->portLineEdit->setEnabled(false);
        }

        void loadSavedState() override
        {
            _mw->portLineEdit->setText(
                QString::fromStdString(_device->serialPort));
        }

    public:
        std::string id() const override
        {
            return "device/" + _id;
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

        _mainWindow->updateGeometry();
    }

private:
    void addForm(ConfigurationForm* form)
    {
        form->loadSavedState();
        form->connectAll();

        _forms.append(form);
        _devicesModel.appendRow(form);
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
        _mainWindow->driveStackedWidget->setCurrentIndex(index);
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
    void collectConfig() override
    {
        int selectedForm = _mainWindow->deviceSelectionComboBox->currentIndex();
        ConfigurationForm* form = _forms.at(selectedForm);
        form->collectConfig();
    }

private:
    static void setByString(QComboBox* combobox, QString value)
    {
        int i = combobox->findData(QVariant(value));
        if (i == -1)
            i = 0;
        combobox->setCurrentIndex(i);
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
