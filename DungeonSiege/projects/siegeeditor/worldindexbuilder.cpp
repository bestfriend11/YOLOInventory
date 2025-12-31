#include "PrecompEditor.h"
#include "worldindexbuilder.h"
#include "fueldb.h"
#include "filesys.h"
#include "world.h"
#include "worldterrain.h"
#include "siege_engine.h"
#include "siege_database.h"
#include "editorterrain.h"
#include "editorregion.h"

using namespace std;
using namespace FileSys;
using namespace siege;


WorldIndexBuilder::WorldIndexBuilder()
{
}


bool WorldIndexBuilder::BuildMapNodeMeshIndex( FuelHandle const & hRegionDir )
{
	FuelHandle hIndex = hRegionDir->GetChildBlock( "index" );
	if ( !hIndex )
	{
		hIndex = hRegionDir->CreateChildDirBlock( "index" );
	}

	if ( hIndex )
	{
		FuelHandle hNodeMesh = hIndex->GetChildBlock( "node_mesh_index" );
		if ( !hNodeMesh )
		{
			hNodeMesh = hIndex->CreateChildBlock( "node_mesh_index", "node_mesh_index.gas" );
		}

		if ( hNodeMesh )
		{
			hIndex->DestroyChildBlock( hNodeMesh );
		}
		hNodeMesh = hIndex->CreateChildBlock( "node_mesh_index", "node_mesh_index.gas" );

		siege::SiegeEngine& engine = gSiegeEngine;
		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator i;
		for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{
			SiegeNode * pNode = (*i);
			siege::MeshDatabase::MeshFileMap::iterator iMesh = gSiegeEngine.MeshDatabase().FileNameMap().find( pNode->GetMeshGUID() );
			if ( iMesh != gSiegeEngine.MeshDatabase().FileNameMap().end() )
			{				
				hNodeMesh->Set( pNode->GetMeshGUID().ToString(), (*iMesh).second );
			}
		}	
	}

	return true;
}


bool WorldIndexBuilder::BuildRegionStreamerIndexes( FuelHandle const & hRegionDir, bool bNodeContent )
{
	// find/create the index block
	gpassert( hRegionDir->IsDirectory() );
	FuelHandle hIndexDirectory = hRegionDir->GetChildBlock( "index" );

	if( !hIndexDirectory.IsValid() )
	{
		hIndexDirectory = hRegionDir->CreateChildDirBlock( "index" );
	}

	bool bNode = BuildRegionStreamerNodeIndex( hRegionDir, hIndexDirectory );
	bool bContent = true;
	
	if ( bNodeContent )
	{
		bContent = BuildRegionStreamerNodeContentIndex( hRegionDir, hIndexDirectory );	
	}

	return bContent | bNode;
}


bool WorldIndexBuilder::BuildRegionStreamerNodeIndex( FuelHandle const & hRegionDir, FuelHandle & hIndexDirectory )
{
	FuelHandle hNodeIndex = hIndexDirectory->GetChildBlock( "streamer_node_index" );
	if ( hNodeIndex.IsValid() == false )
	{
		hNodeIndex = hIndexDirectory->CreateChildBlock( "streamer_node_index", "streamer_node_index.gas" );
	}

	// get handle for terrain nodes dir
	FuelHandle hRegionNodeDir = hRegionDir->GetChildBlock( "terrain_nodes" );
	gpassert( hRegionNodeDir->IsDirectory() );

	// empty the index block before rebuilding it
	hNodeIndex->ClearKeys();

	FuelHandleList NodeBlockList;
	hRegionNodeDir->ListChildBlocks( 2, "snode", NULL, NodeBlockList );
	
	int nodeId = 0;
	for( FuelHandleList::iterator i = NodeBlockList.begin(); i != NodeBlockList.end(); ++i ) 
	{
		if( (*i)->Get( "guid", nodeId ) ) 
		{
			hNodeIndex->Add( "*", nodeId, FVP_HEX_INT );
		}
		else 
		{
			gpassertm( 0, "Node definition invalid - no guid defined." );
			return false;
		}
	}

	gpgenericf(( "WorldIndexBuilder::BuildRegionStreamerNodeIndex - built streamer_node_index at path %s\n", hNodeIndex->GetAddress().c_str() ));
	return true;
}


bool WorldIndexBuilder::BuildRegionStreamerNodeContentIndex( FuelHandle const & hRegionDir, FuelHandle & hIndexDirectory )
{
	//----- create the content index

	FuelHandle hContentIndex = hIndexDirectory->GetChildBlock( "streamer_node_content_index" );
	if( !hContentIndex.IsValid() ) {
		hContentIndex = hIndexDirectory->CreateChildBlock( "streamer_node_content_index", "streamer_node_content_index.gas" );
	}

	//----- get region guid

	unsigned int RegionGUID;
	FuelHandle hRegionDef = hRegionDir->GetChildBlock( "region" );

	// extra guid check
	if( !hRegionDef->Get( "guid", RegionGUID, false ) ) {
		gpassertm( 0, "region doesn't have a guid!" );
		return false;
	}

	// empty the index block
	hContentIndex->ClearKeys();

	// now go to disk and get the region info and store in index

	FuelHandle hObjectDir = hRegionDir->GetChildBlock( "objects" );
	if ( hObjectDir.IsValid() == false )
	{
		hObjectDir = hRegionDir->CreateChildDirBlock( "objects" );
	}

	FuelHandleList ContentBlockList = hObjectDir->ListChildBlocks( 1 );	

	if( ContentBlockList.size() == 0 ) {
		gpwarningf(( "WorldIndexBuilder::BuildRegionStreamerContentIndex - region found to be empty of content.  Region gas path = %s\n", hRegionDir->GetAddress().c_str() ));
		return true;	// this can still be OK, but no point going on
	}

	unsigned int NodeGUID;
	gpstring sTemp;
	unsigned int ContentScid;

	for( FuelHandleList::iterator i = ContentBlockList.begin(); i != ContentBlockList.end(); ++i ) 
	{
		if ( !(*i) || (*i)->IsDirectory() )
		{
			continue;
		}

		FuelHandle hPos = (*i)->GetChildBlock( "placement" );
		if ( !hPos )
		{
			gperrorf( ( "WorldIndexBuilder: Object Scid %s does not have a placement block.  Please check your data.", (*i)->GetName() ) );
		}
		gpstring sPos;
		hPos->Get( "position", sPos );		
		stringtool::GetDelimitedValue( sPos.c_str(), ',', 3, NodeGUID );		
		stringtool::Setx( NodeGUID, sTemp );

		stringtool::Get( (*i)->GetName(), ContentScid );		

		hContentIndex->Add( sTemp.c_str(), ContentScid, FVP_HEX_INT );
	}

	gpgenericf(( "WorldIndexBuilder::BuildRegionStreamerNodeContentIndex - built streamer_node_content_index at path = %s\n", hContentIndex->GetAddress().c_str() ));
	return true;
}