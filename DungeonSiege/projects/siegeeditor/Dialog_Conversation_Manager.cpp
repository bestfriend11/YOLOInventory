// Dialog_Conversation_Manager.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Conversation_Manager.h"
#include "GizmoManager.h"
#include "GoConversation.h"
#include "Dialog_Conversation_Editor.h"
#include "Dialog_Conversation_New.h"
#include "GoData.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Manager dialog


Dialog_Conversation_Manager::Dialog_Conversation_Manager(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Conversation_Manager::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Conversation_Manager)
	//}}AFX_DATA_INIT
}


void Dialog_Conversation_Manager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Conversation_Manager)
	DDX_Control(pDX, IDC_STATIC_TEMPLATE_NAME, m_TemplateName);
	DDX_Control(pDX, IDC_STATIC_SCREEN_NAME, m_ScreenName);
	DDX_Control(pDX, IDC_STATIC_SCID, m_Scid);
	DDX_Control(pDX, IDC_LIST_EXISTING_CONVERSATIONS, m_ExistingConversations);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Conversation_Manager, CDialog)
	//{{AFX_MSG_MAP(Dialog_Conversation_Manager)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Manager message handlers

BOOL Dialog_Conversation_Manager::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	if ( hObject.IsValid() && hObject->HasActor() && hObject->HasConversation() )
	{
		gpstring sTemp;
		sTemp.assignf( "Template: %s", hObject->GetTemplateName() );
		m_TemplateName.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "Screen Name: %S", hObject->GetCommon()->GetScreenName().c_str() );
		m_ScreenName.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "SCID: 0x%08x", MakeInt( hObject->GetScid() ) );
		m_Scid.SetWindowText( sTemp.c_str() );		
	}

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Conversation_Manager::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Conversation_Manager::OnButtonNew() 
{
	Dialog_Conversation_New dlg;
	dlg.DoModal();
	Refresh();	
}

void Dialog_Conversation_Manager::OnButtonEdit() 
{
	CString rConversation;
	int sel = m_ExistingConversations.GetCurSel();
	m_ExistingConversations.GetText( sel, rConversation );
	gEditorRegion.SetSelectedConversation( rConversation.GetBuffer( 0 ) );

	Dialog_Conversation_Editor dlg;
	dlg.DoModal();
}

void Dialog_Conversation_Manager::OnButtonRemove() 
{
	CString rConversation;
	int sel = m_ExistingConversations.GetCurSel();
	m_ExistingConversations.GetText( sel, rConversation );

	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	FastFuelHandle fuel = hObject->GetConversation()->GetOwnedData()->GetInternalField( "conversations" );
	if ( fuel )
	{
		FastFuelFindHandle fh( fuel );
		if ( fh.FindFirstValue( "*" ) )
		{
			gpstring sValue;
			while ( fh.GetNextValue( sValue ) )
			{
				if ( sValue.same_no_case( rConversation.GetBuffer( 0 ) ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "This conversation exists on the template level, so it cannot be removed.", "Remove Conversation", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}
		}
	}

	hObject->GetConversation()->RemoveConversation( rConversation.GetBuffer( 0 ) );
	Refresh();	
}

void Dialog_Conversation_Manager::Refresh()
{
	m_ExistingConversations.ResetContent();
	GoHandle hObject( MakeGoid(gGizmoManager.GetSelectedGizmoID()) );
	if ( hObject.IsValid() && hObject->HasActor() && hObject->HasConversation() )
	{	
		GoConversation::ConvToRefCountMap conversations;
		GoConversation::ConvToRefCountMap::iterator i;
		hObject->GetConversation()->GetAvailableConversations( conversations );
		for ( i = conversations.begin(); i != conversations.end(); ++i )
		{
			m_ExistingConversations.AddString( (*i).first.c_str() );
		}
	}
}
