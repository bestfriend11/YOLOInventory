// Dialog_Starting_Group_New.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Starting_Group_New.h"
#include "EditorGizmos.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_New dialog


Dialog_Starting_Group_New::Dialog_Starting_Group_New(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Group_New::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Group_New)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Group_New::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Group_New)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Group_New, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Group_New)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_New message handlers

BOOL Dialog_Starting_Group_New::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Starting_Group_New::OnOK() 
{
	CString rText;
	m_name.GetWindowText( rText );
	
	CString rDesc;
	m_description.GetWindowText( rDesc );

	gEditorGizmos.AddStartingGroup( rText.GetBuffer( 0 ), rDesc.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

void Dialog_Starting_Group_New::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
