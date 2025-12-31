// Dialog_Text.cpp : implementation file
//

#include "stdafx.h"
#include "MULTIMON.H"
#include "GUIMaster.h"
#include "Dialog_Text.h"

#include "gpcore.h"
#include "gpstring.h"
#include "GUIMasterShell.h"
#include "ui_shell.h"
#include "ui_text.h"
#include "ui_textbox.h"
#include "ui_editbox.h"
#include "ui_textureman.h"
#include "RapiOwner.h"
#include "RapiFont.h"
#include "fuel.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Text dialog


CDialog_Text::CDialog_Text(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Text::IDD, pParent)
	, m_dwColor( 0 )
{
	//{{AFX_DATA_INIT(CDialog_Text)
	//}}AFX_DATA_INIT
}


void CDialog_Text::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Text)
	DDX_Control(pDX, IDC_COMBO_JUSTIFY, m_justify);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_edit_text);
	DDX_Control(pDX, IDC_COMBO_FONT, m_combo_font);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Text, CDialog)
	//{{AFX_MSG_MAP(CDialog_Text)
	ON_BN_CLICKED(IDC_BUTTON_COLOR, OnButtonColor)
	ON_BN_CLICKED(IDC_BUTTON_TEST, OnButtonTest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Text message handlers

void CDialog_Text::OnOK() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{			
		CString rTemp;
		switch ( pWindow->GetType() )
		{
		case UI_TYPE_TEXT:
			{
				UIText * pText = (UIText *)pWindow;
				m_edit_text.GetWindowText( rTemp );
				pText->SetText( ToUnicode( rTemp.GetBuffer( 0 ) ) );

				int sel = m_combo_font.GetCurSel();				
				if ( sel != CB_ERR )
				{
					m_combo_font.GetLBText( sel, rTemp );
					
					pText->SetFont( gUITextureManager.LoadFont( rTemp.GetBuffer( 0 ), true ) );
				}

				int sel2 = m_justify.GetCurSel();
				if ( sel2 != CB_ERR )
				{
					m_justify.GetLBText( sel2, rTemp );
					
					if ( rTemp.CompareNoCase( "left" ) == 0 )
					{
						pText->SetJustification( JUSTIFY_LEFT );
					}
					else if ( rTemp.CompareNoCase( "right" ) == 0 )
					{
						pText->SetJustification( JUSTIFY_RIGHT );
					}
					else if ( rTemp.CompareNoCase( "center" ) == 0 )
					{
						pText->SetJustification( JUSTIFY_CENTER );
					}
				}

				pText->SetColor( m_dwColor );
			}
			break;
		case UI_TYPE_TEXTBOX:
			{
				UITextBox * pBox = (UITextBox *)pWindow;
				m_edit_text.GetWindowText( rTemp );
				pBox->SetText( ToUnicode( rTemp.GetBuffer( 0 ) ) );

				int sel = m_combo_font.GetCurSel();				
				if ( sel != CB_ERR )
				{
					m_combo_font.GetLBText( sel, rTemp );
					pBox->SetFont( gUITextureManager.LoadFont( rTemp.GetBuffer( 0 ), true ) );
				}

				int sel2 = m_justify.GetCurSel();
				if ( sel2 != CB_ERR )
				{
					m_justify.GetLBText( sel2, rTemp );
					
					if ( rTemp.CompareNoCase( "left" ) == 0 )
					{
						pBox->SetJustification( JUSTIFY_LEFT );
					}
					else if ( rTemp.CompareNoCase( "right" ) == 0 )
					{
						pBox->SetJustification( JUSTIFY_RIGHT );
					}
					else if ( rTemp.CompareNoCase( "center" ) == 0 )
					{
						pBox->SetJustification( JUSTIFY_CENTER );
					}
				}

				pBox->SetTextColor( m_dwColor );
			}
			break;
		case UI_TYPE_EDITBOX:
			{
				UIEditBox * pBox = (UIEditBox *)pWindow;
				
				m_edit_text.GetWindowText( rTemp );
				pBox->SetText( ToUnicode( rTemp.GetBuffer( 0 ) ) );

				int sel = m_combo_font.GetCurSel();				
				if ( sel != CB_ERR )
				{
					m_combo_font.GetLBText( sel, rTemp );
					pBox->SetFont( gUITextureManager.LoadFont( rTemp.GetBuffer( 0 ), true ) );
				}

				pBox->SetColor( m_dwColor );
			}
			break;
		}
	}
	
	CDialog::OnOK();
}


BOOL CDialog_Text::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{
		gpstring sText;
		gpstring sTexture;		
		gpstring sJustify;

		switch ( pWindow->GetType() )
		{
		case UI_TYPE_TEXT:
			{
				UIText * pText = (UIText *)pWindow;
				
				if ( pText->GetFont() )
				{
					sTexture = pText->GetFont()->GetTextureName();
					sText = ToAnsi( pText->GetText() );
					switch ( pText->GetJustification() )
					{
					case JUSTIFY_LEFT:
						sJustify = "left";
						break;
					case JUSTIFY_RIGHT:
						sJustify = "right";
						break;
					case JUSTIFY_CENTER:
						sJustify = "center";
						break;
					}
				}

				m_dwColor = pText->GetColor();
			}
			break;
		case UI_TYPE_TEXTBOX:
			{
				UITextBox * pBox = (UITextBox *)pWindow;
				m_edit_text.EnableWindow( false );

				if ( pBox->GetFont() )
				{
					sTexture = pBox->GetFont()->GetTextureName();
					switch ( pBox->GetJustification() )
					{
					case JUSTIFY_LEFT:
						sJustify = "left";
						break;
					case JUSTIFY_RIGHT:
						sJustify = "right";
						break;
					case JUSTIFY_CENTER:
						sJustify = "center";
						break;
					}
				}

				m_dwColor = pBox->GetTextColor();
			}
			break;
		case UI_TYPE_EDITBOX:
			{
				UIEditBox * pBox = (UIEditBox *)pWindow;

				if ( pBox->GetFont() )
				{
					sTexture = pBox->GetFont()->GetTextureName();
					sText = ToAnsi( pBox->GetText() );
				}

				m_dwColor = pBox->GetColor();
			}
			break;
		}

		gpstring sName = gGUIMaster.GetTextureNamingKeyName( sTexture );
		m_edit_text.SetWindowText( sText.c_str() );

		FuelHandle fhFont( "UI:Fonts" );
		FuelHandleList fhlFonts = fhFont->ListChildBlocks( -1, "font" );
		FuelHandleList::iterator i;
		for ( i = fhlFonts.begin(); i != fhlFonts.end(); ++i ) {
			m_combo_font.AddString( (*i)->GetName() );
		}

		int sel = m_combo_font.FindString( 0, sName.c_str() );
		if ( sel != CB_ERR )
		{
			m_combo_font.SetCurSel( sel );
		}

		m_justify.AddString( "left" );
		m_justify.AddString( "right" );
		m_justify.AddString( "center" );

		int sel2 = m_justify.FindString( 0, sJustify.c_str() );
		if ( sel2 != CB_ERR )
		{
			m_justify.SetCurSel( sel2 );
		}		
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_Text::OnButtonColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];

	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= this->GetSafeHwnd();
	cc.hInstance		= 0;				
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) {
		return;
	}

	float blue	= (float)((cc.rgbResult & 0xFFFF0000) >> 16 ) / 255.0f;
	float green	= (float)((cc.rgbResult & 0xFF00FF00) >> 8 ) / 255.0f;
	float red	= (float)( cc.rgbResult & 0xFF0000FF ) / 255.0f; 
	vector_3 color( red, green, blue );				

	m_dwColor = MAKEDWORDCOLOR(color);
}

void CDialog_Text::OnButtonTest() 
{
	CString rTemp;
	gpstring sTemp = rTemp.GetBuffer( 0 );	
	m_edit_text.GetWindowText( rTemp );
		
	for ( int i = 32; i < 256; ++i )
	{		
		sTemp.appendf( "%c", (char)i );		
	}

	m_edit_text.SetWindowText( sTemp.c_str() );
}
