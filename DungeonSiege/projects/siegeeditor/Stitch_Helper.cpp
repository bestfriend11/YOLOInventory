/**************************************************************************

Filename		: Stitch_Helper.cpp

Description		: This class makes a list of nodes and node ids that allows
				  level designers to stitch together regions easier

Creation Date	: 4/3/00

Author			: Chad Queen

**************************************************************************/



// Include Files
#include "PrecompEditor.h"
#include "Stitch_Helper.h"
#include "FuelDB.h"
#include "EditorRegion.h"
#include "WorldMap.h"

#include "siege_logical_mesh.h"
#include "siege_mesh_door.h"

// Used Namespaces
using namespace siege;


// Construct the intial stitch helper
void Stitch_Helper::Load()
{
	FuelHandle hEditor = gEditorRegion.GetRegionHandle()->GetChildBlock( "editor" );
	if ( hEditor.IsValid() )
	{
		FuelHandle hHelper = hEditor->GetChildBlock( "stitch_helper_data" );
		if ( hHelper.IsValid() ) 
		{
			FuelHandleList hlStitch = hEditor->ListChildBlocks( -1, "stitch_editor" );
			FuelHandleList::iterator i;
			for ( i = hlStitch.begin(); i != hlStitch.end(); ++i ) 
			{
				gpstring sRegion;
				(*i)->Get( "dest_region", sRegion );

				FuelHandle hNodes = (*i)->GetChildBlock( "node_ids" );
				if ( !hNodes.IsValid() ) 
				{
					continue;
				}
				
				if ( hNodes->FindFirstKeyAndValue() ) 
				{
					gpstring sKey;
					gpstring sValue;
					while ( hNodes->GetNextKeyAndValue( sKey, sValue ) ) 
					{
						int nodeID		= 0;
						stringtool::Get( sKey, nodeID );
						gpstring sNode;
						int doorID		= 0;
						stringtool::GetDelimitedValue( sValue, ',', 0, sNode );
						stringtool::GetDelimitedValue( sValue, ',', 1, doorID );

						StitchUnit su;
						su.doorID = doorID;
						su.nodeID = nodeID;
						database_guid nodeGUID( sNode.c_str() );
						su.nodeGUID = nodeGUID;
						su.sRegion = sRegion;

						SiegeNode * pNode = gSiegeEngine.IsNodeValid( nodeGUID );
						if ( !pNode )
						{
							continue;
						}
						
						m_stitch_vector.push_back( su );
					}
				}
			}
		}
	}

	StringVec regions;
	StringVec::iterator j;
	gEditorRegion.GetRegionList( gEditorRegion.GetMapName().c_str(), regions );
	for ( j = regions.begin(); j != regions.end(); ++j ) 
	{	
		/*
		if ( (*j).same_no_case( gEditorRegion.GetRegionName() ) ) 
		{
			continue;
		}
		*/

		gpstring sTemp;
		sTemp = gEditorRegion.GetMapHandle()->GetAddress();
		sTemp += ":regions:";
		sTemp += (*j).c_str();
		sTemp += ":editor:stitch_helper_data";
		FuelHandle hData( sTemp.c_str() );

		if ( hData.IsValid() ) 
		{
			FuelHandleList hlStitch = hData->ListChildBlocks( -1, "stitch_editor" );
			FuelHandleList::iterator i;
			StitchVector mapStitches;
			for ( i = hlStitch.begin(); i != hlStitch.end(); ++i ) 
			{
				gpstring sRegion;
				(*i)->Get( "dest_region", sRegion );

				FuelHandle hNodes = (*i)->GetChildBlock( "node_ids" );
				if ( !hNodes.IsValid() ) 
				{
					continue;
				}
				
				if ( hNodes->FindFirstKeyAndValue() ) 
				{
					gpstring sKey;
					gpstring sValue;					
					while ( hNodes->GetNextKeyAndValue( sKey, sValue ) ) 
					{
						int nodeID = 0;
						stringtool::Get( sKey, nodeID );						
						gpstring sNode;
						int doorID		= 0;
						stringtool::GetDelimitedValue( sValue, ',', 0, sNode );
						stringtool::GetDelimitedValue( sValue, ',', 1, doorID );

						StitchUnit su;
						su.doorID = doorID;
						su.nodeID = nodeID;
						database_guid nodeGUID( sNode.c_str() );
						su.nodeGUID = nodeGUID;
						su.sRegion = sRegion;
						
						mapStitches.push_back( su );
					}					
				}
			}

			m_MapStitchMap.insert( std::make_pair( (*j), mapStitches ) );
		}
	}

}



// Save out the current stitch helper
void Stitch_Helper::Save()
{
	FuelHandle hEditor = gEditorRegion.GetRegionHandle()->GetChildBlock( "editor" );
	if ( !hEditor.IsValid() ) {
		hEditor = gEditorRegion.GetRegionHandle()->CreateChildDirBlock( "editor" );
	}

	FuelHandle hHelper = hEditor->GetChildBlock( "stitch_helper_data" );
	if ( hHelper.IsValid() == false ) {
		hHelper = hEditor->CreateChildBlock( "stitch_helper_data", "stitch_helper.gas" );
	}
	
	FuelHandle hMain = gEditorRegion.GetRegionHandle()->GetChildBlock( "region" );
	if ( hMain.IsValid() ) 
	{
		gpstring sGUID;
		gpstring sName;
		hMain->Get( "guid", sGUID );
		sName = hMain->GetParent()->GetName();
		hHelper->Set( "source_region_guid", sGUID );
		hHelper->Set( "source_region_name", sName );
	}

	FuelHandleList hlStitch = hHelper->ListChildBlocks( -1, "stitch_editor" );
	{
		FuelHandleList::iterator i;
		for ( i = hlStitch.begin(); i != hlStitch.end(); ++i ) 
		{
			hHelper->DestroyChildBlock( *i );
		}
	}	

	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) 
	{
		if ( gSiegeEngine.IsNodeValid( (*i).nodeGUID ) )
		{
			FuelHandle hRegion = hHelper->GetChildBlock( (*i).sRegion.c_str(), true );
			hRegion->SetType( "stitch_editor" );
			hRegion->Set( "dest_region", (*i).sRegion );		
			FuelHandle hNodes = hRegion->GetChildBlock( "node_ids", true );
			char szNodeDoor[256] = "";
			sprintf( szNodeDoor, "0x%08X,%d", (*i).nodeGUID.GetValue(), (*i).doorID );		
			char szKey[256] = "";
			sprintf( szKey, "0x%08X", (*i).nodeID );
			hNodes->Set( szKey, (gpstring)szNodeDoor );
		}
	}
}



// Remove the node from the stitch index
void Stitch_Helper::RemoveNodeIDFromStitchHelper( int nodeID )
{
	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) {
		if ( (*i).nodeID == nodeID ) {
			m_stitch_vector.erase( i );
			return;
		}
	}
}


// Add the node to the helper stitch
void Stitch_Helper::AddNodeToStitchHelper( database_guid nodeGUID, int nodeID, int doorID, gpstring sRegion )
{
	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) 
	{
		if ( (*i).nodeID == nodeID ) 
		{
			gpstring sMessage;
			sMessage.assignf( "Cannot complete stitch, the stitch node ID (0x%8x) already exists.", nodeID );
			MessageBoxEx( NULL, sMessage, "Stitch Tool", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}
	}

	StitchUnit su;
	su.doorID	= doorID;
	su.nodeID	= nodeID;
	su.sRegion	= sRegion;	
	su.nodeGUID = nodeGUID;
	m_stitch_vector.push_back( su );
}



// Find node in the stich helper
bool Stitch_Helper::FindNodeDoorInStitchHelper( database_guid nodeGUID, int doorID, int & nodeID, gpstring & sRegion )
{
	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) {
		if ( ((*i).nodeGUID == nodeGUID) && ( ((*i).doorID == doorID) )) {
			nodeID = (*i).nodeID;
			sRegion = (*i).sRegion;
			return true;
		}
	}
	return false;
}


// Set a node's stitch helper information; if it doesn't exist, add it to our database
void Stitch_Helper::SetNodeDoorStitchHelperInfo( siege::database_guid nodeGUID, int nodeID, int doorID, gpstring sRegion )
{
	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) {
		if ( ((*i).nodeGUID == nodeGUID) && ((*i).doorID == doorID) ) {
			(*i).nodeID = nodeID;
			(*i).sRegion = sRegion;
			return;
		}			
	}
	AddNodeToStitchHelper( nodeGUID, nodeID, doorID, sRegion );	
}



// Build a stitch index from a specified map
void Stitch_Helper::BuildStitchIndex( gpstring sMap ) 
{	
	// Clear out any existing nodes
	siege::MeshDatabase::MeshFileMap meshMapCopy = gSiegeEngine.MeshDatabase().FileNameMap();
	gSiegeEngine.MeshDatabase().FileNameMap().clear();

	FuelHandle hMaps( "world:maps" );
	FuelHandleList hlMaps = hMaps->ListChildBlocks( 2, "map" );
	FuelHandleList::iterator i;
	for ( i = hlMaps.begin(); i != hlMaps.end(); ++i ) {
		gpstring sName;
		sName = (*i)->GetParent()->GetName();
		if ( sName.same_no_case( sMap ) ) 
		{			
			bool bNodeMesh = false;
			(*i)->Get( "use_node_mesh_index", bNodeMesh );
			if ( bNodeMesh )
			{
				FuelHandle hRegions = (*i)->GetParent()->GetChildBlock( "regions" );
				FuelHandleList hlRegions = hRegions->ListChildBlocks( 1 );
				FuelHandleList::iterator iRegion;		
				for ( iRegion = hlRegions.begin(); iRegion != hlRegions.end(); ++iRegion )
				{
					gpstring sNodeMesh = (*iRegion)->GetAddress();
					sNodeMesh.appendf( ":index:node_mesh_index" );
					FuelHandle hNodeMesh( sNodeMesh );
					if ( hNodeMesh )
					{			
						if ( hNodeMesh->FindFirstKeyAndValue() )
						{
							gpstring sGuid, sFile;
							while ( hNodeMesh->GetNextKeyAndValue( sGuid, sFile ) )
							{									
								std::pair <siege::MeshDatabase::MeshFileMap::iterator, bool> iFind = gSiegeEngine.MeshDatabase().FileNameMap().insert( std::make_pair( siege::database_guid( sGuid ), sFile ) );								
								if ( !iFind.second && !(*iFind.first).second.same_no_case( sFile )  )
								{
									gperrorf( ( "Serious Error -  Two sno files are mapped to the same guid.  Make sure your node_mesh_index.gas files are up to date.  You can load up a region in SE with 'Remap all region mesh indicies' checked to automatically fix this issue.\nRegion: %s, Mesh Guid: %s, SNO File: %s", (*iRegion)->GetName(), sGuid.c_str(), sFile.c_str() ) );									
								}
							}					
						}
					}
				}
			}			
			else
			{
				// Initialize Global Siege node database.
				FastFuelHandle siegeNodes( "world:global:siege_nodes" );
				FastFuelHandleColl siegeNodeList;

				if ( siegeNodes && siegeNodes.ListChildrenNamed( siegeNodeList, "mesh_file*", -1 ) )
				{
					gpstring sGuid, sFile;

					FastFuelHandleColl::iterator iItem, begin = siegeNodeList.begin(), end = siegeNodeList.end();
					for ( iItem = begin ; iItem != end ; ++iItem )
					{
						if ( (*iItem).Get( "guid", sGuid, false )
							&& ((*iItem).Get( "filename", sFile ) || (*iItem).Get( "genericname", sFile, false )) )
						{
							gSiegeEngine.MeshDatabase().FileNameMap().insert( std::make_pair( siege::database_guid( sGuid ), sFile ) );															
						}
					}
				}
			}

			// Create the stitch index data, if it doesn't already exist
			FuelHandle hIndex = (*i)->GetParentBlock()->GetChildBlock( "index" );
			if ( hIndex.IsValid() == false ) {
				hIndex = (*i)->GetParentBlock()->CreateChildDirBlock( "index" );
			}
			FuelHandle hStitchIndex = hIndex->GetChildBlock( "stitch_index" );
			if ( hStitchIndex.IsValid() == false ) {
				hStitchIndex = hIndex->CreateChildBlock( "stitch_index", "stitch_index.gas" );
			}
			hStitchIndex->DestroyAllChildBlocks( false );

			// Now go through each of the region's stitch helpers and zip them together
			FuelHandleList hlStitchHelpers;
			FuelHandle hRegions( (*i)->GetParent()->GetChildBlock( "regions" ) );
			if( hRegions )
			{
				FuelHandleList regionColl;
				hRegions->ListChildBlocks( 2, "region", NULL, regionColl );

				FuelHandle hsRegion, hdRegion;
				for( FuelHandleList::iterator r = regionColl.begin(); r != regionColl.end(); ++r )
				{
					FuelHandle regParent = (*r)->GetParent();
					if( regParent->IsDirectory() )
					{
						FuelHandle hEditor	= regParent->GetChildBlock( "editor" );
						if( hEditor.IsValid() )
						{
							FuelHandle hStitchHelper = hEditor->GetChildBlock( "stitch_helper_data" );
							if( hStitchHelper.IsValid() )
							{
								hlStitchHelpers.push_back( hStitchHelper );
							}
						}
					}
				}
			}

			FuelHandleList::iterator j;
			for ( j = hlStitchHelpers.begin(); j != hlStitchHelpers.end(); ++j ) {
				
				// Create the child block for the region in the stitch index file
				gpstring sGUID;
				(*j)->Get( "source_region_guid", sGUID );
				gpstring sName;
				(*j)->Get( "source_region_name", sName );

				FuelHandle hStitchRegion = hStitchIndex->CreateChildBlock( sGUID.c_str() );
				hStitchRegion->SetType( "stitch_index" );

				// Now parse through the available nodes to stitch
				FuelHandleList hlStitchEditor = (*j)->ListChildBlocks( 1, "stitch_editor" );
				FuelHandleList::iterator k;
				for ( k = hlStitchEditor.begin(); k != hlStitchEditor.end(); ++k ) {
					gpstring sDestRegion;
					(*k)->Get( "dest_region", sDestRegion );

					FuelHandle hNodes = (*k)->GetChildBlock( "node_ids" );
					if ( hNodes->FindFirstKeyAndValue() ) {
						gpstring sKey;
						gpstring sValue;
						while ( hNodes->GetNextKeyAndValue( sKey, sValue ) ) {
							int nodeID = 0;
							stringtool::Get( sKey, nodeID );							
							gpstring sNode;
							gpstring sDoorID;
							stringtool::GetDelimitedValue( sValue, ',', 0, sNode );
							stringtool::GetDelimitedValue( sValue, ',', 1, sDoorID );

							gpstring sDestNode;
							gpstring sDestDoorID;
							if ( FindMatchingNodeIDInRegion( nodeID, sName.c_str(), sDestRegion.c_str(), hlStitchHelpers, sDestNode, sDestDoorID ) ) {
								gpstring sNewValue = sNode + "," + sDoorID + "," + sDestNode + "," + sDestDoorID;
								hStitchRegion->Set( "*", sNewValue );
								FuelHandle hMap( (*i)->GetParent() );
								CalculateStitchBoundary( hMap, hStitchIndex, sName, sNode, sDoorID, sDestRegion, sDestNode, sDestDoorID );
							}
							else {
								gperrorf( ( "No matching node id found in: %s for region: %s  ( node id = %d )", sDestRegion.c_str(), sName.c_str(), nodeID));
								continue;
							}
						}
					}
				}
			}			
		}
	}
	hMaps->GetDB()->SaveChanges();

	gSiegeEngine.MeshDatabase().FileNameMap() = meshMapCopy;	
}



bool Stitch_Helper::FindMatchingNodeIDInRegion( int nodeID, gpstring sSourceRegion, gpstring sDestRegion, FuelHandleList hlStitchRegions, gpstring & sNode, gpstring & sDoorID )
{
	FuelHandleList::iterator j;
	for ( j = hlStitchRegions.begin(); j != hlStitchRegions.end(); ++j ) {
		gpstring sName;
		(*j)->Get( "source_region_name", sName );
		if ( sName.same_no_case( sDestRegion ) ) {
			FuelHandleList hlStitchEditor = (*j)->ListChildBlocks( 1, "stitch_editor" );
			FuelHandleList::iterator k;
			for ( k = hlStitchEditor.begin(); k != hlStitchEditor.end(); ++k ) {
				gpstring sDestRegion2;
				(*k)->Get( "dest_region", sDestRegion2 );
				if ( sDestRegion2.same_no_case( sSourceRegion ) ) {
					FuelHandle hNodes = (*k)->GetChildBlock( "node_ids" );
					if ( hNodes->FindFirstKeyAndValue() ) {
						gpstring sKey;
						gpstring sValue;
						while ( hNodes->GetNextKeyAndValue( sKey, sValue ) ) {
							int dest_nodeID = 0;
							stringtool::Get( sKey, dest_nodeID );							
							if ( dest_nodeID == nodeID ) {								
								stringtool::GetDelimitedValue( sValue, ',', 0, sNode );
								stringtool::GetDelimitedValue( sValue, ',', 1, sDoorID );
								return true;
							}
						}
					}
				}
			}
		}
	}	
	return false;
}



// Delete a node's references in the stitch helper
void Stitch_Helper::DeleteNodeFromStitchIndex( siege::database_guid nodeGUID )
{
	StitchVector::iterator i;
	for ( i = m_stitch_vector.begin(); i != m_stitch_vector.end(); ++i ) {
		if ( (*i).nodeGUID == nodeGUID ) {
			i = m_stitch_vector.erase( i );
			return;
		}
	}
}


void Stitch_Helper::GetMapStitchVector( gpstring sRegionName, StitchVector & region_stitches )
{
	MapStitchMap::iterator iMap = m_MapStitchMap.find( sRegionName );
	if ( iMap != m_MapStitchMap.end() )
	{
		StitchVector::iterator i;
		for ( i = (*iMap).second.begin(); i != (*iMap).second.end(); ++i ) 
		{
			region_stitches.push_back( *i );			
		}
	}
}

// Connection structure for logical construction
struct LeafConnectInfo
{
	vector_3		wMinBox;
	vector_3		wMaxBox;
	unsigned short	id;
};

void Stitch_Helper::CalculateStitchBoundary( FuelHandle& hMap, FuelHandle& hStitchIndex,
											 gpstring& sRegionName, gpstring& sNode, gpstring& sDoor,
											 gpstring& dRegionName, gpstring& dNode, gpstring& dDoor )
{
	// Reformat node guid strings to full 8 digit hexadecimal representation
	unsigned int node;
	stringtool::Get( sNode.c_str(), node );
	sNode.assignf( "0x%08x", node );

	stringtool::Get( dNode.c_str(), node );
	dNode.assignf( "0x%08x", node );

	// See if we already have a connection set for this node
	FuelHandle hConnectInfo;
	FuelHandle hNodeInfo = hStitchIndex->GetChildBlock( sNode.c_str() );
	if( hNodeInfo.IsValid() )
	{
		FuelHandle hNeighborInfo = hNodeInfo->GetChildBlock( dNode.c_str() );
		if( hNeighborInfo.IsValid() )
		{
			return;
		}
		else
		{
			hConnectInfo = hNodeInfo->CreateChildBlock( dNode.c_str() );
			hConnectInfo->SetType( "neighbor_connections" );
		}
	}
	else
	{
		// Create new block
		hNodeInfo = hStitchIndex->CreateChildBlock( sNode.c_str() );
		hNodeInfo->SetType( "node_connect_info" );

		hConnectInfo = hNodeInfo->CreateChildBlock( dNode.c_str() );
		hConnectInfo->SetType( "neighbor_connections" );
	}

	// Find both region blocks
	FuelHandle hRegions( hMap->GetChildBlock( "regions" ) );
	if( hRegions )
	{
		FuelHandleList regionColl;
		hRegions->ListChildBlocks( 2, "region", NULL, regionColl );

		FuelHandle hsRegion, hdRegion;
		for( FuelHandleList::iterator i = regionColl.begin(); i != regionColl.end(); ++i )
		{
			FuelHandle regParent = (*i)->GetParent();
			if( regParent->IsDirectory() )
			{
				if( sRegionName.same_no_case( regParent->GetName() ) )
				{
					hsRegion	= regParent->GetChildBlock( "terrain_nodes" )->GetChildBlock( "siege_node_list" );
				}
				if( dRegionName.same_no_case( regParent->GetName() ) )
				{
					hdRegion	= regParent->GetChildBlock( "terrain_nodes" )->GetChildBlock( "siege_node_list" );
				}
			}
		}

		// Find node blocks
		FuelHandle hsNode	= hsRegion->GetChildBlock( sNode.c_str() );
		FuelHandle hdNode	= hdRegion->GetChildBlock( dNode.c_str() );

		if ( !hsNode )
		{
			gperrorf( ( "VERY BAD: Stitch Helper: Failed to construct fuel handle to source node: %s, region: %s", sNode.c_str(), sRegionName.c_str() ) );
			return;
		}
		if ( !hdNode )
		{
			gperrorf( ( "VERY BAD: Stitch Helper: Failed to construct fuel handle to destination node: %s, region: %s", dNode.c_str(), dRegionName.c_str() ) );
			return;
		}

		// Now we should be able to get the mesh guids we will need to perform the mesh calculation
		siege::database_guid sMeshGuid( hsNode->GetInt( "mesh_guid" ) );
		siege::database_guid dMeshGuid( hdNode->GetInt( "mesh_guid" ) );

		// Load the meshes
		siege::SiegeMeshHandle sHandle( gSiegeEngine.MeshCache().UseObject( sMeshGuid ) );
		siege::SiegeMesh& sMesh	= sHandle.RequestObject( gSiegeEngine.MeshCache() );

		siege::SiegeMeshHandle dHandle( gSiegeEngine.MeshCache().UseObject( dMeshGuid ) );
		siege::SiegeMesh& dMesh	= dHandle.RequestObject( gSiegeEngine.MeshCache() );

		// With the meshes in hand we should now be able to look up the doors we are connecting through
		siege::SiegeMeshDoor* near_door	= sMesh.GetDoorByID( atoi( sDoor.c_str() ) );
		siege::SiegeMeshDoor* far_door	= dMesh.GetDoorByID( atoi( dDoor.c_str() ) );

		matrix_3x3 inv_far_orient		= far_door->GetOrientation().Transpose_T();
		matrix_3x3 orient				= near_door->GetOrientation();

		orient.SetColumn_0( -orient.GetColumn_0() );
		orient.SetColumn_2( -orient.GetColumn_2() );

		// Calculate the orient/offset pair that identifies the transformation from dest node to src node space.
		matrix_3x3 door_orient			= orient * inv_far_orient;
		vector_3 door_offset			= near_door->GetCenter() + (door_orient * -far_door->GetCenter());

		// Setup expansion vector
		vector_3 mod( 0.05f, 0.05f, 0.05f );

		// Go through all of the LogicalNodes for the near node
		for( unsigned int l = 0; l < sMesh.GetNumLogicalMeshes(); ++l )
		{
			SiegeLogicalMesh* pNearLogicalMesh	= &sMesh.GetLogicalMeshes()[ l ];

			// Get the world space bounding volume for this logical node
			vector_3 minNearWorld = pNearLogicalMesh->GetMinimumBounds();
			vector_3 maxNearWorld = pNearLogicalMesh->GetMaximumBounds();

			// Go through all of the LogicalNodes for the far node
			for ( unsigned int o = 0; o < dMesh.GetNumLogicalMeshes(); ++o )
			{
				SiegeLogicalMesh* pFarLogicalMesh	= &dMesh.GetLogicalMeshes()[ o ];

				// Get the world space bounding volume for this logical node
				vector_3 minFarWorld( DoNotInitialize );
				vector_3 maxFarWorld( DoNotInitialize );
				TransformAxisAlignedBox( door_orient, door_offset,
										 pFarLogicalMesh->GetMinimumBounds(), pFarLogicalMesh->GetMaximumBounds(),
										 minFarWorld, maxFarWorld );

				// See if they intersect
				if ( BoxIntersectsBox( (minNearWorld-mod), (maxNearWorld+mod), (minFarWorld-mod), (maxFarWorld+mod) ) )
				{
					// Connect the leaves of these two logical nodes together
					LMESHLEAFINFO* pNearLeaves		= pNearLogicalMesh->GetLeafConnectionInfo();
					LMESHLEAFINFO* pFarLeaves		= pFarLogicalMesh->GetLeafConnectionInfo();
					unsigned int numNearLeaves		= pNearLogicalMesh->GetNumLeafConnections();
					unsigned int numFarLeaves		= pFarLogicalMesh->GetNumLeafConnections();

					std::vector< NODALLEAFCONNECT >		leafConnectionColl;
					std::vector< LeafConnectInfo >	farConnectionColl;
					farConnectionColl.reserve( numFarLeaves );

					// Find the connections
					for ( unsigned int o = 0; o < numFarLeaves; ++o )
					{
						// Get the far leaf
						LMESHLEAFINFO* pFarLeaf	= &pFarLeaves[ o ];

						// Setup a world space bounding volume
						LeafConnectInfo lci;

						TransformAxisAlignedBox( door_orient, door_offset,
												 pFarLeaf->minBox, pFarLeaf->maxBox,
												 lci.wMinBox, lci.wMaxBox );

						lci.wMinBox		-= mod;
						lci.wMaxBox		+= mod;
						lci.id			= pFarLeaf->id;

						farConnectionColl.push_back( lci );
					}

					for ( unsigned int i = 0; i < numNearLeaves; ++i )
					{
						// Get the near leaf
						LMESHLEAFINFO* pNearLeaf	= &pNearLeaves[ i ];

						// Setup a world space bounding volume
						vector_3 nearminBox			= pNearLeaf->minBox;
						vector_3 nearmaxBox			= pNearLeaf->maxBox;
						nearminBox					-= mod;
						nearmaxBox					+= mod;

						// Find the connections
						for ( std::vector< LeafConnectInfo >::iterator f = farConnectionColl.begin(); f != farConnectionColl.end(); ++f )
						{
							if ( BoxIntersectsBox( nearminBox, nearmaxBox, (*f).wMinBox, (*f).wMaxBox ) )
							{
								gpstring sNewValue;
								sNewValue.assignf( "%d,%d - %d,%d",
												   (int)pNearLogicalMesh->GetID(), pNearLeaf->id,
												   (int)pFarLogicalMesh->GetID(), (*f).id );
								hConnectInfo->Set( "*", sNewValue );
							}
						}
					}
				}
			}
		}
	}
}