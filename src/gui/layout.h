///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include "visualisationcontrol.h"
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/menu.h>
#include <wx/frame.h>
#include "fluxviewercontrol.h"
#include <wx/scrolbar.h>
#include <wx/toolbar.h>
#include <wx/splitter.h>
#include <wx/dataview.h>
#include <wx/scrolwin.h>
#include <wx/simplebook.h>
#include <wx/statusbr.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainWindowGen
///////////////////////////////////////////////////////////////////////////////
class MainWindowGen : public wxFrame
{
	private:
		wxFlexGridSizer* r;

	protected:
		VisualisationControl* visualiser;
		wxButton* stopButton;
		wxStaticText* m_staticText4;
		wxComboBox* deviceCombo;
		wxStaticText* m_staticText5;
		wxComboBox* fluxSourceSinkCombo;
		wxStaticText* m_staticText51;
		wxChoice* formatChoice;
		wxCheckBox* highDensityToggle;
		wxNotebook* notebook;
		wxPanel* m_panel1;
		wxTextCtrl* additionalSettingsEntry;
		wxPanel* m_panel2;
		wxTextCtrl* logEntry;
		wxPanel* m_panel3;
		wxTextCtrl* protoConfigEntry;
		wxButton* readFluxButton;
		wxButton* readImageButton;
		wxButton* writeFluxButton;
		wxButton* writeImageButton;
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;

		// Virtual event handlers, override them in your derived class
		virtual void OnAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }


	public:

		MainWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("FluxEngine"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 587,595 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainWindowGen();

};

///////////////////////////////////////////////////////////////////////////////
/// Class FluxViewerWindowGen
///////////////////////////////////////////////////////////////////////////////
class FluxViewerWindowGen : public wxFrame
{
	private:

	protected:
		wxMenuBar* m_menubar2;
		wxMenu* m_menu1;
		FluxViewerControl* fluxviewer;
		wxScrollBar* scrollbar;

		// Virtual event handlers, override them in your derived class
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }


	public:

		FluxViewerWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Flux Viewer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~FluxViewerWindowGen();

};

///////////////////////////////////////////////////////////////////////////////
/// Class HexViewerWindowGen
///////////////////////////////////////////////////////////////////////////////
class HexViewerWindowGen : public wxFrame
{
	private:

	protected:
		wxMenuBar* m_menubar2;
		wxMenu* m_menu1;
		wxTextCtrl* hexEntry;

		// Virtual event handlers, override them in your derived class
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }


	public:

		HexViewerWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Hex Viewer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~HexViewerWindowGen();

};

///////////////////////////////////////////////////////////////////////////////
/// Class MyFrame4
///////////////////////////////////////////////////////////////////////////////
class MyFrame4 : public wxFrame
{
	private:

	protected:
		wxStaticText* m_staticText7;
		wxStaticText* m_staticText8;
		wxStaticText* m_staticText6;
		wxChoice* deviceChoice;
		wxComboBox* sourceChoice;
		wxChoice* formatChoice;
		wxToolBar* m_toolBar1;
		wxToolBarToolBase* readDiskTool;
		wxToolBarToolBase* writeDiskTool;
		wxToolBarToolBase* browseTool;
		wxToolBarToolBase* stopTool;
		wxNotebook* m_notebook2;
		wxPanel* dataPanel;
		wxSimplebook* m_simplebook4;
		wxPanel* idlePanel;
		wxStaticText* m_staticText10;
		wxPanel* imagePanel;
		wxSplitterWindow* m_splitter3;
		wxPanel* m_panel10;
		VisualisationControl* visualiser;
		wxPanel* m_panel9;
		wxButton* m_button9;
		wxButton* m_button10;
		wxPanel* browsePanel;
		wxScrolledWindow* m_scrolledWindow1;
		wxDataViewCtrl* browserView;
		wxDataViewColumn* browserFilenameColumn;
		wxDataViewColumn* browserModeColumn;
		wxDataViewColumn* browserLengthColumn;
		wxDataViewColumn* browserExtraColumn;
		wxButton* browserDiscardButton;
		wxButton* browserCommitButton;
		wxPanel* loggingPanel;
		wxTextCtrl* logEntry;
		wxPanel* configPanel;
		wxTextCtrl* additionalSettingsEntry;
		wxPanel* debugPanel;
		wxTextCtrl* protoConfigEntry;
		wxMenuBar* m_menubar4;
		wxMenu* m_menu1;
		wxStatusBar* statusBar;

		// Virtual event handlers, override them in your derived class
		virtual void OnAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }


	public:

		MyFrame4( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 745,620 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MyFrame4();

		void m_splitter3OnIdle( wxIdleEvent& )
		{
			m_splitter3->SetSashPosition( 0 );
			m_splitter3->Disconnect( wxEVT_IDLE, wxIdleEventHandler( MyFrame4::m_splitter3OnIdle ), NULL, this );
		}

};

