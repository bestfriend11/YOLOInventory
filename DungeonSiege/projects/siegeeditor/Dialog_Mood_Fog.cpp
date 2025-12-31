// Dialog_Mood_Fog.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Fog.h"
#include "Dialog_Mood_Editor.h"
#include "stringtool.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Fog dialog


Dialog_Mood_Fog::Dialog_Mood_Fog(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Fog::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Fog)
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Fog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Fog)
	DDX_Control(pDX, IDC_STATIC_COLOR, m_FogColor);
	DDX_Control(pDX, IDC_EDIT_NEAR_DISTANCE, m_NearDistance);
	DDX_Control(pDX, IDC_EDIT_LD_NEAR_DISTANCE, m_LdNearDistance);
	DDX_Control(pDX, IDC_EDIT_LD_FAR_DISTANCE, m_LdFarDistance);
	DDX_Control(pDX, IDC_EDIT_FOG_DENSITY, m_FogDensity);
	DDX_Control(pDX, IDC_EDIT_FAR_DISTANCE, m_FarDistance);
	DDX_Control(pDX, IDC_CHECK_FOG_ENABLE, m_FogEnable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Fog, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Fog)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_COLOR, OnButtonChangeColor)
	ON_BN_CLICKED(IDC_CHECK_FOG_ENABLE, OnCheckFogEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Fog message handlers

BOOL Dialog_Mood_Fog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hFog = hMood->GetChildBlock( "fog" );
		if ( hFog )
		{
			m_FogEnable.SetCheck( TRUE );

			float	nearDist	= 0.0f;
			float	farDist		= 0.0f;
			float	ldNearDist	= 0.0f;
			float	ldFarDist	= 0.0f;
			int		color		= 0;
			float	density		= 0.0f;

			hFog->Get( "fog_near_dist", nearDist );
			hFog->Get( "fog_far_dist", farDist );
			hFog->Get( "fog_lowdetail_near_dist", ldNearDist );
			hFog->Get( "fog_lowdetail_far_dist", ldFarDist );
			hFog->Get( "fog_color", color );
			hFog->Get( "fog_density", density );

			gpstring sTemp;

			sTemp.assignf( "%f", nearDist );
			m_NearDistance.SetWindowText( sTemp.c_str() );
			
			sTemp.assignf( "%f", farDist );
			m_FarDistance.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%f", ldNearDist );
			m_LdNearDistance.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%f", ldFarDist );
			m_LdFarDistance.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "0x%x", color );
			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (color & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (color & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (color & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );				
			gDialogMoodEditor.SetFogColor( colorref );

			sTemp.assignf( "%f", density );
			m_FogDensity.SetWindowText( sTemp.c_str() );
		}
		else
		{
			m_NearDistance.EnableWindow( FALSE );
			m_FarDistance.EnableWindow( FALSE );
			m_LdNearDistance.EnableWindow( FALSE );
			m_LdFarDistance.EnableWindow( FALSE );
			m_FogDensity.EnableWindow( FALSE );
		}
	}

	m_FogColor.EnableWindow( FALSE );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mood_Fog::OnOK() 
{
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hFog = hMood->GetChildBlock( "fog" );
		if ( m_FogEnable.GetCheck() )
		{	
			if ( !hFog )
			{
				hFog = hMood->CreateChildBlock( "fog" );
			}
			
			if ( hFog )
			{
				m_FogEnable.SetCheck( TRUE );

				CString rTemp;
				float fTemp = 0.0f;

				m_NearDistance.GetWindowText( rTemp );	
				if ( rTemp.IsEmpty() )
				{
					MessageBoxEx( this->GetSafeHwnd(), "Please enter the near distance for the fog.", "Fog Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );				
				hFog->Set( "fog_near_dist", fTemp );

				m_FarDistance.GetWindowText( rTemp );							
				if ( rTemp.IsEmpty() )
				{
					MessageBoxEx( this->GetSafeHwnd(), "Please enter the far distance for the fog.", "Fog Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );				
				hFog->Set( "fog_far_dist", fTemp );

				m_LdNearDistance.GetWindowText( rTemp );							
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );				
				hFog->Set( "fog_lowdetail_near_dist", fTemp );

				m_LdFarDistance.GetWindowText( rTemp );							
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );				
				hFog->Set( "fog_lowdetail_far_dist", fTemp );	
				
				m_FogDensity.GetWindowText( rTemp );	
				if ( rTemp.IsEmpty() )
				{
					MessageBoxEx( this->GetSafeHwnd(), "Please enter the density of the fog.", "Fog Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );				
				hFog->Set( "fog_density", fTemp );

				COLORREF color = gDialogMoodEditor.GetFogColor();
				float blue	= (float)(GetBValue(color) ) / 255.0f;
				float green	= (float)(GetGValue(color) ) / 255.0f;
				float red	= (float)(GetRValue(color) ) / 255.0f; 
				vector_3 vcolor( red, green, blue );
				DWORD dcolor = MAKEDWORDCOLOR(vcolor);
				hFog->Set( "fog_color", dcolor, FVP_HEX_INT );
			}
		}
		else
		{
			if ( hFog )
			{
				hMood->DestroyChildBlock( hFog );
			}
		}

		hMood->GetDB()->SaveChanges();		
	}
	
	CDialog::OnOK();
}

void Dialog_Mood_Fog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Mood_Fog::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

LRESULT Dialog_Mood_Fog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( message == WM_CTLCOLORSTATIC ) 
	{		
		int id = GetWindowLong( (HWND)lParam, GWL_ID );
		if ( id == IDC_STATIC_COLOR ) 
		{						
			if ( m_hBrush ) 
			{
				DeleteObject( m_hBrush );
			}

			m_hBrush = CreateSolidBrush( gDialogMoodEditor.GetFogColor() );

			return (LRESULT)m_hBrush;
		}
	}	
	
	
	return CDialog::WindowProc(message, wParam, lParam);
}

void Dialog_Mood_Fog::OnButtonChangeColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	cc.rgbResult		= gDialogMoodEditor.GetFogColor();
	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= GetSafeHwnd();
	cc.hInstance		= 0;
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) 
	{
		return;
	}
	
	gDialogMoodEditor.SetFogColor( cc.rgbResult );	
	
	m_FogColor.Invalidate( TRUE );
	
	SetCustomColors( crCustColors );	
}

void Dialog_Mood_Fog::OnCheckFogEnable() 
{
	m_NearDistance.EnableWindow( m_FogEnable.GetCheck() );
	m_FarDistance.EnableWindow( m_FogEnable.GetCheck() );
	m_LdNearDistance.EnableWindow( m_FogEnable.GetCheck() );
	m_LdFarDistance.EnableWindow( m_FogEnable.GetCheck() );
	m_FogDensity.EnableWindow( m_FogEnable.GetCheck() );
}
