// Dialog_Conversation_New.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_New.h"
#include "GoConversation.h"
#include "FileSysUtils.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New dialog


Dialog_Conversation_New::Dialog_Conversation_New(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_New::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_New)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_New::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_New)
	DDX_Control(pDX, IDC_COMBO_EXISTING_CONVERSATIONS, m_Existing);
	DDX_Control(pDX, IDC_EDIT_CONVERSATION, m_Conversation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_New, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_New)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New message handlers

void Dialog_Conversation_New::OnOK() 
{
	CString rConversation;
	m_Conversation.GetWindowText( rConversation );

	CString rSelected;
	m_Existing.GetWindowText( rSelected );

	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	
	if ( rConversation.IsEmpty() && rSelected.IsEmpty() )
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

		hObject->GetConversation()->AddConversation( rConversation.GetBuffer( 0 ) );
	}
	else
	{
		hObject->GetConversation()->AddConversation( rSelected.GetBuffer( 0 ) );
	}
	
	CDialog::OnOK();
}

void Dialog_Conversation_New::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);		
}

BOOL Dialog_Conversation_New::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sAddress;
	sAddress.assignf( "%s:conversations", gEditorRegion.GetRegionHandle()->GetAddress().c_str() );
	FuelHandle hFound( sAddress );	
	if ( hFound.IsValid() )
	{
		FuelHandleList hlText = hFound->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlText.begin(); i != hlText.end(); ++i )
		{
			m_Existing.AddString( (*i)->GetName() );
		}		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
