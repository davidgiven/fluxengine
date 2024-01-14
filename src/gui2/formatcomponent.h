#pragma once

class MainWindow;

class FormatComponent
{
public:
    static std::unique_ptr<FormatComponent> create(MainWindow* mainWindow);
};
