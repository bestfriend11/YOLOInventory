// Dialog_Conversation_New_Global.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_New_Global.h"
#include "Dialog_Conversation_List_Global.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New_Global dialog


Dialog_Conversation_New_Global::Dialog_Conversation_New_Global(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_New_Global::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_New_Global)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_New_Global::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_New_Global)
	DDX_Control(pDX, IDC_EDIT_CONVERSATION, m_Conversation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_New_Global, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_New_Global)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New_Global message handlers

void Dialog_Conversation_New_Global::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_New_Global::OnOK() 
{
	CString rConversation;
	m_Conversation.GetWindowText( rConversation );
	if ( rConversation.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter a name for the conversation.", "New Conversation", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	if ( !rConversation.IsEmpty() )
	{		
		if ( !FileSys::IsValidFileName( rConversation.GetBuffer( 0 ), false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid conversation name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;			
		}

		gConversationListGlobal.SetSelectedConversation( rConversation.GetBuffer( 0 ) );
	}
	
	CDialog::OnOK();
}
