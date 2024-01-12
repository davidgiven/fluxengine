#pragma once

#include "globals.h"

class MainWindow : public QMainWindow, public Ui_MainWindow
{
public:
    static std::unique_ptr<MainWindow> create();
};
