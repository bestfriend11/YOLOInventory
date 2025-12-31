// Dialog_New_Map.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_New_Map.h"
#include "EditorMap.h"
#include "FileSysUtils.h"
#include "EditorRegion.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Map dialog


Dialog_New_Map::Dialog_New_Map(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_New_Map::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_New_Map)
	//}}AFX_DATA_INIT
}


void Dialog_New_Map::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_New_Map)
	DDX_Control(pDX, IDC_EDIT_WORLD_INTEREST_RADIUS, m_worldInterestRadius);
	DDX_Control(pDX, IDC_EDIT_WORLD_FRUSTUM_RADIUS, m_worldFrustumRadius);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	DDX_Control(pDX, IDC_COMBO_MINUTES, m_minutes);
	DDX_Control(pDX, IDC_COMBO_HOURS, m_hours);
	DDX_Control(pDX, IDC_CHECK_DEV_ONLY, m_devOnly);
	DDX_Control(pDX, IDC_EDIT_SCREEN_NAME, m_screen_name);
	DDX_Control(pDX, IDC_EDIT_MAP_NAME, m_map_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_New_Map, CDialog)
	//{{AFX_MSG_MAP(Dialog_New_Map)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_WORLD_FRUSTUM_RADIUS, OnButtonDefaultWorldFrustumRadius)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_WORLD_INTEREST_RADIUS, OnButtonDefaultWorldInterestRadius)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Map message handlers

void Dialog_New_Map::OnOK() 
{
	// TODO: Add extra validation here	
	CString rMap;
	this->GetDlgItemText( IDC_EDIT_MAP_NAME, rMap );
	CString rDesc;
	this->GetDlgItemText( IDC_EDIT_DESCRIPTION, rDesc );
	CString rScreenName;
	m_screen_name.GetWindowText( rScreenName );	

	if ( rMap.IsEmpty() )
	{
		MessageBox( "Please enter a name for the map.", "New Map", MB_OK );
		return;
	}

	StringVec			map_list;
	StringVec::iterator	i;
	gEditorRegion.GetMapList( map_list );		
	for ( i = map_list.begin(); i != map_list.end(); i++ ) 
	{
		if ( (*i).same_no_case( rMap.GetBuffer( 0 ) ) )
		{
			MessageBox( "A map of this name already exists.  Please choose a different name.", "New Map", MB_OK );
			return;
		}
	}	


	gpstring sTemp = rMap.GetBuffer( 0 );
	if( !FileSys::IsValidFileName( sTemp, false, false ) )
	{
		MessageBox( "Invalid Map Name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK );
		return;			
	}

	if ( rScreenName.IsEmpty() )
	{
		MessageBox( "Please enter a screen name for the map.", "New Map", MB_OK );
		return;
	}

	MapCreateInfo mapCreateInfo;
	mapCreateInfo.bDevOnly = m_devOnly.GetCheck() ? true : false;
	mapCreateInfo.sDescription = rDesc.GetBuffer( 0 );
	mapCreateInfo.sMapName = rMap.GetBuffer( 0 );
	mapCreateInfo.sScreenName = rScreenName.GetBuffer( 0 );
	
	// Get time of day info
	CString rHour, rMinute;
	int hour = 0, minute = 0;
	m_hours.GetWindowText( rHour );
	m_minutes.GetWindowText( rMinute );
	stringtool::Get( rHour.GetBuffer( 0 ), hour );
	stringtool::Get( rMinute.GetBuffer( 0 ), minute );
	mapCreateInfo.startTimeHour = hour;
	mapCreateInfo.startTimeMinute = minute;

	// Get advanced world information
	float worldFrustumRadius = 0.0f, worldInterestRadius = 0.0f;
	CString rFrustumRadius;
	CString rInterestRadius;
	m_worldFrustumRadius.GetWindowText( rFrustumRadius );
	m_worldInterestRadius.GetWindowText( rInterestRadius );
	stringtool::Get( rFrustumRadius.GetBuffer( 0 ), worldFrustumRadius );
	stringtool::Get( rInterestRadius.GetBuffer( 0 ), worldInterestRadius );
	mapCreateInfo.worldFrustumRadius = worldFrustumRadius;
	mapCreateInfo.worldInterestRadius = worldInterestRadius;
	
	gEditorMap.CreateNewMap( mapCreateInfo );

	CDialog::OnOK();
}

BOOL Dialog_New_Map::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	// Initialize Starting Time Info
	for ( int hours = 0; hours != 24; ++hours )
	{
		gpstring sNumber;
		sNumber.assignf( "%d", hours );
		m_hours.AddString( sNumber.c_str() );	
	}
	m_hours.SetCurSel( 0 );

	for ( int min = 0; min != 60; ++min )
	{
		gpstring sNumber;
		sNumber.assignf( "%d", min );
		m_minutes.AddString( sNumber.c_str() );
	}
	m_minutes.SetCurSel( 0 );

	// Set other defaults
	OnButtonDefaultWorldFrustumRadius();
	OnButtonDefaultWorldInterestRadius();
	m_devOnly.SetCheck( FALSE );	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_New_Map::OnButtonDefaultWorldFrustumRadius() 
{
	m_worldFrustumRadius.SetWindowText( "45" );
	
}

void Dialog_New_Map::OnButtonDefaultWorldInterestRadius() 
{
	m_worldInterestRadius.SetWindowText( "4" );	
}

void Dialog_New_Map::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
