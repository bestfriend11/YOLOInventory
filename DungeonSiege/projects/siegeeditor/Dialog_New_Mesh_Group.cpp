// Dialog_New_Mesh_Group.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mesh_List.h"
#include "Dialog_New_Mesh_Group.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Mesh_Group dialog


Dialog_New_Mesh_Group::Dialog_New_Mesh_Group(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_New_Mesh_Group::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_New_Mesh_Group)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_New_Mesh_Group::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_New_Mesh_Group)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_New_Mesh_Group, CDialog)
	//{{AFX_MSG_MAP(Dialog_New_Mesh_Group)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Mesh_Group message handlers

BOOL Dialog_New_Mesh_Group::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gDialogMeshList.SetMeshGroupName( "" );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_New_Mesh_Group::OnOK() 
{
	CString rName;
	m_name.GetWindowText( rName );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "mesh_groups" );				
		if ( hGroups.IsValid() )
		{			
			FuelHandleList hlMeshGroups = hGroups->ListChildBlocks( 1, "mesh_group" );
			FuelHandleList::iterator i;
			for ( i = hlMeshGroups.begin(); i != hlMeshGroups.end(); ++i )
			{
				if ( gpstring( (*i)->GetName() ).same_no_case( rName.GetBuffer( 0 ) ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "A group of this name already exits, please enter a different name.", "New Mesh Group", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}
		}
	}

	gDialogMeshList.SetMeshGroupName( rName.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

void Dialog_New_Mesh_Group::OnCancel() 
{	
	CDialog::OnCancel();
}

void Dialog_New_Mesh_Group::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
