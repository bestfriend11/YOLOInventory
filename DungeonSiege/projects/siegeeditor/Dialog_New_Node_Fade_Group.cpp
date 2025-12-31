// Dialog_New_Node_Fade_Group.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_New_Node_Fade_Group.h"
#include "Dialog_Node_Fade_Groups.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Node_Fade_Group dialog


Dialog_New_Node_Fade_Group::Dialog_New_Node_Fade_Group(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_New_Node_Fade_Group::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_New_Node_Fade_Group)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_New_Node_Fade_Group::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_New_Node_Fade_Group)
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_New_Node_Fade_Group, CDialog)
	//{{AFX_MSG_MAP(Dialog_New_Node_Fade_Group)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Node_Fade_Group message handlers

void Dialog_New_Node_Fade_Group::OnOK() 
{
	CString rName;

	m_name.GetWindowText( rName );
	if ( !rName.IsEmpty() )
	{
		FuelHandle hEditor( "config:editor" );
		if ( hEditor.IsValid() )
		{
			FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );				
			if ( hGroups.IsValid() )
			{			
				FuelHandleList hlFadeGroups = hGroups->ListChildBlocks( 1, "fade_group" );
				FuelHandleList::iterator i;
				for ( i = hlFadeGroups.begin(); i != hlFadeGroups.end(); ++i )
				{
					if ( gpstring( (*i)->GetName() ).same_no_case( rName.GetBuffer( 0 ) ) )
					{
						MessageBoxEx( this->GetSafeHwnd(), "A group of this name already exits, please enter a different name.", "New Mesh Group", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
						return;
					}
				}
			}
		}		
	}
	else
	{
		return;
	}

	gDialogNodeFadeGroups.SetNewGroupName( rName.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

void Dialog_New_Node_Fade_Group::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
