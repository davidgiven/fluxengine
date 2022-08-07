///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "layout.h"

///////////////////////////////////////////////////////////////////////////

MainWindowGen::MainWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 450,500 ), wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK ) );

	bSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	bSizer1->AddGrowableCol( 1 );
	bSizer1->AddGrowableRow( 0 );
	bSizer1->SetFlexibleDirection( wxHORIZONTAL );
	bSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer4->AddGrowableRow( 0 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	visualiser = new VisualisationControl( this, wxID_ANY, wxDefaultPosition, wxSize( 200,480 ), wxBORDER_THEME );
	visualiser->SetMinSize( wxSize( 200,480 ) );

	fgSizer4->Add( visualiser, 1, wxALL|wxEXPAND, 5 );

	stopButton = new wxButton( this, wxID_ANY, wxT("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer4->Add( stopButton, 0, wxALIGN_CENTER|wxALL, 5 );


	bSizer1->Add( fgSizer4, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer2->AddGrowableCol( 0 );
	fgSizer2->AddGrowableRow( 1 );
	fgSizer2->SetFlexibleDirection( wxVERTICAL );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText4 = new wxStaticText( this, wxID_ANY, wxT("Device:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer3->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	deviceCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT );
	fgSizer3->Add( deviceCombo, 0, wxALL|wxEXPAND, 5 );

	m_staticText5 = new wxStaticText( this, wxID_ANY, wxT("Flux source/sink:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer3->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	fluxSourceSinkCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fluxSourceSinkCombo->Append( wxT("drive:0") );
	fluxSourceSinkCombo->Append( wxT("drive:1") );
	fgSizer3->Add( fluxSourceSinkCombo, 0, wxALL|wxEXPAND, 5 );

	m_staticText51 = new wxStaticText( this, wxID_ANY, wxT("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText51->Wrap( -1 );
	fgSizer3->Add( m_staticText51, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	wxArrayString formatChoiceChoices;
	formatChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, formatChoiceChoices, wxCB_SORT );
	formatChoice->SetSelection( 0 );
	fgSizer3->Add( formatChoice, 0, wxALL|wxEXPAND, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	highDensityToggle = new wxCheckBox( this, wxID_ANY, wxT("High density disk"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( highDensityToggle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );


	fgSizer2->Add( fgSizer3, 1, wxEXPAND, 5 );

	notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panel1 = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->AddGrowableCol( 0 );
	fgSizer5->AddGrowableRow( 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	additionalSettingsEntry = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	fgSizer5->Add( additionalSettingsEntry, 0, wxALL|wxEXPAND, 5 );


	m_panel1->SetSizer( fgSizer5 );
	m_panel1->Layout();
	fgSizer5->Fit( m_panel1 );
	notebook->AddPage( m_panel1, wxT("Additional settings"), true );
	m_panel2 = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	logEntry = new wxTextCtrl( m_panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH );
	fgSizer8->Add( logEntry, 0, wxALL|wxEXPAND, 5 );


	m_panel2->SetSizer( fgSizer8 );
	m_panel2->Layout();
	fgSizer8->Fit( m_panel2 );
	notebook->AddPage( m_panel2, wxT("Logs"), false );
	m_panel3 = new wxPanel( notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->AddGrowableCol( 0 );
	fgSizer9->AddGrowableRow( 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	protoConfigEntry = new wxTextCtrl( m_panel3, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	fgSizer9->Add( protoConfigEntry, 0, wxALL|wxEXPAND, 5 );


	m_panel3->SetSizer( fgSizer9 );
	m_panel3->Layout();
	fgSizer9->Fit( m_panel3 );
	notebook->AddPage( m_panel3, wxT("Debug info"), false );

	fgSizer2->Add( notebook, 1, wxEXPAND | wxALL, 5 );

	wxGridSizer* m_sizer;
	m_sizer = new wxGridSizer( 0, 2, 0, 0 );

	readFluxButton = new wxButton( this, wxID_ANY, wxT("Read flux"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizer->Add( readFluxButton, 0, wxALL|wxEXPAND, 5 );

	readImageButton = new wxButton( this, wxID_ANY, wxT("Read image"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizer->Add( readImageButton, 0, wxALL|wxEXPAND, 5 );

	writeFluxButton = new wxButton( this, wxID_ANY, wxT("Write flux"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizer->Add( writeFluxButton, 0, wxALL|wxEXPAND, 5 );

	writeImageButton = new wxButton( this, wxID_ANY, wxT("Write image"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizer->Add( writeImageButton, 0, wxALL|wxEXPAND, 5 );


	fgSizer2->Add( m_sizer, 1, wxEXPAND|wxFIXED_MINSIZE, 5 );


	bSizer1->Add( fgSizer2, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_menu1, wxID_ABOUT, wxString( wxT("About") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem2 );

	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_EXIT, wxString( wxT("E&xit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	m_menubar1->Append( m_menu1, wxT("&File") );

	this->SetMenuBar( m_menubar1 );


	// Connect Events
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnAbout ), this, m_menuItem2->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnExit ), this, m_menuItem1->GetId());
}

MainWindowGen::~MainWindowGen()
{
	// Disconnect Events

}

FluxViewerWindowGen::FluxViewerWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar2 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_CLOSE, wxString( wxT("&Close") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	m_menubar2->Append( m_menu1, wxT("&Window") );

	this->SetMenuBar( m_menubar2 );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	fluxviewer = new FluxViewerControl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( fluxviewer, 1, wxEXPAND, 5 );

	scrollbar = new wxScrollBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL );
	bSizer1->Add( scrollbar, 0, wxALIGN_BOTTOM|wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	statusbar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );

	// Connect Events
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( FluxViewerWindowGen::OnExit ), this, m_menuItem1->GetId());
}

FluxViewerWindowGen::~FluxViewerWindowGen()
{
	// Disconnect Events

}

HexViewerWindowGen::HexViewerWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_menubar2 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_CLOSE, wxString( wxT("&Close") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	m_menubar2->Append( m_menu1, wxT("&Window") );

	this->SetMenuBar( m_menubar2 );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 1, 1, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 0 );
	fgSizer8->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	hexEntry = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH );
	hexEntry->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizer8->Add( hexEntry, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer8 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexViewerWindowGen::OnExit ), this, m_menuItem1->GetId());
}

HexViewerWindowGen::~HexViewerWindowGen()
{
	// Disconnect Events

}
