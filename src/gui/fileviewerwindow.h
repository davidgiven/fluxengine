#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

#include "lib/layout.h"

class FileViewerWindow : public FileViewerWindowGen
{
public:
    FileViewerWindow(
        wxWindow* parent, const std::string& title, const Bytes& data);

private:
    void OnClose(wxCloseEvent& event);
};

#endif
