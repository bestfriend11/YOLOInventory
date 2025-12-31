// Dialog_Party_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Party_Editor.h"
#include "ComponentList.h"
#include "EditorObjects.h"
#include "GoParty.h"
#include "player.h"
#include "server.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Party_Editor dialog


Dialog_Party_Editor::Dialog_Party_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Party_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Party_Editor)
	//}}AFX_DATA_INIT
}


void Dialog_Party_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Party_Editor)
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	DDX_Control(pDX, IDC_LIST_ACTORS, m_actor_list);
	DDX_Control(pDX, IDC_MEMBERLIST, m_member_list);
	DDX_Control(pDX, IDC_PARTYCOMBO, m_party_combo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Party_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Party_Editor)
	ON_CBN_SELCHANGE(IDC_PARTYCOMBO, OnEditchangePartycombo)
	ON_BN_CLICKED(IDC_ADDMEMBER, OnAddmember)
	ON_BN_CLICKED(IDC_REMOVEMEMBER, OnRemovemember)
	ON_BN_CLICKED(IDC_REMOVEALLMEMBERS, OnRemoveallmembers)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, OnButtonApply)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Party_Editor message handlers

void Dialog_Party_Editor::OnEditchangePartycombo() 
{
	if ( m_party_combo.GetCurSel() == CB_ERR ) {
		return;
	}

	m_member_list.DeleteAllItems();

	CString rParty;
	m_party_combo.GetLBText( m_party_combo.GetCurSel(), rParty );
	m_edit_name.SetWindowText( rParty );
	int list_index = m_member_list.GetItemCount();

	int partyScid = 0;
	gpstring sParty = rParty.GetBuffer( 0 );
	stringtool::Get( sParty, partyScid );

	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if (( hObject->HasParty() ) && ( MakeScid(partyScid) == hObject->GetScid() ) ) 
		{
			hObject->GetParty()->Update( 0.0f );
			GopColl children = hObject->GetChildren();
			GopColl::iterator j;
			for ( j = children.begin(); j != children.end(); ++j ) 
			{
				gpstring sScid;				
				gpstring sName;
				sScid.assignf( "0x%08X", MakeInt((*j)->GetScid()) );
				
				list_index = m_member_list.InsertItem( list_index, (*j)->GetTemplateName() );
				sScid.assignf( "0x%08X", MakeInt((*j)->GetScid()) );
				m_member_list.SetItem( list_index, 1, LVIF_TEXT, sScid, 0, 0, 0, 0 );
				m_member_list.SetItemData( list_index, MakeInt((*j)->GetScid()) );		
			}		
		}
	}	
}

void Dialog_Party_Editor::OnOK() 
{
	OnButtonApply();

	CDialog::OnOK();
}

void Dialog_Party_Editor::OnAddmember() 
{
	int index = m_actor_list.GetSelectionMark();
	if ( index == LB_ERR ) {
		return;
	}

	CString rFuelName	= m_actor_list.GetItemText( index, 0 );
	CString rScid		= m_actor_list.GetItemText( index, 1 );
	DWORD dwScid		= m_actor_list.GetItemData( index );

	for ( int j = 0; j != m_member_list.GetItemCount(); ++j ) 
	{
		if ( m_member_list.GetItemData( j ) == dwScid ) 
		{
			return;
		}
	}

	gpstring sScid;
	int list_index = m_member_list.GetItemCount();
	list_index = m_member_list.InsertItem( list_index, rFuelName );
	sScid.assignf( "0x%08X", dwScid );
	m_member_list.SetItem( list_index, 1, LVIF_TEXT, sScid, 0, 0, 0, 0 );
	m_member_list.SetItemData( list_index, dwScid );		
}

void Dialog_Party_Editor::OnRemovemember() 
{
	int index = m_member_list.GetSelectionMark();
	if ( index == LB_ERR ) {
		return;
	}

	m_member_list.DeleteItem( index );	
}

void Dialog_Party_Editor::OnRemoveallmembers() 
{
	m_member_list.DeleteAllItems();	
}

BOOL Dialog_Party_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_actor_list.InsertColumn( 0, "Fuel Name", LVCFMT_LEFT, 110 );
	m_actor_list.InsertColumn( 1, "Scid", LVCFMT_LEFT, 60 );

	m_member_list.InsertColumn( 0, "Fuel Name", LVCFMT_LEFT, 110 );
	m_member_list.InsertColumn( 1, "Scid", LVCFMT_LEFT, 60 );

	ComponentVec actors;
	ComponentVec::iterator i;
	gComponentList.RetrieveLocalActorList( actors );
	gpstring sScid;
	int list_index = 0;
	for ( i = actors.begin(); i != actors.end(); ++i ) 
	{
		list_index = m_actor_list.InsertItem( list_index, (*i).sFuelName.c_str() );
		sScid.assignf( "0x%08X", (*i).dwScid );
		m_actor_list.SetItem( list_index, 1, LVIF_TEXT, sScid, 0, 0, 0, 0 );
		m_actor_list.SetItemData( list_index, (*i).dwScid );
		list_index++;
	}
	

	GoidColl objects;
	GoidColl::iterator j;
	gGoDb.GetAllGlobalGoids( objects );
	for ( j = objects.begin(); j != objects.end(); ++j )
	{
		GoHandle hObject( *j );
		if ( hObject->HasParty() ) 
		{
			gpstring sScid;
			sScid.assignf( "0x%08X", MakeInt( hObject->GetScid() ) );
			m_party_combo.AddString( sScid.c_str() );
		}
	}	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Party_Editor::OnButtonApply() 
{
	if ( m_party_combo.GetCurSel() != CB_ERR ) {
		CString rParty;
		m_party_combo.GetLBText( m_party_combo.GetCurSel(), rParty );
		int partyScid = 0;
		gpstring sParty = rParty.GetBuffer( 0 );
		stringtool::Get( sParty, partyScid );

		GoidColl objects;
		GoidColl::iterator j;
		gGoDb.GetAllGlobalGoids( objects );
		for ( j = objects.begin(); j != objects.end(); ++j )
		{
			GoHandle hObject( *j );
			if (( hObject->HasParty() ) && ( MakeScid(partyScid) == hObject->GetScid() ) ) 
			{	
				hObject->RemoveAllChildren();
							
				for ( int i = 0; i != m_member_list.GetItemCount(); ++i ) 
				{
					DWORD dwScid = m_member_list.GetItemData( i );	
					Goid member = gGoDb.FindGoidByScid( MakeScid(dwScid) );					
					if ( member != GOID_INVALID ) 
					{
						hObject->GetParty()->AddMemberDeferred( MakeScid(dwScid) );
					}
				}
				
				hObject->GetParty()->Update( 0.0f );

				gGizmoManager.GetGizmo( MakeInt( hObject->GetGoid() ) )->spos = hObject->GetPlacement()->GetPosition();
			}
		}		
	}		
}

void Dialog_Party_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
