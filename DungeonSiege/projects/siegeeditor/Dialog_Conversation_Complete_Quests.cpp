// Dialog_Conversation_Complete_Quests.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Complete_Quests.h"
#include "Dialog_Conversation_Editor.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Complete_Quests dialog


Dialog_Conversation_Complete_Quests::Dialog_Conversation_Complete_Quests(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Complete_Quests::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Complete_Quests)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Complete_Quests::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Complete_Quests)
	DDX_Control(pDX, IDC_LIST_QUESTS, m_ListQuests);
	DDX_Control(pDX, IDC_COMBO_QUESTS, m_AvailableQuests);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Complete_Quests, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Complete_Quests)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Complete_Quests message handlers

BOOL Dialog_Conversation_Complete_Quests::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hInfo = gEditorRegion.GetMapHandle()->GetChildBlock( "quests" );
	if ( hInfo )
	{
		FuelHandle hQuests = hInfo->GetChildBlock( "quests" );
		if ( hQuests.IsValid() )
		{
			FuelHandleList::iterator i;
			FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
			for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
			{
				m_AvailableQuests.AddString( (*i)->GetName() );
			}
		}
	}
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Complete_Quests::OnButtonAdd() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{	
		CString rQuest;
		m_AvailableQuests.GetWindowText( rQuest );

		if ( rQuest.IsEmpty() )
		{
			MessageBoxEx( gEditor.GetHWnd(), "Please select a quest.", "Add Quest Complete", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}

		hText->Set( "complete_quest*", rQuest.GetBuffer( 0 ) );

		Refresh();
	}
}

void Dialog_Conversation_Complete_Quests::OnButtonRemove() 
{
	m_ListQuests.DeleteString( m_ListQuests.GetCurSel() );
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		hText->Remove( "complete_quest*" );
		
		for ( int i = 0; i != m_ListQuests.GetCount(); ++i )
		{
			CString rName; 
			m_ListQuests.GetText( i, rName );			
			hText->Set( "complete_quest*", rName.GetBuffer( 0 ) );
		}
	}

	Refresh();	
}

void Dialog_Conversation_Complete_Quests::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Complete_Quests::Refresh()
{	
	m_ListQuests.ResetContent();
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		if ( hText->FindFirst( "complete_quest*" ) )
		{
			gpstring sComplete;
			while ( hText->GetNext( sComplete ) )
			{
				m_ListQuests.AddString( sComplete.c_str() );
			}
		}
	}	
}