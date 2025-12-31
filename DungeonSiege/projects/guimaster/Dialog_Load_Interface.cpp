// Dialog_Load_Interface.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Load_Interface.h"
#include "gpcore.h"
#include "gpstring.h"
#include "GUIMasterShell.h"
#include "stringtool.h"
#include "fuel.h"
#include "fueldb.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Load_Interface dialog


CDialog_Load_Interface::CDialog_Load_Interface(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Load_Interface::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Load_Interface)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Load_Interface::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Load_Interface)
	DDX_Control(pDX, IDC_EDIT_ADD, m_add);
	DDX_Control(pDX, IDC_LIST_INTERFACES, m_interfaces);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Load_Interface, CDialog)
	//{{AFX_MSG_MAP(CDialog_Load_Interface)
	ON_BN_CLICKED(IDC_BUTTON_PROCESS, OnButtonProcess)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Load_Interface message handlers

void CDialog_Load_Interface::OnOK() 
{
	int sel = m_interfaces.GetCurSel();
	if ( sel == LB_ERR ) 
	{
		return;
	}

	CString rName;
	m_interfaces.GetText( sel, rName );
	if ( rName.IsEmpty() )
	{
		return;
	}
	
	gGUIMaster.LoadInterface( rName.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

BOOL CDialog_Load_Interface::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec interfaces;
	gGUIMaster.GetAvailableInterfaces( interfaces );
	StringVec::iterator i;
	for ( i = interfaces.begin(); i != interfaces.end(); ++i )
	{
		m_interfaces.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_Load_Interface::OnButtonProcess() 
{
	int sel = m_interfaces.GetCurSel();
	if ( sel == LB_ERR ) 
	{
		return;
	}

	CString rName;
	m_interfaces.GetText( sel, rName );
	if ( rName.IsEmpty() )
	{
		return;
	}

	CString rValue;
	m_add.GetWindowText( rValue );
	int add = 0;
	stringtool::Get( rValue.GetBuffer( 0 ), add );

	FuelHandle hUI( "ui:interfaces" );
	if ( hUI.IsValid() )
	{
		FuelHandleList hlInterfaces = hUI->ListChildBlocks( 3 );
		FuelHandleList::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{
			gpstring sName = (*i)->GetName();
			if ( sName.same_no_case( rName.GetBuffer( 0 ) ) )
			{
				gpstring sAddress = (*i)->GetAddress();
				FuelHandleList hlWindows = (*i)->ListChildBlocks( 3 );
				FuelHandleList::iterator j;
				for ( j = hlWindows.begin(); j != hlWindows.end(); ++j )
				{
					gpstring sRect;
					if ( (*j)->Get( "rect", sRect ) )
					{
						int left = 0;
						int right = 0;
						int top = 0;
						int bottom = 0;
						stringtool::GetDelimitedValue( sRect, ',', 0, left );
						stringtool::GetDelimitedValue( sRect, ',', 1, top );
						stringtool::GetDelimitedValue( sRect, ',', 2, right );
						stringtool::GetDelimitedValue( sRect, ',', 3, bottom );

						top += add + 1;
						bottom += add + 1;
						sRect.assignf( "%d,%d,%d,%d", left, top, right, bottom );
						(*j)->Set( "rect", sRect );
					}
				}
				break;
			}
		}
	}
	
	FuelHandle hRoot( "root" );
	hRoot->GetDB()->SaveChanges();
}
