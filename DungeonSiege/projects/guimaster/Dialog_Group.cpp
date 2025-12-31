// Dialog_Group.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Group.h"
#include "gpstring.h"
#include "GUIMasterShell.h"
#include "ui_shell.h"
#include "ui_radiobutton.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Group dialog


CDialog_Group::CDialog_Group(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Group::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Group)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Group::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Group)
	DDX_Control(pDX, IDC_EDIT_RADIO_GROUP, m_edit_radio);
	DDX_Control(pDX, IDC_EDIT_GROUP, m_edit_group);
	DDX_Control(pDX, IDC_EDIT_DOCKBAR_GROUP, m_edit_dockbar);
	DDX_Control(pDX, IDC_COMBO_PARENT, m_combo_parent);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Group, CDialog)
	//{{AFX_MSG_MAP(CDialog_Group)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Group message handlers

BOOL CDialog_Group::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{
		m_edit_group.SetWindowText( pWindow->GetGroup().c_str() );
		m_edit_dockbar.SetWindowText( pWindow->GetDockGroup().c_str() );

		if ( pWindow->GetType() == UI_TYPE_RADIOBUTTON )
		{
			UIRadioButton * pButton = (UIRadioButton *)pWindow;
			m_edit_radio.SetWindowText( pButton->GetRadioGroup().c_str() );
		}

		UIWindowVec windows;
		UIWindowVec::iterator i;
		gGUIMaster.GetPossibleParents( pWindow, windows );
		m_combo_parent.AddString( "" );
		for ( i = windows.begin(); i != windows.end(); ++i )
		{
			m_combo_parent.AddString( (*i)->GetName() );
		}

		gpstring sParent;
		if ( pWindow->GetParentWindow() )
		{
			sParent = pWindow->GetParentWindow()->GetName();
		}
		else 
		{
			sParent = "";
		}
		int sel = m_combo_parent.FindString( 0, sParent.c_str() );
		m_combo_parent.SetCurSel( sel );
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_Group::OnOK() 
{
	UIWindow * pWindow = gGUIMaster.GetLeadWindow();
	if ( pWindow )
	{
		CString rTemp;
		m_edit_group.GetWindowText( rTemp );
		pWindow->SetGroup( rTemp.GetBuffer( 0 ) );
		m_edit_dockbar.GetWindowText( rTemp );
		pWindow->SetDockGroup( rTemp.GetBuffer( 0 ) );
		
		if ( pWindow->GetType() == UI_TYPE_RADIOBUTTON )
		{
			UIRadioButton * pButton = (UIRadioButton *)pWindow;
			m_edit_radio.GetWindowText( rTemp );
			pButton->SetRadioGroup( rTemp.GetBuffer( 0 ) );
		}

		int sel = m_combo_parent.GetCurSel();
		if ( sel != CB_ERR ) 
		{
			m_combo_parent.GetLBText( sel, rTemp );
			UIWindow * pParent = gUIShell.FindUIWindow( rTemp.GetBuffer( 0 ) );			
			if ( pWindow->GetParentWindow() )
			{
				pWindow->GetParentWindow()->RemoveChild( pWindow );
			}

			if ( pParent )
			{
				pParent->AddChild( pWindow );
			}
			pWindow->SetParentWindow( pParent );
		}
	}	
	
	CDialog::OnOK();
}
