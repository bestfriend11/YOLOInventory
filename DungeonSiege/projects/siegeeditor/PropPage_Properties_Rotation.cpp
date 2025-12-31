// PropPage_Properties_Rotation.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Rotation.h"
#include "math.h"
#include "matrix_3x3.h"
#include "space_3.h"
#include "Preferences.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Rotation property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Rotation, CResizablePage)

PropPage_Properties_Rotation::PropPage_Properties_Rotation() : CResizablePage(PropPage_Properties_Rotation::IDD)
{
	m_bApply = false;
	m_bParamsInit = false;

	//{{AFX_DATA_INIT(PropPage_Properties_Rotation)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

PropPage_Properties_Rotation::~PropPage_Properties_Rotation()
{
}

void PropPage_Properties_Rotation::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Rotation)
	DDX_Control(pDX, IDC_SPIN_SCALE, m_spin_scale);
	DDX_Control(pDX, IDC_EDIT_SCALE, m_edit_scale);
	DDX_Control(pDX, IDC_EDIT_ROTATION_SNAP_INCREMENT, m_rotationIncrement);
	DDX_Control(pDX, IDC_SPIN_Z, m_spin_z);
	DDX_Control(pDX, IDC_SPIN_Y, m_spin_y);
	DDX_Control(pDX, IDC_SPIN_X, m_spin_x);
	DDX_Control(pDX, IDC_SLIDER_Z, m_slider_z);
	DDX_Control(pDX, IDC_SLIDER_Y, m_slider_y);
	DDX_Control(pDX, IDC_SLIDER_X, m_slider_x);
	DDX_Control(pDX, IDC_COMBO_DEGREES_Z, m_degrees_z);
	DDX_Control(pDX, IDC_COMBO_DEGREES_Y, m_degrees_y);
	DDX_Control(pDX, IDC_COMBO_DEGREES_X, m_degrees_x);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Rotation, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Rotation)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_X, OnDeltaposSpinX)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Y, OnDeltaposSpinY)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Z, OnDeltaposSpinZ)
	ON_CBN_SELCHANGE(IDC_COMBO_DEGREES_X, OnSelchangeComboDegreesX)
	ON_CBN_SELCHANGE(IDC_COMBO_DEGREES_Y, OnSelchangeComboDegreesY)
	ON_CBN_SELCHANGE(IDC_COMBO_DEGREES_Z, OnSelchangeComboDegreesZ)
	ON_EN_CHANGE(IDC_EDIT_ROTATION_SNAP_INCREMENT, OnChangeEditRotationSnapIncrement)
	ON_CBN_EDITCHANGE(IDC_COMBO_DEGREES_X, OnEditchangeComboDegreesX)
	ON_CBN_EDITCHANGE(IDC_COMBO_DEGREES_Y, OnEditchangeComboDegreesY)
	ON_CBN_EDITCHANGE(IDC_COMBO_DEGREES_Z, OnEditchangeComboDegreesZ)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_SCALE, OnDeltaposSpinScale)
	ON_EN_CHANGE(IDC_EDIT_SCALE, OnChangeEditScale)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Rotation message handlers

BOOL PropPage_Properties_Rotation::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR * pNm = (NMHDR *)lParam;
	if ( pNm->code == 0xffffff37 )
	{
		if ( gObjectProperties.Evaluate() )
		{
			int result = MessageBoxEx( this->GetSafeHwnd(), "You have changed the properties on this page.  Would you like to apply the changes?", "Apply Changes?", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES ) 
			{
				gObjectProperties.Apply();
			}	
		}
	}
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}

BOOL PropPage_Properties_Rotation::OnInitDialog() 
{
	CResizablePage::OnInitDialog();

	AddAnchor( IDC_SLIDER_X, MIDDLE_CENTER );
	AddAnchor( IDC_SLIDER_Y, MIDDLE_CENTER );
	AddAnchor( IDC_SLIDER_Z, MIDDLE_CENTER );
	AddAnchor( IDC_SPIN_X, MIDDLE_CENTER );
	AddAnchor( IDC_SPIN_Y, MIDDLE_CENTER );
	AddAnchor( IDC_SPIN_Z, MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_DEGREES_X, MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_DEGREES_Y, MIDDLE_CENTER );
	AddAnchor( IDC_COMBO_DEGREES_Z, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_X, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_Y, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_Z, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_INCREMENT_TEXT, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_ROTATION_SNAP_INCREMENT, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_SCALE, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_SCALE, MIDDLE_CENTER );
	AddAnchor( IDC_SPIN_SCALE, MIDDLE_CENTER );

	for ( int i = 0; i != 360; ++i )
	{
		gpstring sNum;
		sNum.assignf( "%d", i );
		m_degrees_x.AddString( sNum.c_str() );
		m_degrees_y.AddString( sNum.c_str() );
		m_degrees_z.AddString( sNum.c_str() );
	}

	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetRotationIncrement() );
	m_rotationIncrement.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gGizmoManager.GetBodyScale() );
	m_edit_scale.SetWindowText( sTemp.c_str() );

	Reset();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT PropPage_Properties_Rotation::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( m_bParamsInit ) {
		
		if ( m_slider_x.GetSafeHwnd() ) 
		{
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

				gpstring sNum;
				sNum.assignf( "%d", x_slider_pos );

				CString rDegrees;
				int degrees = 0;
				
				m_degrees_x.GetWindowText( rDegrees );
				if ( !sNum.same_no_case( rDegrees.GetBuffer( 0 ) ) )
				{
					m_degrees_x.SelectString( 0, sNum.c_str() );
				}
				
				sNum.assignf( "%d", y_slider_pos );

				m_degrees_y.GetWindowText( rDegrees );
				if ( !sNum.same_no_case( rDegrees.GetBuffer( 0 ) ) )
				{
					m_degrees_y.SelectString( 0, sNum.c_str() );
				}

				sNum.assignf( "%d", z_slider_pos );

				m_degrees_z.GetWindowText( rDegrees );
				if ( !sNum.same_no_case( rDegrees.GetBuffer( 0 ) ) )
				{				
					m_degrees_z.SelectString( 0, sNum.c_str() );
				}

				// Set the new orientation		
				gGizmoManager.SetOrientation( orient );			
			}
		}
	}
	
	return CResizablePage::WindowProc(message, wParam, lParam);
}


void PropPage_Properties_Rotation::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}

	m_orient = gGizmoManager.GetQuatOrientation();

	m_slider_x.SetText( "" );
	m_slider_y.SetText( "" );
	m_slider_z.SetText( "" );

	m_bParamsInit = true;	
	m_bApply = false;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Rotation::Apply()
{
	m_bApply = false;
	m_bParamsInit = false;
}


void PropPage_Properties_Rotation::Cancel()
{
	m_bParamsInit = false;
}

void PropPage_Properties_Rotation::OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_x.SetPos( m_slider_x.GetPos() + (pNMUpDown->iDelta * gPreferences.GetRotationIncrement() ) );
	
	*pResult = 0;
}

void PropPage_Properties_Rotation::OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_y.SetPos( m_slider_y.GetPos() + (pNMUpDown->iDelta * gPreferences.GetRotationIncrement() ) );
	
	*pResult = 0;
}

void PropPage_Properties_Rotation::OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	m_slider_z.SetPos( m_slider_z.GetPos() + (pNMUpDown->iDelta * gPreferences.GetRotationIncrement() ) );	
	
	*pResult = 0;
}

void PropPage_Properties_Rotation::OnSelchangeComboDegreesX() 
{
	CString rTemp;
	int sel = m_degrees_x.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_degrees_x.GetLBText( sel, rTemp );		
		int pitch = 0;
		if ( !rTemp.IsEmpty() )
		{
			stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), pitch );
		}
		m_slider_x.SetPos( pitch );
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );
	}	
}

void PropPage_Properties_Rotation::OnSelchangeComboDegreesY() 
{
	CString rTemp;
	int sel = m_degrees_y.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_degrees_y.GetLBText( sel, rTemp );		
	
		int yaw = 0;
		if ( !rTemp.IsEmpty() )
		{
			stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), yaw );
		}
		m_slider_y.SetPos( yaw );	
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );	
	}
}

void PropPage_Properties_Rotation::OnSelchangeComboDegreesZ() 
{
	CString rTemp;
	int sel = m_degrees_z.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_degrees_z.GetLBText( sel, rTemp );		
	
		int roll = 0;
		if ( !rTemp.IsEmpty() )
		{
			stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), roll );
		}
		m_slider_z.SetPos( roll );	
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );	
	}
}

void PropPage_Properties_Rotation::OnChangeEditRotationSnapIncrement() 
{
	CString rTemp;
	m_rotationIncrement.GetWindowText( rTemp );
	float rotation = 0;
	stringtool::Get( rTemp.GetBuffer( 0 ), rotation );
	gPreferences.SetRotationIncrement( rotation );
}

void PropPage_Properties_Rotation::OnEditchangeComboDegreesX() 
{
	CString rDegrees;
	int degrees = 0;
	m_degrees_x.GetWindowText( rDegrees );
	stringtool::Get( rDegrees.GetBuffer( 0 ), degrees );	
	if ( degrees >= 0 && degrees <= 360 )
	{
		m_slider_x.SetPos( degrees );	
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );			
	}
}

void PropPage_Properties_Rotation::OnEditchangeComboDegreesY() 
{
	CString rDegrees;
	int degrees = 0;
	m_degrees_y.GetWindowText( rDegrees );
	stringtool::Get( rDegrees.GetBuffer( 0 ), degrees );	
	if ( degrees >= 0 && degrees <= 360 )
	{
		m_slider_y.SetPos( degrees );	
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );		
	}	
}

void PropPage_Properties_Rotation::OnEditchangeComboDegreesZ() 
{
		CString rDegrees;
	int degrees = 0;
	m_degrees_z.GetWindowText( rDegrees );
	stringtool::Get( rDegrees.GetBuffer( 0 ), degrees );	
	if ( degrees >= 0 && degrees <= 360 )
	{
		m_slider_z.SetPos( degrees );	
		Invalidate( TRUE );

		int x_slider_pos = m_slider_x.GetPos();
		int y_slider_pos = m_slider_y.GetPos();
		int z_slider_pos = m_slider_z.GetPos();
		
		// Build the current object's orientation
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(x_slider_pos) );
		orient.RotateY( DegreesToRadians(y_slider_pos) );
		orient.RotateZ( DegreesToRadians(z_slider_pos) );
		
		// Set the new orientation		
		gGizmoManager.SetOrientation( orient );		
	}	
}

void PropPage_Properties_Rotation::OnDeltaposSpinScale(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	float scale = gGizmoManager.GetBodyScale() - (pNMUpDown->iDelta * 0.1f);
	if ( scale <= 0.0f )
	{
		return;
	}

	gGizmoManager.SetBodyScale( scale );
	gpstring sTemp;
	sTemp.assignf( "%f", gGizmoManager.GetBodyScale() );
	m_edit_scale.SetWindowText( sTemp.c_str() );

	
	*pResult = 0;
}

void PropPage_Properties_Rotation::OnChangeEditScale() 
{
	CString rText;
	m_edit_scale.GetWindowText( rText );
	float scale = 0;
	stringtool::Get( rText.GetBuffer( 0 ), scale );	

	if ( scale <= 0.0f )
	{
		return;
	}

	gGizmoManager.SetBodyScale( scale );		
}
