// Dialog_Region_Hotpoint_Direction.cpp : implementation file
//
#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Region_Hotpoint_Direction.h"
#include "EditorRegion.h"
#include "EditorMap.h"
#include "EditorHotpoints.h"
#include "space_3.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Hotpoint_Direction dialog


Dialog_Region_Hotpoint_Direction::Dialog_Region_Hotpoint_Direction(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Region_Hotpoint_Direction::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Region_Hotpoint_Direction)
	//}}AFX_DATA_INIT
}


void Dialog_Region_Hotpoint_Direction::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Region_Hotpoint_Direction)
	DDX_Control(pDX, IDC_COMBO_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_EDIT_Z, m_editZ);
	DDX_Control(pDX, IDC_EDIT_X, m_editX);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Region_Hotpoint_Direction, CDialog)
	//{{AFX_MSG_MAP(Dialog_Region_Hotpoint_Direction)
	ON_EN_CHANGE(IDC_EDIT_X, OnChangeEditX)
	ON_EN_CHANGE(IDC_EDIT_Z, OnChangeEditZ)
	ON_CBN_SELCHANGE(IDC_COMBO_ORBIT, OnSelchangeComboOrbit)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Hotpoint_Direction message handlers

void Dialog_Region_Hotpoint_Direction::OnOK() 
{
	vector_3 vDir;

	CString rValue;
	m_editX.GetWindowText( rValue );
	vDir.x = atof( rValue.GetBuffer( 0 ) );

	m_editZ.GetWindowText( rValue );
	vDir.z = atof( rValue.GetBuffer( 0 ) );

	if ( vDir.IsZero() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "The north vector cannot be of zero direction.", "North Vector Direction", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	gEditorHotpoints.SetCurrentDirection( vDir );

	gEditorHotpoints.SetCanDraw( false );
	
	CDialog::OnOK();
}

void Dialog_Region_Hotpoint_Direction::OnCancel() 
{
	gEditorHotpoints.SetCanDraw( false );
	gEditorHotpoints.SetCurrentDirection( m_originalNorth );
	
	CDialog::OnCancel();
}

void Dialog_Region_Hotpoint_Direction::OnChangeEditX() 
{
	vector_3 vDir;
	CString rValue;
	m_editX.GetWindowText( rValue );
	vDir.x = atof( rValue.GetBuffer( 0 ) );

	m_editZ.GetWindowText( rValue );
	vDir.z = atof( rValue.GetBuffer( 0 ) );

	if ( !vDir.IsZero() )
	{
		gEditorHotpoints.SetCurrentDirection( vDir );	
	}

	RecalculateAngle();	
}

void Dialog_Region_Hotpoint_Direction::OnChangeEditZ() 
{
	vector_3 vDir;
	CString rValue;
	m_editX.GetWindowText( rValue );
	vDir.x = atof( rValue.GetBuffer( 0 ) );

	m_editZ.GetWindowText( rValue );
	vDir.z = atof( rValue.GetBuffer( 0 ) );

	if ( !vDir.IsZero() )
	{
		gEditorHotpoints.SetCurrentDirection( vDir );	
	}

	RecalculateAngle();	
}

void Dialog_Region_Hotpoint_Direction::OnSelchangeComboOrbit() 
{
	RecalculateDirection();	
}

BOOL Dialog_Region_Hotpoint_Direction::OnInitDialog() 
{
	CDialog::OnInitDialog();

	gEditorMap.CreateNorthVector( gEditorRegion.GetMapHandle()->GetAddress() );
	
	vector_3 vDir = gEditorHotpoints.GetCurrentDirection();
	m_originalNorth = vDir;

	gpstring sTemp;
	sTemp.assignf( "%f", vDir.x );
	m_editX.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", vDir.z );
	m_editZ.SetWindowText( sTemp.c_str() );

	for ( int i = 0; i != 360; ++i )
	{
		gpstring sNum;
		sNum.assignf( "%d", i );
		m_orbit.AddString( sNum.c_str() );		
	}

	RecalculateAngle();

	gEditorHotpoints.SetCanDraw( true );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Region_Hotpoint_Direction::RecalculateDirection()
{
	int sel = m_orbit.GetCurSel();
	CString rTemp;
	m_orbit.GetLBText( sel, rTemp );
	float orbit = 0;
	stringtool::Get( rTemp.GetBuffer( 0 ), orbit );
	orbit = DegreesToRadians( orbit );

	vector_3 northDir( 1.0f, 0.0f, 0.0f );

	northDir = YRotationColumns( orbit ) * northDir;
	vector_3 vx;
	MatrixFromDirection( northDir ).GetColumn0( vx );
	vector_3 finalNorth = AxisRotationColumns( vx, 0 ) * northDir;

	gpstring sDir;
	sDir.assignf( "%f", finalNorth.x );
	m_editX.SetWindowText( sDir.c_str() );
	
	sDir.assignf( "%f", finalNorth.z );
	m_editZ.SetWindowText( sDir.c_str() );	
	
}


void Dialog_Region_Hotpoint_Direction::RecalculateAngle()
{
	CString rTemp;
	vector_3 flatDir;

	m_editX.GetWindowText( rTemp );
	if ( rTemp.IsEmpty() )
	{
		return;
	}
	stringtool::Get( rTemp.GetBuffer( 0 ), flatDir.x );
	
	m_editZ.GetWindowText( rTemp );
	if ( rTemp.IsEmpty() )
	{
		return;
	}
	stringtool::Get( rTemp.GetBuffer( 0 ), flatDir.z );

	gpstring sTemp;
	vector_3 northDir( 1.0f, 0.0f, 0.0f );
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
}


void Dialog_Region_Hotpoint_Direction::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
