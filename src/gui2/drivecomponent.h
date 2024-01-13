#pragma once

class MainWindow;

class DriveComponent
{
public:
    virtual void setDriveConfigurationPane(QWidget* active) = 0;

public:
    static std::unique_ptr<DriveComponent> create(MainWindow* mainWindow);
};
