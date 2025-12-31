// Dialog_Selection_Filters.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Selection_Filters.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Selection_Filters dialog


Dialog_Selection_Filters::Dialog_Selection_Filters(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Selection_Filters::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Selection_Filters)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Selection_Filters::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Selection_Filters)
	DDX_Control(pDX, IDC_CHECK_SELECT_ALL, m_selectAll);
	DDX_Control(pDX, IDC_CHECK_NON_INTERACTIVE, m_checkNonInteractive);
	DDX_Control(pDX, IDC_CHECK_NON_GIZMOS, m_checkNonGizmos);
	DDX_Control(pDX, IDC_CHECK_INTERACTIVE, m_checkInteractive);
	DDX_Control(pDX, IDC_CHECK_GIZMOS, m_checkGizmos);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Selection_Filters, CDialog)
	//{{AFX_MSG_MAP(Dialog_Selection_Filters)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Selection_Filters message handlers

BOOL Dialog_Selection_Filters::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( gPreferences.GetSelectNoninteractive() )
	{
		m_checkNonInteractive.SetCheck( TRUE );
	}
	if ( gPreferences.GetSelectInteractive() )
	{
		m_checkInteractive.SetCheck( TRUE );
	}
	if ( gPreferences.GetSelectGizmos() )
	{
		m_checkGizmos.SetCheck( TRUE );
	}
	if ( gPreferences.GetSelectNonGizmos() )
	{
		m_checkNonGizmos.SetCheck( TRUE );
	}	
	if ( gPreferences.GetSelectAll() )
	{
		m_selectAll.SetCheck( TRUE );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Selection_Filters::OnOK() 
{
	gPreferences.SetSelectNoninteractive( m_checkNonInteractive.GetCheck() ? true : false );
	gPreferences.SetSelectInteractive( m_checkInteractive.GetCheck() ? true : false );
	gPreferences.SetSelectGizmos( m_checkGizmos.GetCheck() ? true : false );
	gPreferences.SetSelectNonGizmos( m_checkNonGizmos.GetCheck() ? true : false );
	gPreferences.SetSelectAll( m_selectAll.GetCheck() ? true : false );
	
	CDialog::OnOK();
}

void Dialog_Selection_Filters::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
