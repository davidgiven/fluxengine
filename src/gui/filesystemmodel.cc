#include "lib/core/globals.h"
#include "gui.h"
#include "filesystemmodel.h"
#include "lib/vfs/vfs.h"
#include <wx/artprov.h>

static uintptr_t nodeCount = 0;
FilesystemNode::FilesystemNode(std::shared_ptr<Dirent> dirent):
    dirent(dirent),
    item((void*)nodeCount++)
{
}

class FilesystemModelImpl : public FilesystemModel
{
public:
    FilesystemModelImpl():
        _fileIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_BUTTON)),
        _folderOpenIcon(
            wxArtProvider::GetIcon(wxART_FOLDER_OPEN, wxART_BUTTON)),
        _folderClosedIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_BUTTON))
    {
        _root = std::make_shared<FilesystemNode>(std::make_shared<Dirent>());
        _root->dirent->file_type = TYPE_DIRECTORY;

        _byItem[_root->item] = _root;

        ItemAdded(wxDataViewItem(), _root->item);
    }

    /* --- DataViewModel API --------------------------------------------- */

    unsigned int GetColumnCount() const override
    {
        return 3;
    }

    wxString GetColumnType(unsigned int column) const override
    {
        switch (column)
        {
            case 0:
                return "wxDataViewIconText";

            case 1:
            case 2:
                return "string";

            default:
                wxFAIL;
                return "<bad>";
        }
    }

    bool IsContainer(const wxDataViewItem& item) const override
    {
        auto node = Find(item);
        if (!node)
            return false;

        return node->dirent->file_type == TYPE_DIRECTORY;
    }

    wxDataViewItem GetParent(const wxDataViewItem& item) const override
    {
        auto node = Find(item);
        if (!node || (node == _root))
            return wxDataViewItem();

        return Find(node->dirent->path.parent())->item;
    }

    void GetValue(wxVariant& value,
        const wxDataViewItem& item,
        unsigned column) const override
    {
        auto node = Find(item);
        if (!node)
            return;

        if (node->stub)
        {
            switch (column)
            {
                case 0:
                    value << wxDataViewIconText("...loading...");
                    break;

                case 1:
                    value = "";
                    break;

                case 2:
                    value = "";
                    break;

                default:
                    wxFAIL;
            }
        }
        else
        {

            switch (column)
            {
                case 0:
                    value << wxDataViewIconText(node->dirent->filename,
                        (node->dirent->file_type == TYPE_DIRECTORY)
                            ? _folderClosedIcon
                            : _fileIcon);
                    break;

                case 1:
                    value = std::to_string(node->dirent->length);
                    break;

                case 2:
                    value = node->dirent->mode;
                    break;

                default:
                    wxFAIL;
            }
        }
    }

    bool SetValue(const wxVariant& value,
        const wxDataViewItem& item,
        unsigned column) override
    {
        auto node = Find(item);
        if (!node || node->stub)
            return false;

        if ((column == 0) && (value.GetType() == "wxDataViewIconText"))
        {
            wxDataViewIconText dvit;
            dvit << value;
            node->newname = dvit.GetText();
            return true;
        }

        return false;
    }

    unsigned GetChildren(const wxDataViewItem& item,
        wxDataViewItemArray& children) const override
    {
        auto node = Find(item);
        if (!node)
            return 0;

        for (auto& e : node->children)
            children.Add(e.second->item);
        return node->children.size();
    }

    /* --- Mutation API -------------------------------------------------- */

    void Clear(const Path& path) override
    {
        auto top = Find(path);
        if (!top)
            return;

        for (;;)
        {
            auto it = top->children.begin();
            if (it == top->children.end())
                break;

            auto child = it->second;
            if (!child->stub)
            {
                Clear(child->dirent->path);
                _byItem.erase(child->item);
            }
            top->children.erase(it);
            ItemDeleted(top->item, child->item);
        }
    }

    std::shared_ptr<FilesystemNode> Find(const Path& path) const override
    {
        if (path.empty())
            return _root;

        auto node = _root;
        for (const auto& element : path)
        {
            try
            {
                node = node->children.at(element);
            }
            catch (std::out_of_range& e)
            {
                return nullptr;
            }
        }

        return node;
    }

    std::shared_ptr<FilesystemNode> Find(
        const wxDataViewItem& item) const override
    {
        if (!item.IsOk())
            return _root;

        auto it = _byItem.find(item);
        if (it == _byItem.end())
            return nullptr;

        auto node = it->second.lock();
        if (node)
            return node;

        /* This node is stale; clean it out of the weak reference map. */

        _byItem.erase(item);
        return nullptr;
    }

    void Delete(const Path& path) override
    {
        auto parent = Find(path.parent());
        if (!parent)
            return;

        auto child = Find(path);
        if (!child)
            return;

        Clear(path);
        _byItem.erase(child->item);
        parent->children.erase(child->dirent->filename);
        ItemDeleted(parent->item, child->item);
    }

    void RemoveStub(const Path& path) override
    {
        auto node = Find(path);
        if (!node)
            return;

        /* If the only item in the directory is a stub, remove it. */

        if ((node->children.size() == 1) &&
            node->children.begin()->second->stub)
        {
        }
    }

    void Add(std::shared_ptr<Dirent> dirent) override
    {
        auto parent = Find(dirent->path.parent());
        if (!parent)
            return;

        /* Add the actual item (the easy bit). */

        auto node = std::make_shared<FilesystemNode>(dirent);
        _byItem[node->item] = node;
        parent->children[dirent->filename] = node;
        ItemAdded(parent->item, node->item);

        /* If this is a new directory, add the stub item to it. */

        if (dirent->file_type == TYPE_DIRECTORY)
        {
            auto stub =
                std::make_shared<FilesystemNode>(std::make_shared<Dirent>());
            _byItem[stub->item] = stub;
            node->children[""] = stub;
            stub->stub = true;
            stub->dirent->path = dirent->path;
            stub->dirent->path.push_back("");
            ItemAdded(node->item, stub->item);
        }
    }

private:
    std::shared_ptr<FilesystemNode> _root;
    mutable std::map<wxDataViewItem, std::weak_ptr<FilesystemNode>> _byItem;
    wxIcon _fileIcon;
    wxIcon _folderOpenIcon;
    wxIcon _folderClosedIcon;
};

FilesystemModel* FilesystemModel::Associate(wxDataViewCtrl* control)
{
    auto model = new FilesystemModelImpl();
    control->AssociateModel(nullptr);
    control->AssociateModel(model);
    return model;
}

// vim: sw=4 ts=4 et
