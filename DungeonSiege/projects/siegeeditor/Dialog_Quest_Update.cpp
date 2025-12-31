// Dialog_Quest_Update.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Update.h"
#include "Dialog_Quest_Editor.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Update dialog


Dialog_Quest_Update::Dialog_Quest_Update(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Update::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Update)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT	
}


void Dialog_Quest_Update::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Update)
	DDX_Control(pDX, IDC_EDIT_SPEAKER, m_Speaker);
	DDX_Control(pDX, IDC_EDIT_ORDER, m_Order);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_Descriptions);
	DDX_Control(pDX, IDC_COMBO_CONVERSATIONS, m_Conversations);
	DDX_Control(pDX, IDC_CHECK_REQUIRED_UPDATE, m_RequiredUpdate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Update, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Update)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Update message handlers

BOOL Dialog_Quest_Update::OnInitDialog() 
{
	CDialog::OnInitDialog();

	StringVec regions;
	StringVec::iterator i;
	gEditorRegion.GetRegionList( gEditorRegion.GetMapName(), regions );

	m_Conversations.AddString( "<none>" );
	for ( i = regions.begin(); i != regions.end(); ++i )
	{
		gpstring sAddress = gEditorRegion.GetMapHandle()->GetAddress();
		sAddress.appendf( ":regions:%s:conversations", (*i).c_str() );
		FuelHandle hConversations( sAddress );
		int index = 0;
		if ( hConversations.IsValid() )
		{
			FuelHandleList hlConversations = hConversations->ListChildBlocks( 1 );
			FuelHandleList::iterator iConv;

			for ( iConv = hlConversations.begin(); iConv != hlConversations.end(); ++iConv )
			{
				gpstring sConv = (*iConv)->GetAddress().c_str();
				int num = stringtool::GetNumDelimitedStrings( sConv.c_str(), ':' );
				gpstring sNew;
				stringtool::GetDelimitedValue( sConv.c_str(), ':', num-1, sNew );
				sNew.assignf( "%s - %s", (*i).c_str(), sNew.c_str() );

				int sel = m_Conversations.AddString( sNew );
				m_Conversations.SetItemData( sel, index++ );
				m_ConversationAddresses.push_back( (*iConv)->GetAddress().c_str() );
			}
		}
	}
	m_Conversations.SetCurSel( 0 );

	FuelHandle hUpdate = gQuestEditor.GetSelectedHandle();
	if ( hUpdate )
	{		
		int order = 0;		
		if ( hUpdate->Get( "order", order ) )
		{
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_Order.SetWindowText( sOrder.c_str() );
			m_sOriginalOrder = sOrder;
		}

		gpstring sDescription;
		hUpdate->Get( "description", sDescription );
		m_Descriptions.SetWindowText( sDescription.c_str() );

		gpstring sSpeaker;
		hUpdate->Get( "speaker", sSpeaker );
		m_Speaker.SetWindowText( sSpeaker.c_str() );

		gpstring sConversation;
		hUpdate->Get( "address", sConversation );
		StringVec::iterator iAddress;
		int addressIndex = 0;
		for ( iAddress = m_ConversationAddresses.begin(); iAddress != m_ConversationAddresses.end(); ++iAddress, ++addressIndex )
		{
			if ( (*iAddress).same_no_case( sConversation ) )
			{
				for ( int comboIndex = 0; comboIndex != m_Conversations.GetCount(); ++comboIndex )
				{
					if ( m_Conversations.GetItemData( comboIndex ) == addressIndex )
					{
						m_Conversations.SetCurSel( comboIndex );
						break;
					}
				}
			}			
		}		

		bool bRequiredUpdate;
		hUpdate->Get( "required_update", bRequiredUpdate );
		m_RequiredUpdate.SetCheck( bRequiredUpdate ? TRUE : FALSE );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Quest_Update::OnOK() 
{
	CString rOrder, rDescription, rSpeaker, rConversation;
	m_Order.GetWindowText( rOrder );
	m_Descriptions.GetWindowText( rDescription );
	m_Speaker.GetWindowText( rSpeaker );
	m_Conversations.GetWindowText( rConversation );
	
	FuelHandle hUpdate = gQuestEditor.GetSelectedHandle();
	if ( hUpdate )
	{
		if ( !rOrder.IsEmpty() )
		{
			int order = 0;
			stringtool::Get( rOrder.GetBuffer( 0 ), order );

			if ( !m_sOriginalOrder.same_no_case( rOrder.GetBuffer( 0 ) ) )
			{
				if ( gQuestEditor.DoesOrderExist( order ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "An update with this order already exists.  Please choose a different order.", "Quest Update", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}

			hUpdate->Set( "order", order );
		}
		else
		{
			hUpdate->Remove( "order" );
		}

		hUpdate->Set( "description", rDescription.GetBuffer( 0 ) );
		hUpdate->Set( "speaker", rSpeaker.GetBuffer( 0 ) );

		if ( !rConversation.IsEmpty() && rConversation.CompareNoCase( "<none>" ) != 0 )
		{
			StringVec::iterator iAddress;
			int addressIndex = 0;
			for ( iAddress = m_ConversationAddresses.begin(); iAddress != m_ConversationAddresses.end(); ++iAddress, ++addressIndex )
			{
				int data = m_Conversations.GetItemData( m_Conversations.GetCurSel() );
				if ( addressIndex == data )
				{
					hUpdate->Set( "address", (*iAddress) );		
					break;
				}			
			}
		}
		else
		{
			hUpdate->Remove( "address" );
		}

		hUpdate->Set( "required_update", m_RequiredUpdate.GetCheck() ? true : false );
	}
	
	CDialog::OnOK();
}

void Dialog_Quest_Update::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}
