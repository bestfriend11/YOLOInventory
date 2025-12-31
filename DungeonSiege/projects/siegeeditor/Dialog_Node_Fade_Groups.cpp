// Dialog_Node_Fade_Groups.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Node_Fade_Groups.h"
#include "Dialog_New_Node_Fade_Group.h"
#include "EditorTerrain.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Fade_Groups dialog


Dialog_Node_Fade_Groups::Dialog_Node_Fade_Groups(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Node_Fade_Groups::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Node_Fade_Groups)
	//}}AFX_DATA_INIT
}


void Dialog_Node_Fade_Groups::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Node_Fade_Groups)
	DDX_Control(pDX, IDC_LIST_FADES, m_listFades);
	DDX_Control(pDX, IDC_EDIT_SECTION, m_section);
	DDX_Control(pDX, IDC_EDIT_OBJECT, m_object);
	DDX_Control(pDX, IDC_EDIT_LEVEL, m_level);
	DDX_Control(pDX, IDC_CHECK_SECTION, m_checkSection);
	DDX_Control(pDX, IDC_CHECK_OBJECT, m_checkObject);
	DDX_Control(pDX, IDC_CHECK_LEVEL, m_checkLevel);
	DDX_Control(pDX, IDC_COMBO_GROUPS, m_groups);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Node_Fade_Groups, CDialog)
	//{{AFX_MSG_MAP(Dialog_Node_Fade_Groups)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPS, OnSelchangeComboGroups)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_SELECTION, OnButtonRemoveSelection)
	ON_BN_CLICKED(IDC_BUTTON_NEW_GROUP, OnButtonNewGroup)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_GROUP, OnButtonDeleteGroup)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, OnButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, OnButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Fade_Groups message handlers

void Dialog_Node_Fade_Groups::OnSelchangeComboGroups() 
{
	m_listFades.DeleteAllItems();

	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );				
		if ( hGroups.IsValid() )
		{			
			FuelHandleList hlFadeGroups = hGroups->ListChildBlocks( 1, "fade_group" );
			FuelHandleList::iterator i;
			for ( i = hlFadeGroups.begin(); i != hlFadeGroups.end(); ++i )
			{
				if ( gpstring( (*i)->GetName() ).same_no_case( rGroup.GetBuffer( 0 ) ) )
				{
					FuelHandleList hlFadeSettings = (*i)->ListChildBlocks( 1, "fade_setting" );
					FuelHandleList::iterator j;
					int index = 0;
					for ( j = hlFadeSettings.begin(); j != hlFadeSettings.end(); ++j ) 
					{						
						gpstring sSection, sLevel, sObject;
						(*j)->Get( "section", sSection );
						(*j)->Get( "level", sLevel );
						(*j)->Get( "object", sObject );

						int newItem = m_listFades.InsertItem( index, sSection.c_str() );												

						if ( !sLevel.empty() )
						{
							m_listFades.SetItem( newItem, 1, LVIF_TEXT, sLevel.c_str(), 0, 0, 0, 0 );
						}

						if ( !sObject.empty() )
						{
							m_listFades.SetItem( newItem, 2, LVIF_TEXT, sObject.c_str(), 0, 0, 0, 0 );
						}

						index++;						
					}				
					break;
				}
			}
		}
	}
}

BOOL Dialog_Node_Fade_Groups::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_listFades.InsertColumn( 0, "Section", LVCFMT_LEFT, 50 );
	m_listFades.InsertColumn( 1, "Level", LVCFMT_LEFT, 50 );
	m_listFades.InsertColumn( 2, "Object", LVCFMT_LEFT, 50 );

	RefreshGroups();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Node_Fade_Groups::RefreshGroups()
{
	m_groups.ResetContent();

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );
		if ( hGroups.IsValid() )
		{
			FuelHandleList hlFadeGroups = hGroups->ListChildBlocks( 1, "fade_group" );
			FuelHandleList::iterator i;
			for ( i = hlFadeGroups.begin(); i != hlFadeGroups.end(); ++i )
			{
				m_groups.AddString( (*i)->GetName() );			
			}
		}
	}
}

void Dialog_Node_Fade_Groups::OnButtonAdd() 
{	
	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	bool bSection	= m_checkSection.GetCheck() ? true : false;
	bool bLevel		= m_checkLevel.GetCheck() ? true : false;
	bool bObject	= m_checkObject.GetCheck() ? true : false;

	if ( !bSection && !bLevel && !bObject )
	{
		return;
	}
	
	CString rSection;
	CString rLevel;
	CString rObject;

	if ( bSection )
	{
		m_section.GetWindowText( rSection );	
	}

	if ( bLevel )
	{
		m_level.GetWindowText( rLevel );	
	}

	if ( bObject )
	{
		m_object.GetWindowText( rObject );
	}

	if ( rSection.IsEmpty() && rLevel.IsEmpty() && rObject.IsEmpty() )
	{
		return;
	}

	gpstring sName;
	gpstring sSection = rSection.GetBuffer( 0 );
	gpstring sLevel = rLevel.GetBuffer( 0 );
	gpstring sObject = rObject.GetBuffer( 0 );
	if ( rSection.IsEmpty() )
	{
		sSection = "e";
	}
	if ( rLevel.IsEmpty() )
	{
		sLevel = "e";
	}
	if ( rObject.IsEmpty() )
	{
		sObject = "e";
	}

	sName.assignf( "%s%s%s", sSection.c_str(), sLevel.c_str(), sObject.c_str() );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );
		
		if ( hGroups.IsValid() )
		{	
			FuelHandle hGroup = hGroups->GetChildBlock( rGroup.GetBuffer( 0 ) );		

			if ( hGroup.IsValid() )
			{				
				FuelHandle hFadeSetting = hGroup->GetChildBlock( sName );
				if ( hFadeSetting )
				{
					return;
				}
				else
				{
					hFadeSetting = hGroup->CreateChildBlock( sName.c_str() );
				}

				hFadeSetting->SetType( "fade_setting" );
				hFadeSetting->Set( "section", rSection.GetBuffer( 0 ) );
				hFadeSetting->Set( "level", rLevel.GetBuffer( 0 ) );
				hFadeSetting->Set( "object", rObject.GetBuffer( 0 ) );
			}
		}
	}

	OnSelchangeComboGroups();
}

void Dialog_Node_Fade_Groups::OnButtonRemoveSelection() 
{
	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	CString rSection;
	CString rLevel;
	CString rObject;

	rSection = m_listFades.GetItemText( m_listFades.GetSelectionMark(), 0 );
	rLevel = m_listFades.GetItemText( m_listFades.GetSelectionMark(), 1 );
	rObject = m_listFades.GetItemText( m_listFades.GetSelectionMark(), 2 );
	
	gpstring sName;
	gpstring sSection = rSection.GetBuffer( 0 );
	gpstring sLevel = rLevel.GetBuffer( 0 );
	gpstring sObject = rObject.GetBuffer( 0 );

	if ( rSection.IsEmpty() )
	{
		sSection = "e";
	}
	if ( rLevel.IsEmpty() )
	{
		sLevel = "e";
	}
	if ( rObject.IsEmpty() )
	{
		sObject = "e";
	}

	sName.assignf( "%s%s%s", sSection.c_str(), sLevel.c_str(), sObject.c_str() );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );
		
		if ( hGroups.IsValid() )
		{	
			FuelHandle hGroup = hGroups->GetChildBlock( rGroup.GetBuffer( 0 ) );

			if ( hGroup.IsValid() )
			{
				FuelHandle hFadeSettings = hGroup->GetChildBlock( sName );
				if ( hFadeSettings.IsValid() )
				{
					hGroup->DestroyChildBlock( hFadeSettings );
				}
			}
		}
	}

	OnSelchangeComboGroups();		
}

void Dialog_Node_Fade_Groups::OnButtonNewGroup() 
{
	Dialog_New_Node_Fade_Group dlg;
	if ( dlg.DoModal() == IDOK )
	{
		m_groups.AddString( GetNewGroupName().c_str() );

		FuelHandle hEditor( "config:editor" );
		if ( hEditor.IsValid() )
		{
			FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );
			if ( !hGroups.IsValid() )
			{
				hGroups = hEditor->CreateChildBlock( "fade_groups", "fade_groups.gas" );				
			}

			if ( hGroups.IsValid() )
			{	
				FuelHandle hGroup = hGroups->CreateChildBlock( GetNewGroupName() );
				hGroup->SetType( "fade_group" );
			}
		}
	}		
}

void Dialog_Node_Fade_Groups::OnButtonDeleteGroup() 
{
	CString rGroup;
	int sel = m_groups.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	m_groups.GetLBText( sel, rGroup );

	FuelHandle hEditor( "config:editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hGroups = hEditor->GetChildBlock( "fade_groups" );				
		if ( hGroups.IsValid() )
		{	
			FuelHandle hChild = hGroups->GetChildBlock( rGroup.GetBuffer( 0 ) );
			if ( hChild.IsValid() )
			{
				hGroups->DestroyChildBlock( hChild );
			}
		}
	}

	RefreshGroups();	
	OnSelchangeComboGroups();	
}

void Dialog_Node_Fade_Groups::OnButtonHide() 
{
	OnButtonSelect();

	{
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( gEditorTerrain.GetSelectedNode() );
		if ( pNode )
		{
			pNode->SetVisible( false );	
		}
	}

	GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
	GuidVec::iterator i;
	for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) 
	{
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( *i );
		if ( pNode )
		{
			pNode->SetVisible( false );				
		}
	}

	gEditorTerrain.ClearAllSelectedNodes();
}

void Dialog_Node_Fade_Groups::OnButtonShow() 
{
	OnButtonSelect();

	{
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( gEditorTerrain.GetSelectedNode() );
		if ( pNode )
		{
			pNode->SetVisible( true );	
		}
	}

	GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
	GuidVec::iterator i;
	for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) 
	{
		SiegeNode * pNode = gSiegeEngine.IsNodeValid( *i );
		if ( pNode )
		{
			pNode->SetVisible( true );					
		}
	}

	gEditorTerrain.ClearAllSelectedNodes();
}

void Dialog_Node_Fade_Groups::OnButtonSelect() 
{	
	gEditorTerrain.SetSelectedNode( UNDEFINED_GUID );

	for ( int iIndex = 0; iIndex != m_listFades.GetItemCount(); ++iIndex )
	{
		int section = 0;
		int level = 0;
		int object = 0;

		CString rSection;
		CString rLevel;
		CString rObject;
		rSection = m_listFades.GetItemText( iIndex, 0 );
		rLevel = m_listFades.GetItemText( iIndex, 1 );
		rObject = m_listFades.GetItemText( iIndex, 2 );

		bool bCheckSection = rSection.IsEmpty() ? false : true;
		bool bCheckLevel = rLevel.IsEmpty() ? false : true;
		bool bCheckObject = rObject.IsEmpty() ? false : true;
		
		if ( bCheckSection )
		{
			stringtool::Get( rSection.GetBuffer( 0 ), section );
		}
		if ( bCheckLevel )
		{
			stringtool::Get( rLevel.GetBuffer( 0 ), level );
		}
		if ( bCheckObject )
		{
			stringtool::Get( rObject.GetBuffer( 0 ), object );
		}

		siege::SiegeEngine& engine = gSiegeEngine;
		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator i;
		for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			SiegeNode * pNode = *i;		

			if ( pNode && 
				 ( (bCheckSection && pNode->GetSection() == section) || !bCheckSection ) &&
				 ( (bCheckLevel && pNode->GetLevel() == level) || !bCheckLevel ) && 
				 ( (bCheckObject && pNode->GetObject() == object) || !bCheckObject ) )
			{
				if ( i == nodeColl.begin() )
				{
					gEditorTerrain.SetSelectedNode( pNode->GetGUID() );
				}
				else
				{
					gEditorTerrain.AddNodeAsSelected( pNode->GetGUID() );
				}
			}
		}		
	}	
}

void Dialog_Node_Fade_Groups::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
