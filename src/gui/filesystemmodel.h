#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <wx/dataview.h>
class Dirent;

class DirentContainer : public wxClientData
{
public:
    DirentContainer(std::shared_ptr<Dirent> dirent): dirent(dirent) {}

    bool populated = false;
    bool populating = false;
    std::shared_ptr<Dirent> dirent;
};

class FilesystemModel : public wxDataViewTreeStore
{
public:
    virtual void Clear() = 0;
    virtual wxDataViewItem GetRootItem() const = 0;
    virtual void SetFiles(const wxDataViewItem& item,
        std::vector<std::shared_ptr<Dirent>>& files) = 0;

public:
    static FilesystemModel* Associate(wxDataViewCtrl* control);
};

#endif
