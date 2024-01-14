#pragma once

class MainWindow;

class DriveComponent
{
public:
    static std::unique_ptr<DriveComponent> create(MainWindow* mainWindow);
};
