// PropPage_Position.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "PropPage_Position.h"
#include "math.h"
#include "matrix_3x3.h"
#include "GizmoManager.h"
#include "space_3.h"
#include "stringtool.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Position dialog


PropPage_Position::PropPage_Position(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Position::IDD, IDS_STRING61210)
	, m_bParamsInit( false )
{
	//{{AFX_DATA_INIT(PropPage_Position)
	m_scale = 0.0f;
	m_pos_x = 0.0f;
	m_pos_y = 0.0f;
	m_pos_z = 0.0f;
	//}}AFX_DATA_INIT
}


void PropPage_Position::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Position)
	DDX_Control(pDX, IDC_SPIN_Z, m_spin_z);
	DDX_Control(pDX, IDC_SPIN_Y, m_spin_y);
	DDX_Control(pDX, IDC_SPIN_X, m_spin_x);
	DDX_Control(pDX, IDC_EDIT_YAW, m_edit_yaw);
	DDX_Control(pDX, IDC_EDIT_ROLL, m_edit_roll);
	DDX_Control(pDX, IDC_EDIT_PITCH, m_edit_pitch);
	DDX_Control(pDX, IDC_SLIDER_X, m_slider_x);
	DDX_Control(pDX, IDC_SLIDER_Y, m_slider_y);
	DDX_Control(pDX, IDC_SLIDER_Z, m_slider_z);
	DDX_Text(pDX, IDC_EDIT_SCALE, m_scale);
	DDX_Text(pDX, IDC_EDIT_X, m_pos_x);
	DDX_Text(pDX, IDC_EDIT_Y, m_pos_y);
	DDX_Text(pDX, IDC_EDIT_Z, m_pos_z);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Position, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Position)
	ON_EN_CHANGE(IDC_EDIT_X, OnChangeEditX)
	ON_EN_CHANGE(IDC_EDIT_Y, OnChangeEditY)
	ON_EN_CHANGE(IDC_EDIT_Z, OnChangeEditZ)
	ON_EN_CHANGE(IDC_EDIT_SCALE, OnChangeEditScale)
	ON_EN_CHANGE(IDC_EDIT_PITCH, OnChangeEditPitch)
	ON_EN_CHANGE(IDC_EDIT_ROLL, OnChangeEditRoll)
	ON_EN_CHANGE(IDC_EDIT_YAW, OnChangeEditYaw)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_X, OnDeltaposSpinX)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Y, OnDeltaposSpinY)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Z, OnDeltaposSpinZ)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Position message handlers

void PropPage_Position::OnChangeEditX() 
{
	CString rText;
	GetDlgItem( IDC_EDIT_X )->GetWindowText( rText );	
	m_pos_x = atof( rText );
	gGizmoManager.SetPosition( m_pos_x, m_pos_y, m_pos_z );		
}

void PropPage_Position::OnChangeEditY() 
{
	CString rText;
	GetDlgItem( IDC_EDIT_Y )->GetWindowText( rText );	
	m_pos_y = atof( rText );
	gGizmoManager.SetPosition( m_pos_x, m_pos_y, m_pos_z );			
}

void PropPage_Position::OnChangeEditZ() 
{
	CString rText;
	GetDlgItem( IDC_EDIT_Z )->GetWindowText( rText );
	m_pos_z = atof( rText );
	gGizmoManager.SetPosition( m_pos_x, m_pos_y, m_pos_z );			
}

void PropPage_Position::OnChangeEditScale() 
{
	CString rText;
	GetDlgItem( IDC_EDIT_SCALE )->GetWindowText( rText );
	m_scale = atof( rText );
	gGizmoManager.SetBodyScale( atof( rText ) );	
}


BOOL PropPage_Position::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{	
	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}

LRESULT PropPage_Position::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( m_bParamsInit ) {
		if ( GetDlgItem( IDC_EDIT_X )->GetSafeHwnd() &&
			 GetDlgItem( IDC_EDIT_Y )->GetSafeHwnd() &&
			 GetDlgItem( IDC_EDIT_Z )->GetSafeHwnd() ) {
			if ( gGizmoManager.GetNumGizmosSelected() != 1 ) {
				GetDlgItem( IDC_EDIT_X )->EnableWindow( false );
				GetDlgItem( IDC_EDIT_Y )->EnableWindow( false );
				GetDlgItem( IDC_EDIT_Z )->EnableWindow( false );
			}
			else {
				GetDlgItem( IDC_EDIT_X )->EnableWindow( true );
				GetDlgItem( IDC_EDIT_Y )->EnableWindow( true );
				GetDlgItem( IDC_EDIT_Z )->EnableWindow( true );
			}
		}


		if ( m_slider_x.GetSafeHwnd() ) {
			static int x_slider_pos = m_slider_x.GetPos();
			static int y_slider_pos = m_slider_y.GetPos();
			static int z_slider_pos = m_slider_z.GetPos();

			if ( ( m_slider_x.GetPos() != x_slider_pos ) ||
				 ( m_slider_y.GetPos() != y_slider_pos ) ||
				 ( m_slider_z.GetPos() != z_slider_pos ) ) {

				x_slider_pos = m_slider_x.GetPos();
				y_slider_pos = m_slider_y.GetPos();
				z_slider_pos = m_slider_z.GetPos();

				
				// Build the current object's orientation
				Quat orient = m_orient;
				orient.RotateX( DegreesToRadians(x_slider_pos) );
				orient.RotateY( DegreesToRadians(y_slider_pos) );
				orient.RotateZ( DegreesToRadians(z_slider_pos) );
				
				// Set the new orientation		
				gGizmoManager.SetOrientation( orient.BuildMatrix() );			
			}
		}
	}
	
	return CPropertyPage::WindowProc(message, wParam, lParam);
}


void PropPage_Position::Apply()
{

}


BOOL PropPage_Position::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	vector_3 pos = gGizmoManager.GetPosition();
	
	m_pos_x = pos.x;
	m_pos_y = pos.y;
	m_pos_z = pos.z;

	if ( gGizmoManager.GetNumGizmosSelected() == 1 ) {
		gpstring sTemp;
		sTemp.assignf( "%f", pos.x );
		GetDlgItem( IDC_EDIT_X )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", pos.y );
		GetDlgItem( IDC_EDIT_Y )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", pos.z );
		GetDlgItem( IDC_EDIT_Z )->SetWindowText( sTemp.c_str() );
	}

	m_scale = gGizmoManager.GetBodyScale();
	gpstring sTemp;
	sTemp.assignf( "%f", m_scale );
	GetDlgItem( IDC_EDIT_SCALE )->SetWindowText( sTemp.c_str() );


	m_orient = Quat( gGizmoManager.GetOrientation() );
	/*
	matrix_3x3 orientation = gGizmoManager.GetOrientation();
	float pitch = 0;
	float yaw	= 0;
	float roll	= 0;
	AnglesFromMatrix( orientation, pitch, yaw, roll );

	m_slider_x.SetPos( RadiansToDegrees( pitch ) );
	m_slider_y.SetPos( RadiansToDegrees( yaw ) );
	m_slider_z.SetPos( RadiansToDegrees( roll ) );
	*/

	m_slider_x.SetText( "" );
	m_slider_y.SetText( "" );
	m_slider_z.SetText( "" );

	m_bParamsInit = true;
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Position::OnChangeEditPitch() 
{
	CString rTemp;
	m_edit_pitch.GetWindowText( rTemp );
	int pitch = 0;
	if ( !rTemp.IsEmpty() )
	{
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), pitch );
	}
	m_slider_x.SetPos( pitch );

	int x_slider_pos = m_slider_x.GetPos();
	int y_slider_pos = m_slider_y.GetPos();
	int z_slider_pos = m_slider_z.GetPos();
	
	// Build the current object's orientation
	Quat orient = m_orient;
	orient.RotateX( DegreesToRadians(x_slider_pos) );
	orient.RotateY( DegreesToRadians(y_slider_pos) );
	orient.RotateZ( DegreesToRadians(z_slider_pos) );
	
	// Set the new orientation		
	gGizmoManager.SetOrientation( orient.BuildMatrix() );		
}

void PropPage_Position::OnChangeEditRoll() 
{
	CString rTemp;
	m_edit_roll.GetWindowText( rTemp );
	int roll = 0;
	if ( !rTemp.IsEmpty() )
	{
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), roll );
	}
	m_slider_z.SetPos( roll );	

	int x_slider_pos = m_slider_x.GetPos();
	int y_slider_pos = m_slider_y.GetPos();
	int z_slider_pos = m_slider_z.GetPos();
	
	// Build the current object's orientation
	Quat orient = m_orient;
	orient.RotateX( DegreesToRadians(x_slider_pos) );
	orient.RotateY( DegreesToRadians(y_slider_pos) );
	orient.RotateZ( DegreesToRadians(z_slider_pos) );
	
	// Set the new orientation		
	gGizmoManager.SetOrientation( orient.BuildMatrix() );		
}

void PropPage_Position::OnChangeEditYaw() 
{
	CString rTemp;
	m_edit_yaw.GetWindowText( rTemp );
	int yaw = 0;
	if ( !rTemp.IsEmpty() )
	{
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), yaw );
	}
	m_slider_y.SetPos( yaw );	

	int x_slider_pos = m_slider_x.GetPos();
	int y_slider_pos = m_slider_y.GetPos();
	int z_slider_pos = m_slider_z.GetPos();
	
	// Build the current object's orientation
	Quat orient = m_orient;
	orient.RotateX( DegreesToRadians(x_slider_pos) );
	orient.RotateY( DegreesToRadians(y_slider_pos) );
	orient.RotateZ( DegreesToRadians(z_slider_pos) );
	
	// Set the new orientation		
	gGizmoManager.SetOrientation( orient.BuildMatrix() );		
}

void PropPage_Position::OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_x.SetPos( m_slider_x.GetPos() + pNMUpDown->iDelta );
	
	*pResult = 0;
}

void PropPage_Position::OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_y.SetPos( m_slider_y.GetPos() + pNMUpDown->iDelta );
	
	*pResult = 0;
}

void PropPage_Position::OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_z.SetPos( m_slider_z.GetPos() + pNMUpDown->iDelta );	
	*pResult = 0;
}
