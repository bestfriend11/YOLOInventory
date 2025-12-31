// Dialog_Starting_Groups.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Starting_Groups.h"
#include "EditorGizmos.h"
#include "Dialog_Starting_Group_New.h"
#include "Dialog_Starting_Group_Properties.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Groups dialog


Dialog_Starting_Groups::Dialog_Starting_Groups(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Groups::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Groups)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Groups::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Groups)
	DDX_Control(pDX, IDC_LIST_STARTING_GROUPS, m_groups);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Groups, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Groups)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Groups message handlers

void Dialog_Starting_Groups::OnButtonNew() 
{
	Dialog_Starting_Group_New dlg;
	dlg.DoModal();	
	Refresh();
}

void Dialog_Starting_Groups::OnButtonEdit() 
{
	int sel = m_groups.GetCurSel();
	if ( sel != LB_ERR )
	{
		CString rTemp;
		m_groups.GetText( sel, rTemp );	
		gEditorGizmos.SetSelectedStartGroup( rTemp.GetBuffer( 0 ) );
		Dialog_Starting_Group_Properties dlg;
		dlg.DoModal();	
	}	
}

void Dialog_Starting_Groups::OnButtonDelete() 
{
	int sel = m_groups.GetCurSel();
	if ( sel != LB_ERR )
	{
		CString rTemp;
		m_groups.GetText( sel, rTemp );
		gEditorGizmos.DeleteStartingGroup( rTemp.GetBuffer( 0 ) );
	}	
	Refresh();
}

BOOL Dialog_Starting_Groups::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Starting_Groups::Refresh()
{
	m_groups.ResetContent();
	
	StringVec groups = gEditorGizmos.GetStartingGroups();
	StringVec::iterator i;
	for ( i = groups.begin(); i != groups.end(); ++i )	
	{
		m_groups.AddString( (*i).c_str() );
	}
}
void Dialog_Starting_Groups::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
