// PropPage_Color.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "PropPage_Color.h"
#include "RapiOwner.h"
#include "ColorPickerDlg.h"
#include "EditorLights.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//void SetCustomColors( COLORREF *colors );
//void GetCustomColors( COLORREF *colors );


/////////////////////////////////////////////////////////////////////////////
// PropPage_Color dialog


PropPage_Color::PropPage_Color(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Color::IDD, IDS_STRING61208)
{
	//{{AFX_DATA_INIT(PropPage_Color)
	m_color_blue = 0.0f;
	m_color_green = 0.0f;
	m_color_red = 0.0f;
	//}}AFX_DATA_INIT
}


void PropPage_Color::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Color)
	DDX_Control(pDX, IDC_EDIT_COLOR, m_color_visual);
	DDX_Text(pDX, IDC_EDIT_COLOR_BLUE, m_color_blue);
	DDV_MinMaxFloat(pDX, m_color_blue, 0.f, 1.f);
	DDX_Text(pDX, IDC_EDIT_COLOR_GREEN, m_color_green);
	DDV_MinMaxFloat(pDX, m_color_green, 0.f, 1.f);
	DDX_Text(pDX, IDC_EDIT_COLOR_RED, m_color_red);
	DDV_MinMaxFloat(pDX, m_color_red, 0.f, 1.f);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Color, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Color)
	ON_BN_CLICKED(IDC_BUTTON_COLOR_STANDARD, OnButtonColorStandard)
	ON_BN_CLICKED(IDC_BUTTON_COLOR_ADVANCED, OnButtonColorAdvanced)
	ON_EN_CHANGE(IDC_EDIT_COLOR_RED, OnChangeEditColorRed)
	ON_EN_CHANGE(IDC_EDIT_COLOR_GREEN, OnChangeEditColorGreen)
	ON_EN_CHANGE(IDC_EDIT_COLOR_BLUE, OnChangeEditColorBlue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Color message handlers

void PropPage_Color::OnButtonColorStandard() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	{		
		vector_3 color;
		CString rTemp;
		GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
		color.x = atof( rTemp );
		GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
		color.y = atof( rTemp );
		GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
		color.z = atof( rTemp );

		cc.rgbResult = MAKEDWORDCOLOR(color);
	}

	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= GetSafeHwnd();
	cc.hInstance		= 0;
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) {
		return;
	}

	float blue	= (float)((cc.rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
	float green	= (float)((cc.rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
	float red	= (float)( cc.rgbResult & 0x000000FF ) / 255.0f; 
	vector_3 color( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dcolor );				
	
	gpstring sTemp;
	sTemp.assignf( "%f", color.x );
	GetDlgItem( IDC_EDIT_COLOR_RED )->SetWindowText( sTemp.c_str() );
	sTemp.assignf( "%f", color.y );
	GetDlgItem( IDC_EDIT_COLOR_GREEN )->SetWindowText( sTemp.c_str() );
	sTemp.assignf( "%f", color.z );
	GetDlgItem( IDC_EDIT_COLOR_BLUE )->SetWindowText( sTemp.c_str() );	 
	
	SetCustomColors( crCustColors );	
	
	m_color_visual.Invalidate( TRUE );			
}

void PropPage_Color::OnButtonColorAdvanced() 
{
	DWORD dwColor = 0;
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );
	dwColor = MAKEDWORDCOLOR(color);

	CColorPickerDlg colorDlg( dwColor );
	if ( colorDlg.DoModal() == IDOK ) {
		COLORREF rgbResult = colorDlg.GetColor();

		float blue	= (float)((rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
		float green	= (float)((rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
		float red	= (float)( rgbResult & 0x000000FF ) / 255.0f; 
		vector_3 color( red, green, blue );
		DWORD dcolor = MAKEDWORDCOLOR(color);
		gEditorLights.SetColor( dcolor );				
		
		gpstring sTemp;
		sTemp.assignf( "%f", color.x );
		GetDlgItem( IDC_EDIT_COLOR_RED )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.y );
		GetDlgItem( IDC_EDIT_COLOR_GREEN )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.z );
		GetDlgItem( IDC_EDIT_COLOR_BLUE )->SetWindowText( sTemp.c_str() );	 		
	}
	m_color_visual.Invalidate( TRUE );			
}

/*
void SetCustomColors( COLORREF *colors )
{
	char szWindows[256] = "";
	GetWindowsDirectory( szWindows, sizeof( szWindows ) );
	char szFile[256] = "siege_colors.bin";
	char szFullFile[256] = "";
	sprintf( szFullFile, "%s\\%s", szWindows, szFile );
	FILE *f = NULL;
	f = fopen( szFullFile, "wb" );
	if ( !f ) {
		return;
	}
	fwrite( colors, sizeof( colors ), 16, f );
	fclose( f );
}


void GetCustomColors( COLORREF *colors )
{
	char szWindows[256] = "";
	GetWindowsDirectory( szWindows, sizeof( szWindows ) );
	char szFile[256] = "siege_colors.bin";
	char szFullFile[256] = "";
	sprintf( szFullFile, "%s\\%s", szWindows, szFile );
	FILE *f = NULL;
	f = fopen( szFullFile, "rb" );
	if ( !f ) {
		return;
	}
	fread( colors, sizeof( colors ), 16, f );
	fclose( f );	
}
*/

LRESULT PropPage_Color::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( message == WM_CTLCOLORSTATIC ) {		
		int id = GetWindowLong( (HWND)lParam, GWL_ID );
		if ( id == IDC_EDIT_COLOR ) {
			double r = 0;
			double g = 0;
			double b = 0;
			CString rTemp;
			GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
			r = atof( rTemp );
			GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );
			g = atof( rTemp );
			GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );				
			b = atof( rTemp );
			BYTE rb, gb, bb = 0;
			rb = (unsigned char)(r * 255);
			gb = (unsigned char)(g * 255);
			bb = (unsigned char)(b * 255);
			COLORREF colorref = RGB( rb, gb, bb );				
			if ( m_hBrush ) {
				DeleteObject( m_hBrush );
			}
			m_hBrush = CreateSolidBrush( colorref );
			return (LRESULT)m_hBrush;
		}
	}	
	
	return CPropertyPage::WindowProc(message, wParam, lParam);
}

BOOL PropPage_Color::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	DWORD dwColor = MAKEDWORDCOLOR( vector_3(1.0f, 1.0f, 1.0f ) );
	if ( gEditorLights.GetColor( dwColor ) ) {

		float blue	= (float)((dwColor & 0x00FF0000) >> 16 ) / 255.0f;
		float green	= (float)((dwColor & 0x0000FF00) >> 8 ) / 255.0f;
		float red	= (float)( dwColor & 0x000000FF ) / 255.0f; 
		vector_3 color( blue, green, red );

		gpstring sTemp;
		sTemp.assignf( "%f", color.x );
		GetDlgItem( IDC_EDIT_COLOR_RED )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.y );
		GetDlgItem( IDC_EDIT_COLOR_GREEN )->SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.z );
		GetDlgItem( IDC_EDIT_COLOR_BLUE )->SetWindowText( sTemp.c_str() );	
	}
	m_color_visual.Invalidate( TRUE );			
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Color::OnChangeEditColorRed() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_color_visual.Invalidate( TRUE );
}

void PropPage_Color::OnChangeEditColorGreen() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_color_visual.Invalidate( TRUE );
}

void PropPage_Color::OnChangeEditColorBlue() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_color_visual.Invalidate( TRUE );
}
