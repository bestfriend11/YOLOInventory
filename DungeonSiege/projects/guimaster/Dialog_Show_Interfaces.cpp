// Dialog_Show_Interfaces.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Show_Interfaces.h"
#include "GUIMasterShell.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Show_Interfaces dialog


CDialog_Show_Interfaces::CDialog_Show_Interfaces(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Show_Interfaces::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Show_Interfaces)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Show_Interfaces::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Show_Interfaces)
	DDX_Control(pDX, IDC_STATIC_CURRENT_INTERFACE, m_current_interface);
	DDX_Control(pDX, IDC_LIST_INTERFACES, m_active_interfaces);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Show_Interfaces, CDialog)
	//{{AFX_MSG_MAP(CDialog_Show_Interfaces)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, OnButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, OnButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_INTERFACE, OnButtonEditInterface)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, OnButtonClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Show_Interfaces message handlers

void CDialog_Show_Interfaces::OnButtonShow() 
{
	int sel = m_active_interfaces.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rName;
	m_active_interfaces.GetText( sel, rName );

	gGUIMaster.ShowInterface( rName.GetBuffer( 0 ), true );	
}

void CDialog_Show_Interfaces::OnButtonHide() 
{
	int sel = m_active_interfaces.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rName;
	m_active_interfaces.GetText( sel, rName );

	gGUIMaster.ShowInterface( rName.GetBuffer( 0 ), false );	
}

void CDialog_Show_Interfaces::OnButtonEditInterface() 
{
	int sel = m_active_interfaces.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rName;
	m_active_interfaces.GetText( sel, rName );
	gGUIMaster.SetInterface( rName.GetBuffer( 0 ) );
	m_current_interface.SetWindowText( rName );
}

void CDialog_Show_Interfaces::OnButtonClose() 
{
	int sel = m_active_interfaces.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rName;
	m_active_interfaces.GetText( sel, rName );
	gGUIMaster.CloseInterface( rName.GetBuffer( 0 ) );	
}

BOOL CDialog_Show_Interfaces::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_current_interface.SetWindowText( gGUIMaster.GetInterface().c_str() );

	StringVec interfaces;
	StringVec::iterator i;
	gGUIMaster.GetActiveInterfaces( interfaces );
	for ( i = interfaces.begin(); i != interfaces.end(); ++i )
	{
		m_active_interfaces.AddString( (*i).c_str() );
	}
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
