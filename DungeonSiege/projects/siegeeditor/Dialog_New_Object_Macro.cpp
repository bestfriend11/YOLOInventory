// Dialog_New_Object_Macro.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_New_Object_Macro.h"
#include "EditorObjects.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Object_Macro dialog


Dialog_New_Object_Macro::Dialog_New_Object_Macro(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_New_Object_Macro::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_New_Object_Macro)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_New_Object_Macro::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_New_Object_Macro)
	DDX_Control(pDX, IDC_EDIT_MACRO_NAME, m_macroName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_New_Object_Macro, CDialog)
	//{{AFX_MSG_MAP(Dialog_New_Object_Macro)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Object_Macro message handlers

BOOL Dialog_New_Object_Macro::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_New_Object_Macro::OnOK() 
{
	CString rName;
	m_macroName.GetWindowText( rName );
	if ( rName.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter a name for the macro.", "New Macro", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	if ( !gEditorObjects.CreateMacroFromSelection( rName.GetBuffer( 0 ) ) )
	{
		MessageBoxEx( gEditor.GetHWnd(), "A macro with this name already exists.  Please choose another name.", "New Macro", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	gEditorObjects.CreateMacroFromSelection( rName.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}
