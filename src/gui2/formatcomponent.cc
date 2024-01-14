#include "lib/globals.h"
#include "lib/proto.h"
#include "globals.h"
#include "formatcomponent.h"
#include "mainwindow.h"
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <range/v3/all.hpp>

static const char* FORMAT = "format/format";
static const char* CUSTOM_FORMAT_FILENAME = "format/customFormatFilename";
static const char* USE_CUSTOM_FORMAT = "format/useCustomFormat";
static const char* FORMAT_OPTIONS_PREFIX = "format/options";

class FormatComponentImpl : public FormatComponent, public QObject
{
    W_OBJECT(FormatComponentImpl)

private:
    class OptionCheckBox : public QCheckBox
    {
    public:
        OptionCheckBox(QString label, const OptionProto* option):
            QCheckBox(label),
            option(option)
        {
        }

        const OptionProto* option;
    };

public:
    FormatComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        setParent(mainWindow);

        /* Configure the formats drop-down list. */

        std::string defaultFormat =
            app->value(FORMAT, QVariant("ibm")).toString().toStdString();

        _formatsModel.clear();
        _formatsModel.setColumnCount(2);
        _formatsModel.setHorizontalHeaderLabels(
            QStringList{"Name", "Description"});
        int selectedRow = 0;
        for (const auto& it : formats)
        {
            if (it.second->is_extension())
                continue;

            if (it.first == defaultFormat)
                selectedRow = _formatsModel.rowCount();

            QStandardItem* nameItem =
                new QStandardItem(QString::fromStdString(it.first));
            nameItem->setData(QVariant(QString::fromStdString(it.first)));

            QList<QStandardItem*> row{nameItem,
                new QStandardItem(
                    QString::fromStdString(it.second->shortname()))};
            _formatsModel.appendRow(row);
        }
        _mainWindow->formatSelectionComboBox->setModel(&_formatsModel);
        _mainWindow->formatSelectionComboBox->setModelColumn(0);

        QTableView* view = new QTableView;
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->hide();
        _mainWindow->formatSelectionComboBox->setView(view);

        /* Update from defaults */

        _mainWindow->formatSelectionComboBox->setCurrentIndex(selectedRow);
        changeSelectedFormat(selectedRow);

        if (app->value(USE_CUSTOM_FORMAT).toBool())
            _mainWindow->customFormatRadioButton->click();
        else
            _mainWindow->builtInFormatRadioButton->click();

        _mainWindow->customFormatFilenameEdit->setText(
            app->value(CUSTOM_FORMAT_FILENAME).toString());

        /* GUI hookup */

        _mainWindow->connect(_mainWindow->formatButtonGroup,
            &QButtonGroup::idClicked,
            this,
            &FormatComponentImpl::updateSavedState);
        _mainWindow->connect(_mainWindow->customFormatFilenameEdit,
            &QLineEdit::editingFinished,
            this,
            &FormatComponentImpl::updateSavedState);
        _mainWindow->connect(_mainWindow->formatSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &FormatComponentImpl::changeSelectedFormat);
        _mainWindow->connect(_mainWindow->formatSelectionComboBox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &FormatComponentImpl::updateSavedState);
    }

public:
    void changeSelectedFormat(int index)
    {
        auto item = _formatsModel.item(index);
        QString formatId = item->data().toString();
        const ConfigProto* config = formats.at(formatId.toStdString());

        auto* layout = _mainWindow->formatOptionsContainerLayout;
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget* w = layout->itemAt(i)->widget();
            if (w)
                w->deleteLater();
        }

        QSet<QString> settings =
            app->value(QString(FORMAT_OPTIONS_PREFIX) + "/" + formatId)
                .value<QStringList>() |
            ranges::to<QSet>();

        for (auto& group : config->option_group())
        {
            std::string groupName = group.comment();
            if (groupName == "$formats")
                groupName = "Format variant";
            groupName += ":";

            QComboBox* qb = new QComboBox();
            int selectedItem = 0;
            for (auto& option : group.option())
            {
                QString name = QString::fromStdString(option.name());
                if (settings.contains(name))
                    selectedItem = qb->count();
                qb->addItem(QString::fromStdString(option.comment()), name);
            }
            qb->setCurrentIndex(selectedItem);

            layout->addWidget(new QLabel(QString::fromStdString(groupName)));
            layout->addWidget(qb);

            _mainWindow->connect(qb,
                QOverload<int>::of(&QComboBox::activated),
                this,
                &FormatComponentImpl::updateSavedState);
        }

        for (auto& option : config->option())
        {
            QCheckBox* rb = new OptionCheckBox(
                QString::fromStdString(option.comment()), &option);
            rb->setCheckState(
                settings.contains(QString::fromStdString(option.name()))
                    ? Qt::Checked
                    : Qt::Unchecked);
            layout->addWidget(rb);

            _mainWindow->connect(rb,
                &QCheckBox::stateChanged,
                this,
                &FormatComponentImpl::updateSavedState);
        }

        if (layout->count() == 0)
            layout->addWidget(new QLabel("No options for this format!"));
    }
    W_SLOT(changeSelectedFormat)

    void updateSavedState()
    {
        app->setValue(USE_CUSTOM_FORMAT,
            _mainWindow->customFormatRadioButton->isChecked());
        app->setValue(CUSTOM_FORMAT_FILENAME,
            _mainWindow->customFormatFilenameEdit->text());

        int index = _mainWindow->formatSelectionComboBox->currentIndex();
        auto item = _formatsModel.item(index);
        QString formatId = item->data().toString();
        const ConfigProto* config = formats.at(formatId.toStdString());
        app->setValue(FORMAT, formatId);

        auto* layout = _mainWindow->formatOptionsContainerLayout;
        QStringList settings;
        for (int i = 0; i < layout->count(); i++)
        {
            QWidget* w = layout->itemAt(i)->widget();
            if (QComboBox* cb = dynamic_cast<QComboBox*>(w))
                settings.append(cb->currentData().toString());
            else if (OptionCheckBox* cb = dynamic_cast<OptionCheckBox*>(w))
            {
                if (cb->isChecked())
                    settings.append(QString::fromStdString(cb->option->name()));
            }
        }

        app->setValue(QString(FORMAT_OPTIONS_PREFIX) + "/" + formatId,
            QVariant(settings));
    }
    W_SLOT(updateSavedState)

private:
    MainWindow* _mainWindow;
    QStandardItemModel _formatsModel;
};
W_OBJECT_IMPL(FormatComponentImpl)

FormatComponent* FormatComponent::create(MainWindow* mainWindow)
{
    return new FormatComponentImpl(mainWindow);
}
