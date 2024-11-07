#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <wx/dataview.h>
class Dirent;
class Path;

class FilesystemNode
{
public:
    FilesystemNode(std::shared_ptr<Dirent> dirent);

    wxDataViewItem item;
    bool populated = false;
    bool populating = false;
    bool stub = false;
    std::shared_ptr<Dirent> dirent;
    std::map<std::string, std::shared_ptr<FilesystemNode>> children;

    std::string newname; /* used for inline renames */
};

class FilesystemModel : public wxDataViewModel
{
public:
    virtual void Clear(const Path& path) = 0;
    virtual std::shared_ptr<FilesystemNode> Find(const Path& path) const = 0;
    virtual std::shared_ptr<FilesystemNode> Find(
        const wxDataViewItem& item) const = 0;
    virtual void Delete(const Path& path) = 0;
    virtual void RemoveStub(const Path& path) = 0;
    virtual void Add(std::shared_ptr<Dirent> dirent) = 0;

public:
    static FilesystemModel* Associate(wxDataViewCtrl* control);
};

#endif
