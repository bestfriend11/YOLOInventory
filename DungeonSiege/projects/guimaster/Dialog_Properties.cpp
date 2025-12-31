// Dialog_Properties.cpp : implementation file
//

#include "stdafx.h"
#include "IgnoredWarnings.h"
#include "RapiOwner.h"
#include "GUIMaster.h"
#include "Dialog_Properties.h"
#include "GUIMasterShell.h"
#include "ui_shell.h"
#include "stringtool.h"
#include "ui_textureman.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Properties dialog


CDialog_Properties::CDialog_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Properties)
	DDX_Control(pDX, IDC_EDIT_HEIGHT, m_edit_height);
	DDX_Control(pDX, IDC_EDIT_WIDTH, m_edit_width);
	DDX_Control(pDX, IDC_STATIC_TEXTURENAME, m_texture_name);
	DDX_Control(pDX, IDC_EDIT_Y2, m_edit_y2);
	DDX_Control(pDX, IDC_EDIT_Y1, m_edit_y1);
	DDX_Control(pDX, IDC_EDIT_X2, m_edit_x2);
	DDX_Control(pDX, IDC_EDIT_X1, m_edit_x1);
	DDX_Control(pDX, IDC_EDIT_V2, m_edit_v2);
	DDX_Control(pDX, IDC_EDIT_V1, m_edit_v1);
	DDX_Control(pDX, IDC_EDIT_U2, m_edit_u2);
	DDX_Control(pDX, IDC_EDIT_U1, m_edit_u1);
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_EDIT_ALPHA, m_edit_alpha);
	DDX_Control(pDX, IDC_CHECK_TILED, m_check_tiled);
	DDX_Control(pDX, IDC_CHECK_RIGHTANCHOR, m_check_right_anchor);
	DDX_Control(pDX, IDC_CHECK_BOTTOMANCHOR, m_check_bottom_anchor);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Properties, CDialog)
	//{{AFX_MSG_MAP(CDialog_Properties)
	ON_BN_CLICKED(IDC_BUTTON_LOADTEXTURE, OnButtonLoadtexture)
	ON_BN_CLICKED(IDC_BUTTON_NOTEXTURE, OnButtonNotexture)
	ON_BN_CLICKED(IDC_BUTTON_RESIZERECT, OnButtonResizerect)
	ON_BN_CLICKED(IDC_BUTTON_RESIZEUVRECT, OnButtonResizeuvrect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Properties message handlers

BOOL CDialog_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow != NULL )
	{
		gpstring sTemp;
		sTemp.assignf( "%d", pWindow->GetRect().left );
		m_edit_x1.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%d", pWindow->GetRect().right );
		m_edit_x2.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%d", pWindow->GetRect().top );
		m_edit_y1.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%d", pWindow->GetRect().bottom );
		m_edit_y2.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%d", pWindow->GetRect().right-pWindow->GetRect().left );
		m_edit_width.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%d", pWindow->GetRect().bottom-pWindow->GetRect().top );
		m_edit_height.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", pWindow->GetUVRect().left );
		m_edit_u1.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", pWindow->GetUVRect().right );
		m_edit_u2.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", pWindow->GetUVRect().top );
		m_edit_v1.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", pWindow->GetUVRect().bottom );
		m_edit_v2.SetWindowText( sTemp.c_str() );

		m_edit_name.SetWindowText( pWindow->GetName().c_str() );

		sTemp.assignf( "%f", pWindow->GetAlpha() );
		m_edit_alpha.SetWindowText( sTemp.c_str() );

		m_check_tiled.SetCheck( (int)pWindow->GetTiledTexture() );

		ANCHOR_DATA anchors = pWindow->GetAnchorData();		
		m_check_right_anchor.SetCheck( (int)anchors.bRightAnchor );
		m_check_bottom_anchor.SetCheck( (int)anchors.bBottomAnchor );

		if ( pWindow->GetTextureIndex() != 0 )
		{
			gpstring sPath = gUIShell.GetRenderer().GetTexturePathname( pWindow->GetTextureIndex() );
			m_texture_name.SetWindowText( sPath.c_str() );
		}

	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// Removes directories from a path
char * RemoveDirectories( char *name )
{
	for ( UINT i = 0; i < strlen( name ); ++i ) {
		if ( name[i] == '\\' ) {
			name = &(name[++i]);
			i = 0;
		}
	}		
	return name;
}


void CDialog_Properties::OnOK() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow != NULL )
	{
		CString rTemp;
		int		iTemp = 0;
		float	fTemp = 0.0f;
		m_edit_x1.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().left = iTemp;

		m_edit_x2.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().right = iTemp;

		m_edit_y1.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().top = iTemp;

		m_edit_y2.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().bottom = iTemp;

		m_edit_width.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().right = pWindow->GetRect().left + iTemp;

		m_edit_height.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), iTemp );
		pWindow->GetRect().bottom = pWindow->GetRect().top + iTemp;

		m_edit_u1.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), fTemp );
		float uvleft = fTemp;

		m_edit_u2.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), fTemp );
		float uvright = fTemp;

		m_edit_v1.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), fTemp );
		float uvtop = fTemp;

		m_edit_v2.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), fTemp );
		float uvbottom = fTemp;

		pWindow->SetUVRect( uvleft, uvright, uvtop, uvbottom );

		m_edit_name.GetWindowText( rTemp );
		pWindow->SetName( rTemp.GetBuffer( 0 ) );

		m_edit_alpha.GetWindowText( rTemp );
		stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), fTemp );
		pWindow->SetAlpha( fTemp );

		pWindow->SetTiledTexture( m_check_tiled.GetCheck() ? true : false );
		
		pWindow->GetAnchorData().bBottomAnchor	= m_check_bottom_anchor.GetCheck() ? true : false;
		pWindow->GetAnchorData().bRightAnchor	= m_check_right_anchor.GetCheck() ? true : false;

		gUIShell.AddWindowToInterface( pWindow, pWindow->GetInterfaceParent(), false );
		
		m_texture_name.GetWindowText( rTemp );
		if ( !rTemp.IsEmpty() )
		{
			gpstring sTemp = RemoveDirectories( rTemp.GetBuffer( 0 ) );
			stringtool::RemoveExtension( sTemp );
			pWindow->LoadTexture( sTemp );
			if ( pWindow->GetTextureIndex() != 0 )
			{
				pWindow->SetHasTexture( true );
			}
		}

	}
	
	CDialog::OnOK();
}

void CDialog_Properties::OnCancel() 
{	
	CDialog::OnCancel();
}

void CDialog_Properties::OnButtonLoadtexture() 
{
	OPENFILENAME ofn;
	char szFile[1024] = "";
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = this->GetSafeHwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "PSD\0*.PSD\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	HRESULT hRet = GetOpenFileName( &ofn );
	if ( hRet == 0 ) {
		return;
	}	

	gpstring sName = gGUIMaster.GetTextureNamingKeyName( ofn.lpstrFile );
	m_texture_name.SetWindowText( sName.c_str() );	
}

void CDialog_Properties::OnButtonNotexture() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow != NULL )
	{
		pWindow->SetTextureIndex( 0 );
		pWindow->SetHasTexture( false );
	}	
}

void CDialog_Properties::OnButtonResizerect() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow != NULL )
	{
		CString rTexture;
		m_texture_name.GetWindowText( rTexture );
		if ( rTexture.IsEmpty() == false )
		{
			unsigned int tex = gUITextureManager.LoadTexture( rTexture.GetBuffer( 0 ) );
			UIRect rect = pWindow->GetRect();
			gGUIMaster.ResizeRectToTexture( rect, tex );

			gpstring sTemp;
			sTemp.assignf( "%d", rect.right );
			m_edit_x2.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%d", rect.bottom );
			m_edit_y2.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%d", rect.right-rect.left );
			m_edit_width.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%d", rect.bottom-rect.top );
			m_edit_height.SetWindowText( sTemp.c_str() );
		}
	}	
}

void CDialog_Properties::OnButtonResizeuvrect() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow != NULL )
	{
		CString rTexture;
		m_texture_name.GetWindowText( rTexture );
		if ( rTexture.IsEmpty() == false )
		{
			unsigned int tex = gUITextureManager.LoadTexture( rTexture.GetBuffer( 0 ) );
			UINormalizedRect uvrect = pWindow->GetUVRect();
			gGUIMaster.ResizeUVRectToTexture( uvrect, pWindow->GetRect(), tex );

			gpstring sTemp;
			sTemp.assignf( "%f", uvrect.left );
			m_edit_u1.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%f", uvrect.right );
			m_edit_u2.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%f", uvrect.top );
			m_edit_v1.SetWindowText( sTemp.c_str() );

			sTemp.assignf( "%f", uvrect.bottom );
			m_edit_v2.SetWindowText( sTemp.c_str() );
		}
	}		
}
