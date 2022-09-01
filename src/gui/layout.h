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
#include "fluxviewercontrol.h"
#include <wx/scrolbar.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/filepicker.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include "visualisationcontrol.h"
#include <wx/splitter.h>
#include <wx/dataview.h>
#include <wx/simplebook.h>
#include <wx/notebook.h>
#include <wx/statusbr.h>

///////////////////////////////////////////////////////////////////////////

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
/// Class MainWindowGen
///////////////////////////////////////////////////////////////////////////////
class MainWindowGen : public wxFrame
{
	private:

	protected:
		wxNotebook* outerNotebook;
		wxPanel* dataPanel;
		wxSimplebook* innerNotebook;
		wxScrolledWindow* idlePanel;
		wxStaticText* m_staticText61;
		wxRadioButton* realDiskRadioButton;
		wxPanel* m_panel8;
		wxComboBox* deviceCombo;
		wxChoice* driveChoice;
		wxCheckBox* highDensityToggle;
		wxRadioButton* fluxImageRadioButton;
		wxPanel* m_panel91;
		wxFilePickerCtrl* fluxImagePicker;
		wxRadioButton* diskImageRadioButton;
		wxPanel* m_panel101;
		wxFilePickerCtrl* diskImagePicker;
		wxStaticText* m_staticText23;
		wxPanel* m_panel11;
		wxChoice* formatChoice;
		wxButton* extraConfigurationButton;
		wxStaticText* m_staticText19;
		wxButton* m_button5;
		wxButton* m_button6;
		wxButton* m_button7;
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
		wxPanel* debugPanel;
		wxTextCtrl* protoConfigEntry;
		wxMenuBar* m_menubar4;
		wxMenu* m_menu1;
		wxStatusBar* statusBar;

		// Virtual event handlers, override them in your derived class
		virtual void OnAbout( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExit( wxCommandEvent& event ) { event.Skip(); }


	public:

		MainWindowGen( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("FluxEngine"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 828,620 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainWindowGen();

		void m_splitter3OnIdle( wxIdleEvent& )
		{
			m_splitter3->SetSashPosition( 0 );
			m_splitter3->Disconnect( wxEVT_IDLE, wxIdleEventHandler( MainWindowGen::m_splitter3OnIdle ), NULL, this );
		}

};

