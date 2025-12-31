// Dialog_Starting_Group_Add_Level.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Starting_Group_Add_Level.h"
#include "EditorGizmos.h"
#include "stringtool.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Add_Level dialog


Dialog_Starting_Group_Add_Level::Dialog_Starting_Group_Add_Level(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Group_Add_Level::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Group_Add_Level)
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Group_Add_Level::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Group_Add_Level)
	DDX_Control(pDX, IDC_EDIT_LEVEL, m_level);
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Group_Add_Level, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Group_Add_Level)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Add_Level message handlers

BOOL Dialog_Starting_Group_Add_Level::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Starting_Group_Add_Level::OnOK() 
{
	CString rName;
	m_name.GetWindowText( rName );
	if ( rName.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please type in a name for the world level type (Example: \"normal\").", "Add World Level", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	gpstring sTemp = rName.GetBuffer( 0 );
	if( !FileSys::IsValidFileName( sTemp, false, false ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Invalid world level name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;			
	}

	CString rLevel;
	m_level.GetWindowText( rLevel );
	if ( rLevel.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please type in the required level.", "Add World Level", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	int level = 0;
	stringtool::Get( rLevel.GetBuffer( 0 ), level );
	gEditorGizmos.AddStartingGroupWorldLevel( gEditorGizmos.GetSelectedStartGroup(), rName.GetBuffer( 0 ), level );
	
	CDialog::OnOK();
}

void Dialog_Starting_Group_Add_Level::OnCancel() 
{	
	CDialog::OnCancel();
}

void Dialog_Starting_Group_Add_Level::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
