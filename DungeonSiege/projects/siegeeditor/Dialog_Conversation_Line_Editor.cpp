// Dialog_Conversation_Line_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Line_Editor.h"
#include "Dialog_Conversation_Editor.h"
#include "FileSysUtils.h"
#include "Dialog_Conversation_Activate_Quests.h"
#include "Dialog_Conversation_Complete_Quests.h"
#include "Dialog_Conversation_Custom_Buttons.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Line_Editor dialog


Dialog_Conversation_Line_Editor::Dialog_Conversation_Line_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Line_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Line_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Line_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Line_Editor)
	DDX_Control(pDX, IDC_EDIT_TEXT_SCROLL_RATE, m_ScrollRate);
	DDX_Control(pDX, IDC_EDIT_ORDER, m_Order);
	DDX_Control(pDX, IDC_EDIT_LINE, m_Line);
	DDX_Control(pDX, IDC_COMBO_BUTTON_CHOICE, m_ButtonChoice);
	DDX_Control(pDX, IDC_COMBO_AUDIO_SAMPLE, m_AudioSample);
	DDX_Control(pDX, IDC_CHECK_QUEST_DIALOGUE, m_QuestDialogue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Line_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Line_Editor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_ACTIVATE_QUESTS, OnButtonActivateQuests)
	ON_BN_CLICKED(IDC_BUTTON_COMPLETE_QUESTS, OnButtonCompleteQuests)
	ON_BN_CLICKED(IDC_BUTTON_CUSTOM_BUTTONS, OnButtonCustomButtons)
	ON_EN_CHANGE(IDC_EDIT_TEXT_SCROLL_RATE, OnChangeEditTextScrollRate)
	ON_EN_CHANGE(IDC_EDIT_ORDER, OnChangeEditOrder)
	ON_BN_CLICKED(IDC_CHECK_QUEST_DIALOGUE, OnCheckQuestDialogue)
	ON_CBN_SELCHANGE(IDC_COMBO_AUDIO_SAMPLE, OnSelchangeComboAudioSample)
	ON_CBN_SELCHANGE(IDC_COMBO_BUTTON_CHOICE, OnSelchangeComboButtonChoice)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Line_Editor message handlers

BOOL Dialog_Conversation_Line_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_AudioSample.AddString( "<none>" );	

	FileSys::RecursiveFinder finder( "sound/voices/*.mp3", 0 );
	gpstring fileName;
	while ( finder.GetNext( fileName ) )
	{
		m_AudioSample.AddString( FileSys::GetFileNameOnly( fileName ) );
	}

	m_AudioSample.SetCurSel( 0 );

	m_ButtonChoice.AddString( "<none>" );
	m_ButtonChoice.AddString( "shop" );
	m_ButtonChoice.AddString( "buy_packmule" );
	m_ButtonChoice.AddString( "potential_member" );
	m_ButtonChoice.AddString( "more" );
	m_ButtonChoice.SetCurSel( 0 );

	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		float	scrollRate = 10.0f;
		int		order = 0;
		bool	bQuestDialogue = false;
		gpstring sText;
		gpstring sSample;
		gpstring sChoice;
		gpstring sTemp;

		hText->Get( "scroll_rate", scrollRate );
		if ( hText->Get( "order", order ) )
		{
			sTemp.assignf( "%d", order );
			m_Order.SetWindowText( sTemp.c_str() );	
		}

		hText->Get( "screen_text", sText );
		hText->Get( "sample", sSample );
		hText->Get( "quest_dialogue", bQuestDialogue );
		hText->Get( "choice", sChoice );
		
		sTemp.assignf( "%f", scrollRate );
		m_ScrollRate.SetWindowText( sTemp.c_str() );
		
		int find = m_AudioSample.FindString( 0, sSample.c_str() );
		if ( find != CB_ERR )
		{
			m_AudioSample.SetCurSel( find );
		}		
		
		m_QuestDialogue.SetCheck( bQuestDialogue );

		find = m_ButtonChoice.FindString( 0, sChoice.c_str() );
		if ( find != CB_ERR )
		{
			m_ButtonChoice.SetCurSel( find );
		}

		m_Line.SetWindowText( sText.c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Line_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Line_Editor::OnButtonActivateQuests() 
{
	Dialog_Conversation_Activate_Quests dlg;
	dlg.DoModal();
	
}

void Dialog_Conversation_Line_Editor::OnButtonCompleteQuests() 
{
	Dialog_Conversation_Complete_Quests dlg;
	dlg.DoModal();	
}

void Dialog_Conversation_Line_Editor::OnButtonCustomButtons() 
{
	Dialog_Conversation_Custom_Buttons dlg;
	dlg.DoModal();
}

void Dialog_Conversation_Line_Editor::OnChangeEditTextScrollRate() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		float scrollRate = 0.0f;
		CString rScroll;
		m_ScrollRate.GetWindowText( rScroll );
		stringtool::Get( rScroll.GetBuffer( 0 ), scrollRate );
		hText->Set( "scroll_rate", scrollRate );
	}	
}

void Dialog_Conversation_Line_Editor::OnChangeEditOrder() 
{	
}

void Dialog_Conversation_Line_Editor::OnCheckQuestDialogue() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		hText->Set( "quest_dialogue", m_QuestDialogue.GetCheck() ? true : false );
	}	
}

void Dialog_Conversation_Line_Editor::OnSelchangeComboAudioSample() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		CString rSample;
		m_AudioSample.GetWindowText( rSample );
		if ( rSample.CompareNoCase( "<none>" ) != 0 )
		{
			hText->Set( "sample", rSample.GetBuffer( 0 ) );
		}
	}	
}

void Dialog_Conversation_Line_Editor::OnSelchangeComboButtonChoice() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		CString rChoice;
		m_ButtonChoice.GetWindowText( rChoice );
		if ( rChoice.CompareNoCase( "<none>" ) != 0 )
		{
			hText->Set( "choice", rChoice.GetBuffer( 0 ) );
		}
	}		
}

void Dialog_Conversation_Line_Editor::OnOK() 
{
	int order = 0;
	CString rOrder;
	m_Order.GetWindowText( rOrder );
	if ( !rOrder.IsEmpty() )
	{
		stringtool::Get( rOrder.GetBuffer( 0 ), order );
	}
	
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	
	if ( hText.IsValid() )
	{
		FuelHandle hParent = hText->GetParent();
		FuelHandleList hlText = hParent->ListChildBlocks( 1, "", "text*" );
		FuelHandleList::iterator iText; 
		for ( iText = hlText.begin(); iText != hlText.end(); ++iText )
		{
			int textOrder = 0;
			if( (*iText)->Get( "order", textOrder ) && textOrder != order && (*iText) != hText )
			{
				MessageBoxEx( gEditor.GetHWnd(), "Conversation text with this order already exists, please select a different order, or do not use an order.", "Conversation Line Editor", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
		}	

		if ( rOrder.IsEmpty() )
		{
			hText->Remove( "order" );
		}
		else
		{
			hText->Set( "order", order );
		}

		CString rText;
		m_Line.GetWindowText( rText );
		hText->Set( "screen_text", rText.GetBuffer( 0 ), FVP_QUOTED_STRING );
	}	
	
	CDialog::OnOK();
}
