// Dialog_Object_Validation.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Object_Validation.h"
#include "EditorRegion.h"
#include "worldindexbuilder.h"
#include "GoData.h"
#include "BottomView.h"
#include "EditorMap.h"
#include "GoEdit.h"
#include "EditorCamera.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Validation dialog


Dialog_Object_Validation::Dialog_Object_Validation(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Validation::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Validation)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Validation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Validation)
	DDX_Control(pDX, IDC_LIST_SEARCH_RESULTS, m_searchResults);
	DDX_Control(pDX, IDC_BUTTON_DELETE_INVENTORY, m_delete_inventory);
	DDX_Control(pDX, IDC_LIST_MAPS, m_map_list);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Validation, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Validation)
	ON_BN_CLICKED(IDC_BUTTON_VALIDATE_MAP, OnButtonValidateMap)
	ON_BN_CLICKED(IDC_BUTTON_VALIDATE_ALL, OnButtonValidateAll)
	ON_BN_CLICKED(IDC_BUTTON_VALIDATE_ALL_SCIDS, OnButtonValidateAllScids)
	ON_BN_CLICKED(IDC_BUTTON_VALIDATE_TEMPLATES, OnButtonValidateTemplates)
	ON_BN_CLICKED(IDC_BUTTON_VALIDATE_BOOKMARKS, OnButtonValidateBookmarks)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_ASPECT_OVERRIDE, OnButtonDeleteAspectOverride)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_INVENTORY, OnButtonDeleteInventory)
	ON_BN_CLICKED(IDC_BUTTON_SEARCH_CMD, OnButtonSearchCmd)
	ON_LBN_SELCHANGE(IDC_LIST_SEARCH_RESULTS, OnSelchangeListSearchResults)
	ON_BN_CLICKED(IDC_BUTTON_AUTO_CLEAN, OnButtonAutoClean)
	ON_BN_CLICKED(IDC_BUTTON_EQUIPMENT_LIST, OnButtonEquipmentList)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Validation message handlers

void Dialog_Object_Validation::OnButtonValidateMap() 
{
	if ( m_map_list.GetCurSel() != LB_ERR ) {

		CString rMap;
		m_map_list.GetText( m_map_list.GetCurSel(), rMap );

		FuelHandle maps_parent( "world:maps" );
		gpassert( maps_parent.IsValid() );
		FuelHandleList maps = maps_parent->ListChildBlocks( 2, "map" );
		for( FuelHandleList::iterator j = maps.begin(); j != maps.end(); ++j ) {				
			gpstring name;
			(*j)->Get( "name", name );
			
			if ( name.same_no_case( rMap.GetBuffer( 0 ) ) ) {
//				gEditor.GetWorldIndexBuilder()->ValidateBasicContentParentLinks( (*j)->GetParentBlock() );
				return;
			}
		}	
	}
	
}

void Dialog_Object_Validation::OnButtonValidateAll() 
{
	//gEditor.GetWorldIndexBuilder()->ValidateBasicContentParentLinks( true );	
}

void Dialog_Object_Validation::OnButtonValidateAllScids() 
{
//	gEditor.GetWorldIndexBuilder()->ValidateAllScidIndexes( true );	
}

BOOL Dialog_Object_Validation::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec			map_list;
	StringVec::iterator	i;		
	map_list.clear();
	gEditorRegion.GetMapList( map_list );

	for ( i = map_list.begin(); i != map_list.end(); ++i ) {
		m_map_list.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_Validation::OnButtonValidateTemplates() 
{
	StringVec vBadTemplates;
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveValidateTemplates( *i, vBadTemplates );		
	}	

	CString rText;
	gBottomView.GetConsoleEditCtrl().GetWindowText( rText );
	rText.Insert( rText.GetLength(), "\r\n\r\nInvalid Templates:" );
	rText.Insert( rText.GetLength(), "\r\n------------------" );
	
	StringVec::iterator j;
	for ( j = vBadTemplates.begin(); j != vBadTemplates.end(); ++j )
	{
		rText.Insert( rText.GetLength(), "\r\n" );
		rText.Insert( rText.GetLength(), (*j).c_str() );
		rText.Insert( rText.GetLength(), "\r\n" );
	}
	rText.Insert( rText.GetLength(), "\r\n" );

	gBottomView.GetConsoleEditCtrl().SetWindowText( rText );
}


void Dialog_Object_Validation::RecursiveValidateTemplates( GoDataTemplate * pTemplate, StringVec & vBadTemplates )
{
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{
		Goid object = gGoDb.FindCloneSource( pTemplate->GetName() );
		if ( object == GOID_INVALID )
		{
			vBadTemplates.push_back( pTemplate->GetName() );
		}
	}

	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveValidateTemplates( *i, vBadTemplates );
	}
}

void Dialog_Object_Validation::OnButtonValidateBookmarks() 
{
	int sel = m_map_list.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rMap;
	m_map_list.GetText( sel, rMap );
	
	gEditorMap.ValidateBookmarks( rMap.GetBuffer( 0 ) );
}

void Dialog_Object_Validation::OnButtonDeleteAspectOverride() 
{
	if ( m_map_list.GetCurSel() != LB_ERR ) 
	{
		CString rMap;
		m_map_list.GetText( m_map_list.GetCurSel(), rMap );

		FuelHandle maps_parent( "world:maps" );
		gpassert( maps_parent.IsValid() );
		FuelHandleList maps = maps_parent->ListChildBlocks( 1, "map" );
		for( FuelHandleList::iterator j = maps.begin(); j != maps.end(); ++j ) 
		{				
			gpstring name = (*j)->GetName();			
			if ( name.same_no_case( rMap.GetBuffer( 0 ) ) ) 
			{
				FuelHandle hRegions = (*j)->GetParentBlock()->GetChildBlock( "regions" );
				FuelHandleList hlRegions = hRegions->ListChildBlocks( 1 );
				FuelHandleList::iterator i;
				for ( i = hlRegions.begin(); i != hlRegions.end(); ++i )
				{
					gpstring sName = (*i)->GetName();
					gpstring sAddress = hRegions->GetAddress();
					sAddress.appendf( ":%s:objects", sName.c_str() );
					FuelHandle hAddress( sAddress.c_str() );
					FuelHandleList hlObjects = hAddress->ListChildBlocks( 2, "", "aspect" );
					FuelHandleList::iterator k;
					for ( k = hlObjects.begin(); k != hlObjects.end(); ++k )
					{
						//FuelHandle hParent = (*k)->GetParentBlock();
						//hParent->DestroyChildBlock( *k );

						(*k)->Remove( "gold_value" );
					}

				}

				return;
			}
		}	
	}	
}

void Dialog_Object_Validation::OnButtonDeleteInventory() 
{
	if ( m_map_list.GetCurSel() != LB_ERR ) 
	{
		CString rMap;
		m_map_list.GetText( m_map_list.GetCurSel(), rMap );

		FuelHandle maps_parent( "world:maps" );
		gpassert( maps_parent.IsValid() );
		FuelHandleList maps = maps_parent->ListChildBlocks( 2, "map" );
		for( FuelHandleList::iterator j = maps.begin(); j != maps.end(); ++j ) 
		{				
			gpstring name;
			(*j)->Get( "name", name );			
			if ( name.same_no_case( rMap.GetBuffer( 0 ) ) ) 
			{
				FuelHandle hRegions = (*j)->GetParentBlock()->GetChildBlock( "regions" );
				FuelHandleList hlRegions = hRegions->ListChildBlocks( 1 );
				FuelHandleList::iterator i;
				for ( i = hlRegions.begin(); i != hlRegions.end(); ++i )
				{
					gpstring sName = (*i)->GetName();
					gpstring sAddress = hRegions->GetAddress();
					sAddress.appendf( ":%s:objects", sName.c_str() );
					FuelHandle hAddress( sAddress.c_str() );
					FuelHandleList hlObjects = hAddress->ListChildBlocks( 2, "", "inventory" );
					FuelHandleList::iterator k;
					for ( k = hlObjects.begin(); k != hlObjects.end(); ++k )
					{
						FuelHandle hParent = (*k)->GetParentBlock();
						hParent->DestroyChildBlock( *k );
					}

				}

				return;
			}
		}	
	}		
}

void Dialog_Object_Validation::OnButtonSearchCmd() 
{
	m_searchResults.ResetContent();
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() )
		{
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "mind" );													
			if ( pRecord )
			{
				gpstring sScid;
				int intScid = 0;
				if ( pRecord->GetAsString( "initial_command", sScid ) )
				{
					stringtool::Get( sScid, intScid );
					Scid initialCommand = MakeScid( intScid );	
					GoHandle hCommand( gGoDb.FindGoidByScid( initialCommand ) );
					
					gpstring sTarget;
					if ( hCommand.IsValid() )
					{
						sTarget = hCommand->GetTemplateName();
					}
					
					gpstring sGo;
					sGo.assignf( "0x%08X - %s -> %s", hObject->GetGoid(), hObject->GetTemplateName(), sTarget.c_str() );						
					if ( hCommand.IsValid() && !hCommand->IsCommand() )
					{
						int sel = m_searchResults.AddString( sGo.c_str() );
						m_searchResults.SetItemData( sel, MakeInt( hObject->GetGoid() ) );
					}
					else if ( !hCommand.IsValid() && initialCommand != SCID_INVALID )
					{					
						int sel = m_searchResults.AddString( sGo.c_str() );
						m_searchResults.SetItemData( sel, MakeInt( hObject->GetGoid() ) );
					}				
				}
			}
		}
	}
}

void Dialog_Object_Validation::OnSelchangeListSearchResults() 
{
	CString rGo;
	int sel = m_searchResults.GetCurSel();
	int intGoid = m_searchResults.GetItemData( sel );	 	
	if ( intGoid != MakeInt(GOID_INVALID) )
	{
		GoHandle hObject( MakeGoid( intGoid ) );
		if ( hObject.IsValid() )
		{
			SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ));
			SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
			vector_3 worldPos = node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
			gEditorCamera.SetPosition( worldPos.x, worldPos.y, worldPos.z );
			gGizmoManager.ForceSelectGizmo( intGoid );
		}
	}
}

void Dialog_Object_Validation::OnButtonAutoClean() 
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() )
		{
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "mind" );													
			if ( pRecord )
			{
				gpstring sScid;
				int intScid = 0;
				if ( pRecord->GetAsString( "initial_command", sScid ) )
				{
					stringtool::Get( sScid, intScid );
					Scid initialCommand = MakeScid( intScid );	
					GoHandle hCommand( gGoDb.FindGoidByScid( initialCommand ) );
					
					bool bSet = false;
					if ( hCommand.IsValid() && !hCommand->IsCommand() )
					{
						bSet = true;
					}
					else if ( !hCommand.IsValid() && initialCommand != SCID_INVALID )
					{					
						bSet = true;
					}				

					if ( bSet )
					{
						if ( !hObject->GetEdit()->IsEditing() )
						{
							hObject->GetEdit()->BeginEdit();
						}
						pRecord->SetAsString( "initial_command", "0xffffffff" );
						hObject->GetEdit()->EndEdit();
					}
				}
			}
		}
	}	
}

void Dialog_Object_Validation::OnButtonEquipmentList() 
{
	FuelHandle hConfig( "config:editor" );
	FuelHandle hItem = hConfig->CreateChildBlock( "pcontent_items", "pcontent_items.gas" );
	RetrieveObjectList( hItem );
}


// Retrieve a list of all game objects ( excluding triggers and generators, and stores )
void Dialog_Object_Validation::RetrieveObjectList( FuelHandle & hItems )
{	
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveRetrieveObjectList( (*i), hItems );		
	}
}


void Dialog_Object_Validation::RecursiveRetrieveObjectList( GoDataTemplate * pTemplate, FuelHandle & hItems )
{	
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{				
		FastFuelHandle hTemplate	= pTemplate->GetFuelHandle();
		FastFuelHandle hCommon		= hTemplate.GetChildNamed( "common" );
		FastFuelHandle hActor		= hTemplate.GetChildNamed( "actor" );
		
		if ( !hActor && (gpstring( "armor" ).same_no_case( pTemplate->GetMetaGroupName() )
					 || gpstring( "interactive" ).same_no_case( pTemplate->GetMetaGroupName() ) 					 
					 || gpstring( "weapon" ).same_no_case( pTemplate->GetMetaGroupName() ) 
					 || gpstring( "treasure" ).same_no_case( pTemplate->GetMetaGroupName() ) ) )					 					 
		{
			GoCloneReq objReq( pTemplate->GetName() );
			objReq.m_OmniGo = true;
			Goid object = gGoDb.CloneLocalGo( objReq );
			GoHandle hObject( object );
			if ( hObject && hObject->HasGui() && !hObject->HasActor() && IsEquippableSlot( hObject->GetGui()->GetEquipSlot() ) )
			{
				if ( hCommon )
				{
					bool bAllowed = true;
					hCommon.Get( "is_pcontent_allowed", bAllowed );				
					if ( bAllowed )
					{
						hItems->Set( "*", ToAnsi( hObject->GetCommon()->GetScreenName() ).c_str(), FVP_QUOTED_STRING );
					}
				}			
			}
		}
	}
	
	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveRetrieveObjectList( (*i), hItems );
	}
}

void Dialog_Object_Validation::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
