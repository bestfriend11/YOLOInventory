//////////////////////////////////////////////////////////////////////////////
//
// File     :  ComponentList.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



// Include Files
#include "PrecompEditor.h"
#include "ComponentList.h"
#include "namingkey.h"
#include "EditorRegion.h"
#include "Preferences.h"
#include "GoData.h"
#include "GoInventory.h"
#include "worldmessage.h"
#include "GoSupport.h"
#include "FuBiDefs.h"
#include "FuBiSchema.h"
#include "FileSys.h"
#include "BottomView.h"
#include "WorldMap.h"
#include "FileSys.h"
#include "siege_database.h"
#include "EditorMap.h"

using namespace siege;
using namespace FileSys;

void ComponentList::RetrieveNodeCollection( StringVec & node_coll )
{	
	m_node_component_list.clear();

	StringVec			missingSnos;
	DuplicateNodeColl	duplicateNodes;
	
	FastFuelHandle hNodesDir( "world:global:siege_nodes" );
	gpassert( hNodesDir.IsValid() );

	FastFuelHandleColl NodeDefBlockList;
	if( !hNodesDir.ListChildrenTyped( NodeDefBlockList, "siege_nodes", -1 ) )
	{
		gpassertm( 0, "Sir, something horrible has happened." );
	}

	FastFuelHandleColl::iterator i;	
	for ( i = NodeDefBlockList.begin(); i != NodeDefBlockList.end(); ++i ) 
	{
		gpstring sType = (*i).GetType();
		gpassert( sType.same_no_case( "siege_nodes" ) );
		FastFuelHandleColl fblNodes;
		(*i).ListChildren( fblNodes, 1 );		
		FastFuelHandleColl::iterator j;
		for ( j = fblNodes.begin(); j != fblNodes.end(); ++j ) 
		{				
			//
			// Check for generic node name
			//

			gpstring szGenericName;
			gpstring szFileName;
			TATTOO_NODE_COMPONENT tc;

			if ((*j).Get( "filename", szFileName )) 
			{
				// Parse the generic node format, building the path using the NamingKey
				tc.Name = szFileName+".SNO";

				// Get the full path name from the naming key
				gpstring szPathName;
				gNamingKey.BuildSNOLocation(szFileName.c_str(),szPathName);

				int pos = szPathName.find("Art\\Terrain\\");

				if (pos != (int)szPathName.npos) 
				{
					tc.FileName = szPathName.substr(sizeof("Art\\Terrain\\")-1);
				} else 
				{
					tc.FileName = szPathName;
				}

				(*j).Get( "guid", tc.GUIDstring );

				if ( tc.FileName.empty() ) 
				{
					tc.FileName = tc.Name + "- NOT IN NAMING KEY";
				}

				gpstring fullname;
				gpstring sFilename = tc.Name.substr( 0, tc.Name.size()-4 ); 
				if ( gNamingKey.BuildSNOLocation( sFilename, fullname ) )
				{
					if ( !FileSys::DoesResourceFileExist( fullname ) )
					{
						missingSnos.push_back( fullname );
					}
				}
			}			

			/*
			int nodeID = 0;
			stringtool::Get( tc.GUIDstring, nodeID );
			NodeComponentMap::iterator iNode = m_node_component_list.find( nodeID );
			if ( iNode != m_node_component_list.end() )
			{
				DuplicateNode dn;
				dn.nodeID = nodeID;
				dn.sName1 = tc.FileName;
				dn.sName2 = (*iNode).second.FileName;
				duplicateNodes.push_back( dn );
			}
			else
			{
			*/
				m_node_component_list.insert( NodeComponentPair( tc.Name, tc ) );			
				node_coll.push_back( tc.FileName );
			//}
		}
	}	
	
	if ( missingSnos.size() != 0 )
	{
		StringVec::iterator k;
		CString rText;
		gBottomView.GetConsoleEditCtrl().GetWindowText( rText );
		rText.Insert( rText.GetLength(), "\r\n\r\nMissing Sno's:" );
		rText.Insert( rText.GetLength(), "\r\n--------------" );
		
		for ( k = missingSnos.begin(); k != missingSnos.end(); ++k )
		{
			rText.Insert( rText.GetLength(), "\r\n" );
			rText.Insert( rText.GetLength(), (*k).c_str() );
			rText.Insert( rText.GetLength(), "\r\n" );
		}

		gBottomView.GetConsoleEditCtrl().SetWindowText( rText );
	}

	if ( duplicateNodes.size() != 0 )
	{
		DuplicateNodeColl::iterator k;
		CString rText;
		gBottomView.GetConsoleEditCtrl().GetWindowText( rText );
		rText.Insert( rText.GetLength(), "\r\n\r\nDuplicate Sno's:" );
		rText.Insert( rText.GetLength(), "\r\n----------------" );
		
		for ( k = duplicateNodes.begin(); k != duplicateNodes.end(); ++k )
		{
			rText.Insert( rText.GetLength(), "\r\n" );
			gpstring sError;
			sError.assignf( "Mesh GUID: 0x%08X points to %s and %s", (*k).nodeID, (*k).sName1.c_str(), (*k).sName2.c_str() );
			rText.Insert( rText.GetLength(), sError.c_str() );
			rText.Insert( rText.GetLength(), "\r\n" );
		}

		gBottomView.GetConsoleEditCtrl().SetWindowText( rText );
	}
}


void ComponentList::RetrieveExistingNodeCollection( StringVec & nodeColl )
{
	NodeComponentMap::iterator i;
	for ( i = m_node_component_list.begin(); i != m_node_component_list.end(); ++i )
	{
		nodeColl.push_back( (*i).second.FileName );
	}
}



void ComponentList::RetrieveNodeMeshCollection( StringVec & node_list )
{	
	gSiegeEngine.MeshDatabase().FileNameMap().clear();
	m_node_component_list.clear();

	int meshId = gEditorRegion.GetMeshRange() << 20;
	RecursiveFinder hSnos( "art/terrain/*.sno" );
	if ( hSnos )
	{
		gpstring sFile;
		while ( hSnos.GetNext( sFile, true ) )
		{
			int pos = sFile.find("art\\terrain\\");
			if (pos != (int)sFile.npos) 
			{
				sFile = sFile.substr(sizeof("art\\terrain\\")-1);
			}

			pos = sFile.find("art/terrain/");
			if (pos != (int)sFile.npos) 
			{
				sFile = sFile.substr(sizeof("art/terrain/")-1);
			}

			TATTOO_NODE_COMPONENT tc;
			tc.FileName = sFile;				
			tc.Name = GetFileFromFullPath( sFile );			
			tc.GUIDstring = gpstringf( "0x%08X", meshId++ );
			node_list.push_back( sFile );
			m_node_component_list.insert( std::make_pair( tc.Name, tc ) );

			gpstring sMesh = tc.Name;
			stringtool::RemoveExtension( sMesh );

			gpstring sContentLocation;
			if ( gNamingKey.BuildContentLocation( sMesh, sContentLocation ) )
			{
				gSiegeEngine.MeshDatabase().InsertMeshMapping( siege::database_guid( tc.GUIDstring ), sMesh );
			}
			else
			{
				gperrorf( ( "Siege node mesh: %s does not properly adhere to the naming key conventions.", sMesh.c_str() ) );
			}
		}
	}
}


// Retrieve a list of all game objects ( excluding triggers and generators, and stores )
void ComponentList::RetrieveObjectList( ComponentVec & object_list, eDisplayType disp_type )
{	
	m_objectColl.clear();
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveRetrieveObjectList( (*i), object_list, disp_type );		
	}
}


void ComponentList::RecursiveRetrieveObjectList( GoDataTemplate * pTemplate, ComponentVec & object_list, eDisplayType disp_type )
{	
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{				
		ObjectComponent  oc;
		oc.sFuelName = pTemplate->GetName();
		FastFuelHandle hTemplate	= pTemplate->GetFuelHandle();
		FastFuelHandle hCommon		= hTemplate.GetChildNamed( "common" );

		if ( hCommon )
		{
			oc.sDevName = pTemplate->GetDocs(); 
			hCommon.Get( "screen_name", oc.sScreenName );
		}	
		
		switch ( disp_type )
		{
		case BLOCK_NAME:
			oc.sName = oc.sFuelName;
			break;
		case SCREEN_NAME:
			oc.sName = oc.sScreenName;
			break;
		case DEV_NAME:
			oc.sName = oc.sDevName;
			break;
		}

		
		object_list.push_back( oc );
		m_objectColl.push_back( oc );
	}
	
	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveRetrieveObjectList( (*i), object_list, disp_type );
	}
}


void ComponentList::RetrieveEquipmentList( ComponentVec & item_vec, eEquipSlot filter )
{
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveRetrieveEquipmentList( (*i), item_vec, filter );		
	}	
}


void ComponentList::RecursiveRetrieveEquipmentList( GoDataTemplate * pTemplate, ComponentVec & item_vec, eEquipSlot filter )
{	
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{
		const GoDataComponent * pComponent = pTemplate->FindComponentByName( "gui" );
		if ( pComponent )
		{
			eEquipSlot slot = ES_NONE;
			pComponent->GetRecord()->Get( "equip_slot", slot );

			if ( filter == slot ) 
			{
				ObjectComponent  oc;
				oc.sFuelName = pTemplate->GetName();
				FastFuelHandle hTemplate = pTemplate->GetFuelHandle();
				FastFuelHandle hCommon = hTemplate.GetChildNamed( "common" );
				if ( hCommon )
				{
					oc.sDevName = pTemplate->GetDocs(); 
					hCommon.Get( "screen_name", oc.sScreenName );
				}

				switch ( gPreferences.GetContentDisplay() )
				{
					case BLOCK_NAME:
						oc.sName = oc.sFuelName;
						break;
					case SCREEN_NAME:
						oc.sName = oc.sScreenName;
						break;
					case DEV_NAME:
						oc.sName = oc.sDevName;
						break;
				}
				item_vec.push_back( oc );
			}
		}
	}
	
	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveRetrieveEquipmentList( (*i), item_vec, filter );
	}	
}


void ComponentList::SearchAndRetrieveContentNames(	ComponentVec & content, 
													gpstring block_name, 
													gpstring screen_name,
													gpstring description,
													gpstring type )
{	
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveSearchAndRetrieveContentNames( (*i), content, block_name, screen_name, description, type );		
	}		
}


void ComponentList::RecursiveSearchAndRetrieveContentNames(	GoDataTemplate * pTemplate,
															ComponentVec & content, gpstring block_name, 
															gpstring screen_name,
															gpstring description,
															gpstring type )
{
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{	
		const GoDataComponent * pComponent = pTemplate->FindComponentByName( "common" );
		if ( pComponent )
		{
			ObjectComponent  oc;
			oc.sDevName = pTemplate->GetDocs();			
			pComponent->GetRecord()->Get( "screen_name", oc.sScreenName );
			oc.sFuelName = pTemplate->GetName();

			switch ( gPreferences.GetContentDisplay() )
			{
			case BLOCK_NAME:
				oc.sName = oc.sFuelName;
				break;
			case SCREEN_NAME:
				oc.sName = oc.sScreenName;
				break;
			case DEV_NAME:
				oc.sName = oc.sDevName;
				break;
			}

			gpstring sBaseTemplate = pTemplate->GetGroupName();		

			bool bMatch1 = false;
			bool bMatch2 = false;
			bool bMatch3 = false;
			bool bMatch4 = false;				
			
			if ( ( block_name.empty() ) || ( oc.sFuelName.find( block_name ) != -1 ) ) {
				bMatch1 = true;
			}	
			
			if ( ( screen_name.empty() ) || ( oc.sScreenName.find( screen_name ) != -1 ) ) {
				bMatch2 = true;
			}
			
			if ( ( description.empty() ) || ( oc.sDevName.find( description ) != -1 ) ) {
				bMatch3 = true;
			}
			
			if ( type.empty() || DoesSpecialize( pTemplate, type ) )
			{				
				bMatch4 = true;							
			}
			
			if ( bMatch1 && bMatch2 && bMatch3 && bMatch4 ) {
				oc.dwScid = MakeInt(SCID_INVALID);
				content.push_back( oc );
			}
		}				
	}
	
	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveSearchAndRetrieveContentNames( (*i), content, block_name, screen_name, description, type );
	}	
}


bool ComponentList::DoesSpecialize( GoDataTemplate * pTemplate, gpstring sBase )
{
	const GoDataTemplate * pParent = pTemplate->GetBaseDataTemplate();
	while ( pParent )
	{
		if ( sBase.same_no_case( pParent->GetName() ) )
		{
			return true;
		}

		pParent = pParent->GetBaseDataTemplate();
	}

	return false;
}


void ComponentList::InsertTemplateId( gpstring sTemplate )
{
	const GoDataTemplate * pTemplate = gContentDb.FindTemplateByName( sTemplate );

	ObjectComponent  oc;	
	oc.sFuelName = pTemplate->GetName();
	FastFuelHandle hTemplate	= pTemplate->GetFuelHandle();
	FastFuelHandle hCommon		= hTemplate.GetChildNamed( "common" );
	oc.sDevName = pTemplate->GetDocs();

	if ( hCommon )
	{
		hCommon.Get( "screen_name", oc.sScreenName );
	}
	
	oc.dwScid = m_currId;
	m_ComponentIds.insert( ComponentIdPair(m_currId, oc) );
	m_ComponentTemplates.insert( ComponentTemplatePair( oc.sFuelName, m_currId++ ) );
}


bool ComponentList::GetNameFromMapId( int id, gpstring & sName )
{
	if ( m_ComponentIds.size() )
	{
		ComponentIdMap::iterator i = m_ComponentIds.find( id );

		if ( m_ComponentIds.end() != i )
		{
			switch ( gPreferences.GetContentDisplay() )
			{
			case BLOCK_NAME:
				sName = (*i).second.sFuelName;				
				return true;
			case SCREEN_NAME:
				sName = (*i).second.sScreenName;
				return true;
			case DEV_NAME:
				sName = (*i).second.sDevName;
				return true;
			}
		}
	}	
	return false;
}


bool ComponentList::GetTemplateNameFromMapId( int id, gpstring & sTemplate )
{
	if ( m_ComponentIds.size() )
	{
		ComponentIdMap::iterator i = m_ComponentIds.find( id );

		if ( m_ComponentIds.end() != i )
		{
			sTemplate = (*i).second.sFuelName;
			return true;
		}
	}	
	return false;
}


bool ComponentList::GetMapIdFromTemplateName( gpstring sTemplate, int & id )
{
	if ( m_ComponentTemplates.size() )
	{
		ComponentTemplateMap::iterator i = m_ComponentTemplates.find( sTemplate );

		if ( m_ComponentTemplates.end() != i )
		{
			id = (*i).second;
			return true;
		}
	}
	return false;
}


void ComponentList::RetrieveLocalActorList( ComponentVec & actor_list )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );		
	for ( i = objects.begin(); i != objects.end(); ++i ) 
	{
		GoHandle hObject( *i );
		if ( hObject->HasActor() )
		{
			ObjectComponent oc;
			oc.sName = hObject->GetTemplateName();
			oc.sFuelName = hObject->GetTemplateName();

			oc.sScreenName = ToAnsi( hObject->GetCommon()->GetScreenName() );			
			oc.sDevName	= hObject->GetDataTemplate()->GetDocs();

			oc.dwScid = MakeInt(hObject->GetScid());

			actor_list.push_back( oc );
		}
	}
}


gpstring ComponentList::GetNodeGUID( char *szNode )
{
	gpstring NodeName;
	NodeName.assign( szNode );
	NodeComponentMap::iterator i = m_node_component_list.find( szNode );
	if ( i != m_node_component_list.end() )
	{
		gpstring fullname;
		gpstring sFilename = (*i).second.Name.substr( 0, (*i).second.Name.size()-4 ); 
		if ( gNamingKey.BuildSNOLocation( sFilename, fullname ) )
		{
			if ( !FileSys::DoesResourceFileExist( fullname ) )
			{
				gperrorf(( "Error: could not find file '%s' for mesh guid '%s'!\n",
						   fullname.c_str(), (*i).second.GUIDstring.c_str() ));
				return "";
			}
		}

		return ( (*i).second.GUIDstring );		
	}

	return "";
}


void ComponentList::RetrieveUsedNodeList( StringVec & used_nodes )
{		
	FuelHandleList fhlNodes = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, "snode" );	
	FuelHandleList::iterator i;
	for ( i = fhlNodes.begin(); i != fhlNodes.end(); ++i ) 
	{
		gpstring szGUID;
		(*i)->Get( "guid", szGUID );
		used_nodes.push_back( szGUID );
	}	
}


void ComponentList::GetMeshToSnoMap( std::map< gpstring, gpstring, istring_less > & mesh_list )
{	
	SiegeEngine & engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator iNode;

	if ( gEditorMap.GetHasNodeMeshIndex() )
	{	
		for ( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
		{
			siege::MeshDatabase::MeshFileMap meshMapCopy = gSiegeEngine.MeshDatabase().FileNameMap();
			siege::MeshDatabase::MeshFileMap::iterator iMesh;
			for ( iMesh = meshMapCopy.begin(); iMesh != meshMapCopy.end(); ++iMesh )
			{
				if ( (*iMesh).first == (*iNode)->GetMeshGUID() ) 
				{				
					std::map< gpstring, gpstring >::value_type entry( (*iMesh).first.ToString(), RemoveDirectories( (*iMesh).second ) );
					mesh_list.insert( entry );
					break;
				}			
			}
		}
	}
	else
	{
		FuelHandle hMesh( "world:global:siege_nodes" );
		FuelHandleList fhlMeshes = hMesh->ListChildBlocks( -1, NULL, "mesh_file*" );
		FuelHandleList::iterator j;
		
		for ( iNode = nodeColl.begin(); iNode != nodeColl.end(); ++iNode )
		{		
			for ( j = fhlMeshes.begin(); j != fhlMeshes.end(); ++j ) 
			{
				gpstring guid;
				(*j)->Get( "guid", guid );
				siege::database_guid mesh_guid_source( guid.c_str() );
				if ( mesh_guid_source == (*iNode)->GetMeshGUID() ) 
				{
					gpstring filename;				
					if ( (*j)->Get( "filename", filename ) == false ) {
						(*j)->Get( "genericname", filename );
					}
					char szNewSno[1024] = "";
					sprintf( szNewSno, "%s", filename.c_str() );
					std::map< gpstring, gpstring >::value_type entry( guid, RemoveDirectories( szNewSno ) );
					mesh_list.insert( entry );
					break;
				}
			}				
		}	
	}
}



gpstring ComponentList::GetMeshSnoFile( gpstring sMesh )
{
	FuelHandle hMesh( "world:global:siege_nodes" );
	FuelHandleList fhlMeshes = hMesh->ListChildBlocks( -1, NULL, "mesh_file*" );
	FuelHandleList::iterator j;
	for ( j = fhlMeshes.begin(); j != fhlMeshes.end(); ++j ) {
		gpstring guid;
		(*j)->Get( "guid", guid );
		guid.to_lower();
		if ( guid.same_no_case( sMesh ) ) {
			gpstring filename;				
			if ( (*j)->Get( "filename", filename ) == false ) {
				(*j)->Get( "genericname", filename );
			}
			return filename;
		}
	}
	return "";
}


void ComponentList::RetrieveObjectInventoryVec( Goid object, ComponentVec & inventory )
{	
	GoHandle hGO( object );
	if ( hGO.IsValid() ) 
	{			
		if ( hGO->HasInventory() )
		{
			GopColl items;
			GopColl::iterator i;
			hGO->GetInventory()->ListItems( IL_MAIN, items );
			for ( i = items.begin(); i != items.end(); ++i )
			{						
				ObjectComponent oc;					
				oc.dwScid = MakeInt((*i)->GetScid());
				oc.object = (*i)->GetGoid();

				oc.sFuelName = (*i)->GetTemplateName();
				
				if ( (*i)->HasCommon() )
				{
					oc.sDevName = (*i)->GetDataTemplate()->GetDocs();
					oc.sScreenName = ToAnsi( (*i)->GetCommon()->GetScreenName() );
					oc.sName = ToAnsi( (*i)->GetCommon()->GetScreenName() );										
				}
				else
				{
					oc.sName = (*i)->GetTemplateName();										
				}
				inventory.push_back( oc );					
			}
		}						
	}	
}


void ComponentList::GetTemplateTypes( StringVec & types ) 
{	
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{		
		RecursiveGetTemplateTypes( *i, types );
		if ( !(*i)->IsLeafTemplate() )
		{		
			types.push_back( (*i)->GetName() );
		}
	}
}


void ComponentList::RecursiveGetTemplateTypes( const GoDataTemplate * pTemplate, StringVec & types )
{
	GoDataTemplate::ChildIter j;
	for ( j = pTemplate->GetChildBegin(); j != pTemplate->GetChildEnd(); ++j )
	{
		RecursiveGetTemplateTypes( *j, types );
		if ( !(*j)->IsLeafTemplate() )
		{
			types.push_back( (*j)->GetName() );
		}
	}
}


gpstring ComponentList::GetFuelName( Scid scid )
{
	gpstring sGasPath;
	FastFuelHandle hPath = gWorldMap.MakeObjectFuel( scid, gEditorRegion.GetRegionGUID() );

	if ( hPath )
	{
		return hPath.GetName();
	}
	return "";
}


// Removes directories from a path
char * RemoveDirectories( char *name )
{
	for ( UINT i = 0; i < strlen( name ); ++i ) {
		if ( name[i] == '\\' || name[i] == '/' ) {
			name = &(name[++i]);
			i = 0;
		}
	}		
	return name;
}

gpstring RemoveDirectories( const gpstring & sName )
{
	gpstring sNewName = sName;
	gpstring::const_iterator i;
	int index = 0;
	for ( i = sNewName.begin(); i != sNewName.end(); ++i )
	{
		if ( (*i) == '\\' || (*i) == '/' )
		{
			sNewName = sNewName.substr( 0, index );
			i = sNewName.begin();
		}
		index++;
	}
	
	return sNewName;
}

gpstring GetFileFromFullPath( const gpstring & sFullPath )
{
	gpstring sNewName = sFullPath;
	gpstring::const_iterator i;
	int index = 0;
	for ( i = sNewName.begin(); i != sNewName.end(); )
	{
		if ( (*i) == '\\' || (*i) == '/' )
		{
			sNewName = sNewName.substr( index+1, sNewName.size() );
			i = sNewName.begin();
			index = 0;
		}
		else
		{
			index++;
			i++;
		}
	}

	return sNewName;
}


ObjectComponent ComponentList::GetObjectComponent( gpstring sTemplate )
{
	if ( m_objectColl.size() == 0 )
	{
		ComponentVec objectColl;
		RetrieveObjectList( objectColl );
	}

	ComponentVec::iterator i;
	for ( i = m_objectColl.begin(); i != m_objectColl.end(); ++i )
	{
		if ( (*i).sFuelName.same_no_case( sTemplate ) )
		{
			return (*i);
		}
	}

	return ObjectComponent();
}




void ComponentList::ViewSearch(	ViewSearchFolderVec & searchFolders )
{	
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveViewSearch( (*i), searchFolders );		
	}		
}


void ComponentList::RecursiveViewSearch( GoDataTemplate * pTemplate, ViewSearchFolderVec & searchFolders )
{
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{	
		const GoDataComponent * pComponent = pTemplate->FindComponentByName( "common" );
		if ( pComponent )
		{

			ViewSearchFolderVec::iterator j;
			for ( j = searchFolders.begin(); j != searchFolders.end(); ++j )
			{
				ViewSearchVec::iterator k;
				for ( k = (*j).searchRequests.begin(); k != (*j).searchRequests.end(); ++k )
				{
					gpstring sName;
					switch ( (*k).type )
					{
					case ST_SCREEN_NAME:
						pComponent->GetRecord()->Get( "screen_name", sName );
						break;
					case ST_TEMPLATE:
						sName = pTemplate->GetName();
						break;
					case ST_DOCUMENTATION:
						sName = pTemplate->GetDocs();		
						break;
					case ST_CATEGORY:
						sName = pTemplate->GetCategoryName();
						break;
					}
					
					sName.to_lower();
					gpstring sRequest = (*k).sName;
					sRequest.to_lower();
					if ( sName.find( sRequest ) != -1 )
					{
						int id = 0;
						if ( GetMapIdFromTemplateName( pTemplate->GetName(), id ) )
						{
							(*j).matches.push_back( id );
						}
					}				
				}
			}		
		}				
	}
	
	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveViewSearch( (*i), searchFolders );
	}	
}


void ComponentList::LoadViewSearchDb( ViewSearchFolderVec & searchFolders )
{
	FastFuelHandle hSearchData( "config:editor:se_view_search_db" );
	if ( hSearchData.IsValid() )
	{
		FastFuelHandleColl hFolders;
		FastFuelHandleColl::iterator i;
		hSearchData.ListChildren( hFolders, -1 );
		for ( i = hFolders.begin(); i != hFolders.end(); ++i )
		{
			SearchFolder folder;
			folder.sName = (*i).GetName();
			folder.sParent = (*i).GetParent().GetName();			
			if ( folder.sParent.same_no_case( "se_view_search_db" ) )
			{
				folder.sParent.clear();
			}
						
			FastFuelFindHandle hFind( *i );
			bool bContinue = hFind.FindFirstValue( "search_screen_name" );
			
			while ( bContinue )
			{				
				ComponentSearchData sd;
				sd.type = ST_SCREEN_NAME;
				bContinue = hFind.GetNextValue( sd.sName ) ? true : false;
				if ( bContinue )
				{
					folder.searchRequests.push_back( sd );
				}
			}

			bContinue = hFind.FindFirstValue( "search_template" );
			while ( bContinue )
			{
				ComponentSearchData sd;
				sd.type = ST_TEMPLATE;
				bContinue = hFind.GetNextValue( sd.sName ) ? true : false;
				if ( bContinue )
				{
					folder.searchRequests.push_back( sd );
				}
			}

			bContinue = hFind.FindFirstValue( "search_doc" );
			while ( bContinue )
			{
				ComponentSearchData sd;
				sd.type = ST_DOCUMENTATION;
				bContinue = hFind.GetNextValue( sd.sName ) ? true : false;
				if ( bContinue )
				{
					folder.searchRequests.push_back( sd );
				}
			}

			bContinue = hFind.FindFirstValue( "search_category" );
			while ( bContinue )
			{
				ComponentSearchData sd;
				sd.type = ST_CATEGORY;
				bContinue = hFind.GetNextValue( sd.sName ) ? true : false;
				if ( bContinue )
				{
					folder.searchRequests.push_back( sd );
				}
			}
			
			searchFolders.push_back( folder );
		}		
	}
}


void ComponentList::BuildCategoryNames()
{
	TemplateColl coll;
	TemplateColl::iterator i;
	gContentDb.FindTemplatesByWildcard( coll, "*" );
	for ( i = coll.begin(); i != coll.end(); ++i )
	{
		if ( m_categoryNames.find( (*i)->GetCategoryName() ) == m_categoryNames.end() )
		{
			m_categoryNames.insert( (*i)->GetCategoryName() );
		}
	}
}


void ComponentList::GetCategoryNames( StringSet & categoryNames )
{
	categoryNames = m_categoryNames;
}