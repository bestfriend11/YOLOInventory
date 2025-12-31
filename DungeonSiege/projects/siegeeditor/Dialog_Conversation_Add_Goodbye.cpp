// Dialog_Conversation_Add_Goodbye.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Add_Goodbye.h"
#include "Fuel.h"
#include "FileSysUtils.h"
#include "Dialog_Conversation_Editor.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Add_Goodbye dialog


Dialog_Conversation_Add_Goodbye::Dialog_Conversation_Add_Goodbye(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Add_Goodbye::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Add_Goodbye)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Add_Goodbye::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Add_Goodbye)
	DDX_Control(pDX, IDC_EDIT_ORDER, m_Order);
	DDX_Control(pDX, IDC_COMBO_SAMPLES, m_Samples);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Add_Goodbye, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Add_Goodbye)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Add_Goodbye message handlers

void Dialog_Conversation_Add_Goodbye::OnOK() 
{
	FuelHandle hText = gConversationEditor.GetSelectedGoodbyeHandle();
	if ( hText.IsValid() )
	{
		int order = 0;
		CString rOrder;
		m_Order.GetWindowText( rOrder );
		stringtool::Get( rOrder.GetBuffer( 0 ), order );

		CString rSample;
		m_Samples.GetWindowText( rSample );
		if ( rSample.IsEmpty() )
		{
			MessageBoxEx( gEditor.GetHWnd(), "Please select a sample.", "Add Goodbye Sample", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}
		
		FuelHandle hParent = hText->GetParent();
		FuelHandleList hlText = hParent->ListChildBlocks( 1, "", "goodbye*" );
		FuelHandleList::iterator iText; 
		for ( iText = hlText.begin(); iText != hlText.end(); ++iText )
		{
			int textOrder = 0;
			if( (*iText)->Get( "order", textOrder ) && textOrder != order && hText != (*iText) )
			{
				MessageBoxEx( gEditor.GetHWnd(), "A goodbye sample with this order already exists, please select a different order, or do not use an order.", "Add Goodbye Sample", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
		}

		if ( !rOrder.IsEmpty() )
		{
			hText->Set( "order", order );
		}
		hText->Set( "sample", rSample.GetBuffer( 0 ) );		
	}		
	
	CDialog::OnOK();
}

BOOL Dialog_Conversation_Add_Goodbye::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FileSys::RecursiveFinder finder( "sound/voices/*.mp3", 0 );
	gpstring fileName;
	while ( finder.GetNext( fileName ) )
	{
		m_Samples.AddString( FileSys::GetFileNameOnly( fileName ) );
	}

	m_Samples.SetCurSel( 0 );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
