#pragma once

#include "userinterface.h"

class UserInterface : public Ui_MainWindow
{
};

class Application : public QApplication
{
public:
    Application(int& argc, char** argv): QApplication(argc, argv) {}
    virtual ~Application() {}

public:
};

extern std::unique_ptr<Application> app;
