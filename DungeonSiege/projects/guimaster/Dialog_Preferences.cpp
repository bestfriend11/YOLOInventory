// Dialog_Preferences.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Preferences.h"
#include "GUIMasterShell.h"
#include "RapiMath.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDialog_Preferences dialog


CDialog_Preferences::CDialog_Preferences(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Preferences::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Preferences)
	//}}AFX_DATA_INIT
}


void CDialog_Preferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Preferences)
	DDX_Control(pDX, IDC_CHECK_DISABLE_MOVE, m_check_move);
	DDX_Control(pDX, IDC_CHECK_DISABLE_RESIZE, m_check_resize);
	DDX_Control(pDX, IDC_CHECK_HIDE_BLANKS, m_check_hide_blanks);
	DDX_Control(pDX, IDC_CHECK_DRAW_GUIDES, m_draw_guides);
	DDX_Control(pDX, IDC_CHECK_PARENT_MOVE, m_check_parent);
	DDX_Control(pDX, IDC_CHECK_RADIO_GROUP_MOVE, m_check_radio);
	DDX_Control(pDX, IDC_CHECK_GROUP_MOVE, m_check_group);
	DDX_Control(pDX, IDC_CHECK_DOCK_GROUP_MOVE, m_check_dock);
	DDX_Control(pDX, IDC_CHECK_SNAPTOGUIDES, m_check_snap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Preferences, CDialog)
	//{{AFX_MSG_MAP(CDialog_Preferences)
	ON_BN_CLICKED(IDC_BUTTON_BG_COLOR, OnButtonBgColor)
	ON_BN_CLICKED(IDC_CHECK_SNAPTOGUIDES, OnCheckSnaptoguides)
	ON_BN_CLICKED(IDC_CHECK_PARENT_MOVE, OnCheckParentMove)
	ON_BN_CLICKED(IDC_CHECK_GROUP_MOVE, OnCheckGroupMove)
	ON_BN_CLICKED(IDC_CHECK_DOCK_GROUP_MOVE, OnCheckDockGroupMove)
	ON_BN_CLICKED(IDC_CHECK_RADIO_GROUP_MOVE, OnCheckRadioGroupMove)
	ON_BN_CLICKED(IDC_CHECK_DRAW_GUIDES, OnCheckDrawGuides)
	ON_BN_CLICKED(IDC_CHECK_HIDE_BLANKS, OnCheckHideBlanks)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_RESIZE, OnCheckDisableResize)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_MOVE, OnCheckDisableMove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Preferences message handlers

void CDialog_Preferences::OnButtonBgColor() 
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

	gGUIMaster.SetClearColor( MAKEDWORDCOLOR(color) );		
}

void CDialog_Preferences::OnCheckSnaptoguides() 
{
	gGUIMaster.SetSnapToGuides( m_check_snap.GetCheck() ? true : false );
	
}

void CDialog_Preferences::OnCheckParentMove() 
{
	gGUIMaster.SetParentMove( m_check_parent.GetCheck() ? true : false );	
}

void CDialog_Preferences::OnCheckGroupMove() 
{
	gGUIMaster.SetGroupMove( m_check_group.GetCheck() ? true : false );
	
}

void CDialog_Preferences::OnCheckDockGroupMove() 
{
	gGUIMaster.SetDockGroupMove( m_check_dock.GetCheck() ? true : false );
	
}

void CDialog_Preferences::OnCheckRadioGroupMove() 
{
	gGUIMaster.SetRadioGroupMove( m_check_radio.GetCheck() ? true : false );	
}

void CDialog_Preferences::OnCheckDrawGuides() 
{
	gGUIMaster.SetReferenceLines( m_draw_guides.GetCheck() ? true : false );
}

BOOL CDialog_Preferences::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( gGUIMaster.GetReferenceLines() )
	{
		m_draw_guides.SetCheck( TRUE );
	}

	if ( gGUIMaster.GetResizeMode() )
	{
		m_check_resize.SetCheck( TRUE );
	}

	if ( gGUIMaster.GetMoveMode() )
	{
		m_check_move.SetCheck( TRUE );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_Preferences::OnCheckHideBlanks() 
{
	gGUIMaster.SetHideBlankWindows( m_check_hide_blanks.GetCheck() ? true : false );
}

void CDialog_Preferences::OnCheckDisableResize() 
{
	gGUIMaster.SetResizeMode( m_check_resize.GetCheck() ? true : false );
	
}

void CDialog_Preferences::OnCheckDisableMove() 
{
	gGUIMaster.SetMoveMode( m_check_move.GetCheck() ? true : false );	
}
