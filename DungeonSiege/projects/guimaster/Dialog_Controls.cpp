// Dialog_Controls.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Controls.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Controls dialog


CDialog_Controls::CDialog_Controls(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Controls::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Controls)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Controls::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Controls)
	DDX_Control(pDX, IDC_LIST_CONTROLS, m_controls);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Controls, CDialog)
	//{{AFX_MSG_MAP(CDialog_Controls)
	ON_LBN_SELCHANGE(IDC_LIST_CONTROLS, OnSelchangeListControls)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Controls message handlers

BOOL CDialog_Controls::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// $$$ chad: you could just iterate from UI_TYPE_BEGIN to UI_TYPE_END and add ToGasString of each -sb

	/*
	for ( UI_CONTROL_TYPE i = UI_TYPE_BEGIN ; i != UI_TYPE_END ; ++i )
	{
		m_controls.AddString( ToGasString( i ) );
	}
	*/

	m_controls.AddString( "window" );
	m_controls.AddString( "button" );
	m_controls.AddString( "checkbox" );
	m_controls.AddString( "slider" );
	m_controls.AddString( "text" );
	m_controls.AddString( "radio_button" );
	m_controls.AddString( "listbox" );
	m_controls.AddString( "button_multistage" );
	m_controls.AddString( "popupmenu" );
	m_controls.AddString( "gridbox" );
	m_controls.AddString( "item" );
	m_controls.AddString( "itemslot" );
	m_controls.AddString( "infoslot" );
	m_controls.AddString( "status_bar" );
	m_controls.AddString( "edit_box" );
	m_controls.AddString( "combo_box" );
	m_controls.AddString( "dockbar" );
	m_controls.AddString( "listener" );
	m_controls.AddString( "listreport" );
	m_controls.AddString( "chat_box" );
	m_controls.AddString( "text_box" );
	m_controls.AddString( "dialog_box" );
	m_controls.AddString( "tab" );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_Controls::OnSelchangeListControls() 
{
	int sel = m_controls.GetCurSel();
	if ( sel != LB_ERR )
	{
		CString rString;
		m_controls.GetText( sel, rString );
		gGUIMaster.SetControlType( rString.GetBuffer( 0 ) );
	}
}
