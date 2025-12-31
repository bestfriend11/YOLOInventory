// Dialog_Common_Settings.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Common_Settings.h"
#include "GUIMasterShell.h"
#include "ui_shell.h"
#include "ui_dialogbox.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Common_Settings dialog


CDialog_Common_Settings::CDialog_Common_Settings(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Common_Settings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Common_Settings)
	//}}AFX_DATA_INIT
}


void CDialog_Common_Settings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Common_Settings)
	DDX_Control(pDX, IDC_EDIT_TEMPLATE, m_edit_template);	
	DDX_Control(pDX, IDC_CHECK_COMMON, m_check_common);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Common_Settings, CDialog)
	//{{AFX_MSG_MAP(CDialog_Common_Settings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Common_Settings message handlers

void CDialog_Common_Settings::OnOK() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{
		if ( m_check_common.GetCheck() )
		{
			CString rTemp;
			m_edit_template.GetWindowText( rTemp );
			pWindow->CreateCommonCtrl( rTemp.GetBuffer( 0 ) );
		}
	}
	
	CDialog::OnOK();
}

BOOL CDialog_Common_Settings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{
		if ( pWindow->GetIsCommonControl() )
		{
			m_edit_template.SetWindowText( pWindow->GetCommonTemplate().c_str() );

			m_edit_template.EnableWindow( FALSE );
			if ( pWindow->GetIsCommonControl() )
			{
				m_check_common.SetCheck( 1 );
			}
			m_check_common.EnableWindow( FALSE );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
