/******************************************************************************************
**
**								SiegeTransition
**
**		See .h file for details
**
******************************************************************************************/

// Precompiled header
#include "precomp_siege.h"

// Core includes
#include "gpcore.h"
#include "fubipersist.h"
#include "fubitraits.h"
#include "fubibitpacker.h"

// Siege includes
#include "siege_engine.h"
#include "siege_hotpoint_database.h"
#include "siege_persist.h"
#include "siege_transition.h"

// Special includes
#include "tattooversion.h"


using namespace siege;

FUBI_EXPORT_ENUM( eAxisHint, AH_NONE, AH_COUNT );

FUBI_DECLARE_ENUM_TRAITS( eAxisHint, AH_NONE );

FUBI_DECLARE_XFER_TRAITS( SiegeTransition::NodeTransition )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_targetGuid",		obj.m_targetGuid );
		persist.Xfer( "m_targetDoor",		obj.m_targetDoor );
		persist.Xfer( "m_oldTargetDoor",	obj.m_oldTargetDoor );
		persist.Xfer( "m_connectGuid",		obj.m_connectGuid );
		persist.Xfer( "m_connectDoor",		obj.m_connectDoor );
		persist.Xfer( "m_oldConnectGuid",	obj.m_oldConnectGuid );
		persist.Xfer( "m_oldConnectDoor",	obj.m_oldConnectDoor );

		if ( ::IsDs1VersionRtm( persist.GetVersion() ) )
		{
			// $ 1.0 saved these as floats. they were converted to doubles for
			//   the 1.09b and above, and this code compensates.

			float f;
			
			f = (float)obj.m_currentTime;
			persist.Xfer( "m_currentTime",		obj.m_currentTime );
			obj.m_currentTime = f;

			f = (float)obj.m_startTime;
			persist.Xfer( "m_startTime",		obj.m_startTime );
			obj.m_startTime = f;

			f = (float)obj.m_endTime;
			persist.Xfer( "m_endTime",			obj.m_endTime );
			obj.m_endTime = f;
		}
		else
		{
			persist.Xfer( "m_currentTime",		obj.m_currentTime );
			persist.Xfer( "m_startTime",		obj.m_startTime );
			persist.Xfer( "m_endTime",			obj.m_endTime );
		}

		persist.Xfer( "m_percentComplete",	obj.m_percentComplete );
		persist.Xfer( "m_axisHint",			obj.m_axisHint );
		persist.Xfer( "m_owner",			obj.m_owner );
		persist.Xfer( "m_bConnect",			obj.m_bConnect );

		return true;
	}
};

FUBI_DECLARE_XFER_TRAITS( SiegeTransition::NodeDoorInfo )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.Xfer( "m_nearDoor",	obj.m_nearDoor );
		persist.Xfer( "m_farNode",	obj.m_farNode );
		persist.Xfer( "m_farDoor",	obj.m_farDoor );
		persist.Xfer( "m_bConnect",	obj.m_bConnect );

		return true;
	}
};

FUBI_DECLARE_XFER_TRAITS( SiegeTransition::NodeInfo )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		persist.XferVector( "DoorCollection", obj.m_nodeDoorColl );
		persist.XferVector( "DisconnectCollection", obj.m_nodeDisconnectColl );

		return true;
	}
};


// Construction
SiegeTransition::SiegeTransition()
{

}

// Destruction
SiegeTransition::~SiegeTransition()
{
	Clear();
}

// Persistence
bool SiegeTransition::Xfer( FuBi::PersistContext& persist )
{
	// Persist active transitions
	persist.XferMap( "m_TransitionMap", m_TransitionMap );

	// Persist door mapping
	persist.XferMap( "m_NodeInfoMap", m_NodeInfoMap );

	if( persist.IsRestoring() )
	{
		for( TransitionMap::iterator i = m_TransitionMap.begin(); i != m_TransitionMap.end(); ++i )
		{
			// Setup node replacements in all frustums
			gSiegeEngine.AddFrustumNodeReplacement( (*i).second.m_targetGuid, (*i).second.m_connectGuid );
		}
	}

	return true;
}

// Xfer for synchronization with server
void SiegeTransition::XferForSync( FuBi::BitPacker& packer )
{
	// Xfer size of the transition mapping
	unsigned int guid = 0;
	unsigned int size = m_TransitionMap.size();
	packer.XferCount( size );

	if( packer.IsSaving() )
	{
		// Store information
		for( TransitionMap::iterator i = m_TransitionMap.begin(); i != m_TransitionMap.end(); ++i )
		{
			guid	= (*i).first.GetValue();
			packer.XferRaw( guid );
			packer.XferBytes( &(*i).second, sizeof( NodeTransition ) );
		}
	}
	else
	{
		// Parse information
		database_guid node;
		NodeTransition transition;
		for( unsigned int i = 0; i < size; ++i )
		{
			packer.XferRaw( guid );
			node.SetValue( guid );

			packer.XferBytes( &transition, sizeof( NodeTransition ) );
			m_TransitionMap.insert( std::pair< database_guid, NodeTransition >( node, transition ) );
		}
	}

	// Xfer size of the node info mapping
	size = m_NodeInfoMap.size();
	packer.XferCount( size );

	if( packer.IsSaving() )
	{
		// Store information
		for( NodeInfoMap::iterator i = m_NodeInfoMap.begin(); i != m_NodeInfoMap.end(); ++i )
		{
			// Xfer guid
			guid	= (*i).first.GetValue();
			packer.XferRaw( guid );

			// Xfer node door info
			size	= (*i).second.m_nodeDoorColl.size();
			packer.XferCount( size );
			packer.XferBytes( &*(*i).second.m_nodeDoorColl.begin(), sizeof( NodeDoorInfo ) * size );

			// Xfer node disconnect info
			size	= (*i).second.m_nodeDisconnectColl.size();
			packer.XferCount( size );
			packer.XferBytes( &*(*i).second.m_nodeDisconnectColl.begin(), sizeof( database_guid ) * size );
		}
	}
	else
	{
		// Parse information
		database_guid node;
		for( unsigned int i = 0; i < size; ++i )
		{
			// Xfer guid
			packer.XferRaw( guid );
			node.SetValue( guid );

			unsigned int collsize;
			NodeInfo nodeInfo;

			// Xfer node door info
			packer.XferCount( collsize );
			nodeInfo.m_nodeDoorColl.resize( collsize );
			packer.XferBytes( &*nodeInfo.m_nodeDoorColl.begin(), sizeof( NodeDoorInfo ) * collsize );

			// Xfer node disconnect info
			packer.XferCount( collsize );
			nodeInfo.m_nodeDisconnectColl.resize( collsize );
			packer.XferBytes( &*nodeInfo.m_nodeDisconnectColl.begin(), sizeof( database_guid ) * collsize );

			m_NodeInfoMap.insert( std::pair< database_guid, NodeInfo >( node, nodeInfo ) );
		}
	}

	if( packer.IsRestoring() )
	{
		for( TransitionMap::iterator i = m_TransitionMap.begin(); i != m_TransitionMap.end(); ++i )
		{
			// Setup node replacements in all frustums
			gSiegeEngine.AddFrustumNodeReplacement( (*i).second.m_targetGuid, (*i).second.m_connectGuid );
		}
	}
}

// Clear
void SiegeTransition::Clear()
{
	m_TransitionMap.clear();
	m_NodeInfoMap.clear();
}

// Update all active transitions
void SiegeTransition::Update( float secondsElapsed )
{
	gpassert( m_CompleteTransitions.empty() );

	// Go through all active transitions and update their time
	for( TransitionMap::iterator i = m_TransitionMap.begin(); i != m_TransitionMap.end(); )
	{
		NodeTransition& nt	= (*i).second;

		// Add timeslice to current time
		nt.m_currentTime	= PreciseAdd( nt.m_currentTime, secondsElapsed );

		if( nt.m_currentTime > nt.m_startTime )
		{
			if( IsZero( (float) PreciseSubtract(nt.m_endTime, nt.m_startTime) ) )
			{
				nt.m_percentComplete	= 1.0f;
			}
			else
			{
				nt.m_percentComplete	= min_t( 1.0f, (float) PreciseDivide( PreciseSubtract(nt.m_currentTime, nt.m_startTime), PreciseSubtract(nt.m_endTime, nt.m_startTime)) );
			}
		}

		// See if the transition is complete
		if( IsOne( nt.m_percentComplete ) )
		{
			// Complete the transition
			CompleteTransition( nt, false );

			// Save the owner before we delete the transition
			m_CompleteTransitions.push_back( nt );

			// Erase the transition
			i = m_TransitionMap.erase( i );
		}
		else
		{
			++i;
		}
	}

	// Go through complete transitions and notify.  This must be done outside of the main update loop to prevent
	// transition recursion.
	for( TransitionColl::iterator t = m_CompleteTransitions.begin(); t != m_CompleteTransitions.end(); ++t )
	{
		// Notify of completion
		gSiegeEngine.TransitionComplete( (*t).m_targetGuid, (*t).m_owner );
	}

	// Clear complete transition collection
	m_CompleteTransitions.clear();
}

// Start a transition
bool SiegeTransition::StartTransition( const database_guid targetGuid, const DWORD targetDoor,
									   const database_guid connectGuid, const DWORD connectDoor,
									   const double currentTime, const double startTime, const double endTime,
									   const eAxisHint ahint, const DWORD owner, bool bConnect, bool bForceComplete )
{
	// Check to see if we should force complete any existing transitions
	if( bForceComplete )
	{
		// Look for target transition
		TransitionMap::iterator t = m_TransitionMap.find( targetGuid );
		if( t != m_TransitionMap.end() )
		{
			// Complete this transition
			CompleteTransition( (*t).second, true );
		}

		// Look for connect transition
		t = m_TransitionMap.find( connectGuid );
		if( t != m_TransitionMap.end() )
		{
			// Complete this transition
			CompleteTransition( (*t).second, true );
		}
	}

	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetTransitionCritical() );

	// Setup node replacements in all frustums
	gSiegeEngine.AddFrustumNodeReplacement( targetGuid, connectGuid );

	database_guid oldConnectGuid	= UNDEFINED_GUID;
	DWORD oldConnectDoor			= 0;
	DWORD oldTargetDoor				= 0;

	// Persist the doors for the target
	PersistDoors( targetGuid );

	NodeInfoMap::iterator i = m_NodeInfoMap.find( targetGuid );
	gpassert( i != m_NodeInfoMap.end() );

	// Erase all existing door connections
	for( NodeDoorColl::iterator d = (*i).second.m_nodeDoorColl.begin(); d != (*i).second.m_nodeDoorColl.end(); ++d )
	{
		// Look up the neighbor
		PersistDoors( (*d).m_farNode );
		NodeInfoMap::iterator n = m_NodeInfoMap.find( (*d).m_farNode );
		gpassert( n != m_NodeInfoMap.end() );

		// Erase this door from the far node
		for( NodeDoorColl::iterator nd = (*n).second.m_nodeDoorColl.begin(); nd != (*n).second.m_nodeDoorColl.end(); ++nd )
		{
			if( (*nd).m_nearDoor == (*d).m_farDoor )
			{
				(*n).second.m_nodeDoorColl.erase( nd );
				break;
			}
		}

		if( (*d).m_farNode == connectGuid )
		{
			oldConnectGuid	= connectGuid;
			oldConnectDoor	= (*d).m_farDoor;
			oldTargetDoor	= (*d).m_nearDoor;
		}

		// Rebuild the doors for this node
		RebuildDoors( (*d).m_farNode );

		// Register a disconnect
		RegisterNodeDisconnect( targetGuid, (*d).m_farNode );
	}

	if( !oldConnectGuid.IsValid() )
	{
		oldConnectGuid	= (*i).second.m_nodeDoorColl.front().m_farNode;
		oldConnectDoor	= (*i).second.m_nodeDoorColl.front().m_farDoor;
		oldTargetDoor	= (*i).second.m_nodeDoorColl.front().m_nearDoor;
	}

	// Clear door listing
	(*i).second.m_nodeDoorColl.clear();

	// Add new door
	NodeDoorInfo doorInfo;
	doorInfo.m_nearDoor	= targetDoor;
	doorInfo.m_farNode	= connectGuid;
	doorInfo.m_farDoor	= connectDoor;
	doorInfo.m_bConnect	= false;

	(*i).second.m_nodeDoorColl.push_back( doorInfo );

	// Rebuild doors for the target
	RebuildDoors( targetGuid );

	// Persist the doors for the connect
	PersistDoors( connectGuid );

	i = m_NodeInfoMap.find( connectGuid );
	gpassert( i != m_NodeInfoMap.end() );

	// Look for the door that we are connection through
	for( d = (*i).second.m_nodeDoorColl.begin(); d != (*i).second.m_nodeDoorColl.end(); ++d )
	{
		if( (*d).m_nearDoor == connectDoor )
		{
			// Look up the neighbor
			PersistDoors( (*d).m_farNode );
			NodeInfoMap::iterator n = m_NodeInfoMap.find( (*d).m_farNode );
			gpassert( n != m_NodeInfoMap.end() );

			// Erase this door from the far node
			for( NodeDoorColl::iterator nd = (*n).second.m_nodeDoorColl.begin(); nd != (*n).second.m_nodeDoorColl.end(); ++nd )
			{
				if( (*nd).m_nearDoor == (*d).m_farDoor )
				{
					(*n).second.m_nodeDoorColl.erase( nd );
					break;
				}
			}

			// Rebuild the doors for this node
			RebuildDoors( (*d).m_farNode );

			// Register a disconnect
			RegisterNodeDisconnect( connectGuid, (*d).m_farNode );
			(*i).second.m_nodeDoorColl.erase( d );
			break;
		}
	}

	// Add new door
	doorInfo.m_nearDoor	= connectDoor;
	doorInfo.m_farNode	= targetGuid;
	doorInfo.m_farDoor	= targetDoor;
	doorInfo.m_bConnect	= false;

	(*i).second.m_nodeDoorColl.push_back( doorInfo );

	// Rebuild doors for the connect
	RebuildDoors( connectGuid );

	// Build the transition
	NodeTransition nodeTrans;
	nodeTrans.m_targetGuid		= targetGuid;
	nodeTrans.m_targetDoor		= targetDoor;
	nodeTrans.m_oldTargetDoor	= oldTargetDoor;
	nodeTrans.m_connectGuid		= connectGuid;
	nodeTrans.m_connectDoor		= connectDoor;
	nodeTrans.m_oldConnectGuid	= oldConnectGuid;
	nodeTrans.m_oldConnectDoor	= oldConnectDoor;
	nodeTrans.m_currentTime		= currentTime;
	nodeTrans.m_startTime		= startTime;
	nodeTrans.m_endTime			= endTime;
	nodeTrans.m_percentComplete	= 0.0f;
	nodeTrans.m_axisHint		= ahint;
	nodeTrans.m_owner			= owner;
	nodeTrans.m_bConnect		= bConnect;

	// Put this transition into our active list
	m_TransitionMap.insert( std::pair< database_guid, NodeTransition >( targetGuid, nodeTrans ) );

	return true;
}

// Connect two nodes
bool SiegeTransition::NodeConnection( const database_guid targetGuid, const DWORD targetDoor,
									  const database_guid connectGuid, const DWORD connectDoor,
									  bool bConnect, bool bForceComplete )
{
	// Check to see if we should force complete any existing transitions
	if( bForceComplete )
	{
		// Look for target transition
		TransitionMap::iterator t = m_TransitionMap.find( targetGuid );
		if( t != m_TransitionMap.end() )
		{
			// Complete this transition
			CompleteTransition( (*t).second, true );
		}

		// Look for connect transition
		t = m_TransitionMap.find( connectGuid );
		if( t != m_TransitionMap.end() )
		{
			// Complete this transition
			CompleteTransition( (*t).second, true );
		}
	}

	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetTransitionCritical() );

	// Persist the doors for the target
	PersistDoors( targetGuid );

	NodeInfoMap::iterator i = m_NodeInfoMap.find( targetGuid );
	gpassert( i != m_NodeInfoMap.end() );

	// Erase all existing door connections
	for( NodeDoorColl::iterator d = (*i).second.m_nodeDoorColl.begin(); d != (*i).second.m_nodeDoorColl.end(); ++d )
	{
		if( (*d).m_nearDoor == targetDoor )
		{
			// Look up the neighbor
			PersistDoors( (*d).m_farNode );
			NodeInfoMap::iterator n = m_NodeInfoMap.find( (*d).m_farNode );
			gpassert( n != m_NodeInfoMap.end() );

			// Erase this door from the far node
			for( NodeDoorColl::iterator nd = (*n).second.m_nodeDoorColl.begin(); nd != (*n).second.m_nodeDoorColl.end(); ++nd )
			{
				if( (*nd).m_nearDoor == (*d).m_farDoor )
				{
					(*n).second.m_nodeDoorColl.erase( nd );
					break;
				}
			}

			// Rebuild the doors for this node
			RebuildDoors( (*d).m_farNode );

			// Register a disconnect
			RegisterNodeDisconnect( targetGuid, (*d).m_farNode );
			(*i).second.m_nodeDoorColl.erase( d );
			break;
		}
	}

	// Add new door
	NodeDoorInfo doorInfo;
	doorInfo.m_nearDoor	= targetDoor;
	doorInfo.m_farNode	= connectGuid;
	doorInfo.m_farDoor	= connectDoor;
	doorInfo.m_bConnect	= bConnect;

	(*i).second.m_nodeDoorColl.push_back( doorInfo );

	// Rebuild doors for the target
	RebuildDoors( targetGuid );

	// Persist the doors for the connect
	PersistDoors( connectGuid );

	i = m_NodeInfoMap.find( connectGuid );
	gpassert( i != m_NodeInfoMap.end() );

	// Look for the door that we are connection through
	for( d = (*i).second.m_nodeDoorColl.begin(); d != (*i).second.m_nodeDoorColl.end(); ++d )
	{
		if( (*d).m_nearDoor == connectDoor )
		{
			// Look up the neighbor
			PersistDoors( (*d).m_farNode );
			NodeInfoMap::iterator n = m_NodeInfoMap.find( (*d).m_farNode );
			gpassert( n != m_NodeInfoMap.end() );

			// Erase this door from the far node
			for( NodeDoorColl::iterator nd = (*n).second.m_nodeDoorColl.begin(); nd != (*n).second.m_nodeDoorColl.end(); ++nd )
			{
				if( (*nd).m_nearDoor == (*d).m_farDoor )
				{
					(*n).second.m_nodeDoorColl.erase( nd );
					break;
				}
			}

			// Rebuild the doors for this node
			RebuildDoors( (*d).m_farNode );

			// Register a disconnect
			RegisterNodeDisconnect( connectGuid, (*d).m_farNode );
			(*i).second.m_nodeDoorColl.erase( d );
			break;
		}
	}

	// Add new door
	doorInfo.m_nearDoor	= connectDoor;
	doorInfo.m_farNode	= targetGuid;
	doorInfo.m_farDoor	= targetDoor;
	doorInfo.m_bConnect	= bConnect;

	(*i).second.m_nodeDoorColl.push_back( doorInfo );

	// Rebuild doors for the connect
	RebuildDoors( connectGuid );

	// Connect the nodes if requested
	if( bConnect )
	{
		gSiegeEngine.ConnectSiegeNodes( targetGuid, connectGuid );
	}

	return true;
}

// Rebuild all of the doors that this node needs
bool SiegeTransition::RebuildDoors( const database_guid nodeGuid )
{
	// Find this node in our door mapping
	NodeInfoMap::iterator i = m_NodeInfoMap.find( nodeGuid );
	if( i != m_NodeInfoMap.end() )
	{
		// See if the node exists
		if( nodeGuid.IsValidSlow() )
		{
			SiegeNodeHandle handle		= gSiegeEngine.NodeCache().UseObject( nodeGuid );
			SiegeNode& node				= handle.RequestObject( gSiegeEngine.NodeCache() );

			// Destroy existing doors
			SiegeNodeDoorList& dl	= node.m_DoorList;
			for( SiegeNodeDoorIter d = dl.begin(); d != dl.end(); ++d )
			{
				delete (*d);
			}
			dl.clear();

			// See if the node has a mesh yet
			SiegeMesh* pMesh	= NULL;
			if( node.GetMeshGUID().IsValid() )
			{
				// Get the mesh
				pMesh	= &node.GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() );
			}

			// Rebuild the doors
			for( NodeDoorColl::iterator n = (*i).second.m_nodeDoorColl.begin(); n != (*i).second.m_nodeDoorColl.end(); ++n )
			{
				if( pMesh )
				{
					SiegeMeshDoor* pMeshDoor	= pMesh->GetDoorByID( (*n).m_nearDoor );
					if( !pMeshDoor )
					{
						gperrorf(( "Node %s had been requested to transition using near door %d which is not valid for its mesh!", nodeGuid.ToString().c_str(), (*n).m_nearDoor ));
						continue;
					}

					node.AddDoor( (*n).m_nearDoor, pMeshDoor->GetCenter(), pMeshDoor->GetOrientation(),
								  (*n).m_farNode, (*n).m_farDoor, (*n).m_bConnect );
				}
				else
				{
					node.AddDoor( (*n).m_nearDoor, (*n).m_farNode, (*n).m_farDoor, (*n).m_bConnect );
				}
			}
		}

		return true;
	}

	return false;
}

// Process needed nodal disconnects for this node
void SiegeTransition::ProcessDisconnects( const database_guid nodeGuid )
{
	kerneltool::Critical::Lock autoLock( gSiegeEngine.GetTransitionCritical() );

	NodeInfoMap::iterator i = m_NodeInfoMap.find( nodeGuid );
	if( i != m_NodeInfoMap.end() )
	{
		// Go through the disconnects registered for this node
		for( NodeDisconnectColl::iterator d = (*i).second.m_nodeDisconnectColl.begin(); d != (*i).second.m_nodeDisconnectColl.end(); ++d )
		{
			gSiegeEngine.DisconnectSiegeNodes( nodeGuid, (*d) );
		}
	}
}

// Update all active transitions
void SiegeTransition::UpdateTransitionPositions()
{
	// Iterate over all transitions and update them
	for( TransitionMap::iterator i = m_TransitionMap.begin(); i != m_TransitionMap.end(); ++i )
	{
		UpdateTransitionPosition( (*i).second );
	}
}

// Update the given node's position/orientation if it is in transition
void SiegeTransition::UpdateTransitionPosition( const database_guid nodeGuid )
{
	// See if this node is currently in transition
	TransitionMap::iterator i = m_TransitionMap.find( nodeGuid );
	if( i != m_TransitionMap.end() )
	{
		UpdateTransitionPosition( (*i).second );
	}
}

// Update the position/orientation for the given transition info
void SiegeTransition::UpdateTransitionPosition( const NodeTransition& trans )
{
	// Get the target
	SiegeNode* pTargetNode				= gSiegeEngine.IsNodeValid( trans.m_targetGuid );
	SiegeNode* pConnectNode				= gSiegeEngine.IsNodeValid( trans.m_connectGuid );
	if( pTargetNode && pConnectNode )
	{
		SiegeMeshDoor* pTargetDoor		= pTargetNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() ).GetDoorByID( trans.m_targetDoor );
		SiegeMeshDoor* pConnectDoor		= pConnectNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() ).GetDoorByID( trans.m_connectDoor );

		if( pTargetDoor && pConnectDoor )
		{
			if( IsOne( trans.m_percentComplete ) )
			{
				// Calculate the final resting position
				matrix_3x3 inv_far_orient	= pTargetDoor->GetOrientation().Transpose_T();
				matrix_3x3 orient			= pConnectDoor->GetOrientation();
				
				orient.SetColumn_0( -orient.GetColumn_0() );
				orient.SetColumn_2( -orient.GetColumn_2() );

				matrix_3x3 door_orient		= orient * inv_far_orient;
				vector_3 door_offset		= pConnectDoor->GetCenter() + (door_orient * -pTargetDoor->GetCenter());

				// These are the orienatations that the target_node should be at when the transition is complete.
				vector_3 final_center		= pConnectNode->GetCurrentCenter() + (pConnectNode->GetCurrentOrientation() * door_offset);
				matrix_3x3 final_orient		= pConnectNode->GetCurrentOrientation() * door_orient;

				pTargetNode->SetCurrentCenter( final_center );
				pTargetNode->SetCurrentOrientation( final_orient );
			}
			else
			{
				SiegeNode* pOldConnectNode			= gSiegeEngine.IsNodeValid( trans.m_oldConnectGuid );
				if( pOldConnectNode )
				{
					// Get the corresponding doors
					SiegeMeshDoor* pOldTargetDoor	= pTargetNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() ).GetDoorByID( trans.m_oldTargetDoor );
					SiegeMeshDoor* pOldConnectDoor	= pOldConnectNode->GetMeshHandle().RequestObject( gSiegeEngine.MeshCache() ).GetDoorByID( trans.m_oldConnectDoor );

					if( pOldTargetDoor && pOldConnectDoor )
					{
						vector_3 final_center( DoNotInitialize );
						matrix_3x3 final_orient( DoNotInitialize );

						{
							// Calculate the final resting position
							matrix_3x3 inv_far_orient	= pTargetDoor->GetOrientation().Transpose_T();
							matrix_3x3 orient			= pConnectDoor->GetOrientation();
							
							orient.SetColumn_0( -orient.GetColumn_0() );
							orient.SetColumn_2( -orient.GetColumn_2() );

							matrix_3x3 door_orient		= orient * inv_far_orient;
							vector_3 door_offset		= pConnectDoor->GetCenter() + (door_orient * -pTargetDoor->GetCenter());

							// These are the orienatations that the target_node should be at when the transition is complete.
							final_center				= pConnectNode->GetCurrentCenter() + (pConnectNode->GetCurrentOrientation() * door_offset);
							final_orient				= pConnectNode->GetCurrentOrientation() * door_orient;
						}

						vector_3 old_center( DoNotInitialize );
						matrix_3x3 old_orient( DoNotInitialize );

						{
							// Calculate old resting position
							matrix_3x3 inv_far_orient	= pOldTargetDoor->GetOrientation().Transpose_T();
							matrix_3x3 orient			= pOldConnectDoor->GetOrientation();
							
							orient.SetColumn_0( -orient.GetColumn_0() );
							orient.SetColumn_2( -orient.GetColumn_2() );

							matrix_3x3 door_orient		= orient * inv_far_orient;
							vector_3 door_offset		= pOldConnectDoor->GetCenter() + (door_orient * -pOldTargetDoor->GetCenter());

							// These are the orienatations that the target_node should be at when the transition is complete.
							old_center					= pOldConnectNode->GetCurrentCenter() + (pOldConnectNode->GetCurrentOrientation() * door_offset);
							old_orient					= pOldConnectNode->GetCurrentOrientation() * door_orient;
						}

						// Modify our transition target node's position
						pTargetNode->SetCurrentCenter( final_center + ((old_center - final_center) * (1.0f - trans.m_percentComplete)) );

						// Modify our transition target node's orientation
						if( trans.m_axisHint == AH_NONE )
						{
							pTargetNode->SetCurrentOrientation( BlendMatrices( final_orient, old_orient, (1.0f - trans.m_percentComplete) ) );
						}
						else
						{
							matrix_3x3			blendMat( DoNotInitialize );
										
							switch( trans.m_axisHint )
							{
								case AH_XAXIS_POSITIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn2_T(), old_orient.GetColumn2_T() );
									if( IsNegative( final_orient.GetColumn1_T().DotProduct( old_orient.GetColumn2_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= XRotationColumns( angleDiff * trans.m_percentComplete );
								} break;
								case AH_XAXIS_NEGATIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn2_T(), old_orient.GetColumn2_T() );
									if( IsPositive( final_orient.GetColumn1_T().DotProduct( old_orient.GetColumn2_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= XRotationColumns( -angleDiff * trans.m_percentComplete );
								} break;
								case AH_YAXIS_POSITIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn2_T(), old_orient.GetColumn2_T() );
									if( IsPositive( final_orient.GetColumn0_T().DotProduct( old_orient.GetColumn2_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= YRotationColumns( angleDiff * trans.m_percentComplete );
								} break;
								case AH_YAXIS_NEGATIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn2_T(), old_orient.GetColumn2_T() );
									if( IsNegative( final_orient.GetColumn0_T().DotProduct( old_orient.GetColumn2_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= YRotationColumns( -angleDiff * trans.m_percentComplete );
								} break;
								case AH_ZAXIS_POSITIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn1_T(), old_orient.GetColumn1_T() );
									if( IsNegative( final_orient.GetColumn1_T().DotProduct( old_orient.GetColumn0_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= ZRotationColumns( angleDiff * trans.m_percentComplete );
								} break;
								case AH_ZAXIS_NEGATIVE:
								{
									float angleDiff = AngleBetween( final_orient.GetColumn1_T(), old_orient.GetColumn1_T() );
									if( IsPositive( final_orient.GetColumn1_T().DotProduct( old_orient.GetColumn0_T() ) ) )
									{
										angleDiff	+= PI;
									}
									blendMat		= ZRotationColumns( -angleDiff * trans.m_percentComplete );
								} break;
							}

							pTargetNode->SetCurrentOrientation( old_orient * blendMat );
						}

						// Request hotpoint update
						pTargetNode->RequestHotpointUpdate( true );
					}
				}
			}

			// Callback notification
			gSiegeEngine.NotifyNodeSpaceChanged( trans.m_targetGuid, true );
		}
	}
}

// Complete a transition
void SiegeTransition::CompleteTransition( NodeTransition& transition, bool bRemoveAndNotify )
{
	// Force completion if it is not already
	if( !IsOne( transition.m_percentComplete ) )
	{
		transition.m_percentComplete	= 1.0f;
	}

	// Update space for this node
	UpdateTransitionPosition( transition.m_targetGuid );

	// Remove node replacements in all frustums
	gSiegeEngine.RemoveFrustumNodeReplacement( transition.m_targetGuid );

	// Perform the connection if it was requested
	if( transition.m_bConnect )
	{
		// Lock the transition critical
		kerneltool::Critical::Lock autoLock( gSiegeEngine.GetTransitionCritical() );

		// Find the door connection for the target
		NodeInfoMap::iterator n = m_NodeInfoMap.find( transition.m_targetGuid );
		gpassert( n != m_NodeInfoMap.end() );

		for( NodeDoorColl::iterator d = (*n).second.m_nodeDoorColl.begin(); d != (*n).second.m_nodeDoorColl.end(); ++d )
		{
			if( (*d).m_nearDoor == transition.m_targetDoor )
			{
				(*d).m_bConnect	= true;
				break;
			}
		}

		RebuildDoors( transition.m_targetGuid );

		// Find the door connection for the connect
		n = m_NodeInfoMap.find( transition.m_connectGuid );
		gpassert( n != m_NodeInfoMap.end() );

		for( d = (*n).second.m_nodeDoorColl.begin(); d != (*n).second.m_nodeDoorColl.end(); ++d )
		{
			if( (*d).m_nearDoor == transition.m_connectDoor )
			{
				(*d).m_bConnect	= true;
				break;
			}
		}

		RebuildDoors( transition.m_connectGuid );

		// Connect the nodes
		gSiegeEngine.ConnectSiegeNodes( transition.m_targetGuid, transition.m_connectGuid );
	}

	if( bRemoveAndNotify )
	{
		// Save values
		DWORD owner					= transition.m_owner;
		database_guid targetGuid	= transition.m_targetGuid;

		// Erase entry
		m_TransitionMap.erase( targetGuid );

		// Notify of completion
		gSiegeEngine.TransitionComplete( targetGuid, owner );
	}
}

// Save out the doors for this node if we need to
void SiegeTransition::PersistDoors( const database_guid nodeGuid )
{
	// Find this node in our door mapping
	NodeInfoMap::iterator i = m_NodeInfoMap.find( nodeGuid );
	if( i == m_NodeInfoMap.end() )
	{
		NodeInfo nInfo;

		// See if the node is loaded
		if( nodeGuid.IsValidSlow() )
		{
			SiegeNodeHandle handle		= gSiegeEngine.NodeCache().UseObject( nodeGuid );
			SiegeNode& node				= handle.RequestObject( gSiegeEngine.NodeCache() );

			// Copy the current doors into the list
			const SiegeNodeDoorList& dl	= node.GetDoors();
			for( SiegeNodeDoorConstIter d = dl.begin(); d != dl.end(); ++d )
			{
				NodeDoorInfo doorInfo;
				doorInfo.m_nearDoor	= (*d)->GetID();
				doorInfo.m_farNode	= (*d)->GetNeighbour();
				doorInfo.m_farDoor	= (*d)->GetNeighbourID();
				doorInfo.m_bConnect	= (*d)->GetConnection();

				nInfo.m_nodeDoorColl.push_back( doorInfo );
			}
		}
		else
		{
			// Callback to collect doors
			gSiegeEngine.CollectDoors( nodeGuid, nInfo.m_nodeDoorColl );
		}

		// Put this node onto our persistent listing
		m_NodeInfoMap.insert( std::pair< database_guid, NodeInfo >( nodeGuid, nInfo ) );
	}
}

// Register a nodal disconnect
void SiegeTransition::RegisterNodeDisconnect( const database_guid nearGuid, const database_guid farGuid )
{
	// Register disconnect with near node
	NodeInfoMap::iterator i = m_NodeInfoMap.find( nearGuid );
	gpassert( i != m_NodeInfoMap.end() );

	for( NodeDisconnectColl::iterator ndc = (*i).second.m_nodeDisconnectColl.begin(); ndc != (*i).second.m_nodeDisconnectColl.end(); ++ndc )
	{
		if( (*ndc) == farGuid )
		{
			break;
		}
	}
	if( ndc == (*i).second.m_nodeDisconnectColl.end() )
	{
		(*i).second.m_nodeDisconnectColl.push_back( farGuid );
	}

	// Register disconnect with far node
	i = m_NodeInfoMap.find( farGuid );
	gpassert( i != m_NodeInfoMap.end() );

	for( ndc = (*i).second.m_nodeDisconnectColl.begin(); ndc != (*i).second.m_nodeDisconnectColl.end(); ++ndc )
	{
		if( (*ndc) == nearGuid )
		{
			break;
		}
	}
	if( ndc == (*i).second.m_nodeDisconnectColl.end() )
	{
		(*i).second.m_nodeDisconnectColl.push_back( nearGuid );
	}

	// Process disconnect
	gSiegeEngine.DisconnectSiegeNodes( nearGuid, farGuid );
}