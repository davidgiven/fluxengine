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
    }

public:
    void setDriveConfigurationPane(QWidget* active)
    {
        for (auto* w : driveConfigurationContainer->findChildren<QWidget*>())
            w->setVisible(w == active);
    }
};

std::unique_ptr<MainWindow> MainWindow::create()
{
    return std::make_unique<MainWindowImpl>();
}
