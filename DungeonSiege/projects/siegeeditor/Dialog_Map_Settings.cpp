// Dialog_Map_Settings.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Map_Settings.h"
#include "EditorMap.h"
#include "EditorRegion.h"
#include "EditorCamera.h"
#include "EditorTerrain.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Settings dialog


Dialog_Map_Settings::Dialog_Map_Settings(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Map_Settings::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Map_Settings)
	//}}AFX_DATA_INIT
}


void Dialog_Map_Settings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Map_Settings)
	DDX_Control(pDX, IDC_EDIT_WORLD_INTEREST_RADIUS, m_worldInterestRadius);
	DDX_Control(pDX, IDC_EDIT_WORLD_FRUSTUM_RADIUS, m_worldFrustumRadius);
	DDX_Control(pDX, IDC_EDIT_MAP_NAME, m_screen_name);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	DDX_Control(pDX, IDC_EDIT_NODE, m_nodeid);
	DDX_Control(pDX, IDC_COMBO_MINUTES, m_minutes);
	DDX_Control(pDX, IDC_COMBO_HOURS, m_hour);
	DDX_Control(pDX, IDC_UP, m_up);
	DDX_Control(pDX, IDC_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_NORTH, m_north);
	DDX_Control(pDX, IDC_NEARCLIP, m_nearclip);
	DDX_Control(pDX, IDC_MINDISTANCE, m_min_distance);
	DDX_Control(pDX, IDC_MINAZIMUTH, m_min_azimuth);
	DDX_Control(pDX, IDC_MAXDISTANCE, m_max_distance);
	DDX_Control(pDX, IDC_MAXAZIMUTH, m_max_azimuth);
	DDX_Control(pDX, IDC_MAP_NAME, m_map_name);
	DDX_Control(pDX, IDC_FOV, m_fov);
	DDX_Control(pDX, IDC_FARCLIP, m_farclip);
	DDX_Control(pDX, IDC_EAST, m_east);
	DDX_Control(pDX, IDC_DISTANCE, m_distance);
	DDX_Control(pDX, IDC_AZIMUTH, m_azimuth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Map_Settings, CDialog)
	//{{AFX_MSG_MAP(Dialog_Map_Settings)
	ON_BN_CLICKED(IDC_BUTTON_USE_CURRENT_POSITION, OnButtonUseCurrentPosition)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE, OnButtonUseSelectedNode)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_WORLD_FRUSTUM, OnButtonDefaultWorldFrustum)
	ON_BN_CLICKED(IDC_BUTTON_WORLD_INTEREST_RADIUS, OnButtonWorldInterestRadius)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Settings message handlers

BOOL Dialog_Map_Settings::OnInitDialog() 
{
	CDialog::OnInitDialog();
		
	m_map_name.SetWindowText( gEditorRegion.GetMapName() );
		
	float y				= 0;
	float z				= 0;
	float x				= 0;
	int	  nodeId		= 0;
	float azimuth		= 0;
	float orbit			= 0;
	float distance		= 0;
	float minDistance	= 0;
	float maxDistance	= 0;
	float nearclip		= 0;
	float farclip		= 0;
	float fov			= 0;
	float minAzimuth	= 0;
	float maxAzimuth	= 0;

	gEditorMap.GetCameraSettings(	y, z, x, nodeId, azimuth, orbit, distance, 
									minDistance, maxDistance, nearclip, farclip, 
									fov, minAzimuth, maxAzimuth );
	
	gpstring sTemp;
	sTemp.assignf( "%f", y );
	m_up.SetWindowText( sTemp );

	sTemp.assignf( "%f", z );
	m_east.SetWindowText( sTemp );

	sTemp.assignf( "%f", x );
	m_north.SetWindowText( sTemp );

	sTemp.assignf( "%f", azimuth );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", orbit );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", distance );
	m_distance.SetWindowText( sTemp );

	sTemp.assignf( "%f", minDistance );
	m_min_distance.SetWindowText( sTemp );

	sTemp.assignf( "%f", maxDistance );
	m_max_distance.SetWindowText( sTemp );

	sTemp.assignf( "%f", nearclip );
	m_nearclip.SetWindowText( sTemp );

	sTemp.assignf( "%f", farclip );
	m_farclip.SetWindowText( sTemp );

	sTemp.assignf( "%f", fov );
	m_fov.SetWindowText( sTemp );

	sTemp.assignf( "%f", minAzimuth );
	m_min_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", maxAzimuth );
	m_max_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "0x%08X", nodeId );
	m_nodeid.SetWindowText( sTemp );

	for ( int hours = 0; hours != 24; ++hours )
	{
		gpstring sNumber;
		sNumber.assignf( "%d", hours );
		m_hour.AddString( sNumber.c_str() );
	}

	for ( int min = 0; min != 60; ++min )
	{
		gpstring sNumber;
		sNumber.assignf( "%d", min );
		m_minutes.AddString( sNumber.c_str() );
	}

	int hour	= 0;
	int minutes	= 0;
	gEditorMap.GetStartTime( hour, minutes );

	gpstring sHour;
	sHour.assignf( "%d", hour );
	int sel = m_hour.FindString( 0, sHour.c_str() );
	if ( sel != CB_ERR )
	{
		m_hour.SetCurSel( sel );
	}

	gpstring sMinute;
	sMinute.assignf( "%d", minutes );
	sel = m_minutes.FindString( 0, sMinute.c_str() );
	if ( sel != CB_ERR )
	{
		m_minutes.SetCurSel( sel );
	}

	FuelHandle hMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMain.IsValid() )
	{
		gpstring sName;
		gpstring sDescription;
		hMain->Get( "screen_name", sName );
		hMain->Get( "description", sDescription );

		m_screen_name.SetWindowText( sName.c_str() );
		m_description.SetWindowText( sDescription.c_str() );
	}	

	sTemp.assignf( "%f", gEditorMap.GetWorldInterestRadius() );
	m_worldInterestRadius.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gEditorMap.GetWorldFrustumRadius() );
	m_worldFrustumRadius.SetWindowText( sTemp.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Map_Settings::OnOK() 
{
	float y				= 0;
	float z				= 0;
	float x				= 0;
	int   nodeId		= 0;
	float azimuth		= 0;
	float orbit			= 0;
	float distance		= 0;
	float minDistance	= 0;
	float maxDistance	= 0;
	float nearclip		= 0;
	float farclip		= 0;
	float fov			= 0;
	float minAzimuth	= 0;
	float maxAzimuth	= 0;

	CString rTemp;
	m_up.GetWindowText( rTemp );
	y = atof( rTemp );

	m_east.GetWindowText( rTemp );
	z = atof( rTemp );

	m_north.GetWindowText( rTemp );
	x = atof( rTemp );

	m_nodeid.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), nodeId );
	
	m_azimuth.GetWindowText( rTemp );
	azimuth = atof( rTemp );

	m_orbit.GetWindowText( rTemp );
	orbit = atof( rTemp );

	m_distance.GetWindowText( rTemp );
	distance = atof( rTemp );

	m_min_distance.GetWindowText( rTemp );
	minDistance = atof( rTemp );

	m_max_distance.GetWindowText( rTemp );
	maxDistance = atof( rTemp );

	m_nearclip.GetWindowText( rTemp );
	nearclip = atof( rTemp );

	m_farclip.GetWindowText( rTemp );
	farclip = atof( rTemp );

	m_fov.GetWindowText( rTemp );
	fov = atof( rTemp );

	m_min_azimuth.GetWindowText( rTemp );
	minAzimuth = atof( rTemp );

	m_max_azimuth.GetWindowText( rTemp );
	maxAzimuth = atof( rTemp );

	gEditorMap.SetCameraSettings(	y, z, x, nodeId, azimuth, orbit, distance, 
									minDistance, maxDistance, nearclip, farclip, 
									fov, minAzimuth, maxAzimuth );	
	
	int index = 0;
	CString rHour;
	index = m_hour.GetCurSel();
	if ( index != CB_ERR ) 
	{		
		m_hour.GetLBText( index, rHour );		
	}

	CString rMinute;
	index = m_minutes.GetCurSel();
	if ( index != CB_ERR )
	{
		m_minutes.GetLBText( index, rMinute );
	}

	int hour = 0;
	int minute = 0;

	stringtool::Get( gpstring(rHour.GetBuffer( 0 )), hour );
	stringtool::Get( gpstring(rMinute.GetBuffer( 0 )), minute );

	gEditorMap.SetStartTime( hour, minute );

	CString rScreenName;
	CString rDescription;
	m_screen_name.GetWindowText( rScreenName );
	m_description.GetWindowText( rDescription );

	FuelHandle hMain = gEditorRegion.GetMapHandle()->GetChildBlock( "map" );
	if ( hMain.IsValid() )
	{
		hMain->Set( "screen_name", rScreenName.GetBuffer( 0 ), FVP_QUOTED_STRING );
		hMain->Set( "description", rDescription.GetBuffer( 0 ), FVP_QUOTED_STRING );
	}

	// Get advanced world information
	float worldFrustumRadius = 0.0f, worldInterestRadius = 0.0f;
	CString rFrustumRadius;
	CString rInterestRadius;
	m_worldFrustumRadius.GetWindowText( rFrustumRadius );
	m_worldInterestRadius.GetWindowText( rInterestRadius );
	stringtool::Get( rFrustumRadius.GetBuffer( 0 ), worldFrustumRadius );
	stringtool::Get( rInterestRadius.GetBuffer( 0 ), worldInterestRadius );
	gEditorMap.SetWorldFrustumRadius( worldFrustumRadius );
	gEditorMap.SetWorldInterestRadius( worldInterestRadius );

	CDialog::OnOK();
}


void Dialog_Map_Settings::OnButtonUseCurrentPosition() 
{
	gpstring sTemp;
	sTemp.assignf( "%f", gEditorCamera.GetAzimuth() );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetDistance() );
	m_distance.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetOrbit() );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetPosition().x );
	m_north.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetPosition().y );
	m_up.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetPosition().z );
	m_east.SetWindowText( sTemp );	
}

void Dialog_Map_Settings::OnButtonUseSelectedNode() 
{
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );
	m_nodeid.SetWindowText( sTemp );		
}

void Dialog_Map_Settings::OnButtonDefaultWorldFrustum() 
{
	m_worldFrustumRadius.SetWindowText( "45.0" );	
}

void Dialog_Map_Settings::OnButtonWorldInterestRadius() 
{
	m_worldInterestRadius.SetWindowText( "10.0" );	
}

void Dialog_Map_Settings::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
