#pragma once

class MainWindow;

class DriveComponent
{
public:
    static DriveComponent* create(MainWindow* mainWindow);

public:
    virtual void collectConfig() = 0;
};
