#include "lib/globals.h"
#include "lib/proto.h"
#include "lib/usb/usbfinder.h"
#include "mainwindow.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>

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
        _formatsModel.clear();
        _formatsModel.setColumnCount(2);
        _formatsModel.setHorizontalHeaderLabels(
            QStringList{"Name", "Description"});
        for (const auto& it : formats)
        {
            if (it.second->is_extension())
                continue;

            QStandardItem* nameItem =
                new QStandardItem(QString::fromStdString(it.first));
            nameItem->setData(QVariant(it.second));

            QList<QStandardItem*> row{nameItem,
                new QStandardItem(
                    QString::fromStdString(it.second->shortname()))};
            _formatsModel.appendRow(row);
        }
        formatsList->setModel(&_formatsModel);
        formatsList->setModelColumn(0);

        QTableView* view = new QTableView;
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->hide();
        formatsList->setView(view);
    }

    void initialiseDevices()
    {
        auto devices = runOnWorkerThread(findUsbDevices).result();

        for (const auto& it : devices) {}
        fmt::print("device count = {}\n", devices.size());
    }

private:
    QStandardItemModel _formatsModel;
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
