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
#include <wx/stattext.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/filepicker.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include "visualisationcontrol.h"
#include <wx/dataview.h>
#include <wx/simplebook.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>
#include "fluxviewercontrol.h"
#include <wx/scrolbar.h>
#include <wx/notebook.h>
#include <wx/gbsizer.h>

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
		wxScrolledWindow* idlePanel;
		wxStaticText* m_staticText61;
		wxRadioButton* realDiskRadioButton;
		wxPanel* realDiskRadioButtonPanel;
		wxComboBox* deviceCombo;
		wxChoice* driveChoice;
		wxCheckBox* highDensityToggle;
		wxRadioButton* fluxImageRadioButton;
		wxPanel* fluxImageRadioButtonPanel;
		wxFilePickerCtrl* fluxImagePicker;
		wxRadioButton* diskImageRadioButton;
		wxPanel* diskImageRadioButtonPanel;
		wxFilePickerCtrl* diskImagePicker;
		wxStaticText* m_staticText23;
		wxPanel* m_panel11;
		wxChoice* formatChoice;
		wxButton* customConfigurationButton;
		wxStaticText* m_staticText19;
		wxButton* readButton;
		wxButton* writeButton;
		wxButton* browseButton;
		wxPanel* imagePanel;
		wxAuiToolBar* imagerToolbar;
		wxAuiToolBarItem* imagerBackTool;
		VisualisationControl* visualiser;
		wxButton* imagerSaveImageButton;
		wxButton* imagerSaveFluxButton;
		wxStaticText* m_staticText4;
		wxButton* imagerGoAgainButton;
		wxPanel* browsePanel;
		wxAuiToolBar* browserToolbar;
		wxAuiToolBarItem* browserBackTool;
		wxAuiToolBarItem* browserInfoTool;
		wxAuiToolBarItem* browserViewTool;
		wxAuiToolBarItem* browserSaveTool;
		wxAuiToolBarItem* browserFileMenuButton;
		wxMenu* browserFileMenu;
		wxMenuItem* browserAddMenuItem;
		wxMenuItem* browserNewDirectoryMenuItem;
		wxMenuItem* browserRenameMenuItem;
		wxMenuItem* browserDeleteMenuItem;
		wxAuiToolBarItem* browserFormatTool;
		wxDataViewCtrl* browserTree;
		wxDataViewColumn* m_dataViewColumn1;
		wxDataViewColumn* m_dataViewColumn2;
		wxDataViewColumn* m_dataViewColumn3;
		wxButton* browserDiscardButton;
		wxButton* browserCommitButton;
		wxStaticText* m_staticText12;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnAboutMenuItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowLogWindow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowConfigWindow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnConfigRadioButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnControlsChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnControlsChanged( wxFileDirPickerEvent& event ) { event.Skip(); }
		virtual void OnCustomConfigurationButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReadButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnWriteButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowseButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBackButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveImageButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveFluxButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnImagerGoAgainButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserInfoButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserViewButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserSaveButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserAddMenuItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserNewDirectoryMenuItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserRenameMenuItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserDeleteMenuItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserFormatButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserDirectoryExpanding( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnBrowserSelectionChanged( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnBrowserDiscardButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserCommitButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		MainWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("FluxEngine"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 616,607 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );

		~MainWindowGen();

		void browserFileMenuButtonOnDropDownMenu( wxAuiToolBarEvent &event )
		{
			if ( event.IsDropDownClicked() )
			{
				browserToolbar->SetToolSticky( event.GetId(), true );
				wxRect rect = browserToolbar->GetToolRect( event.GetId() );
				wxPoint pt = browserToolbar->ClientToScreen( rect.GetBottomLeft() );
				pt = ScreenToClient( pt );
				browserToolbar->PopupMenu( browserFileMenu, pt );
				browserToolbar->SetToolSticky( event.GetId(), false );
			}
		}

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
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnClose( wxCommandEvent& event ) { event.Skip(); }


	public:

		TextViewerWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 208,143 ), long style = wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

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
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnClose( wxCommandEvent& event ) { event.Skip(); }


	public:

		FluxViewerWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,200 ), long style = wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

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
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSave( wxCommandEvent& event ) { event.Skip(); }


	public:

		TextEditorWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

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
		wxPanel* m_panel7;
		wxTextCtrl* hexControl;
		wxPanel* m_panel8;
		wxTextCtrl* textControl;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCommandEvent& event ) { event.Skip(); }


	public:

		FileViewerWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 408,269 ), long style = wxDEFAULT_DIALOG_STYLE );

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
		wxStdDialogButtonSizer* m_sdbSizer6;
		wxButton* m_sdbSizer6OK;
		wxButton* m_sdbSizer6Cancel;

	public:
		wxTextCtrl* filenameText;
		wxFilePickerCtrl* targetFilePicker;

		GetfileDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Copy file off disk"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

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
		wxStdDialogButtonSizer* m_sdbSizer6;
		wxButton* m_sdbSizer6OK;
		wxButton* m_sdbSizer6Cancel;

	public:
		wxTextCtrl* newNameText;
		wxTextCtrl* oldNameText;

		FileConflictDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Filename conflict"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~FileConflictDialog();

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
		wxStdDialogButtonSizer* m_sdbSizer6;
		wxButton* m_sdbSizer6OK;
		wxButton* m_sdbSizer6Cancel;

	public:
		wxTextCtrl* volumeNameText;
		wxCheckBox* quickFormatCheckBox;

		FormatDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Format disk"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~FormatDialog();

};

