// PropPage_Lighting.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "PropPage_Lighting.h"
#include "EditorLights.h"
#include "Dialog_Lighting_GOLight.h"
#include "EditorTerrain.h"
#include "siege_hotpoint_database.h"
#include "space_3.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Lighting dialog


PropPage_Lighting::PropPage_Lighting(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Lighting::IDD, IDS_STRING61211)
{
	//{{AFX_DATA_INIT(PropPage_Lighting)
	//}}AFX_DATA_INIT
}


void PropPage_Lighting::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Lighting)
	DDX_Control(pDX, IDC_COMBO_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_COMBO_AZIMUTH, m_azimuth);
	DDX_Control(pDX, IDC_CHECK_ONTIMER, m_ontimer);
	DDX_Control(pDX, IDC_CHECK_AFFECT_TERRAIN, m_affect_terrain);
	DDX_Control(pDX, IDC_CHECK_AFFECT_ITEMS, m_affect_items);
	DDX_Control(pDX, IDC_CHECK_AFFECT_ACTORS, m_affect_actors);
	DDX_Control(pDX, IDC_CHECK_ACTIVE, m_active);
	DDX_Control(pDX, IDC_CHECK_DRAWSHADOWS, m_draw_shadows);
	DDX_Control(pDX, IDC_CHECK_OCCLUDE, m_occlude);
	DDX_Control(pDX, IDC_EDIT_DIR_X, m_dir_x);
	DDX_Control(pDX, IDC_EDIT_DIR_Y, m_dir_y);
	DDX_Control(pDX, IDC_EDIT_DIR_Z, m_dir_z);
	DDX_Control(pDX, IDC_EDIT_INTENSITY, m_intensity);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Lighting, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Lighting)
	ON_BN_CLICKED(IDC_BUTTON_GOLIGHT, OnButtonGolight)
	ON_CBN_SELCHANGE(IDC_COMBO_ORBIT, OnSelchangeComboOrbit)
	ON_CBN_SELCHANGE(IDC_COMBO_AZIMUTH, OnSelchangeComboAzimuth)
	ON_EN_CHANGE(IDC_EDIT_DIR_X, OnChangeEditDirX)
	ON_EN_CHANGE(IDC_EDIT_DIR_Y, OnChangeEditDirY)
	ON_EN_CHANGE(IDC_EDIT_DIR_Z, OnChangeEditDirZ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Lighting message handlers

BOOL PropPage_Lighting::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	bool bActive		= false;
	bool bDrawShadows	= false;
	bool bOcclude		= false;
	bool bAffectActors	= true;
	bool bAffectItems	= true;
	bool bAffectTerrain	= true;
	bool bOnTimer		= false;

	vector_3 vDir;
	float intensity		= 1.0f;
	float iradius		= 0.0f;
	float oradius		= 20.0f;

	gEditorLights.GetLightProperties(	vDir, iradius, oradius, intensity, bOcclude, bDrawShadows, bActive, 
										bAffectActors, bAffectItems, bAffectTerrain, bOnTimer );
	
	m_active.SetCheck( (int)bActive );
	m_draw_shadows.SetCheck( (int)bDrawShadows );
	m_occlude.SetCheck( (int)bOcclude );
	m_affect_actors.SetCheck( (int)bAffectActors );
	m_affect_items.SetCheck( (int)bAffectItems );
	m_affect_terrain.SetCheck( (int)bAffectTerrain );
	m_ontimer.SetCheck( (int)bOnTimer );

	gpstring sTemp;
	sTemp.assignf( "%f", vDir.x );
	m_dir_x.SetWindowText( sTemp );
	sTemp.assignf( "%f", vDir.y );
	m_dir_y.SetWindowText( sTemp );
	sTemp.assignf( "%f", vDir.z );
	m_dir_z.SetWindowText( sTemp );

	sTemp.assignf( "%f", intensity );
	m_intensity.SetWindowText( sTemp );

	for ( int i = 0; i != 360; ++i )
	{
		gpstring sNum;
		sNum.assignf( "%d", i );
		m_orbit.AddString( sNum.c_str() );		
	}

	for ( i = 0; i <= 90; ++i )
	{
		gpstring sNum;
		sNum.assignf( "%d", i );
		m_azimuth.AddString( sNum.c_str() );
	}

	vector_3 flatDir = vDir;
	flatDir.y = 0;

	if (( vDir.IsZero() || flatDir.IsZero() ) == false )
	{
		vector_3 northDir = gSiegeHotpointDatabase.GetHotpointDirection( L"north", gEditorTerrain.GetTargetNode() );
		if ( northDir.IsZero() )
		{
			m_azimuth.EnableWindow( FALSE );
			m_orbit.EnableWindow( FALSE );
		}
		else
		{
			float azimuth	= AngleBetween( vDir, flatDir );
			float orbit		= AngleBetween( northDir, flatDir );
			
			flatDir.Normalize();
			northDir.Normalize();

			{			
				vector_3 newNorth = YRotationColumns( orbit ) * northDir;
				if ( !newNorth.IsEqual( flatDir, 0.01f ) ) 
				{
					orbit = (2*RealPi) - orbit;
				}

				azimuth = RadiansToDegrees( azimuth );
				orbit	= RadiansToDegrees( orbit );
				
				sTemp.assignf( "%0.0f", orbit );
				m_orbit.SelectString( 0, sTemp.c_str() );
			}

			{
				sTemp.assignf( "%0.0f", azimuth );
				m_azimuth.SelectString( 0, sTemp.c_str() );
			}
		}
	}
	else
	{
		m_azimuth.SelectString( 0, "0" );
		m_orbit.SelectString( 0, "0" );
		m_azimuth.EnableWindow( FALSE );
		m_orbit.EnableWindow( FALSE );
	}
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Lighting::OnButtonGolight() 
{
//	Dialog_Lighting_GOLight dlgGolight;
//	dlgGolight.DoModal();
}


void PropPage_Lighting::Apply()
{
	if ( GetSafeHwnd() ) {
		bool bActive		= m_active.GetCheck() ? true : false;
		bool bDrawShadows	= m_draw_shadows.GetCheck() ? true : false;
		bool bOcclude		= m_occlude.GetCheck() ? true : false;
		bool bAffectActors	= m_affect_actors.GetCheck() ? true : false;
		bool bAffectItems	= m_affect_items.GetCheck() ? true : false;
		bool bAffectTerrain	= m_affect_terrain.GetCheck() ? true : false;
		bool bOnTimer		= m_ontimer.GetCheck() ? true : false;

		CString rTemp;

		vector_3 vDir;
		m_dir_x.GetWindowText( rTemp );
		vDir.x = atof( rTemp );
		m_dir_y.GetWindowText( rTemp );
		vDir.y = atof( rTemp );
		m_dir_z.GetWindowText( rTemp );
		vDir.z = atof( rTemp );

		m_intensity.GetWindowText( rTemp );
		float intensity = atof( rTemp );
		float iradius = 0;		
		float oradius = 0;

		gEditorLights.SetLightProperties(	vDir, iradius, oradius, intensity, bOcclude, bDrawShadows, bActive,
											bAffectActors, bAffectItems, bAffectTerrain, bOnTimer );
	}
}

void PropPage_Lighting::OnSelchangeComboOrbit() 
{
	RecalculateDirection();
}

void PropPage_Lighting::OnSelchangeComboAzimuth() 
{	
	RecalculateDirection();
}

void PropPage_Lighting::OnChangeEditDirX() 
{
	RecalculateAngles();
}

void PropPage_Lighting::OnChangeEditDirY() 
{
	RecalculateAngles();
}

void PropPage_Lighting::OnChangeEditDirZ() 
{
	RecalculateAngles();
}


void PropPage_Lighting::RecalculateAngles()
{
	vector_3 vDir;
	CString rTemp;

	m_dir_x.GetWindowText( rTemp );

	if ( rTemp.IsEmpty() )
	{
		return;
	}

	stringtool::Get( rTemp.GetBuffer( 0 ), vDir.x );

	m_dir_y.GetWindowText( rTemp );

	if ( rTemp.IsEmpty() )
	{
		return;
	}

	stringtool::Get( rTemp.GetBuffer( 0 ), vDir.y );

	m_dir_z.GetWindowText( rTemp );

	if ( rTemp.IsEmpty() )
	{
		return;
	}

	stringtool::Get( rTemp.GetBuffer( 0 ), vDir.z );

	vector_3 flatDir = vDir;
	flatDir.y = 0;

	if (( vDir.IsZero() || flatDir.IsZero() ) == false )
	{
		vector_3 northDir = gSiegeHotpointDatabase.GetHotpointDirection( L"north", gEditorTerrain.GetTargetNode() );
		if ( northDir.IsZero() )
		{
			m_azimuth.EnableWindow( FALSE );
			m_orbit.EnableWindow( FALSE );
		}
		else
		{
			gpstring sTemp;
			float azimuth	= AngleBetween( vDir, flatDir );
			float orbit		= AngleBetween( northDir, flatDir );
			
			flatDir.Normalize();
			northDir.Normalize();

			{			
				vector_3 newNorth = YRotationColumns( orbit ) * northDir;
				if ( !newNorth.IsEqual( flatDir, 0.01f ) ) 
				{
					orbit = (2*RealPi) - orbit;
				}
				
				orbit	= RadiansToDegrees( orbit );				
				sTemp.assignf( "%0.0f", orbit );
				m_orbit.SelectString( 0, sTemp.c_str() );
			}

			{
				azimuth = RadiansToDegrees( azimuth );
				sTemp.assignf( "%0.0f", azimuth );
				m_azimuth.SelectString( 0, sTemp.c_str() );
			}
		}
	}
	else
	{
		m_azimuth.SelectString( 0, "0" );
		m_orbit.SelectString( 0, "0" );
		m_azimuth.EnableWindow( FALSE );
		m_orbit.EnableWindow( FALSE );
	}
}


void PropPage_Lighting::RecalculateDirection()
{
	int sel = m_orbit.GetCurSel();
	CString rTemp;
	m_orbit.GetLBText( sel, rTemp );
	float orbit = 0;
	stringtool::Get( rTemp.GetBuffer( 0 ), orbit );
	orbit = DegreesToRadians( orbit );

	sel = m_azimuth.GetCurSel();
	m_azimuth.GetLBText( sel, rTemp );
	float azimuth = 0;
	stringtool::Get( rTemp.GetBuffer( 0 ), azimuth );
	azimuth = DegreesToRadians( azimuth );

	vector_3 northDir = gSiegeHotpointDatabase.GetHotpointDirection( L"north", gEditorTerrain.GetTargetNode() );

	northDir = YRotationColumns( orbit ) * northDir;
	vector_3 vx;
	MatrixFromDirection( northDir ).GetColumn0( vx );
	vector_3 finalNorth = AxisRotationColumns( vx, -azimuth ) * northDir;

	gpstring sDir;
	sDir.assignf( "%f", finalNorth.x );
	m_dir_x.SetWindowText( sDir.c_str() );

	sDir.assignf( "%f", finalNorth.y );
	m_dir_y.SetWindowText( sDir.c_str() );

	sDir.assignf( "%f", finalNorth.z );
	m_dir_z.SetWindowText( sDir.c_str() );	
}
