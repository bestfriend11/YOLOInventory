// Dialog_Decal.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Decal.h"
#include "EditorGizmos.h"
#include "siege_decal_database.h"
#include "siege_decal.h"
#include "namingkey.h"
#include "Preferences.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal dialog


Dialog_Decal::Dialog_Decal(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Decal::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Decal)
	//}}AFX_DATA_INIT
}


void Dialog_Decal::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Decal)
	DDX_Control(pDX, IDC_EDIT_LOD, m_lod);
	DDX_Control(pDX, IDC_EDIT_ID, m_id);
	DDX_Control(pDX, IDC_CHECK_VISIBLE, m_visible);
	DDX_Control(pDX, IDC_STATIC_NODE, m_pos_node);
	DDX_Control(pDX, IDC_STATIC_Z, m_pos_z);
	DDX_Control(pDX, IDC_STATIC_Y, m_pos_y);
	DDX_Control(pDX, IDC_STATIC_X, m_pos_x);
	DDX_Control(pDX, IDC_SPIN_Z, m_spin_z);
	DDX_Control(pDX, IDC_SPIN_Y, m_spin_y);
	DDX_Control(pDX, IDC_SPIN_X, m_spin_x);
	DDX_Control(pDX, IDC_EDIT_NEAR_PLANE, m_near_plane);
	DDX_Control(pDX, IDC_EDIT_FAR_PLANE, m_far_plane);
	DDX_Control(pDX, IDC_COMBO_Z, m_combo_z);
	DDX_Control(pDX, IDC_COMBO_Y, m_combo_y);
	DDX_Control(pDX, IDC_COMBO_X, m_combo_x);
	DDX_Control(pDX, IDC_STATIC_TEXTURE, m_texture_name);
	DDX_Control(pDX, IDC_EDIT_VERTICAL, m_vertical);
	DDX_Control(pDX, IDC_EDIT_HORIZONTAL, m_horizontal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Decal, CDialog)
	//{{AFX_MSG_MAP(Dialog_Decal)
	ON_EN_CHANGE(IDC_EDIT_HORIZONTAL, OnChangeEditHorizontal)
	ON_EN_CHANGE(IDC_EDIT_VERTICAL, OnChangeEditVertical)
	ON_EN_CHANGE(IDC_EDIT_NEAR_PLANE, OnChangeEditNearPlane)
	ON_EN_CHANGE(IDC_EDIT_FAR_PLANE, OnChangeEditFarPlane)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_X, OnDeltaposSpinX)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Y, OnDeltaposSpinY)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_Z, OnDeltaposSpinZ)
	ON_CBN_SELCHANGE(IDC_COMBO_X, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO_Y, OnSelchangeComboY)
	ON_CBN_SELCHANGE(IDC_COMBO_Z, OnSelchangeComboZ)
	ON_BN_CLICKED(IDC_CHECK_VISIBLE, OnCheckVisible)
	ON_EN_CHANGE(IDC_EDIT_LOD, OnChangeEditLod)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal message handlers

void Dialog_Decal::OnOK() 
{
	CDialog::OnOK();
}

BOOL Dialog_Decal::OnInitDialog() 
{
	CDialog::OnInitDialog();

	gpstring sTemp;
	for ( int i = 0; i != 360; ++i )
	{
		sTemp.assignf( "%d", i );
		m_combo_x.AddString( sTemp.c_str() );
		m_combo_y.AddString( sTemp.c_str() );
		m_combo_z.AddString( sTemp.c_str() );
	}

	Reset();
				
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Decal::Reset()
{
	m_selected.clear();

	std::vector< DWORD > decals;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetGizmosOfType( GIZMO_DECAL, decals );
	
	for ( i = decals.begin(); i != decals.end(); ++i )
	{
		if ( gGizmoManager.GetGizmo( *i )->bSelected )
		{
			m_selected.push_back( *i );
		}
	}
	

	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		if ( gGizmoManager.GetGizmo( *i )->bSelected )
		{			
			SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );		

			if ( m_selected.size() == 1 )
			{
				gpstring sId;
				sId.assignf( "0x%08X", pDecal->GetGUID().GetValue() );
				m_id.SetWindowText( sId.c_str() );
				if ( pDecal->GetActive() )
				{
					m_visible.SetCheck( TRUE );
				}
			}

			gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
			m_texture_name.SetWindowText( sTexture.c_str() );
			
			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			gpstring sTemp;
			sTemp.assignf( "%f", pDecal->GetHorizontalMeters() );
			m_horizontal.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );
			
			sTemp.assignf( "%f", pDecal->GetVerticalMeters() );
			m_vertical.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );
			
			sTemp.assignf( "%f", pDecal->GetNearPlane() );
			m_near_plane.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );
			
			sTemp.assignf( "%f", pDecal->GetFarPlane() );
			m_far_plane.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			sTemp.assignf( "%f", pDecal->GetDecalOrigin().pos.x );
			m_pos_x.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			sTemp.assignf( "%f", pDecal->GetDecalOrigin().pos.y );
			m_pos_y.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			sTemp.assignf( "%f", pDecal->GetDecalOrigin().pos.z );
			m_pos_z.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			sTemp.assignf( "0x%08X", pDecal->GetDecalOrigin().node.GetValue() );
			m_pos_node.SetWindowText( sTemp.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			gpstring sLod;
			sLod.assignf( "%f", pDecal->GetLevelOfDetail() );
			m_lod.SetWindowText( sLod.c_str() );

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			m_combo_x.SetCurSel( 0 );
			m_combo_y.SetCurSel( 0 );
			m_combo_z.SetCurSel( 0 );	

			pDecal = gSiegeDecalDatabase.GetDecalPointer( gGizmoManager.GetGizmo( *i )->index );

			m_orient = Quat( pDecal->GetDecalOrientation() );			

			return;		
		}
	}	
}

void Dialog_Decal::OnChangeEditHorizontal() 
{	
	CString rHoriz;
	m_horizontal.GetWindowText( rHoriz );
	float horiz = 0;
	stringtool::Get( rHoriz.GetBuffer( 0 ), horiz );

	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		
		if ( pDecal->GetHorizontalMeters() != horiz )
		{
			siege::database_guid decalGuid = pDecal->GetGUID();
			int index = pGizmo->index;
			float lod = pDecal->GetLevelOfDetail();
			gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );		
			pGizmo->index = gSiegeDecalDatabase.CreateDecal( pDecal->GetDecalOrigin(), pDecal->GetDecalOrientation(), 
															 horiz, pDecal->GetVerticalMeters(), 
															 pDecal->GetNearPlane(), pDecal->GetFarPlane(), sTexture );
			gSiegeDecalDatabase.DestroyDecal( index );		

			pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
			pDecal->SetGUID( decalGuid );
			pDecal->SetLevelOfDetail( lod );
		}
	}	

	std::vector< DWORD > decals;
	gGizmoManager.GetGizmosOfType( GIZMO_DECAL, decals );
	m_selected.clear();
	for ( i = decals.begin(); i != decals.end(); ++i )
	{
		if ( gGizmoManager.GetGizmo( *i )->bSelected )
		{
			m_selected.push_back( *i );
		}
	}
}

void Dialog_Decal::OnChangeEditVertical() 
{
	CString rVert;
	m_vertical.GetWindowText( rVert );
	float vert = 0;
	stringtool::Get( rVert.GetBuffer( 0 ), vert );

	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );

		if ( pDecal->GetVerticalMeters() != vert )
		{
			siege::database_guid decalGuid = pDecal->GetGUID();
			float lod = pDecal->GetLevelOfDetail();
			int index = pGizmo->index;
			gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( pDecal->GetDecalTexture() );
			pGizmo->index = gSiegeDecalDatabase.CreateDecal( pDecal->GetDecalOrigin(), pDecal->GetDecalOrientation(), 
															 pDecal->GetHorizontalMeters(), vert, 
															 pDecal->GetNearPlane(), pDecal->GetFarPlane(), sTexture );
			gSiegeDecalDatabase.DestroyDecal( index );
			pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
			pDecal->SetGUID( decalGuid );
			pDecal->SetLevelOfDetail( lod );
		}
	}	

	std::vector< DWORD > decals;
	gGizmoManager.GetGizmosOfType( GIZMO_DECAL, decals );
	m_selected.clear();
	for ( i = decals.begin(); i != decals.end(); ++i )
	{
		if ( gGizmoManager.GetGizmo( *i )->bSelected )
		{
			m_selected.push_back( *i );
		}
	}
}


void Dialog_Decal::OnChangeEditNearPlane() 
{
	CString rNear;
	m_near_plane.GetWindowText( rNear );
	float nearPlane = 0;
	stringtool::Get( rNear.GetBuffer( 0 ), nearPlane );

	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		pDecal->SetNearPlane( nearPlane );
	}	
}

void Dialog_Decal::OnChangeEditFarPlane() 
{
	CString rFar;
	m_far_plane.GetWindowText( rFar );
	float farPlane = 0;
	stringtool::Get( rFar.GetBuffer( 0 ), farPlane );

	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		pDecal->SetFarPlane( farPlane );
	}	

}

void Dialog_Decal::OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta > 0 )
	{
		if ( m_combo_x.GetCurSel() == 359 )
		{
			m_combo_x.SetCurSel( 0 );
		}
		else
		{
			m_combo_x.SetCurSel( m_combo_x.GetCurSel() + gPreferences.GetRotationIncrement() );
		}
	}
	else
	{
		if ( m_combo_x.GetCurSel() == 0 )
		{
			m_combo_x.SetCurSel( 359 );
		}
		else
		{
			m_combo_x.SetCurSel( m_combo_x.GetCurSel() - gPreferences.GetRotationIncrement() );
		}
	}

	CalculateOrientation();
	
	*pResult = 0;
}

void Dialog_Decal::OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta > 0 )
	{
		if ( m_combo_y.GetCurSel() == 359 )
		{
			m_combo_y.SetCurSel( 0 );
		}
		else
		{
			m_combo_y.SetCurSel( m_combo_y.GetCurSel() + gPreferences.GetRotationIncrement() );
		}
	}
	else
	{
		if ( m_combo_y.GetCurSel() == 0 )
		{
			m_combo_y.SetCurSel( 359 );
		}
		else
		{
			m_combo_y.SetCurSel( m_combo_y.GetCurSel() - gPreferences.GetRotationIncrement() );
		}
	}

	CalculateOrientation();
	
	*pResult = 0;
}

void Dialog_Decal::OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	if ( pNMUpDown->iDelta > 0 )
	{
		if ( m_combo_z.GetCurSel() == 359 )
		{
			m_combo_z.SetCurSel( 0 );
		}
		else
		{
			m_combo_z.SetCurSel( m_combo_z.GetCurSel() + gPreferences.GetRotationIncrement() );
		}
	}
	else
	{
		if ( m_combo_z.GetCurSel() == 0 )
		{
			m_combo_z.SetCurSel( 359 );
		}
		else
		{
			m_combo_z.SetCurSel( m_combo_z.GetCurSel() - gPreferences.GetRotationIncrement() );
		}
	}

	CalculateOrientation();
	
	*pResult = 0;
}

void Dialog_Decal::OnSelchangeComboX() 
{	
	CalculateOrientation();
}

void Dialog_Decal::OnSelchangeComboY() 
{
	CalculateOrientation();	
}

void Dialog_Decal::OnSelchangeComboZ() 
{
	CalculateOrientation();
}


void Dialog_Decal::CalculateOrientation()
{
	float xAngle = 0.0f;
	float yAngle = 0.0f;
	float zAngle = 0.0f;
	CString rText;

	int sel = m_combo_x.GetCurSel();
	if ( sel != CB_ERR )
	{		
		m_combo_x.GetLBText( sel, rText );
		stringtool::Get( rText.GetBuffer( 0 ), xAngle );
	}

	sel = m_combo_y.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_combo_y.GetLBText( sel, rText );
		stringtool::Get( rText.GetBuffer( 0 ), yAngle );		
	}

	sel = m_combo_z.GetCurSel();
	if ( sel != CB_ERR )
	{
		m_combo_z.GetLBText( sel, rText );
		stringtool::Get( rText.GetBuffer( 0 ), zAngle );		
	}
	
	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );		
		
		Quat orient = m_orient;
		orient.RotateX( DegreesToRadians(xAngle) );
		orient.RotateY( DegreesToRadians(yAngle) );
		orient.RotateZ( DegreesToRadians(zAngle) );		
		pDecal->SetDecalOrientation( orient.BuildMatrix() );
	}
}

void Dialog_Decal::OnCheckVisible() 
{
	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );
		pDecal->SetActive( m_visible.GetCheck() ? true : false );
	}		
}

void Dialog_Decal::OnChangeEditLod() 
{
	std::vector< DWORD >::iterator i;
	for ( i = m_selected.begin(); i != m_selected.end(); ++i )
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		SiegeDecal * pDecal = gSiegeDecalDatabase.GetDecalPointer( pGizmo->index );

		CString rDetail;
		m_lod.GetWindowText( rDetail );
		if ( !rDetail.IsEmpty() )
		{
			float detail = 0.0f;
			stringtool::Get( rDetail.GetBuffer( 0 ), detail );
			pDecal->SetLevelOfDetail( detail );
		}
	}		
}

void Dialog_Decal::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
