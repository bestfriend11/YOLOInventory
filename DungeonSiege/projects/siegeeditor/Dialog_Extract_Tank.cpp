// Dialog_Extract_Tank.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Build_Tank.h"
#include "Dialog_Extract_Tank.h"
#include "Config.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Extract_Tank dialog


Dialog_Extract_Tank::Dialog_Extract_Tank(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Extract_Tank::IDD, pParent)
{
	Registry prefs( gAppModule.GetConfigInit().m_CompanyName, gAppModule.GetConfigInit().m_RegistryName, true );

	//{{AFX_DATA_INIT(Dialog_Extract_Tank)
	m_SourceFile		= prefs.GetString( "Dialogs/Extract_Tank/SourceFile" );
	m_DstFolder			= prefs.GetString( "Dialogs/Extract_Tank/DstFolder", gConfig.ResolvePathVars( "%out%" ) );
	m_DefaultDstFolder  = prefs.GetBool( "Dialogs/Extract_Tank/DefaultDstFolder", true );
	m_OverwriteFiles	= prefs.GetBool( "Dialogs/Extract_Tank/OverwriteFiles", false );
	m_ExtractLqdFiles	= prefs.GetBool( "Dialogs/Extract_Tank/ExtractLqdFiles", false );
	//}}AFX_DATA_INIT
}


void Dialog_Extract_Tank::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Extract_Tank)
	DDX_Text(pDX, IDC_SOURCE_FILE_CHOICE, m_SourceFile);
	DDX_Text(pDX, IDC_DEST_FOLDER_CHOICE, m_DstFolder);
	DDX_Check(pDX, IDC_DEFAULT_DEST_FOLDER, m_DefaultDstFolder);
	DDX_Check(pDX, IDC_OVERWRITE_FILES, m_OverwriteFiles);
	DDX_Check(pDX, IDC_EXTRACT_LQD, m_ExtractLqdFiles);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Extract_Tank, CDialog)
	//{{AFX_MSG_MAP(Dialog_Extract_Tank)
	ON_BN_CLICKED(IDC_BROWSE_SOURCE_FILE, OnBrowseSourceFile)
	ON_BN_CLICKED(IDC_BROWSE_DEST_FOLDER, OnBrowseDestFolder)
	ON_BN_CLICKED(IDC_DEFAULT_DEST_FOLDER, OnDefaultDstFolder)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Extract_Tank message handlers

void Dialog_Extract_Tank::OnBrowseSourceFile() 
{
	UpdateData();

	char path[ MAX_PATH ];
	::strcpy( path, m_SourceFile );
	gpstring startingPath = gConfig.ResolvePathVars( "%ds_exe_path%/Maps/", Config::PVM_NONE );

	// i have no idea why, but MFC crashes if i use CFileDialog. -sb
	OPENFILENAME ofn;
	::ZeroObject( ofn );
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner   = m_hWnd;
	ofn.lpstrFilter = "Dungeon Siege Map Files (*.dsmap)\0*.dsmap\0"
					  "Dungeon Siege Resource Files (*.dsres)\0*.dsres\0"
					  "Dungeon Siege Mod Files (*.dsmod)\0*.dsmod\0"
					  "All Files (*.*)\0*.*\0\0";
	ofn.lpstrFile   = path;
	ofn.nMaxFile    = ELEMENT_COUNT( path );
	ofn.lpstrTitle  = "Choose source file";
	ofn.Flags       = OFN_FILEMUSTEXIST;

	// do it
	if ( ::GetOpenFileName( &ofn ) )
	{
		m_SourceFile = path;
		UpdateData( FALSE );
	}
}

void Dialog_Extract_Tank::OnBrowseDestFolder() 
{
	char buffer[ MAX_PATH ];

	// set up query
	BROWSEINFO info;
	info.hwndOwner		= m_hWnd;
	info.pidlRoot		= NULL;
	info.pszDisplayName	= buffer;
	info.lpszTitle		= "Choose destination folder:";
	info.ulFlags		= BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
	info.lpfn			= NULL;
	info.lParam			= NULL;
	info.iImage			= NULL;

	// use dialog
	LPITEMIDLIST pidl = SHBrowseForFolder( &info );
	if ( SHGetPathFromIDList( pidl, buffer ) )
	{
		m_DstFolder = buffer;
		UpdateData( FALSE );
	}

	// free memory
	if ( pidl != NULL )
	{
		LPMALLOC ptr = NULL;
		if ( SUCCEEDED( SHGetMalloc( &ptr ) ) )
		{
			ptr->Free( pidl );
		}
	}
}

void Dialog_Extract_Tank::OnOK() 
{
	UpdateData();

	if ( Dialog_Build_Tank( this ).DoModal() == IDOK )
	{
		Registry prefs( gAppModule.GetConfigInit().m_CompanyName, gAppModule.GetConfigInit().m_RegistryName, true );

		prefs.SetString( "Dialogs/Extract_Tank/SourceFile",         m_SourceFile       );
		prefs.SetString( "Dialogs/Extract_Tank/DstFolder",          m_DstFolder        );
		prefs.SetBool  ( "Dialogs/Extract_Tank/DefaultDstFolder", !!m_DefaultDstFolder );
		prefs.SetBool  ( "Dialogs/Extract_Tank/OverwriteFiles",   !!m_OverwriteFiles   );
		prefs.SetBool  ( "Dialogs/Extract_Tank/ExtractLqdFiles",  !!m_ExtractLqdFiles  );

		CDialog::OnOK();
	}
}

void Dialog_Extract_Tank::OnDefaultDstFolder() 
{
	UpdateData();
	GetDlgItem( IDC_DEST_FOLDER_CHOICE )->EnableWindow( !m_DefaultDstFolder );
	GetDlgItem( IDC_BROWSE_DEST_FOLDER )->EnableWindow( !m_DefaultDstFolder );
}

BOOL Dialog_Extract_Tank::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	OnDefaultDstFolder();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Extract_Tank::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
