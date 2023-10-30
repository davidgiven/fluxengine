///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-234-gd93c9fc0-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include "fluxviewercontrol.h"
#include <wx/scrolbar.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/filepicker.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/statbmp.h>
#include <wx/wrapsizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include "visualisationcontrol.h"
#include "histogramviewer.h"
#include <wx/dataview.h>
#include <wx/gauge.h>
#include <wx/spinctrl.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainWindowGen
///////////////////////////////////////////////////////////////////////////////
class MainWindowGen : public wxFrame
{
private:
protected:
    wxMenuBar* menuBar;
    wxMenu* m_menu1;
    wxMenu* m_menu2;
    wxSimplebook* dataNotebook;

    // Virtual event handlers, override them in your derived class
    virtual void OnClose(wxCloseEvent& event)
    {
        event.Skip();
    }
    virtual void OnAboutMenuItem(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnExit(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnShowLogWindow(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnShowConfigWindow(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    MainWindowGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("FluxEngine"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(616, 607),
        long style = wxDEFAULT_FRAME_STYLE | wxRESIZE_BORDER |
                     wxFULL_REPAINT_ON_RESIZE | wxTAB_TRAVERSAL);

    ~MainWindowGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class TextViewerWindowGen
///////////////////////////////////////////////////////////////////////////////
class TextViewerWindowGen : public wxDialog
{
private:
protected:
    wxTextCtrl* textControl;
    wxStdDialogButtonSizer* m_sdbSizer2;
    wxButton* m_sdbSizer2OK;

    // Virtual event handlers, override them in your derived class
    virtual void OnClose(wxCloseEvent& event)
    {
        event.Skip();
    }
    virtual void OnClose(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    TextViewerWindowGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(208, 143),
        long style = wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxMAXIMIZE_BOX |
                     wxMINIMIZE_BOX | wxRESIZE_BORDER);

    ~TextViewerWindowGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FluxViewerWindowGen
///////////////////////////////////////////////////////////////////////////////
class FluxViewerWindowGen : public wxDialog
{
private:
protected:
    FluxViewerControl* fluxviewer;
    wxScrollBar* scrollbar;
    wxStdDialogButtonSizer* m_sdbSizer2;
    wxButton* m_sdbSizer2OK;

    // Virtual event handlers, override them in your derived class
    virtual void OnClose(wxCloseEvent& event)
    {
        event.Skip();
    }
    virtual void OnClose(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    FluxViewerWindowGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(400, 200),
        long style = wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxMAXIMIZE_BOX |
                     wxMINIMIZE_BOX | wxRESIZE_BORDER);

    ~FluxViewerWindowGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class TextEditorWindowGen
///////////////////////////////////////////////////////////////////////////////
class TextEditorWindowGen : public wxDialog
{
private:
protected:
    wxTextCtrl* textControl;
    wxStdDialogButtonSizer* m_sdbSizer2;
    wxButton* m_sdbSizer2Save;
    wxButton* m_sdbSizer2Cancel;

    // Virtual event handlers, override them in your derived class
    virtual void OnClose(wxCloseEvent& event)
    {
        event.Skip();
    }
    virtual void OnCancel(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnSave(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    TextEditorWindowGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxCLOSE_BOX | wxDEFAULT_DIALOG_STYLE | wxMAXIMIZE_BOX |
                     wxMINIMIZE_BOX | wxRESIZE_BORDER);

    ~TextEditorWindowGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FileViewerWindowGen
///////////////////////////////////////////////////////////////////////////////
class FileViewerWindowGen : public wxDialog
{
private:
protected:
    wxNotebook* m_notebook1;
    wxPanel* m_panel8;
    wxTextCtrl* textControl;
    wxPanel* m_panel7;
    wxTextCtrl* hexControl;
    wxStdDialogButtonSizer* m_sdbSizer2;
    wxButton* m_sdbSizer2OK;

    // Virtual event handlers, override them in your derived class
    virtual void OnClose(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    FileViewerWindowGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(408, 269),
        long style = wxDEFAULT_DIALOG_STYLE | wxMAXIMIZE_BOX | wxMINIMIZE_BOX |
                     wxRESIZE_BORDER);

    ~FileViewerWindowGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class GetfileDialog
///////////////////////////////////////////////////////////////////////////////
class GetfileDialog : public wxDialog
{
private:
protected:
    wxStaticText* m_staticText7;
    wxStaticText* m_staticText9;

public:
    wxTextCtrl* filenameText;
    wxFilePickerCtrl* targetFilePicker;
    wxStdDialogButtonSizer* buttons_;
    wxButton* buttons_OK;
    wxButton* buttons_Cancel;

    GetfileDialog(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("Copy file off disk"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE);

    ~GetfileDialog();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FileConflictDialog
///////////////////////////////////////////////////////////////////////////////
class FileConflictDialog : public wxDialog
{
private:
protected:
    wxStaticText* m_staticText91;
    wxStaticText* m_staticText7;
    wxStaticText* m_staticText9;

public:
    wxTextCtrl* newNameText;
    wxTextCtrl* oldNameText;
    wxStdDialogButtonSizer* buttons_;
    wxButton* buttons_OK;
    wxButton* buttons_Cancel;

    FileConflictDialog(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("Filename conflict"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE);

    ~FileConflictDialog();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FileRenameDialog
///////////////////////////////////////////////////////////////////////////////
class FileRenameDialog : public wxDialog
{
private:
protected:
    wxStaticText* m_staticText91;
    wxStaticText* m_staticText7;
    wxStaticText* m_staticText9;

public:
    wxTextCtrl* newNameText;
    wxTextCtrl* oldNameText;
    wxStdDialogButtonSizer* buttons_;
    wxButton* buttons_OK;
    wxButton* buttons_Cancel;

    FileRenameDialog(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("Rename or move file"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE);

    ~FileRenameDialog();
};

///////////////////////////////////////////////////////////////////////////////
/// Class CreateDirectoryDialog
///////////////////////////////////////////////////////////////////////////////
class CreateDirectoryDialog : public wxDialog
{
private:
protected:
    wxStaticText* m_staticText91;
    wxStaticText* m_staticText9;

public:
    wxTextCtrl* newNameText;
    wxStdDialogButtonSizer* buttons_;
    wxButton* buttons_OK;
    wxButton* buttons_Cancel;

    CreateDirectoryDialog(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("Create new directory"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE);

    ~CreateDirectoryDialog();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FormatDialog
///////////////////////////////////////////////////////////////////////////////
class FormatDialog : public wxDialog
{
private:
protected:
    wxStaticText* m_staticText91;
    wxStaticText* m_staticText7;

public:
    wxTextCtrl* volumeNameText;
    wxCheckBox* quickFormatCheckBox;
    wxStdDialogButtonSizer* buttons_;
    wxButton* buttons_OK;
    wxButton* buttons_Cancel;

    FormatDialog(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& title = wxT("Format disk"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE);

    ~FormatDialog();
};

///////////////////////////////////////////////////////////////////////////////
/// Class IdlePanelGen
///////////////////////////////////////////////////////////////////////////////
class IdlePanelGen : public wxPanel
{
private:
protected:
    wxStaticBitmap* applicationBitmap;
    wxPanel* sourceIconPanel;
    wxSimplebook* sourceBook;
    wxChoice* formatChoice;
    wxButton* customConfigurationButton;
    wxPanel* formatOptionsContainer;
    wxButton* readButton;
    wxButton* writeButton;
    wxButton* browseButton;
    wxButton* formatButton;
    wxButton* exploreButton;

    // Virtual event handlers, override them in your derived class
    virtual void OnControlsChanged(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnCustomConfigurationButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnReadButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnWriteButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowseButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnFormatButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnExploreButton(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    IdlePanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(-1, -1),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~IdlePanelGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class ImagerPanelGen
///////////////////////////////////////////////////////////////////////////////
class ImagerPanelGen : public wxPanel
{
private:
protected:
    wxAuiToolBar* imagerToolbar;
    wxAuiToolBarItem* imagerBackTool;
    VisualisationControl* visualiser;
    wxButton* imagerSaveImageButton;
    wxButton* imagerSaveFluxButton;
    HistogramViewer* histogram;
    wxButton* imagerGoAgainButton;

    // Virtual event handlers, override them in your derived class
    virtual void OnBackButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnSaveImageButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnSaveFluxButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnImagerGoAgainButton(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    ImagerPanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(500, 300),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~ImagerPanelGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class BrowserPanelGen
///////////////////////////////////////////////////////////////////////////////
class BrowserPanelGen : public wxPanel
{
private:
protected:
    wxAuiToolBar* browserToolbar;
    wxAuiToolBarItem* browserBackTool;
    wxAuiToolBarItem* browserInfoTool;
    wxAuiToolBarItem* browserViewTool;
    wxAuiToolBarItem* browserSaveTool;
    wxAuiToolBarItem* browserMoreMenuButton;
    wxMenu* browserMoreMenu;
    wxMenuItem* browserAddMenuItem;
    wxMenuItem* browserNewDirectoryMenuItem;
    wxMenuItem* browserRenameMenuItem;
    wxMenuItem* browserDeleteMenuItem;
    wxAuiToolBarItem* browserFormatTool;
    wxDataViewCtrl* browserTree;
    wxDataViewColumn* m_dataViewColumn1;
    wxDataViewColumn* m_dataViewColumn2;
    wxDataViewColumn* m_dataViewColumn3;
    wxGauge* diskSpaceGauge;
    wxButton* browserDiscardButton;
    wxButton* browserCommitButton;
    wxStaticText* m_staticText12;

    // Virtual event handlers, override them in your derived class
    virtual void OnBackButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserInfoButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserViewButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserSaveButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserAddMenuItem(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserNewDirectoryMenuItem(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserRenameMenuItem(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserDeleteMenuItem(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserFormatButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserBeginDrag(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserDrop(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserDropPossible(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserDirectoryExpanding(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserFilenameChanged(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserSelectionChanged(wxDataViewEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserDiscardButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnBrowserCommitButton(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    BrowserPanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(500, 300),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~BrowserPanelGen();

    void browserMoreMenuButtonOnDropDownMenu(wxAuiToolBarEvent& event)
    {
        if (event.IsDropDownClicked())
        {
            browserToolbar->SetToolSticky(event.GetId(), true);
            wxRect rect = browserToolbar->GetToolRect(event.GetId());
            wxPoint pt = browserToolbar->ClientToScreen(rect.GetBottomLeft());
            pt = ScreenToClient(pt);
            browserToolbar->PopupMenu(browserMoreMenu, pt);
            browserToolbar->SetToolSticky(event.GetId(), false);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
/// Class ExplorerPanelGen
///////////////////////////////////////////////////////////////////////////////
class ExplorerPanelGen : public wxPanel
{
private:
protected:
    wxAuiToolBar* explorerToolbar;
    wxAuiToolBarItem* explorerBackTool;
    wxAuiToolBarItem* explorerRefreshTool;
    wxStaticText* m_staticText22;
    wxSpinCtrl* explorerTrackSpinCtrl;
    wxStaticText* m_staticText26;
    wxSpinCtrl* explorerSideSpinCtrl;
    wxStaticText* m_staticText231;
    wxSpinCtrlDouble* explorerStartTimeSpinCtrl;
    wxStaticText* m_staticText24;
    wxButton* guessButton;
    wxSpinCtrlDouble* explorerClockSpinCtrl;
    wxStaticText* m_staticText25;
    wxSpinCtrl* explorerBitOffsetSpinCtrl;
    wxStaticText* m_staticText27;
    wxChoice* explorerDecodeChoice;
    wxStaticText* m_staticText241;
    wxCheckBox* explorerReverseCheckBox;
    HistogramViewer* histogram;
    wxTextCtrl* explorerText;

    // Virtual event handlers, override them in your derived class
    virtual void OnBackButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnExplorerRefreshButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnExplorerSettingChange(wxSpinEvent& event)
    {
        event.Skip();
    }
    virtual void OnExplorerSettingChange(wxSpinDoubleEvent& event)
    {
        event.Skip();
    }
    virtual void OnGuessClockButton(wxCommandEvent& event)
    {
        event.Skip();
    }
    virtual void OnExplorerSettingChange(wxCommandEvent& event)
    {
        event.Skip();
    }

public:
    ExplorerPanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(620, 426),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~ExplorerPanelGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class HardwareSourcePanelGen
///////////////////////////////////////////////////////////////////////////////
class HardwareSourcePanelGen : public wxPanel
{
private:
protected:
    wxStaticText* m_staticText30;
    wxStaticText* m_staticText29;

public:
    wxStaticText* label;
    wxCheckBox* highDensityToggle;
    wxChoice* driveTypeChoice;

    HardwareSourcePanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(-1, -1),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~HardwareSourcePanelGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class FluxfileSourcePanelGen
///////////////////////////////////////////////////////////////////////////////
class FluxfileSourcePanelGen : public wxPanel
{
private:
protected:
    wxStaticText* m_staticText28;
    wxStaticText* m_staticText27;
    wxStaticText* m_staticText281;

public:
    wxFilePickerCtrl* fluxImagePicker;
    wxChoice* fluxImageFormat;

    FluxfileSourcePanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(-1, -1),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~FluxfileSourcePanelGen();
};

///////////////////////////////////////////////////////////////////////////////
/// Class ImagefileSourcePanelGen
///////////////////////////////////////////////////////////////////////////////
class ImagefileSourcePanelGen : public wxPanel
{
private:
protected:
    wxStaticText* m_staticText29;
    wxStaticText* m_staticText28;

public:
    wxFilePickerCtrl* diskImagePicker;

    ImagefileSourcePanelGen(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(-1, -1),
        long style = wxTAB_TRAVERSAL,
        const wxString& name = wxEmptyString);

    ~ImagefileSourcePanelGen();
};
