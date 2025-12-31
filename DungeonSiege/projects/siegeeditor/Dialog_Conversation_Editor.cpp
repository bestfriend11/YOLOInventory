// Dialog_Conversation_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Editor.h"
#include "GoConversation.h"
#include "Dialog_Conversation_Manager.h"
#include "WorldMap.h"
#include "Dialog_Conversation_Line_Editor.h"
#include "EditorRegion.h"
#include "Dialog_Conversation_Add_Goodbye.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Editor dialog


Dialog_Conversation_Editor::Dialog_Conversation_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Editor)
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Editor)
	DDX_Control(pDX, IDC_LIST_LINES, m_Lines);
	DDX_Control(pDX, IDC_LIST_GOODBYES, m_Goodbyes);
	DDX_Control(pDX, IDC_STATIC_CONVERSATION, m_Conversation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Editor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_NEW_LINE, OnButtonNewLine)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_LINE, OnButtonEditLine)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_LINE, OnButtonRemoveLine)
	ON_BN_CLICKED(IDC_BUTTON_NEW_GOODBYE, OnButtonNewGoodbye)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_GOODBYE, OnButtonRemoveGoodbye)
	ON_LBN_DBLCLK(IDC_LIST_LINES, OnDblclkListLines)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Editor message handlers

BOOL Dialog_Conversation_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_Lines.InsertColumn( 0, "Text", LVCFMT_LEFT, 270 );
	m_Lines.InsertColumn( 1, "Order", LVCFMT_LEFT, 50 );

	m_Goodbyes.InsertColumn( 0, "Sample", LVCFMT_LEFT, 270 );
	m_Goodbyes.InsertColumn( 1, "Order", LVCFMT_LEFT, 50 );

	gpstring sTemp;
	sTemp.assignf( "Conversation: %s", gEditorRegion.GetSelectedConversation().c_str() );
	m_Conversation.SetWindowText( sTemp.c_str() );
	
	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	gpstring sAddress;
	sAddress.assignf( "%s:conversations:%s", gEditorRegion.GetRegionHandle()->GetAddress().c_str(), gEditorRegion.GetSelectedConversation().c_str() );
	FuelHandle hFound( sAddress );	
	if ( hFound.IsValid() )
	{
		FuelHandleList hlText;
		hlText = hFound->ListChildBlocks( 1, "", "text*" );
		m_hlLines = hlText;

		FuelHandleList hlGoodbyes;
		hlGoodbyes = hFound->ListChildBlocks( 1, "", "goodbye*" );
		m_hlGoodbyes = hlGoodbyes;
	}

	Refresh();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Editor::OnButtonNewLine() 
{
	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	gpstring sAddress;
	sAddress.assignf( "%s:conversations:%s", gEditorRegion.GetRegionHandle()->GetAddress().c_str(), gEditorRegion.GetSelectedConversation().c_str() );
	FuelHandle hFound( sAddress );
	if ( !hFound.IsValid() )
	{
		FuelHandle hConversations = gEditorRegion.GetRegionHandle()->GetChildBlock( "conversations" );
		if ( !hConversations )
		{
			hConversations = gEditorRegion.GetRegionHandle()->CreateChildDirBlock( "conversations" );
		}
		if ( hConversations )
		{
			hFound = hConversations->CreateChildBlock( gEditorRegion.GetSelectedConversation().c_str(), "conversations.gas" );
		}
	}
	if ( hFound.IsValid() )
	{
		FuelHandle hText = hFound->CreateChildBlock( "text*" );
		if ( hText )
		{
			m_hlLines.push_back( hText );
		}
	}
	
	Refresh();
}

void Dialog_Conversation_Editor::OnButtonEditLine() 
{
	int sel = m_Lines.GetSelectionMark();
	int findIndex = m_Lines.GetItemData( sel );

	int index = 0;
	FuelHandleList::iterator i;
	for ( i = m_hlLines.begin(); i != m_hlLines.end(); ++i, ++index )
	{
		if ( index == findIndex )
		{
			m_hSelected = *i;
			Dialog_Conversation_Line_Editor dlg;
			dlg.DoModal();
		}
	}	

	Refresh();
}

void Dialog_Conversation_Editor::OnButtonRemoveLine() 
{
	int sel = m_Lines.GetSelectionMark();
	int findIndex = m_Lines.GetItemData( sel );

	int index = 0;
	FuelHandleList::iterator i;
	for ( i = m_hlLines.begin(); i != m_hlLines.end(); ++i, ++index )
	{
		if ( index == findIndex )
		{
			FuelHandle hParent = (*i)->GetParent();
			hParent->DestroyChildBlock( *i );
			m_hlLines.erase( i );
			Refresh();
			return;
		}
	}

	Refresh();
}

void Dialog_Conversation_Editor::OnButtonNewGoodbye() 
{
	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	gpstring sAddress;
	sAddress.assignf( "%s:conversations:%s", gEditorRegion.GetRegionHandle()->GetAddress().c_str(), gEditorRegion.GetSelectedConversation().c_str() );
	FuelHandle hFound( sAddress );
	if ( !hFound.IsValid() )
	{
		FuelHandle hConversations = gEditorRegion.GetRegionHandle()->GetChildBlock( "conversations" );
		if ( !hConversations )
		{
			hConversations = gEditorRegion.GetRegionHandle()->CreateChildDirBlock( "conversations" );
		}
		if ( hConversations )
		{
			hFound = hConversations->CreateChildBlock( gEditorRegion.GetSelectedConversation().c_str(), "conversations.gas" );
		}
	}
	if ( hFound.IsValid() )
	{
		FuelHandle hText = hFound->CreateChildBlock( "goodbye*" );
		if ( hText )
		{
			m_hlGoodbyes.push_back( hText );
			m_hGoodbye = hText;
		}
	}
	
	Dialog_Conversation_Add_Goodbye dlg;
	dlg.DoModal();

	gpstring sText;
	if( m_hGoodbye.IsValid() )
	{
		m_hGoodbye->Get( "sample", sText );
		if ( sText.empty() )
		{
			FuelHandle hParent = m_hGoodbye->GetParent();
			hParent->DestroyChildBlock( m_hGoodbye );
			m_hlGoodbyes.pop_back();
		}
	}

	Refresh();
}


void Dialog_Conversation_Editor::OnButtonRemoveGoodbye() 
{
	int sel = m_Goodbyes.GetSelectionMark();
	int findIndex = m_Goodbyes.GetItemData( sel );

	int index = 0;
	FuelHandleList::iterator i;
	for ( i = m_hlGoodbyes.begin(); i != m_hlGoodbyes.end(); ++i, ++index )
	{
		if ( index == findIndex )
		{
			FuelHandle hParent = (*i)->GetParent();
			hParent->DestroyChildBlock( *i );
			m_hlGoodbyes.erase( i );
			Refresh();
			return;
		}
	}

	Refresh();	
}

void Dialog_Conversation_Editor::Refresh()
{	
	m_Lines.DeleteAllItems();	
	m_Goodbyes.DeleteAllItems();
	
	int index = 0;
	FuelHandleList::iterator i;	
	for ( i = m_hlLines.begin(); i != m_hlLines.end(); ++i, index++ )
	{
		gpstring sText;
		(*i)->Get( "screen_text", sText );
		if ( sText.empty() )
		{
			sText = "<No text added yet>";
		}
		int item = m_Lines.InsertItem( index, sText.c_str() );
		m_Lines.SetItemData( item, index );

		int order = -1;
		if( (*i)->Get( "order", order ) )
		{
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_Lines.SetItemText( item, 1, sOrder.c_str() );
		}			
	}	
	
	index = 0;
	for ( i = m_hlGoodbyes.begin(); i!= m_hlGoodbyes.end(); ++i, index++ )
	{
		gpstring sText;
		(*i)->Get( "sample", sText );
		if ( sText.empty() )
		{
			sText = "<No sample>";
		}
		int item = m_Goodbyes.InsertItem( index, sText.c_str() );
		m_Goodbyes.SetItemData( item, index );

		int order = -1;
		if ( (*i)->Get( "order", order ) )		
		{
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_Goodbyes.SetItemText( item, 1, sOrder.c_str() );
		}
	}
}

void Dialog_Conversation_Editor::OnDblclkListLines() 
{
	OnButtonEditLine();	
}
