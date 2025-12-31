// Dialog_Conversation_Activate_Quests.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Activate_Quests.h"
#include "Dialog_Conversation_Editor.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Activate_Quests dialog


Dialog_Conversation_Activate_Quests::Dialog_Conversation_Activate_Quests(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Activate_Quests::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Activate_Quests)
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Activate_Quests::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Activate_Quests)
	DDX_Control(pDX, IDC_COMBO_ORDER, m_Order);
	DDX_Control(pDX, IDC_LIST_QUESTS, m_ListQuests);
	DDX_Control(pDX, IDC_COMBO_QUESTS, m_AvailableQuests);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Activate_Quests, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Activate_Quests)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_QUESTS, OnSelchangeComboQuests)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Activate_Quests message handlers

BOOL Dialog_Conversation_Activate_Quests::OnInitDialog() 
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

	m_ListQuests.InsertColumn( 0, "Quest", LVCFMT_LEFT, 150 );
	m_ListQuests.InsertColumn( 1, "Step", LVCFMT_LEFT, 50 );

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Activate_Quests::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Activate_Quests::OnButtonRemove() 
{
	m_ListQuests.DeleteItem( m_ListQuests.GetSelectionMark() );
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		hText->Remove( "activate_quest*" );
		
		for ( int i = 0; i != m_ListQuests.GetItemCount(); ++i )
		{
			CString rName = m_ListQuests.GetItemText( i, 0 );
			CString rOrder = m_ListQuests.GetItemText( i, 1 );			

			gpstring sActivate;
			sActivate.assignf( "%s,%s", rName.GetBuffer( 0 ), rOrder.GetBuffer( 0 ) );
			
			hText->Set( "activate_quest*", sActivate );
		}
	}

	Refresh();
}

void Dialog_Conversation_Activate_Quests::OnButtonAdd() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{	
		CString rQuest;
		m_AvailableQuests.GetWindowText( rQuest );

		if ( rQuest.IsEmpty() )
		{
			MessageBoxEx( gEditor.GetHWnd(), "Please select a quest.", "Add Quest Activate", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}

		CString rOrder;
		m_Order.GetWindowText( rOrder );

		if ( rOrder.IsEmpty() )
		{
			MessageBoxEx( gEditor.GetHWnd(), "Please select an order.", "Add Quest Activate", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}		

		gpstring sActivate;
		sActivate.assignf( "%s,%s", rQuest.GetBuffer( 0 ), rOrder.GetBuffer( 0 ) );
		
		hText->Set( "activate_quest*", sActivate );				
	}	

	Refresh();
}

void Dialog_Conversation_Activate_Quests::OnSelchangeComboQuests() 
{
	m_Order.ResetContent();
	CString rQuest;
	m_AvailableQuests.GetWindowText( rQuest );
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
				if ( gpstring( (*i)->GetName() ).same_no_case( rQuest.GetBuffer( 0 ) ) )
				{
					FuelHandleList hlOrders = (*i)->ListChildBlocks( 1 );
					FuelHandleList::iterator iOrder;
					for ( iOrder = hlOrders.begin(); iOrder != hlOrders.end(); ++iOrder )
					{
						int order = 0;
						if ( (*iOrder)->Get( "order", order ) )
						{
							gpstring sOrder;
							sOrder.assignf( "%d", order );
							m_Order.AddString( sOrder.c_str() );
						}
					}
				}
			}
		}
	}	

	if ( m_Order.GetCount() == 0 )
	{
		m_Order.AddString( "0" );
	}

	m_Order.SetCurSel( 0 );
}


void Dialog_Conversation_Activate_Quests::Refresh()
{
	m_ListQuests.DeleteAllItems();
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		if ( hText->FindFirst( "activate_quest*" ) )
		{
			gpstring sActivate;
			while ( hText->GetNext( sActivate ) )
			{
				gpstring sName;
				gpstring sOrder = "0";
				stringtool::GetDelimitedValue( sActivate, ',', 0, sName );

				if ( stringtool::GetNumDelimitedStrings( sActivate.c_str(), ',' ) == 2 )
				{
					stringtool::GetDelimitedValue( sActivate, ',', 1, sOrder );	
				}

				int item = m_ListQuests.InsertItem( 0, sName.c_str() );
				m_ListQuests.SetItemText( item, 1, sOrder.c_str() );				
			}
		}
	}
}