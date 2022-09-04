#include "lib/globals.h"
#include "gui.h"
#include "filesystemmodel.h"
#include "lib/vfs/vfs.h"
#include <wx/artprov.h>
#include <fmt/format.h>

class FilesystemModelImpl : public FilesystemModel
{
public:
    FilesystemModelImpl():
        _fileIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_BUTTON)),
        _folderOpenIcon(
            wxArtProvider::GetIcon(wxART_FOLDER_OPEN, wxART_BUTTON)),
        _folderClosedIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_BUTTON))
    {
    }

public:
    unsigned int GetColumnCount() const override
    {
        return 3;
    }

    wxString GetColumnType(unsigned int column) const override
    {
        switch (column)
        {
            case 1:
            case 2:
                return "string";

            default:
                return FilesystemModel::GetColumnType(column);
        }
    }

    void GetValue(wxVariant& value,
        const wxDataViewItem& item,
        unsigned int column) const override
    {
        auto* data = (DirentContainer*)GetItemData(item);
        switch (column)
        {
            case 1:
                value = (data && data->dirent)
                            ? std::to_string(data->dirent->length)
                            : "";
                break;

            case 2:
                value = (data && data->dirent) ? data->dirent->mode : "";
                break;

            default:
                FilesystemModel::GetValue(value, item, column);
        }
    }

#if 0
    int Compare(const wxDataViewItem& item1,
        const wxDataViewItem& item2,
        unsigned int column,
        bool ascending) const override
    {
        auto* data1 = (DirentContainer*)GetItemData(item1);
        auto* data2 = (DirentContainer*)GetItemData(item2);
        if (data1 && data1->dirent && data2 && data2->dirent)
        {
            int r;

            switch (column)
            {
                case 0:
                    r = data1->dirent->filename.compare(
                        data2->dirent->filename);
                    break;

                case 1:
                    r = data2->dirent->length - data1->dirent->length;
                    break;

                case 2:
                    r = data1->dirent->mode.compare(data2->dirent->mode);
                    break;
            }

            if (ascending)
                r = -r;
            return r;
        }

        return 0;
    }
#endif

public:
    wxDataViewItem GetRootItem() const
    {
        return wxDataViewItem();
    }

    void SetFiles(const wxDataViewItem& parent,
        std::vector<std::shared_ptr<Dirent>>& files)
    {
        for (int i = 0; i < GetChildCount(parent); i++)
            ItemDeleted(parent, GetNthChild(parent, i));
        DeleteChildren(parent);

        for (auto& dirent : files)
        {
            if (dirent->file_type == TYPE_FILE)
            {
                auto item = AppendItem(parent,
                    dirent->filename,
                    _fileIcon,
                    new DirentContainer(dirent));
                ItemAdded(parent, item);
            }
            else
            {
                auto item = AppendContainer(parent,
                    dirent->filename,
                    _folderOpenIcon,
                    _folderClosedIcon,
                    new DirentContainer(dirent));

                auto child = AppendItem(item, "...loading...");
                ItemAdded(parent, item);
                ItemAdded(item, child);
            }
        }

        ItemChanged(parent);
    }

private:
    wxIcon _fileIcon;
    wxIcon _folderOpenIcon;
    wxIcon _folderClosedIcon;
};

FilesystemModel* FilesystemModel::Associate(wxDataViewCtrl* control)
{
    auto model = new FilesystemModelImpl();
    control->AssociateModel(model);
    return model;
}

// vim: sw=4 ts=4 et
