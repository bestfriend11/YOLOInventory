// Dialog_Conversation_Custom_Buttons.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Custom_Buttons.h"
#include "Dialog_Conversation_Editor.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Custom_Buttons dialog


Dialog_Conversation_Custom_Buttons::Dialog_Conversation_Custom_Buttons(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Custom_Buttons::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Custom_Buttons)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Custom_Buttons::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Custom_Buttons)
	DDX_Control(pDX, IDC_EDIT_VALUE_2, m_Value2);
	DDX_Control(pDX, IDC_EDIT_VALUE_1, m_Value1);
	DDX_Control(pDX, IDC_EDIT_TEXT_2, m_Text2);
	DDX_Control(pDX, IDC_EDIT_TEXT_1, m_Text1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Custom_Buttons, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Custom_Buttons)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Custom_Buttons message handlers

BOOL Dialog_Conversation_Custom_Buttons::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		gpstring sText;
		hText->Get( "button_1_text", sText );
		m_Text1.SetWindowText( sText.c_str() );
		sText.clear();
		hText->Get( "button_1_value", sText );
		m_Value1.SetWindowText( sText.c_str() );
		sText.clear();
		hText->Get( "button_2_text", sText );
		m_Text2.SetWindowText( sText.c_str() );
		sText.clear();
		hText->Get( "button_2_value", sText );
		m_Value2.SetWindowText( sText.c_str() );		
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Custom_Buttons::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Custom_Buttons::OnOK() 
{
	FuelHandle hText = gConversationEditor.GetSelectedTextHandle();
	if ( hText.IsValid() )
	{
		CString rText;
		m_Text1.GetWindowText( rText );
		if ( rText.IsEmpty() )
		{
			hText->Remove( "button_1_text" );
		}
		else
		{			
			hText->Set( "button_1_text", rText.GetBuffer( 0 ) );
		}

		m_Text2.GetWindowText( rText );
		if ( rText.IsEmpty() )
		{
			hText->Remove( "button_2_text" );
		}
		else
		{			
			hText->Set( "button_2_text", rText.GetBuffer( 0 ) );
		}

		m_Value1.GetWindowText( rText );
		if ( rText.IsEmpty() )
		{
			hText->Remove( "button_1_value" );
		}
		else
		{
			if( !FileSys::IsValidFileName( rText.GetBuffer( 0 ), false, false ) )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Invalid button 1 value.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;			
			}

			hText->Set( "button_1_value", rText.GetBuffer( 0 ) );
		}

		m_Value2.GetWindowText( rText );
		if ( rText.IsEmpty() )
		{
			hText->Remove( "button_2_value" );
		}
		else
		{
			if( !FileSys::IsValidFileName( rText.GetBuffer( 0 ), false, false ) )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Invalid button 2 value.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;			
			}

			hText->Set( "button_2_value", rText.GetBuffer( 0 ) );
		}
	}
	
	CDialog::OnOK();
}
