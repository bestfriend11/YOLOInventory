// Dialog_Conversation_List_Global.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_List_Global.h"
#include "Dialog_Conversation_Editor.h"
#include "GizmoManager.h"
#include "GoConversation.h"
#include "Dialog_Conversation_New_Global.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_List_Global dialog


Dialog_Conversation_List_Global::Dialog_Conversation_List_Global(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_List_Global::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_List_Global)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_List_Global::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_List_Global)
	DDX_Control(pDX, IDC_LIST_CONVERSATIONS, m_Conversations);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_List_Global, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_List_Global)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_List_Global message handlers

BOOL Dialog_Conversation_List_Global::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_List_Global::OnButtonHelp() 
{	
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);		
}

void Dialog_Conversation_List_Global::OnButtonNew() 
{
	SetSelectedConversation( "" );
	Dialog_Conversation_New_Global dlg;
	dlg.DoModal();

	if ( !GetSelectedConversation().empty() )
	{
		m_Conversations.AddString( GetSelectedConversation() );
	}
}

void Dialog_Conversation_List_Global::OnButtonEdit() 
{	
	CString rConversation;
	int sel = m_Conversations.GetCurSel();
	m_Conversations.GetText( sel, rConversation );
	gEditorRegion.SetSelectedConversation( rConversation.GetBuffer( 0 ) );

	Dialog_Conversation_Editor dlg;
	dlg.DoModal();	
}

void Dialog_Conversation_List_Global::OnButtonRemove() 
{
	CString rName;
	int sel = m_Conversations.GetCurSel();
	m_Conversations.GetText( sel, rName );
	gpstring sAddress;
	sAddress.assignf( "%s:conversations", gEditorRegion.GetRegionHandle()->GetAddress().c_str() );
	FuelHandle hFound( sAddress );	
	if ( hFound.IsValid() )
	{
		FuelHandleList hlText = hFound->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlText.begin(); i != hlText.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( rName.GetBuffer( 0 ) ) )
			{
				FuelHandle hParent = (*i)->GetParent();
				hParent->DestroyChildBlock( *i );
				Refresh();
				return;
			}
		}		
	}	

	Refresh();	
}


void Dialog_Conversation_List_Global::Refresh()
{
	m_Conversations.ResetContent();
	gpstring sAddress;
	sAddress.assignf( "%s:conversations", gEditorRegion.GetRegionHandle()->GetAddress().c_str() );
	FuelHandle hFound( sAddress );	
	if ( hFound.IsValid() )
	{
		FuelHandleList hlText = hFound->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlText.begin(); i != hlText.end(); ++i )
		{
			m_Conversations.AddString( (*i)->GetName() );
		}		
	}
}
