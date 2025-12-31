// Dialog_Save_As_DSMap.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Save_As_DSMap.h"
#include "Dialog_Save_As_DSMod.h"
#include "TankMgr.h"
#include "Config.h"
#include "FileSysUtils.h"
#include "EditorRegion.h"
#include "MenuCommands.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_As_DSMap dialog


Dialog_Save_As_DSMap::Dialog_Save_As_DSMap(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Save_As_DSMap::IDD, pParent)
	, m_DstFileName(_T("Click a map name from above"))
{
	//{{AFX_DATA_INIT(Dialog_Save_Region)
	//}}AFX_DATA_INIT
}

void Dialog_Save_As_DSMap::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(Dialog_Save_As_DSMap)
	DDX_Control(pDX, IDOK, m_OkButton);
	DDX_Control(pDX, IDC_MAP_CHOICE, m_MapChoice);
	DDX_Text(pDX, IDC_DEST_FILE_CHOICE, m_DstFileName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Dialog_Save_As_DSMap, CDialog)
	//{{AFX_MSG_MAP(Dialog_Save_As_DSMap)
	ON_BN_CLICKED(IDC_BROWSE_DEST_FILE, OnBnClickedBrowseDestFile)
	ON_NOTIFY(NM_CLICK, IDC_MAP_CHOICE, OnClickMapChoice)
	ON_EN_CHANGE(IDC_DEST_FILE_CHOICE, OnChangeDestFileChoice)
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_MAP_CHOICE, OnItemActivateMapChoice)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Region message handlers

BOOL Dialog_Save_As_DSMap::OnInitDialog()
{
	CDialog::OnInitDialog();

	MapInfoColl mapInfos;
	gTankMgr.GetMapInfos( "world/maps", mapInfos );

	CRect rect;
	m_MapChoice.GetClientRect( rect );
	int width = rect.Width() - ::GetSystemMetrics( SM_CXVSCROLL );

	m_MapChoice.InsertColumn( 0, "System Name", LVCFMT_LEFT, width / 3 );
	m_MapChoice.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, width / 3 );
	m_MapChoice.InsertColumn( 2, "Description", LVCFMT_LEFT, width - ((width / 3) * 2) );

	int index = 0;
	for ( MapInfoColl::iterator i = mapInfos.begin() ; i != mapInfos.end() ; ++i, ++index )
	{
		m_MapChoice.InsertItem( index, i->m_InternalName );
		m_MapChoice.SetItemText( index, 1, ::ToAnsi( i->m_ScreenName ) );
		m_MapChoice.SetItemText( index, 2, ::ToAnsi( i->m_Description ) );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Save_As_DSMap::OnOK()
{
	UpdateData();

	// get some vars
	gpwstring pathVar( L"%out%/world/maps/" );
	gpstring mapName = GetSelectedMap();
	MapInfo info;
	gTankMgr.GetMapInfo( "world/maps", mapName, info );

	if ( gEditorRegion.GetMapName().same_no_case( GetSelectedMap() ) && gEditorRegion.GetRegionDirty() )
	{
		HRESULT hResult = MessageBoxEx( this->GetSafeHwnd(), "A region from the map you want to save as a .dsmap is currently loaded.  Do you wish to save this region so its changes are included in the .dsmap?", "Save as DSMap", MB_YESNO | MB_ICONEXCLAMATION, LANG_ENGLISH );
		if ( hResult == IDYES )
		{
			MenuFunctions mf;
			mf.SaveRegion();
		}
	}

	// create dialog and configure it
	Dialog_Save_As_DSMod dlg;
	dlg.m_DstFileName  = m_DstFileName;
	dlg.m_OutputPrefix = "world/maps/" + mapName;
	dlg.m_SourceFolder = gConfig.ResolvePathVars( pathVar + ::ToUnicode( mapName ) );
	dlg.m_Title        = ::ToAnsi( info.m_ScreenName );
	dlg.m_Description  = ::ToAnsi( info.m_Description );
	dlg.m_DevOnly      = info.m_IsDevOnly;

	// do it!
	if ( dlg.DoModal() == IDOK )
	{
		CDialog::OnOK();
	}
}

void Dialog_Save_As_DSMap::OnBnClickedBrowseDestFile()
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
			m_DstFileName += ".dsmap";
		}
		UpdateData( FALSE );
	}
}

void Dialog_Save_As_DSMap::OnClickMapChoice(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CString name = GetSelectedMap();
	if ( !name.IsEmpty() )
	{
		name.SetAt( 0, ::toupper( name[ 0 ] ) );
		for ( ; ; )
		{
			int found = name.Find( '_' );
			if ( found != -1 )
			{
				name = name.Left( found ) + name.Mid( found + 1 );
				name.SetAt( found, ::toupper( name[ found ] ) );
			}
			else
			{
				break;
			}
		}
		name = gConfig.ResolvePathVars( "%map_paths%/" ) + name + ".dsmap";

		m_DstFileName = name;
		UpdateData( FALSE );

		OnChangeDestFileChoice();
	}

	*pResult = 0;
}

CString Dialog_Save_As_DSMap::GetSelectedMap()
{
	int sel = m_MapChoice.GetNextItem( -1, LVNI_SELECTED );
	if ( sel != -1 )
	{
		return ( m_MapChoice.GetItemText( sel, 0 ) );
	}

	return "";
}

void Dialog_Save_As_DSMap::OnChangeDestFileChoice() 
{
	UpdateData();
	m_OkButton.EnableWindow( !m_DstFileName.IsEmpty() );
}

void Dialog_Save_As_DSMap::OnItemActivateMapChoice(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnOK();
	*pResult = 0;
}

void Dialog_Save_As_DSMap::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
