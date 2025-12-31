// Dialog_Camera_Controller.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Camera_Controller.h"
#include "EditorCamera.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Camera_Controller dialog


Dialog_Camera_Controller::Dialog_Camera_Controller(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Camera_Controller::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Camera_Controller)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Camera_Controller::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Camera_Controller)
	DDX_Control(pDX, IDC_EDIT_SNAP_INCREMENT, m_snapIncrement);
	DDX_Control(pDX, IDC_EDIT_TRANSLATION, m_translation);
	DDX_Control(pDX, IDC_EDIT_ROTATION, m_rotation);
	DDX_Control(pDX, IDC_EDIT_DISTANCE, m_distance);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Camera_Controller, CDialog)
	//{{AFX_MSG_MAP(Dialog_Camera_Controller)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_AZIMUTH, OnDeltaposSpinAzimuth)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_FORWARDBACKWARD, OnDeltaposSpinForwardbackward)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_INOUT, OnDeltaposSpinInout)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LEFTRIGHT, OnDeltaposSpinLeftright)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ORBIT, OnDeltaposSpinOrbit)
	ON_EN_CHANGE(IDC_EDIT_TRANSLATION, OnChangeEditTranslation)
	ON_EN_CHANGE(IDC_EDIT_ROTATION, OnChangeEditRotation)
	ON_EN_CHANGE(IDC_EDIT_DISTANCE, OnChangeEditDistance)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ORBIT_SNAP, OnDeltaposSpinOrbitSnap)
	ON_BN_CLICKED(IDC_BUTTON_AZIMUTH_RESET, OnButtonAzimuthReset)
	ON_BN_CLICKED(IDC_BUTTON_ORBIT_RESET, OnButtonOrbitReset)
	ON_EN_CHANGE(IDC_EDIT_SNAP_INCREMENT, OnChangeEditSnapIncrement)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_AZIMUTH_SNAP, OnDeltaposSpinAzimuthSnap)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Camera_Controller message handlers

void Dialog_Camera_Controller::OnDeltaposSpinAzimuth(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	float magnitude = RadiansToDegrees(gEditorCamera.GetAzimuth()) + (gPreferences.GetCameraRotateStep() * pNMUpDown->iDelta);
	if ( magnitude >= 90 )
	{
		magnitude = 89;
	}
	else if ( magnitude < 0 )
	{
		magnitude = 0;
	}

	gEditorCamera.SetAzimuthAngle( DegreesToRadians(magnitude) );
	gEditorCamera.Update();
	
	*pResult = 0;
}

void Dialog_Camera_Controller::OnDeltaposSpinForwardbackward(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta >= 0 )
	{
		gEditorCamera.Scroll( E, gPreferences.GetCameraTranslateStep() * abs(pNMUpDown->iDelta) );
	}
	else
	{
		gEditorCamera.Scroll( W, gPreferences.GetCameraTranslateStep() * abs(pNMUpDown->iDelta) );
	}
	
	*pResult = 0;
}

void Dialog_Camera_Controller::OnDeltaposSpinInout(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta >= 0 )
	{
		gEditorCamera.UpdateZoom( S, abs(pNMUpDown->iDelta) * gPreferences.GetCameraZoomStep() );
	}
	else
	{
		gEditorCamera.UpdateZoom( N, abs(pNMUpDown->iDelta) * gPreferences.GetCameraZoomStep() );
	}
	
	*pResult = 0;
}

void Dialog_Camera_Controller::OnDeltaposSpinLeftright(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta >= 0 )
	{
		gEditorCamera.Scroll( S, gPreferences.GetCameraTranslateStep() * abs(pNMUpDown->iDelta) );
	}
	else
	{
		gEditorCamera.Scroll( N, gPreferences.GetCameraTranslateStep() * abs(pNMUpDown->iDelta) );
	}
	
	*pResult = 0;
}

void Dialog_Camera_Controller::OnDeltaposSpinOrbit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	float magnitude = RadiansToDegrees(gEditorCamera.GetOrbit()) + (gPreferences.GetCameraRotateStep() * pNMUpDown->iDelta);
	if ( magnitude > 360 ) 
	{
		magnitude = 0;
	}
	else if ( magnitude < 0 )
	{
		magnitude = 360;
	}

	gEditorCamera.SetOrbitAngle( DegreesToRadians(magnitude) );
	gEditorCamera.Update();
	
	*pResult = 0;
}

BOOL Dialog_Camera_Controller::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetCameraTranslateStep() );
	m_translation.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetCameraRotateStep() );
	m_rotation.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetCameraZoomStep() );
	m_distance.SetWindowText( sTemp.c_str() );	

	sTemp.assignf( "%f", gPreferences.GetCameraSnapIncrement() );
	m_snapIncrement.SetWindowText( sTemp.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Camera_Controller::OnChangeEditTranslation() 
{
	float value = 0;
	CString rTemp;
	m_translation.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), value );
	gPreferences.SetCameraTranslateStep( value );
}

void Dialog_Camera_Controller::OnChangeEditRotation() 
{
	float value = 0;
	CString rTemp;
	m_rotation.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), value );
	gPreferences.SetCameraRotateStep( value );	
}

void Dialog_Camera_Controller::OnChangeEditDistance() 
{
	float value = 0;
	CString rTemp;
	m_distance.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), value );
	gPreferences.SetCameraZoomStep( value );	
}

void Dialog_Camera_Controller::OnDeltaposSpinOrbitSnap(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	float magnitude = RadiansToDegrees(gEditorCamera.GetOrbit()) + gPreferences.GetCameraSnapIncrement();
	if ( magnitude > 360 ) 
	{
		magnitude = magnitude - 360;
	}
	else if ( magnitude < 0 )
	{
		magnitude = 360 - magnitude;
	}

	gEditorCamera.SetOrbitAngle( DegreesToRadians(magnitude) );
	gEditorCamera.Update();
	
	*pResult = 0;
}

void Dialog_Camera_Controller::OnButtonAzimuthReset() 
{
	gEditorCamera.OverheadView();	
}

void Dialog_Camera_Controller::OnButtonOrbitReset() 
{
	gEditorCamera.SetOrbitAngle( 0 );
	gEditorCamera.Update();
}

void Dialog_Camera_Controller::OnChangeEditSnapIncrement() 
{
	CString rIncrement;
	m_snapIncrement.GetWindowText( rIncrement );
	float increment = 0.0f;
	stringtool::Get( rIncrement.GetBuffer( 0 ), increment );
	gPreferences.SetCameraSnapIncrement( increment );
}

void Dialog_Camera_Controller::OnDeltaposSpinAzimuthSnap(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	float magnitude = RadiansToDegrees(gEditorCamera.GetAzimuth()) + gPreferences.GetCameraSnapIncrement();
	if ( magnitude > 360 ) 
	{
		magnitude = magnitude - 360;
	}
	else if ( magnitude < 0 )
	{
		magnitude = 360 - magnitude;
	}

	gEditorCamera.SetAzimuthAngle( DegreesToRadians(magnitude) );
	gEditorCamera.Update();
		
	*pResult = 0;
}
