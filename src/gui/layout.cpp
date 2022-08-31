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

	r = new wxFlexGridSizer( 0, 2, 0, 0 );
	r->AddGrowableCol( 1 );
	r->AddGrowableRow( 0 );
	r->SetFlexibleDirection( wxHORIZONTAL );
	r->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

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


	r->Add( fgSizer4, 1, wxEXPAND, 5 );

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


	r->Add( fgSizer2, 1, wxEXPAND, 5 );


	this->SetSizer( r );
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

MyFrame4::MyFrame4( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTION ) );

	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer9->AddGrowableCol( 0 );
	fgSizer9->AddGrowableRow( 1 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer10->AddGrowableCol( 0 );
	fgSizer10->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgSizer11->AddGrowableCol( 0 );
	fgSizer11->AddGrowableCol( 1 );
	fgSizer11->AddGrowableCol( 2 );
	fgSizer11->AddGrowableRow( 0 );
	fgSizer11->SetFlexibleDirection( wxVERTICAL );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Device"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	fgSizer11->Add( m_staticText7, 0, wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_staticText8 = new wxStaticText( this, wxID_ANY, wxT("Drive / Image"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	fgSizer11->Add( m_staticText8, 0, wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_staticText6 = new wxStaticText( this, wxID_ANY, wxT("Format"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	fgSizer11->Add( m_staticText6, 0, wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	wxArrayString deviceChoiceChoices;
	deviceChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, deviceChoiceChoices, 0 );
	deviceChoice->SetSelection( 0 );
	fgSizer11->Add( deviceChoice, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );

	sourceChoice = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizer11->Add( sourceChoice, 0, wxALL|wxEXPAND, 5 );

	wxArrayString formatChoiceChoices;
	formatChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, formatChoiceChoices, 0 );
	formatChoice->SetSelection( 0 );
	fgSizer11->Add( formatChoice, 0, wxALL|wxEXPAND, 5 );


	fgSizer10->Add( fgSizer11, 1, wxEXPAND, 5 );

	m_toolBar1 = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_TEXT );
	readDiskTool = m_toolBar1->AddTool( wxID_ANY, wxT("Read"), wxArtProvider::GetBitmap( wxART_COPY, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxT("Read disk to image"), wxEmptyString, NULL );

	writeDiskTool = m_toolBar1->AddTool( wxID_ANY, wxT("Write"), wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browseTool = m_toolBar1->AddTool( wxID_ANY, wxT("Browse"), wxArtProvider::GetBitmap( wxART_FOLDER_OPEN, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	m_toolBar1->AddSeparator();

	stopTool = m_toolBar1->AddTool( wxID_ANY, wxT("Stop"), wxArtProvider::GetBitmap( wxART_CROSS_MARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	m_toolBar1->Realize();

	fgSizer10->Add( m_toolBar1, 0, wxEXPAND, 5 );


	fgSizer9->Add( fgSizer10, 1, wxEXPAND, 5 );

	m_notebook2 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	dataPanel = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer10;
	gSizer10 = new wxGridSizer( 1, 1, 0, 0 );

	m_simplebook4 = new wxSimplebook( dataPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	idlePanel = new wxPanel( m_simplebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer11;
	gSizer11 = new wxGridSizer( 1, 1, 0, 0 );

	m_staticText10 = new wxStaticText( idlePanel, wxID_ANY, wxT("Select a device, drive and format,\nand then press one of the buttons above."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText10->Wrap( -1 );
	gSizer11->Add( m_staticText10, 0, wxALIGN_CENTER|wxALL, 5 );


	idlePanel->SetSizer( gSizer11 );
	idlePanel->Layout();
	gSizer11->Fit( idlePanel );
	m_simplebook4->AddPage( idlePanel, wxT("a page"), false );
	imagePanel = new wxPanel( m_simplebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_splitter3 = new wxSplitterWindow( imagePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter3->Connect( wxEVT_IDLE, wxIdleEventHandler( MyFrame4::m_splitter3OnIdle ), NULL, this );

	m_panel10 = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer5;
	gSizer5 = new wxGridSizer( 0, 1, 0, 0 );

	visualiser = new VisualisationControl( m_panel10, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxBORDER_THEME );
	gSizer5->Add( visualiser, 0, wxALL|wxEXPAND, 5 );


	m_panel10->SetSizer( gSizer5 );
	m_panel10->Layout();
	gSizer5->Fit( m_panel10 );
	m_panel9 = new wxPanel( m_splitter3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer7;
	gSizer7 = new wxGridSizer( 1, 1, 0, 0 );

	wxGridSizer* gSizer8;
	gSizer8 = new wxGridSizer( 2, 1, 0, 0 );

	m_button9 = new wxButton( m_panel9, wxID_ANY, wxT("Save decoded image"), wxDefaultPosition, wxDefaultSize, 0 );

	m_button9->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_BUTTON ) );
	gSizer8->Add( m_button9, 0, wxALL|wxEXPAND, 5 );

	m_button10 = new wxButton( m_panel9, wxID_ANY, wxT("Save raw flux"), wxDefaultPosition, wxDefaultSize, 0 );

	m_button10->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE_AS, wxART_BUTTON ) );
	gSizer8->Add( m_button10, 0, wxALL|wxEXPAND, 5 );


	gSizer7->Add( gSizer8, 1, wxALIGN_CENTER, 5 );


	m_panel9->SetSizer( gSizer7 );
	m_panel9->Layout();
	gSizer7->Fit( m_panel9 );
	m_splitter3->SplitVertically( m_panel10, m_panel9, 0 );
	bSizer4->Add( m_splitter3, 1, wxEXPAND, 5 );


	imagePanel->SetSizer( bSizer4 );
	imagePanel->Layout();
	bSizer4->Fit( imagePanel );
	m_simplebook4->AddPage( imagePanel, wxT("a page"), false );
	browsePanel = new wxPanel( m_simplebook4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer23;
	fgSizer23 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer23->AddGrowableCol( 0 );
	fgSizer23->AddGrowableRow( 0 );
	fgSizer23->SetFlexibleDirection( wxBOTH );
	fgSizer23->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_scrolledWindow1 = new wxScrolledWindow( browsePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxGridSizer* gSizer13;
	gSizer13 = new wxGridSizer( 1, 1, 0, 0 );

	browserView = new wxDataViewCtrl( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	browserFilenameColumn = browserView->AppendTextColumn( wxT("Filename"), 0, wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	browserModeColumn = browserView->AppendTextColumn( wxT("Mode"), 1, wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	browserLengthColumn = browserView->AppendTextColumn( wxT("Length"), 2, wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	browserExtraColumn = browserView->AppendTextColumn( wxT("Additional properties"), 0, wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	gSizer13->Add( browserView, 0, wxEXPAND, 5 );


	m_scrolledWindow1->SetSizer( gSizer13 );
	m_scrolledWindow1->Layout();
	gSizer13->Fit( m_scrolledWindow1 );
	fgSizer23->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );

	wxGridSizer* gSizer12;
	gSizer12 = new wxGridSizer( 0, 2, 0, 0 );

	browserDiscardButton = new wxButton( browsePanel, wxID_ANY, wxT("Discard pending changes"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer12->Add( browserDiscardButton, 0, wxALIGN_CENTER|wxALL, 5 );

	browserCommitButton = new wxButton( browsePanel, wxID_ANY, wxT("Commit pending changes to disk"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer12->Add( browserCommitButton, 0, wxALIGN_CENTER|wxALL, 5 );


	fgSizer23->Add( gSizer12, 1, wxEXPAND, 5 );


	browsePanel->SetSizer( fgSizer23 );
	browsePanel->Layout();
	fgSizer23->Fit( browsePanel );
	m_simplebook4->AddPage( browsePanel, wxT("a page"), false );

	gSizer10->Add( m_simplebook4, 1, wxEXPAND | wxALL, 5 );


	dataPanel->SetSizer( gSizer10 );
	dataPanel->Layout();
	gSizer10->Fit( dataPanel );
	m_notebook2->AddPage( dataPanel, wxT("Data"), false );
	loggingPanel = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer6;
	gSizer6 = new wxGridSizer( 1, 1, 0, 0 );

	logEntry = new wxTextCtrl( loggingPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH );
	gSizer6->Add( logEntry, 0, wxALL|wxEXPAND, 5 );


	loggingPanel->SetSizer( gSizer6 );
	loggingPanel->Layout();
	gSizer6->Fit( loggingPanel );
	m_notebook2->AddPage( loggingPanel, wxT("Logging"), true );
	configPanel = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer5->AddGrowableCol( 0 );
	fgSizer5->AddGrowableRow( 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	additionalSettingsEntry = new wxTextCtrl( configPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
	fgSizer5->Add( additionalSettingsEntry, 0, wxALL|wxEXPAND, 5 );


	configPanel->SetSizer( fgSizer5 );
	configPanel->Layout();
	fgSizer5->Fit( configPanel );
	m_notebook2->AddPage( configPanel, wxT("Extra configuration"), false );
	debugPanel = new wxPanel( m_notebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer91;
	fgSizer91 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer91->AddGrowableCol( 0 );
	fgSizer91->AddGrowableRow( 0 );
	fgSizer91->SetFlexibleDirection( wxBOTH );
	fgSizer91->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	protoConfigEntry = new wxTextCtrl( debugPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	fgSizer91->Add( protoConfigEntry, 0, wxALL|wxEXPAND, 5 );


	debugPanel->SetSizer( fgSizer91 );
	debugPanel->Layout();
	fgSizer91->Fit( debugPanel );
	m_notebook2->AddPage( debugPanel, wxT("Current configuration"), false );

	fgSizer9->Add( m_notebook2, 1, wxEXPAND, 5 );


	this->SetSizer( fgSizer9 );
	this->Layout();
	m_menubar4 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_menu1, wxID_ABOUT, wxString( wxT("About") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem2 );

	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_EXIT, wxString( wxT("E&xit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	m_menubar4->Append( m_menu1, wxT("&File") );

	this->SetMenuBar( m_menubar4 );

	statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

	this->Centre( wxBOTH );

	// Connect Events
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MyFrame4::OnAbout ), this, m_menuItem2->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MyFrame4::OnExit ), this, m_menuItem1->GetId());
}

MyFrame4::~MyFrame4()
{
	// Disconnect Events

}
