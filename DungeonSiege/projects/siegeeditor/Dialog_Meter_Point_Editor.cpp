// Dialog_Meter_Point_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Meter_Point_Editor.h"
#include "Preferences.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Meter_Point_Editor dialog


Dialog_Meter_Point_Editor::Dialog_Meter_Point_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Meter_Point_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Meter_Point_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Meter_Point_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Meter_Point_Editor)
	DDX_Control(pDX, IDC_STATIC_COLOR, m_Color);
	DDX_Control(pDX, IDC_CHECK_SHOW_POINT_DISTANCES, m_ShowDistances);
	DDX_Control(pDX, IDC_CHECK_DRAW_METER_POINTS, m_DrawMeterPoints);
	DDX_Control(pDX, IDC_CHECK_METER_POINTS, m_PlaceMeterPoint);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Meter_Point_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Meter_Point_Editor)
	ON_BN_CLICKED(IDC_CHECK_METER_POINTS, OnCheckMeterPoints)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK_DRAW_METER_POINTS, OnCheckDrawMeterPoints)
	ON_BN_CLICKED(IDC_CHECK_SHOW_POINT_DISTANCES, OnCheckShowPointDistances)
	ON_BN_CLICKED(IDC_BUTTON_COLOR, OnButtonColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Meter_Point_Editor message handlers

void Dialog_Meter_Point_Editor::OnCheckMeterPoints() 
{
	gPreferences.SetPlaceMeterPoints( m_PlaceMeterPoint.GetCheck() ? TRUE : FALSE );			
}

void Dialog_Meter_Point_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);		
}

void Dialog_Meter_Point_Editor::OnOK() 
{
	gPreferences.SetPlaceMeterPoints( false );
	
	CDialog::OnOK();
}

void Dialog_Meter_Point_Editor::OnClose() 
{
	gPreferences.SetPlaceMeterPoints( false );
	
	CDialog::OnClose();
}

void Dialog_Meter_Point_Editor::OnCheckDrawMeterPoints() 
{
	gPreferences.SetDrawMeterPoints( m_DrawMeterPoints.GetCheck() ? TRUE : FALSE );	
}

void Dialog_Meter_Point_Editor::OnCheckShowPointDistances() 
{
	gPreferences.SetShowMeterPointDistance( m_ShowDistances.GetCheck() ? TRUE : FALSE );
}

void Dialog_Meter_Point_Editor::OnButtonColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	// Convert dword color to colorref	
	BYTE rb, gb, bb = 0;
	rb = (unsigned char)( (gPreferences.GetGridColor() & 0x00ff0000) >> 16 );
	gb = (unsigned char)( (gPreferences.GetGridColor() & 0x0000ff00) >> 8 );
	bb = (unsigned char)( (gPreferences.GetGridColor() & 0x000000ff));
	COLORREF colorref = RGB( rb, gb, bb );	

	cc.rgbResult		= colorref;
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
	
	float blue	= (float)(GetBValue(cc.rgbResult) ) / 255.0f;
	float green	= (float)(GetGValue(cc.rgbResult) ) / 255.0f;
	float red	= (float)(GetRValue(cc.rgbResult) ) / 255.0f; 
	vector_3 vcolor( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(vcolor);
	gPreferences.SetMeterPointColor( dcolor );	
	
	m_Color.Invalidate( TRUE );
	
	SetCustomColors( crCustColors );		
	
}

LRESULT Dialog_Meter_Point_Editor::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
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

			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (gPreferences.GetMeterPointColor() & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (gPreferences.GetMeterPointColor() & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (gPreferences.GetMeterPointColor() & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );	

			m_hBrush = CreateSolidBrush( colorref );

			return (LRESULT)m_hBrush;
		}
	}
	
	return CDialog::WindowProc(message, wParam, lParam);
}

BOOL Dialog_Meter_Point_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_DrawMeterPoints.SetCheck( gPreferences.GetDrawMeterPoints() );
	m_ShowDistances.SetCheck( gPreferences.GetShowMeterPointDistance() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
