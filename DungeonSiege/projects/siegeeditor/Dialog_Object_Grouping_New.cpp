// Dialog_Object_Grouping_New.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Object_Grouping_New.h"
#include "GizmoManager.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping_New dialog


Dialog_Object_Grouping_New::Dialog_Object_Grouping_New(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Grouping_New::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Grouping_New)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Grouping_New::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Grouping_New)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Grouping_New, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Grouping_New)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping_New message handlers

BOOL Dialog_Object_Grouping_New::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_Grouping_New::OnOK() 
{
	CString rName;
	m_name.GetWindowText( rName );
	if ( rName.IsEmpty() )
	{
		MessageBoxEx( GetSafeHwnd(), "You must enter a name for the group.", "Add Group", MB_OK | MB_ICONQUESTION, LANG_ENGLISH );
		return;
	}

	if ( !gGizmoManager.NewGroup( rName.GetBuffer( 0 ) ) )
	{
		MessageBoxEx( GetSafeHwnd(), "A group with this name already exists.", "Add Group", MB_OK | MB_ICONQUESTION, LANG_ENGLISH );
		return;
	}
	
	CDialog::OnOK();
}

void Dialog_Object_Grouping_New::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
