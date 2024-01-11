#include <QApplication>
#include <QPushButton>
#include "userinterface.h"

class UserInterface : public Ui_MainWindow {};

int main(int argc, char **argv)
{
 QApplication app (argc, argv);
  Q_INIT_RESOURCE(resources);
  QMainWindow mainWindow;
  UserInterface ui;
  ui.setupUi(&mainWindow);
  mainWindow.setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  mainWindow.setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
  mainWindow.show();

 return app.exec();
}