// Dialog_Save_As_DSMod.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Save_As_DSMod.h"
#include "Dialog_Build_Tank.h"
#include "TankMgr.h"
#include "Config.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_As_DSMod dialog


Dialog_Save_As_DSMod::Dialog_Save_As_DSMod(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Save_As_DSMod::IDD, pParent)
{
	Registry prefs( gAppModule.GetConfigInit().m_CompanyName, gAppModule.GetConfigInit().m_RegistryName, true );

	//{{AFX_DATA_INIT(Dialog_Save_As_DSMod)
	m_SourceFolder		= prefs.GetString( "Dialogs/Save_As_DSMod/SourceFolder" );
	m_DstFileName		= prefs.GetString( "Dialogs/Save_As_DSMod/DstFileName" );
	m_OutputPrefix		= prefs.GetString( "Dialogs/Save_As_DSMod/OutputPrefix" );
	m_Title				= prefs.GetString( "Dialogs/Save_As_DSMod/Title" );
	m_Author			= prefs.GetString( "Dialogs/Save_As_DSMod/Author", SysInfo::GetUserName() );
	m_Copyright			= prefs.GetString( "Dialogs/Save_As_DSMod/Copyright" );
	m_MinVersion		= _T("");
	m_Description		= prefs.GetString( "Dialogs/Save_As_DSMod/Description" );
	m_Priority			= prefs.GetString( "Dialogs/Save_As_DSMod/Priority", "User" );
	m_LqdInclude		= prefs.GetBool  ( "Dialogs/Save_As_DSMod/LqdInclude", false );
	m_LqdCompile		= prefs.GetBool  ( "Dialogs/Save_As_DSMod/LqdCompile", false );
	m_LqdDelete			= prefs.GetBool  ( "Dialogs/Save_As_DSMod/LqdDelete", false );
	m_DevOnly			= prefs.GetBool  ( "Dialogs/Save_As_DSMod/DevOnly", false );
	m_CompressFiles		= prefs.GetBool  ( "Dialogs/Save_As_DSMod/CompressFiles", true );
	m_ReprocessFiles	= prefs.GetBool  ( "Dialogs/Save_As_DSMod/ReprocessFiles", true );
	m_ProtectedContent	= prefs.GetBool  ( "Dialogs/Save_As_DSMod/ProtectedContent", false );
	m_VerifyTank		= prefs.GetBool  ( "Dialogs/Save_As_DSMod/VerifyTank", true );
	m_UseBackup			= prefs.GetBool  ( "Dialogs/Save_As_DSMod/UseBackup", true );
	m_GenerateDump		= prefs.GetBool  ( "Dialogs/Save_As_DSMod/GenerateDump", true );
	//}}AFX_DATA_INIT

	SetDefaultCopyrightText();

	gpversion version = gTankMgr.GetNewestMinVersion();
	if ( !version.IsZero() )
	{
		m_MinVersion = ::ToString( version, gpversion::MODE_MSQA_ONLY_LONG );
	}
	else
	{
		// try using DS EXE
		m_MinVersion = gConfig.ResolvePathVars( "%ds_exe_path%/DungeonSiege.exe", Config::PVM_NONE );

		// fall back to SE detected version
		if ( m_MinVersion.IsEmpty() )
		{
			m_MinVersion = ::GetDungeonSiegeVersionText( gpversion::MODE_MSQA_ONLY_LONG );
		}
	}
}


void Dialog_Save_As_DSMod::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Save_As_DSMod)
	DDX_Text(pDX, IDC_SOURCE_FOLDER_CHOICE, m_SourceFolder);
	DDX_Text(pDX, IDC_DEST_FILE_CHOICE, m_DstFileName);
	DDX_Text(pDX, IDC_OUTPUT_PREFIX, m_OutputPrefix);
	DDX_Text(pDX, IDC_TITLE, m_Title);
	DDX_Text(pDX, IDC_AUTHOR, m_Author);
	DDX_Text(pDX, IDC_COPYRIGHT, m_Copyright);
	DDX_Text(pDX, IDC_MIN_VERSION, m_MinVersion);
	DDX_Text(pDX, IDC_DESCRIPTION, m_Description);
	DDX_CBString(pDX, IDC_PRIORITY, m_Priority);
	DDX_Check(pDX, IDC_LQD_INCLUDE, m_LqdInclude);
	DDX_Check(pDX, IDC_LQD_COMPILE, m_LqdCompile);
	DDX_Check(pDX, IDC_LQD_DELETE, m_LqdDelete);
	DDX_Check(pDX, IDC_DEV_ONLY, m_DevOnly);
	DDX_Check(pDX, IDC_COMPRESS_FILES, m_CompressFiles);
	DDX_Check(pDX, IDC_REPROCESS_FILES, m_ReprocessFiles);
	DDX_Check(pDX, IDC_PROTECTED_CONTENT, m_ProtectedContent);
	DDX_Check(pDX, IDC_VERIFY_TANK, m_VerifyTank);
	DDX_Check(pDX, IDC_USE_BACKUP, m_UseBackup);
	DDX_Check(pDX, IDC_GENERATE_DUMP, m_GenerateDump);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Save_As_DSMod, CDialog)
	//{{AFX_MSG_MAP(Dialog_Save_As_DSMod)
	ON_BN_CLICKED(IDC_BROWSE_SOURCE_FOLDER, OnBrowseSourceFolder)
	ON_BN_CLICKED(IDC_BROWSE_DEST_FILE, OnBrowseDestFile)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void Dialog_Save_As_DSMod::SetDefaultCopyrightText()
{
	SYSTEMTIME systemTime;
	::GetLocalTime( &systemTime );

	m_Copyright = gpstringf( "Copyright (c) %d %s", systemTime.wYear, (LPCTSTR)m_Author );
}

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_As_DSMod message handlers

BOOL Dialog_Save_As_DSMod::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem( IDOK )->SetFocus();

	return FALSE;
}

void Dialog_Save_As_DSMod::OnOK()
{
	UpdateData();

	if ( Dialog_Build_Tank( this ).DoModal() == IDOK )
	{
		Registry prefs( gAppModule.GetConfigInit().m_CompanyName, gAppModule.GetConfigInit().m_RegistryName, true );

		prefs.SetString( "Dialogs/Save_As_DSMod/SourceFolder",       m_SourceFolder     );
		prefs.SetString( "Dialogs/Save_As_DSMod/DstFileName",        m_DstFileName      );
		prefs.SetString( "Dialogs/Save_As_DSMod/OutputPrefix",       m_OutputPrefix     );
		prefs.SetString( "Dialogs/Save_As_DSMod/Title",              m_Title            );
		prefs.SetString( "Dialogs/Save_As_DSMod/Author",             m_Author           );
		prefs.SetString( "Dialogs/Save_As_DSMod/Copyright",          m_Copyright        );
		prefs.SetString( "Dialogs/Save_As_DSMod/Description",        m_Description      );
		prefs.SetString( "Dialogs/Save_As_DSMod/Priority",           m_Priority         );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/LqdInclude",       !!m_LqdInclude       );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/LqdCompile",       !!m_LqdCompile       );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/LqdDelete",        !!m_LqdDelete        );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/DevOnly",          !!m_DevOnly          );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/CompressFiles",    !!m_CompressFiles    );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/ReprocessFiles",   !!m_ReprocessFiles   );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/ProtectedContent", !!m_ProtectedContent );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/VerifyTank",       !!m_VerifyTank       );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/UseBackup",        !!m_UseBackup        );
		prefs.SetBool  ( "Dialogs/Save_As_DSMod/GenerateDump",     !!m_GenerateDump     );

		CDialog::OnOK();
	}
}

void Dialog_Save_As_DSMod::OnBrowseSourceFolder()
{
	char buffer[ MAX_PATH ];

	// set up query
	BROWSEINFO info;
	info.hwndOwner		= m_hWnd;
	info.pidlRoot		= NULL;
	info.pszDisplayName	= buffer;
	info.lpszTitle		= "Choose source folder:";
	info.ulFlags		= BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
	info.lpfn			= NULL;
	info.lParam			= NULL;
	info.iImage			= NULL;

	// use dialog
	LPITEMIDLIST pidl = SHBrowseForFolder( &info );
	if ( SHGetPathFromIDList( pidl, buffer ) )
	{
		m_SourceFolder = buffer;
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

void Dialog_Save_As_DSMod::OnBrowseDestFile()
{
	UpdateData();

	char path[ MAX_PATH ];
	::strcpy( path, m_DstFileName );

	// i have no idea why, but MFC crashes if i use CFileDialog. -sb
	OPENFILENAME ofn;
	::ZeroObject( ofn );
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner   = m_hWnd;
	ofn.lpstrFilter = "Dungeon Siege Map Files (*.dsmap)\0*.dsmap\0"
					  "Dungeon Siege Mod Files (*.dsmod)\0*.dsmod\0"
					  "All Files (*.*)\0*.*\0\0";
	ofn.lpstrFile   = path;
	ofn.nMaxFile    = ELEMENT_COUNT( path );
	ofn.lpstrTitle  = "Choose destination file";
	ofn.Flags       = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	// do it
	if ( ::GetSaveFileName( &ofn ) )
	{
		m_DstFileName = path;
		if ( !FileSys::HasExtension( m_DstFileName ) )
		{
			m_DstFileName += ".dsmod";
		}
		UpdateData( FALSE );
	}
}

void Dialog_Save_As_DSMod::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
