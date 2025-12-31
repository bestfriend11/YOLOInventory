// Dialog_Create_Interface.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Create_Interface.h"
#include "gpcore.h"
#include "gpstring.h"
#include "GUIMasterShell.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Create_Interface dialog


CDialog_Create_Interface::CDialog_Create_Interface(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Create_Interface::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Create_Interface)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Create_Interface::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Create_Interface)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Create_Interface, CDialog)
	//{{AFX_MSG_MAP(CDialog_Create_Interface)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Create_Interface message handlers

void CDialog_Create_Interface::OnOK() 
{
	StringVec interfaces;
	StringVec::iterator i;
	gGUIMaster.GetAvailableInterfaces( interfaces );

	CString rName;
	m_name.GetWindowText( rName );
	
	if ( rName.IsEmpty() )
	{
		return;
	}

	gpstring sName = rName.GetBuffer( 0 );
	for ( i = interfaces.begin(); i != interfaces.end(); ++i )
	{
		if ( (*i).same_no_case( sName ) )  
		{
			return;
		}
	}

	gGUIMaster.CreateInterface( sName );
	
	CDialog::OnOK();
}
