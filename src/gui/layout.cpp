///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-234-gd93c9fc0-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "layout.h"

#include "icon.png.h"

///////////////////////////////////////////////////////////////////////////

MainWindowGen::MainWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	menuBar = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem2;
	m_menuItem2 = new wxMenuItem( m_menu1, wxID_ABOUT, wxString( wxT("&About") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem2 );

	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_EXIT, wxString( wxT("E&xit") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );

	menuBar->Append( m_menu1, wxT("&File") );

	m_menu2 = new wxMenu();
	wxMenuItem* m_menuItem3;
	m_menuItem3 = new wxMenuItem( m_menu2, wxID_ANY, wxString( wxT("Show &logs...") ) + wxT('\t') + wxT("CTRL+L"), wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( m_menuItem3 );

	wxMenuItem* m_menuItem4;
	m_menuItem4 = new wxMenuItem( m_menu2, wxID_ANY, wxString( wxT("Show current &configuration...") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu2->Append( m_menuItem4 );

	menuBar->Append( m_menu2, wxT("&View") );

	this->SetMenuBar( menuBar );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	dataNotebook = new wxSimplebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	bSizer4->Add( dataNotebook, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer4 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainWindowGen::OnClose ) );
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnAboutMenuItem ), this, m_menuItem2->GetId());
	m_menu1->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnExit ), this, m_menuItem1->GetId());
	m_menu2->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnShowLogWindow ), this, m_menuItem3->GetId());
	m_menu2->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainWindowGen::OnShowConfigWindow ), this, m_menuItem4->GetId());
}

MainWindowGen::~MainWindowGen()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainWindowGen::OnClose ) );

}

TextViewerWindowGen::TextViewerWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	textControl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH );
	textControl->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizer8->Add( textControl, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2->Realize();

	fgSizer8->Add( m_sdbSizer2, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer8 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( TextViewerWindowGen::OnClose ) );
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextViewerWindowGen::OnClose ), NULL, this );
}

TextViewerWindowGen::~TextViewerWindowGen()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( TextViewerWindowGen::OnClose ) );
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextViewerWindowGen::OnClose ), NULL, this );

}

FluxViewerWindowGen::FluxViewerWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 200,100 ), wxDefaultSize );

	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer5->AddGrowableCol( 0 );
	fgSizer5->AddGrowableRow( 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fluxviewer = new FluxViewerControl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( fluxviewer, 1, wxEXPAND, 5 );

	scrollbar = new wxScrollBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL );
	fgSizer5->Add( scrollbar, 0, wxALIGN_BOTTOM|wxEXPAND, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2->Realize();

	fgSizer5->Add( m_sdbSizer2, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer5 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FluxViewerWindowGen::OnClose ) );
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FluxViewerWindowGen::OnClose ), NULL, this );
}

FluxViewerWindowGen::~FluxViewerWindowGen()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( FluxViewerWindowGen::OnClose ) );
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FluxViewerWindowGen::OnClose ), NULL, this );

}

TextEditorWindowGen::TextEditorWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	textControl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_TAB );
	textControl->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizer8->Add( textControl, 0, wxALL|wxEXPAND, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2Save = new wxButton( this, wxID_SAVE );
	m_sdbSizer2->AddButton( m_sdbSizer2Save );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();

	fgSizer8->Add( m_sdbSizer2, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer8 );
	this->Layout();
	fgSizer8->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( TextEditorWindowGen::OnClose ) );
	m_sdbSizer2Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextEditorWindowGen::OnCancel ), NULL, this );
	m_sdbSizer2Save->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextEditorWindowGen::OnSave ), NULL, this );
}

TextEditorWindowGen::~TextEditorWindowGen()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( TextEditorWindowGen::OnClose ) );
	m_sdbSizer2Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextEditorWindowGen::OnCancel ), NULL, this );
	m_sdbSizer2Save->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TextEditorWindowGen::OnSave ), NULL, this );

}

FileViewerWindowGen::FileViewerWindowGen( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panel8 = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer101;
	gSizer101 = new wxGridSizer( 1, 1, 0, 0 );

	textControl = new wxTextCtrl( m_panel8, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	textControl->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	gSizer101->Add( textControl, 0, wxALL|wxEXPAND, 5 );


	m_panel8->SetSizer( gSizer101 );
	m_panel8->Layout();
	gSizer101->Fit( m_panel8 );
	m_notebook1->AddPage( m_panel8, wxT("Text"), false );
	m_panel7 = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxGridSizer* gSizer10;
	gSizer10 = new wxGridSizer( 1, 1, 0, 0 );

	hexControl = new wxTextCtrl( m_panel7, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	hexControl->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	gSizer10->Add( hexControl, 0, wxALL|wxEXPAND, 5 );


	m_panel7->SetSizer( gSizer10 );
	m_panel7->Layout();
	gSizer10->Fit( m_panel7 );
	m_notebook1->AddPage( m_panel7, wxT("Hex"), false );

	fgSizer8->Add( m_notebook1, 1, wxEXPAND | wxALL, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2->Realize();

	fgSizer8->Add( m_sdbSizer2, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer8 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FileViewerWindowGen::OnClose ), NULL, this );
}

FileViewerWindowGen::~FileViewerWindowGen()
{
	// Disconnect Events
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( FileViewerWindowGen::OnClose ), NULL, this );

}

GetfileDialog::GetfileDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("File on disk:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbSizer1->Add( m_staticText7, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	filenameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), wxTE_READONLY );
	filenameText->Enable( false );

	gbSizer1->Add( filenameText, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("File to save as:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	gbSizer1->Add( m_staticText9, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	targetFilePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a file"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_OVERWRITE_PROMPT|wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	gbSizer1->Add( targetFilePicker, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	buttons_ = new wxStdDialogButtonSizer();
	buttons_OK = new wxButton( this, wxID_OK );
	buttons_->AddButton( buttons_OK );
	buttons_Cancel = new wxButton( this, wxID_CANCEL );
	buttons_->AddButton( buttons_Cancel );
	buttons_->Realize();

	gbSizer1->Add( buttons_, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

GetfileDialog::~GetfileDialog()
{
}

FileConflictDialog::FileConflictDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( this, wxID_ANY, wxT("That name is already in use.\nPlease specify another:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText91->Wrap( -1 );
	gbSizer1->Add( m_staticText91, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Old name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbSizer1->Add( m_staticText7, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	newNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
	gbSizer1->Add( newNameText, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("New name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	gbSizer1->Add( m_staticText9, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	oldNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	oldNameText->Enable( false );

	gbSizer1->Add( oldNameText, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	buttons_ = new wxStdDialogButtonSizer();
	buttons_OK = new wxButton( this, wxID_OK );
	buttons_->AddButton( buttons_OK );
	buttons_Cancel = new wxButton( this, wxID_CANCEL );
	buttons_->AddButton( buttons_Cancel );
	buttons_->Realize();

	gbSizer1->Add( buttons_, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

FileConflictDialog::~FileConflictDialog()
{
}

FileRenameDialog::FileRenameDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( this, wxID_ANY, wxT("Please specify the new name\n(and path) of the file."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText91->Wrap( -1 );
	gbSizer1->Add( m_staticText91, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Old name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbSizer1->Add( m_staticText7, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	newNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
	gbSizer1->Add( newNameText, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("New name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	gbSizer1->Add( m_staticText9, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	oldNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	oldNameText->Enable( false );

	gbSizer1->Add( oldNameText, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	buttons_ = new wxStdDialogButtonSizer();
	buttons_OK = new wxButton( this, wxID_OK );
	buttons_->AddButton( buttons_OK );
	buttons_Cancel = new wxButton( this, wxID_CANCEL );
	buttons_->AddButton( buttons_Cancel );
	buttons_->Realize();

	gbSizer1->Add( buttons_, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

FileRenameDialog::~FileRenameDialog()
{
}

CreateDirectoryDialog::CreateDirectoryDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( this, wxID_ANY, wxT("Please specify the name (and path)\nof the new directory."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText91->Wrap( -1 );
	gbSizer1->Add( m_staticText91, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	newNameText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 300,-1 ), 0 );
	gbSizer1->Add( newNameText, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_staticText9 = new wxStaticText( this, wxID_ANY, wxT("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	gbSizer1->Add( m_staticText9, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	buttons_ = new wxStdDialogButtonSizer();
	buttons_OK = new wxButton( this, wxID_OK );
	buttons_->AddButton( buttons_OK );
	buttons_Cancel = new wxButton( this, wxID_CANCEL );
	buttons_->AddButton( buttons_Cancel );
	buttons_->Realize();

	gbSizer1->Add( buttons_, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

CreateDirectoryDialog::~CreateDirectoryDialog()
{
}

FormatDialog::FormatDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText91 = new wxStaticText( this, wxID_ANY, wxT("This will erase the entire disk. (But remember that\nyou can always undo changes until you press the 'commit changes' button.)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText91->Wrap( -1 );
	gbSizer1->Add( m_staticText91, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	m_staticText7 = new wxStaticText( this, wxID_ANY, wxT("Volume name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	gbSizer1->Add( m_staticText7, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	volumeNameText = new wxTextCtrl( this, wxID_ANY, wxT("FluxEngineDisk"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( volumeNameText, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	quickFormatCheckBox = new wxCheckBox( this, wxID_ANY, wxT("Quick format: if the disk is not already correctly formatted,\nvery bad things will happen!"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( quickFormatCheckBox, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	buttons_ = new wxStdDialogButtonSizer();
	buttons_OK = new wxButton( this, wxID_OK );
	buttons_->AddButton( buttons_OK );
	buttons_Cancel = new wxButton( this, wxID_CANCEL );
	buttons_->AddButton( buttons_Cancel );
	buttons_->Realize();

	gbSizer1->Add( buttons_, wxGBPosition( 3, 0 ), wxGBSpan( 1, 2 ), wxALL|wxEXPAND, 5 );


	this->SetSizer( gbSizer1 );
	this->Layout();
	gbSizer1->Fit( this );

	this->Centre( wxBOTH );
}

FormatDialog::~FormatDialog()
{
}

IdlePanelGen::IdlePanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer8->AddGrowableCol( 0 );
	fgSizer8->AddGrowableRow( 1 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	fgSizer8->SetMinSize( wxSize( 400,-1 ) );
	applicationBitmap = new wxStaticBitmap( this, wxID_ANY, icon_png_to_wx_bitmap(), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( applicationBitmap, 1, wxALIGN_CENTER|wxALL, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Source / destination") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer12->AddGrowableCol( 0 );
	fgSizer12->AddGrowableRow( 1 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	sourceIconPanel = new wxPanel( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxWrapSizer* wSizer3;
	wSizer3 = new wxWrapSizer( wxHORIZONTAL, 0 );


	sourceIconPanel->SetSizer( wSizer3 );
	sourceIconPanel->Layout();
	wSizer3->Fit( sourceIconPanel );
	fgSizer12->Add( sourceIconPanel, 1, wxEXPAND | wxALL, 5 );

	sourceBook = new wxSimplebook( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	fgSizer12->Add( sourceBook, 1, wxEXPAND | wxALL, 5 );


	sbSizer1->Add( fgSizer12, 1, wxEXPAND, 5 );


	fgSizer8->Add( sbSizer1, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Format and format options") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 2, 1, 0, 0 );
	fgSizer11->AddGrowableCol( 0 );
	fgSizer11->AddGrowableRow( 1 );
	fgSizer11->SetFlexibleDirection( wxVERTICAL );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 1, 2, 0, 0 );
	fgSizer6->AddGrowableCol( 0 );
	fgSizer6->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxArrayString formatChoiceChoices;
	formatChoice = new wxChoice( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, formatChoiceChoices, 0 );
	formatChoice->SetSelection( 0 );
	formatChoice->SetToolTip( wxT("The format of the disk.") );

	fgSizer6->Add( formatChoice, 0, wxALL|wxEXPAND, 5 );

	customConfigurationButton = new wxButton( sbSizer2->GetStaticBox(), wxID_ANY, wxT("Customise..."), wxDefaultPosition, wxDefaultSize, 0 );
	customConfigurationButton->SetToolTip( wxT("Allows you to enter arbitrary configuration parameters.") );

	fgSizer6->Add( customConfigurationButton, 0, wxALL|wxEXPAND, 5 );


	fgSizer11->Add( fgSizer6, 1, wxEXPAND, 5 );

	formatOptionsContainer = new wxPanel( sbSizer2->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );


	formatOptionsContainer->SetSizer( bSizer7 );
	formatOptionsContainer->Layout();
	bSizer7->Fit( formatOptionsContainer );
	fgSizer11->Add( formatOptionsContainer, 1, wxALL|wxEXPAND, 5 );


	sbSizer2->Add( fgSizer11, 1, wxEXPAND, 5 );


	fgSizer8->Add( sbSizer2, 1, wxALL|wxEXPAND, 5 );

	wxGridSizer* gSizer9;
	gSizer9 = new wxGridSizer( 1, 5, 0, 0 );

	readButton = new wxButton( this, wxID_ANY, wxT("Read disk"), wxDefaultPosition, wxDefaultSize, 0 );
	readButton->SetLabelMarkup( wxT("Read disk") );
	readButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_OPEN, wxART_TOOLBAR ) );
	readButton->SetToolTip( wxT("Read and decode, producing a disk image from a real disk or flux file.") );

	gSizer9->Add( readButton, 0, wxALL|wxEXPAND, 5 );

	writeButton = new wxButton( this, wxID_ANY, wxT("Write disk"), wxDefaultPosition, wxDefaultSize, 0 );

	writeButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_TOOLBAR ) );
	writeButton->SetBitmapDisabled( wxNullBitmap );
	writeButton->SetToolTip( wxT("Encode and write to either a real disk or a flux file.") );

	gSizer9->Add( writeButton, 0, wxALL|wxEXPAND, 5 );

	browseButton = new wxButton( this, wxID_ANY, wxT("Browse files"), wxDefaultPosition, wxDefaultSize, 0 );

	browseButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FOLDER_OPEN, wxART_TOOLBAR ) );
	browseButton->SetToolTip( wxT("Access the files on the disk directly without needing to image it.") );

	gSizer9->Add( browseButton, 0, wxALL|wxEXPAND, 5 );

	formatButton = new wxButton( this, wxID_ANY, wxT("Format disk"), wxDefaultPosition, wxDefaultSize, 0 );

	formatButton->SetBitmap( wxArtProvider::GetBitmap( wxART_DELETE, wxART_BUTTON ) );
	gSizer9->Add( formatButton, 0, wxALL|wxEXPAND, 5 );

	exploreButton = new wxButton( this, wxID_ANY, wxT("Explore disk"), wxDefaultPosition, wxDefaultSize, 0 );

	exploreButton->SetBitmap( wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_TOOLBAR ) );
	gSizer9->Add( exploreButton, 0, wxALL|wxEXPAND, 5 );


	fgSizer8->Add( gSizer9, 1, wxEXPAND, 5 );


	this->SetSizer( fgSizer8 );
	this->Layout();
	fgSizer8->Fit( this );

	// Connect Events
	formatChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( IdlePanelGen::OnControlsChanged ), NULL, this );
	customConfigurationButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnCustomConfigurationButton ), NULL, this );
	readButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnReadButton ), NULL, this );
	writeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnWriteButton ), NULL, this );
	browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnBrowseButton ), NULL, this );
	formatButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnFormatButton ), NULL, this );
	exploreButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnExploreButton ), NULL, this );
}

IdlePanelGen::~IdlePanelGen()
{
	// Disconnect Events
	formatChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( IdlePanelGen::OnControlsChanged ), NULL, this );
	customConfigurationButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnCustomConfigurationButton ), NULL, this );
	readButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnReadButton ), NULL, this );
	writeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnWriteButton ), NULL, this );
	browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnBrowseButton ), NULL, this );
	formatButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnFormatButton ), NULL, this );
	exploreButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( IdlePanelGen::OnExploreButton ), NULL, this );

}

ImagerPanelGen::ImagerPanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );

	imagerToolbar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT|wxAUI_TB_TEXT );
	imagerBackTool = imagerToolbar->AddTool( wxID_ANY, wxT("Back"), wxArtProvider::GetBitmap( wxART_GO_BACK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	imagerToolbar->Realize();

	bSizer41->Add( imagerToolbar, 0, wxEXPAND, 5 );

	wxGridSizer* gSizer122;
	gSizer122 = new wxGridSizer( 0, 2, 0, 0 );

	wxGridSizer* gSizer5;
	gSizer5 = new wxGridSizer( 0, 1, 0, 0 );

	visualiser = new VisualisationControl( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxBORDER_THEME );
	gSizer5->Add( visualiser, 0, wxALL|wxEXPAND, 5 );


	gSizer122->Add( gSizer5, 1, wxEXPAND, 5 );

	wxGridSizer* gSizer7;
	gSizer7 = new wxGridSizer( 1, 1, 0, 0 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	imagerSaveImageButton = new wxButton( this, wxID_ANY, wxT("Save decoded image"), wxDefaultPosition, wxDefaultSize, 0 );

	imagerSaveImageButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_BUTTON ) );
	bSizer8->Add( imagerSaveImageButton, 0, wxALL|wxEXPAND, 5 );

	imagerSaveFluxButton = new wxButton( this, wxID_ANY, wxT("Save raw flux"), wxDefaultPosition, wxDefaultSize, 0 );

	imagerSaveFluxButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE_AS, wxART_BUTTON ) );
	bSizer8->Add( imagerSaveFluxButton, 0, wxALL|wxEXPAND, 5 );

	histogram = new HistogramViewer( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( histogram, 0, wxALL|wxEXPAND, 5 );

	imagerGoAgainButton = new wxButton( this, wxID_ANY, wxT("Go again"), wxDefaultPosition, wxDefaultSize, 0 );

	imagerGoAgainButton->SetBitmap( wxArtProvider::GetBitmap( wxART_REDO, wxART_BUTTON ) );
	bSizer8->Add( imagerGoAgainButton, 0, wxALL|wxEXPAND, 5 );


	gSizer7->Add( bSizer8, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5 );


	gSizer122->Add( gSizer7, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bSizer41->Add( gSizer122, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer41 );
	this->Layout();

	// Connect Events
	this->Connect( imagerBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnBackButton ) );
	imagerSaveImageButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnSaveImageButton ), NULL, this );
	imagerSaveFluxButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnSaveFluxButton ), NULL, this );
	imagerGoAgainButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnImagerGoAgainButton ), NULL, this );
}

ImagerPanelGen::~ImagerPanelGen()
{
	// Disconnect Events
	this->Disconnect( imagerBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnBackButton ) );
	imagerSaveImageButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnSaveImageButton ), NULL, this );
	imagerSaveFluxButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnSaveFluxButton ), NULL, this );
	imagerGoAgainButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ImagerPanelGen::OnImagerGoAgainButton ), NULL, this );

}

BrowserPanelGen::BrowserPanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxFlexGridSizer* fgSizer23;
	fgSizer23 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer23->AddGrowableCol( 0 );
	fgSizer23->AddGrowableRow( 1 );
	fgSizer23->SetFlexibleDirection( wxBOTH );
	fgSizer23->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	browserToolbar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT|wxAUI_TB_TEXT );
	browserBackTool = browserToolbar->AddTool( wxID_ANY, wxT("Back"), wxArtProvider::GetBitmap( wxART_GO_BACK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browserToolbar->AddSeparator();

	browserInfoTool = browserToolbar->AddTool( wxID_ANY, wxT("Info"), wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browserViewTool = browserToolbar->AddTool( wxID_ANY, wxT("View"), wxArtProvider::GetBitmap( wxART_FILE_OPEN, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browserSaveTool = browserToolbar->AddTool( wxID_ANY, wxT("Save"), wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browserMoreMenuButton = browserToolbar->AddTool( wxID_ANY, wxT("More"), wxArtProvider::GetBitmap( wxART_PLUS, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );
	browserToolbar->SetToolDropDown( browserMoreMenuButton->GetId(), true );

	browserMoreMenu = new wxMenu();
	browserAddMenuItem = new wxMenuItem( browserMoreMenu, wxID_ANY, wxString( wxT("Add file") ) , wxEmptyString, wxITEM_NORMAL );
	browserMoreMenu->Append( browserAddMenuItem );

	browserNewDirectoryMenuItem = new wxMenuItem( browserMoreMenu, wxID_ANY, wxString( wxT("New directory") ) , wxEmptyString, wxITEM_NORMAL );
	#ifdef __WXMSW__
	browserNewDirectoryMenuItem->SetBitmaps( wxNullBitmap );
	#elif (defined( __WXGTK__ ) || defined( __WXOSX__ ))
	browserNewDirectoryMenuItem->SetBitmap( wxNullBitmap );
	#endif
	browserMoreMenu->Append( browserNewDirectoryMenuItem );

	browserRenameMenuItem = new wxMenuItem( browserMoreMenu, wxID_ANY, wxString( wxT("Move/Rename file") ) , wxEmptyString, wxITEM_NORMAL );
	#ifdef __WXMSW__
	browserRenameMenuItem->SetBitmaps( wxNullBitmap );
	#elif (defined( __WXGTK__ ) || defined( __WXOSX__ ))
	browserRenameMenuItem->SetBitmap( wxNullBitmap );
	#endif
	browserMoreMenu->Append( browserRenameMenuItem );

	browserDeleteMenuItem = new wxMenuItem( browserMoreMenu, wxID_ANY, wxString( wxT("Delete file") ) , wxEmptyString, wxITEM_NORMAL );
	#ifdef __WXMSW__
	browserDeleteMenuItem->SetBitmaps( wxNullBitmap );
	#elif (defined( __WXGTK__ ) || defined( __WXOSX__ ))
	browserDeleteMenuItem->SetBitmap( wxNullBitmap );
	#endif
	browserMoreMenu->Append( browserDeleteMenuItem );

	browserToolbar->Connect( browserMoreMenuButton->GetId(), wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN, wxAuiToolBarEventHandler( BrowserPanelGen::browserMoreMenuButtonOnDropDownMenu ), NULL, this );


	browserToolbar->AddSeparator();

	browserFormatTool = browserToolbar->AddTool( wxID_ANY, wxT("Format"), wxArtProvider::GetBitmap( wxART_DELETE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	browserToolbar->Realize();

	fgSizer23->Add( browserToolbar, 0, wxEXPAND, 5 );

	browserTree = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE );
	m_dataViewColumn1 = browserTree->AppendIconTextColumn( wxT("Filename"), 0, wxDATAVIEW_CELL_EDITABLE, 250, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
	m_dataViewColumn2 = browserTree->AppendTextColumn( wxT("Size"), 1, wxDATAVIEW_CELL_INERT, 100, static_cast<wxAlignment>(wxALIGN_RIGHT), wxDATAVIEW_COL_RESIZABLE );
	m_dataViewColumn3 = browserTree->AppendTextColumn( wxT("Mode"), 2, wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
	fgSizer23->Add( browserTree, 0, wxALL|wxEXPAND, 5 );

	diskSpaceGauge = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH );
	diskSpaceGauge->SetValue( 0 );
	fgSizer23->Add( diskSpaceGauge, 0, wxALL|wxEXPAND, 5 );

	wxGridSizer* gSizer12;
	gSizer12 = new wxGridSizer( 0, 2, 0, 0 );

	browserDiscardButton = new wxButton( this, wxID_ANY, wxT("Discard pending changes"), wxDefaultPosition, wxDefaultSize, 0 );

	browserDiscardButton->SetBitmap( wxArtProvider::GetBitmap( wxART_WARNING, wxART_BUTTON ) );
	gSizer12->Add( browserDiscardButton, 0, wxALIGN_CENTER|wxALL, 5 );

	browserCommitButton = new wxButton( this, wxID_ANY, wxT("Commit pending changes to disk"), wxDefaultPosition, wxDefaultSize, 0 );

	browserCommitButton->SetBitmap( wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_BUTTON ) );
	gSizer12->Add( browserCommitButton, 0, wxALIGN_CENTER|wxALL, 5 );


	fgSizer23->Add( gSizer12, 1, wxEXPAND, 5 );

	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("No changes will be written until the 'commit' button is pressed."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL );
	m_staticText12->Wrap( -1 );
	fgSizer23->Add( m_staticText12, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( fgSizer23 );
	this->Layout();

	// Connect Events
	this->Connect( browserBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBackButton ) );
	this->Connect( browserInfoTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserInfoButton ) );
	this->Connect( browserViewTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserViewButton ) );
	this->Connect( browserSaveTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserSaveButton ) );
	browserMoreMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( BrowserPanelGen::OnBrowserAddMenuItem ), this, browserAddMenuItem->GetId());
	browserMoreMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( BrowserPanelGen::OnBrowserNewDirectoryMenuItem ), this, browserNewDirectoryMenuItem->GetId());
	browserMoreMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( BrowserPanelGen::OnBrowserRenameMenuItem ), this, browserRenameMenuItem->GetId());
	browserMoreMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( BrowserPanelGen::OnBrowserDeleteMenuItem ), this, browserDeleteMenuItem->GetId());
	this->Connect( browserFormatTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserFormatButton ) );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_BEGIN_DRAG, wxDataViewEventHandler( BrowserPanelGen::OnBrowserBeginDrag ), NULL, this );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_DROP, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDrop ), NULL, this );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_DROP_POSSIBLE, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDropPossible ), NULL, this );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_EXPANDING, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDirectoryExpanding ), NULL, this );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( BrowserPanelGen::OnBrowserFilenameChanged ), NULL, this );
	browserTree->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( BrowserPanelGen::OnBrowserSelectionChanged ), NULL, this );
	browserDiscardButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserDiscardButton ), NULL, this );
	browserCommitButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserCommitButton ), NULL, this );
}

BrowserPanelGen::~BrowserPanelGen()
{
	// Disconnect Events
	this->Disconnect( browserBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBackButton ) );
	this->Disconnect( browserInfoTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserInfoButton ) );
	this->Disconnect( browserViewTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserViewButton ) );
	this->Disconnect( browserSaveTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserSaveButton ) );
	this->Disconnect( browserFormatTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserFormatButton ) );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_BEGIN_DRAG, wxDataViewEventHandler( BrowserPanelGen::OnBrowserBeginDrag ), NULL, this );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_DROP, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDrop ), NULL, this );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_DROP_POSSIBLE, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDropPossible ), NULL, this );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_EXPANDING, wxDataViewEventHandler( BrowserPanelGen::OnBrowserDirectoryExpanding ), NULL, this );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_ITEM_VALUE_CHANGED, wxDataViewEventHandler( BrowserPanelGen::OnBrowserFilenameChanged ), NULL, this );
	browserTree->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( BrowserPanelGen::OnBrowserSelectionChanged ), NULL, this );
	browserDiscardButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserDiscardButton ), NULL, this );
	browserCommitButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BrowserPanelGen::OnBrowserCommitButton ), NULL, this );

	delete browserMoreMenu;
}

ExplorerPanelGen::ExplorerPanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer411;
	bSizer411 = new wxBoxSizer( wxVERTICAL );

	explorerToolbar = new wxAuiToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT|wxAUI_TB_TEXT );
	explorerBackTool = explorerToolbar->AddTool( wxID_ANY, wxT("Back"), wxArtProvider::GetBitmap( wxART_GO_BACK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	explorerRefreshTool = explorerToolbar->AddTool( wxID_ANY, wxT("Refresh"), wxArtProvider::GetBitmap( wxART_REDO, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	explorerToolbar->Realize();

	bSizer411->Add( explorerToolbar, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer12;
	fgSizer12 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer12->AddGrowableCol( 1 );
	fgSizer12->AddGrowableRow( 0 );
	fgSizer12->SetFlexibleDirection( wxBOTH );
	fgSizer12->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizer13;
	fgSizer13 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer13->AddGrowableCol( 0 );
	fgSizer13->AddGrowableRow( 1 );
	fgSizer13->SetFlexibleDirection( wxBOTH );
	fgSizer13->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer10->AddGrowableCol( 1 );
	fgSizer10->SetFlexibleDirection( wxHORIZONTAL );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText22 = new wxStaticText( this, wxID_ANY, wxT("Track"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer10->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	explorerTrackSpinCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 82, 0 );
	fgSizer10->Add( explorerTrackSpinCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText26 = new wxStaticText( this, wxID_ANY, wxT("Side"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	fgSizer10->Add( m_staticText26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	explorerSideSpinCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 0 );
	fgSizer10->Add( explorerSideSpinCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText231 = new wxStaticText( this, wxID_ANY, wxT("Start time (ms)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText231->Wrap( -1 );
	fgSizer10->Add( m_staticText231, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	explorerStartTimeSpinCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999, 0.000000, 1 );
	explorerStartTimeSpinCtrl->SetDigits( 3 );
	fgSizer10->Add( explorerStartTimeSpinCtrl, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer121;
	fgSizer121 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer121->AddGrowableCol( 0 );
	fgSizer121->AddGrowableRow( 0 );
	fgSizer121->SetFlexibleDirection( wxBOTH );
	fgSizer121->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText24 = new wxStaticText( this, wxID_ANY, wxT("Clock (us)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgSizer121->Add( m_staticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	guessButton = new wxButton( this, wxID_ANY, wxT("Guess"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	fgSizer121->Add( guessButton, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );


	fgSizer10->Add( fgSizer121, 1, wxEXPAND, 5 );

	explorerClockSpinCtrl = new wxSpinCtrlDouble( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 4, 1 );
	explorerClockSpinCtrl->SetDigits( 1 );
	fgSizer10->Add( explorerClockSpinCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText25 = new wxStaticText( this, wxID_ANY, wxT("Raw bit offset"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgSizer10->Add( m_staticText25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	explorerBitOffsetSpinCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 99999999, 0 );
	fgSizer10->Add( explorerBitOffsetSpinCtrl, 0, wxALL|wxEXPAND, 5 );

	m_staticText27 = new wxStaticText( this, wxID_ANY, wxT("Decode as"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	fgSizer10->Add( m_staticText27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString explorerDecodeChoiceChoices[] = { wxT("Unencoded bits"), wxT("Unencoded bytes"), wxT("MFM / M²FM / FM") };
	int explorerDecodeChoiceNChoices = sizeof( explorerDecodeChoiceChoices ) / sizeof( wxString );
	explorerDecodeChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, explorerDecodeChoiceNChoices, explorerDecodeChoiceChoices, 0 );
	explorerDecodeChoice->SetSelection( 0 );
	fgSizer10->Add( explorerDecodeChoice, 0, wxALL|wxEXPAND, 5 );

	m_staticText241 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText241->Wrap( -1 );
	fgSizer10->Add( m_staticText241, 0, wxALL, 5 );

	explorerReverseCheckBox = new wxCheckBox( this, wxID_ANY, wxT("Reverse bytes"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer10->Add( explorerReverseCheckBox, 0, wxALL, 5 );


	fgSizer13->Add( fgSizer10, 1, wxEXPAND, 5 );

	histogram = new HistogramViewer( this, wxID_ANY, wxDefaultPosition, wxSize( 256,100 ), 0 );
	fgSizer13->Add( histogram, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxALL, 5 );


	fgSizer12->Add( fgSizer13, 1, wxEXPAND, 5 );

	explorerText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	explorerText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	fgSizer12->Add( explorerText, 0, wxALL|wxEXPAND, 5 );


	bSizer411->Add( fgSizer12, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer411 );
	this->Layout();

	// Connect Events
	this->Connect( explorerBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnBackButton ) );
	this->Connect( explorerRefreshTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerRefreshButton ) );
	explorerTrackSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerSideSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerStartTimeSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	guessButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnGuessClockButton ), NULL, this );
	explorerClockSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerBitOffsetSpinCtrl->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerDecodeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerReverseCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
}

ExplorerPanelGen::~ExplorerPanelGen()
{
	// Disconnect Events
	this->Disconnect( explorerBackTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnBackButton ) );
	this->Disconnect( explorerRefreshTool->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerRefreshButton ) );
	explorerTrackSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerSideSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerStartTimeSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	guessButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnGuessClockButton ), NULL, this );
	explorerClockSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRLDOUBLE_UPDATED, wxSpinDoubleEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerBitOffsetSpinCtrl->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerDecodeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );
	explorerReverseCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( ExplorerPanelGen::OnExplorerSettingChange ), NULL, this );

}

HardwareSourcePanelGen::HardwareSourcePanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticText30 = new wxStaticText( this, wxID_ANY, wxT("Read from or write to real floppy disk:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText30->Wrap( -1 );
	bSizer3->Add( m_staticText30, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	label = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	label->Wrap( -1 );
	label->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer3->Add( label, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	highDensityToggle = new wxCheckBox( this, wxID_ANY, wxT("This is a high density disk"), wxDefaultPosition, wxDefaultSize, 0 );
	highDensityToggle->SetToolTip( wxT("If you are using a high density disk, select this.\nThis can be detected automatically for 3.5\"\ndisks but needs to be set manually for everything\nelse.") );

	bSizer3->Add( highDensityToggle, 0, wxALL|wxEXPAND, 5 );

	wxFlexGridSizer* fgSizer14;
	fgSizer14 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer14->AddGrowableCol( 1 );
	fgSizer14->SetFlexibleDirection( wxBOTH );
	fgSizer14->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText29 = new wxStaticText( this, wxID_ANY, wxT("Drive type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText29->Wrap( -1 );
	fgSizer14->Add( m_staticText29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxArrayString driveTypeChoiceChoices;
	driveTypeChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, driveTypeChoiceChoices, 0 );
	driveTypeChoice->SetSelection( 0 );
	fgSizer14->Add( driveTypeChoice, 0, wxALL|wxEXPAND, 5 );


	bSizer3->Add( fgSizer14, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer3 );
	this->Layout();
	bSizer3->Fit( this );
}

HardwareSourcePanelGen::~HardwareSourcePanelGen()
{
}

FluxfileSourcePanelGen::FluxfileSourcePanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_staticText28 = new wxStaticText( this, wxID_ANY, wxT("Read or modify flux-level data from file."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	bSizer8->Add( m_staticText28, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	m_staticText27 = new wxStaticText( this, wxID_ANY, wxT("Filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText27->Wrap( -1 );
	bSizer8->Add( m_staticText27, 0, wxALL, 5 );

	fluxImagePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a file"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_OPEN|wxFLP_USE_TEXTCTRL );
	fluxImagePicker->SetToolTip( wxT("Path to a .flux, .scp or other flux file.") );

	bSizer8->Add( fluxImagePicker, 0, wxALL|wxEXPAND, 5 );

	m_staticText281 = new wxStaticText( this, wxID_ANY, wxT("Format:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText281->Wrap( -1 );
	bSizer8->Add( m_staticText281, 0, wxALL, 5 );

	wxArrayString fluxImageFormatChoices;
	fluxImageFormat = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, fluxImageFormatChoices, 0 );
	fluxImageFormat->SetSelection( 0 );
	bSizer8->Add( fluxImageFormat, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizer8 );
	this->Layout();
	bSizer8->Fit( this );
}

FluxfileSourcePanelGen::~FluxfileSourcePanelGen()
{
}

ImagefileSourcePanelGen::ImagefileSourcePanelGen( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_staticText29 = new wxStaticText( this, wxID_ANY, wxT("Read or modify sector-level data from file."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText29->Wrap( -1 );
	bSizer9->Add( m_staticText29, 0, wxALIGN_LEFT|wxALL|wxEXPAND, 5 );

	m_staticText28 = new wxStaticText( this, wxID_ANY, wxT("Filename:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText28->Wrap( -1 );
	bSizer9->Add( m_staticText28, 0, wxALL, 5 );

	diskImagePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a file"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_OPEN|wxFLP_USE_TEXTCTRL );
	diskImagePicker->SetToolTip( wxT("The path to the disk image.") );

	bSizer9->Add( diskImagePicker, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizer9 );
	this->Layout();
	bSizer9->Fit( this );
}

ImagefileSourcePanelGen::~ImagefileSourcePanelGen()
{
}
