///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
#include <wx/toolbar.h>
#include "visualisationcontrol.h"
#include <wx/dataview.h>
#include <wx/simplebook.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>
#include "fluxviewercontrol.h"
#include <wx/scrolbar.h>
#include <wx/notebook.h>

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
		wxToolBar* imagerToolbar;
		wxToolBarToolBase* imagerBackTool;
		VisualisationControl* visualiser;
		wxButton* imagerSaveImageButton;
		wxButton* imagerSaveFluxButton;
		wxStaticText* m_staticText4;
		wxButton* imagerGoAgainButton;
		wxPanel* browsePanel;
		wxToolBar* browserToolbar;
		wxToolBarToolBase* browserBackTool;
		wxToolBarToolBase* browserInfoTool;
		wxToolBarToolBase* browserOpenTool;
		wxToolBarToolBase* browserSaveTool;
		wxToolBarToolBase* browserNewTool;
		wxToolBarToolBase* browserNewDirectoryTool;
		wxToolBarToolBase* browserRenameTool;
		wxToolBarToolBase* browserDeleteTool;
		wxToolBarToolBase* browserFormatTool;
		wxDataViewCtrl* browserTree;
		wxDataViewColumn* m_dataViewColumn1;
		wxDataViewColumn* m_dataViewColumn2;
		wxDataViewColumn* m_dataViewColumn3;
		wxButton* browserDiscardButton;
		wxButton* browserCommitButton;

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
		virtual void OnBrowserOpenButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserSaveButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserNewButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserNewDirectoryButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserDeleteButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserFormatButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBrowserDirectoryExpanding( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnBrowserSelectionChanged( wxDataViewEvent& event ) { event.Skip(); }


	public:

		MainWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("FluxEngine"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 616,607 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );

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

