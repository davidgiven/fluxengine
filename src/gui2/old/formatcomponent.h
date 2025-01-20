#pragma once

class MainWindow;

class FormatComponent
{
public:
    static FormatComponent* create(MainWindow* mainWindow);

public:
    virtual void collectConfig() = 0;
};
