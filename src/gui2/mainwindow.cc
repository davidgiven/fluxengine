#include "lib/globals.h"
#include "lib/proto.h"
#include "mainwindow.h"
#include "drivecomponent.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>

class MainWindowImpl : public MainWindow
{
public:
    MainWindowImpl()
    {
        setupUi(this);
        _driveComponent = DriveComponent::create(this);

        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        initialiseFormats();

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

private:
    QStandardItemModel _formatsModel;
    std::unique_ptr<DriveComponent> _driveComponent;
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
