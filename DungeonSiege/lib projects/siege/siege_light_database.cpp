/******************************************************************************
**
**						SiegeLightDatabase
**
**		See .h file for implementation notes and details
**
******************************************************************************/

#include "precomp_siege.h"

#include "siege_engine.h"
#include "siege_pos.h"
#include "siege_light_database.h"

namespace siege
{

// String conversions

static const char* LIGHT_TYPE_STRINGS[] =
{
	"point",
	"directional",
	"spot",
};

COMPILER_ASSERT( ELEMENT_COUNT( LIGHT_TYPE_STRINGS ) == LT_COUNT );

const char* ToString( LIGHT_TYPE var )
{
	gpassert( (var >= LT_BEGIN) && (var < LT_END) );
	return ( LIGHT_TYPE_STRINGS[ var ] );
}

static const char* LIGHT_SUBTYPE_STRINGS[] =
{
	"static",
	"dynamic",
};

COMPILER_ASSERT( ELEMENT_COUNT( LIGHT_SUBTYPE_STRINGS ) == LS_COUNT );

const char* ToString( LIGHT_SUBTYPE var )
{
	gpassert( (var >= LS_BEGIN) && (var < LS_END) );
	return ( LIGHT_SUBTYPE_STRINGS[ var ] );
}


// Construction
SiegeLightDatabase::SiegeLightDatabase()
{
	m_TimedLightColor	= 0xFFFFFFFF;
}

// Destruction
SiegeLightDatabase::~SiegeLightDatabase()
{
	// Clear out the database
	Clear();
}

// Insertion of a point light source
database_guid
SiegeLightDatabase::InsertPointLightSource( const SiegePos& position, const vector_3& color, const database_guid guid,
										    const bool bstatic, const bool bactive,
										    const float iradius, const float oradius, const float intensity,
										    const bool bdrawshadow, const bool boccludegeometry,
											const bool baffectsactors, const bool baffectsitems, const bool baffectsterrain,
											const bool bontimer )
{
	// Check for collisions
	if( !( guid == UNDEFINED_GUID ) && (m_Lights.find( guid ) != m_Lights.end()) )
	{
		gperror( "Tried to insert a light with same guid as existing light!  Collision detected!" );
		return UNDEFINED_GUID;
	}

	if( !gSiegeEngine.IsNodeValid( position.node ) )
	{
		gperror( "Cannot insert lightsource that is placed in an unloaded node!" );
		return UNDEFINED_GUID;
	}

	// Fill in a LightDescriptor for the new light
	LightDescriptor* pDescriptor	= new LightDescriptor;
	memset( pDescriptor, 0, sizeof( LightDescriptor ) );

	pDescriptor->m_Type				= LT_POINT;
	pDescriptor->m_Color			= MAKEDWORDCOLOR( color );
	pDescriptor->m_Subtype			= bstatic ? LS_STATIC : LS_DYNAMIC;
	pDescriptor->m_bActive			= bactive;
	pDescriptor->m_InnerRadius		= iradius;
	pDescriptor->m_OuterRadius		= oradius;
	pDescriptor->m_Intensity		= intensity;
	pDescriptor->m_bDrawShadow		= bdrawshadow;
	pDescriptor->m_bOccludeGeometry	= boccludegeometry;
	pDescriptor->m_bAffectsActors	= baffectsactors;
	pDescriptor->m_bAffectsItems	= baffectsitems;
	pDescriptor->m_bAffectsTerrain	= baffectsterrain;
	pDescriptor->m_bOnTimer			= bontimer;

	// Create a new pair to hold this light
	LightConstruct lConstruct;
	lConstruct.m_pDescriptor		= pDescriptor;

	database_guid nGuid( guid );
	if( nGuid == UNDEFINED_GUID )
	{
		nGuid = GenerateRandomLightGuid();
	}

	// Insert the light source
	if( RecursivelyInsertLightSource( nGuid, position, SiegePos(), lConstruct ) )
	{
		// Put the new source in our mapping
		m_Lights.insert( std::pair< database_guid, LightConstruct >( nGuid, lConstruct ) );
	}
	else
	{
		// Cleanup since this light failed insertion
		delete pDescriptor;

		// Return failure
		return UNDEFINED_GUID;
	}

	// Return the guid
	return nGuid;
}

// Insertion of a direction light source
database_guid
SiegeLightDatabase::InsertDirectionalLightSource( const SiegePos& direction, const vector_3& color, const database_guid guid,
											      const bool bstatic, const bool bactive, const float intensity, 
											      const bool bdrawshadow, const bool boccludegeometry,
												  const bool baffectsactors, const bool baffectsitems, const bool baffectsterrain,
												  const bool bontimer )
{
	// Check for collisions
	if( !( guid == UNDEFINED_GUID ) && (m_Lights.find( guid ) != m_Lights.end()) )
	{
		gperror( "Tried to insert a light with same guid as existing light!  Collision detected!" );
		return UNDEFINED_GUID;
	}

	if( direction.pos.IsZero() )
	{
		gperror( "Cannot insert directional light source that has no direction!" );
		return UNDEFINED_GUID;
	}

	if( !gSiegeEngine.IsNodeValid( direction.node ) )
	{
		gperror( "Cannot insert lightsource that is placed in an unloaded node!" );
		return UNDEFINED_GUID;
	}

	// Fill in a LightDescriptor for the new light
	LightDescriptor* pDescriptor	= new LightDescriptor;
	memset( pDescriptor, 0, sizeof( LightDescriptor ) );

	pDescriptor->m_Type				= LT_DIRECTIONAL;
	pDescriptor->m_Color			= MAKEDWORDCOLOR( color );
	pDescriptor->m_Subtype			= bstatic ? LS_STATIC : LS_DYNAMIC;
	pDescriptor->m_bActive			= bactive;
	pDescriptor->m_Intensity		= intensity;
	pDescriptor->m_bDrawShadow		= bdrawshadow;
	pDescriptor->m_bOccludeGeometry	= boccludegeometry;
	pDescriptor->m_bAffectsActors	= baffectsactors;
	pDescriptor->m_bAffectsItems	= baffectsitems;
	pDescriptor->m_bAffectsTerrain	= baffectsterrain;
	pDescriptor->m_bOnTimer			= bontimer;

	// Create a new pair to hold this light
	LightConstruct lConstruct;
	lConstruct.m_pDescriptor		= pDescriptor;

	database_guid nGuid( guid );
	if( nGuid == UNDEFINED_GUID )
	{
		nGuid = GenerateRandomLightGuid();
	}

	// Insert the light source
	if( RecursivelyInsertLightSource( nGuid, SiegePos(), direction, lConstruct ) )
	{
		// Put the new source in our mapping
		m_Lights.insert( std::pair< database_guid, LightConstruct >( nGuid, lConstruct ) );
	}
	else
	{
		// Cleanup since this light failed insertion
		delete pDescriptor;

		// Return failure
		return UNDEFINED_GUID;
	}

	// Return the guid
	return nGuid;
}

// Insertion of a spot light source
database_guid
SiegeLightDatabase::InsertSpotLightSource( const SiegePos& position, const SiegePos& direction, const vector_3& color, 
										   const database_guid guid, const bool bstatic, const bool bactive,
										   const float iradius, const float oradius, const float intensity,
										   const bool bdrawshadow, const bool boccludegeometry,
										   const bool baffectsactors, const bool baffectsitems, const bool baffectsterrain,
										   const bool bontimer )
{
	// Check for collisions
	if( !( guid == UNDEFINED_GUID ) && (m_Lights.find( guid ) != m_Lights.end()) )
	{
		gperror( "Tried to insert a light with same guid as existing light!  Collision detected!" );
		return UNDEFINED_GUID;
	}

	if( !gSiegeEngine.IsNodeValid( position.node ) )
	{
		gperror( "Cannot insert lightsource that is placed in an unloaded node!" );
		return UNDEFINED_GUID;
	}

	// Fill in a LightDescriptor for the new light
	LightDescriptor* pDescriptor	= new LightDescriptor;
	memset( pDescriptor, 0, sizeof( LightDescriptor ) );

	pDescriptor->m_Type				= LT_SPOT;
	pDescriptor->m_Color			= MAKEDWORDCOLOR( color );
	pDescriptor->m_Subtype			= bstatic ? LS_STATIC : LS_DYNAMIC;
	pDescriptor->m_bActive			= bactive;
	pDescriptor->m_InnerRadius		= iradius;
	pDescriptor->m_OuterRadius		= oradius;
	pDescriptor->m_Intensity		= intensity;
	pDescriptor->m_bDrawShadow		= bdrawshadow;
	pDescriptor->m_bOccludeGeometry	= boccludegeometry;
	pDescriptor->m_bAffectsActors	= baffectsactors;
	pDescriptor->m_bAffectsItems	= baffectsitems;
	pDescriptor->m_bAffectsTerrain	= baffectsterrain;
	pDescriptor->m_bOnTimer			= bontimer;

	// Create a new pair to hold this light
	LightConstruct lConstruct;
	lConstruct.m_pDescriptor		= pDescriptor;

	database_guid nGuid( guid );
	if( nGuid == UNDEFINED_GUID )
	{
		nGuid = GenerateRandomLightGuid();
	}

	// Insert the light source
	gpassertm( position.node == direction.node, "Spotlight does not have matching node information!" );
	if( RecursivelyInsertLightSource( nGuid, position, direction, lConstruct ) )
	{
		// Put the new source in our mapping
		m_Lights.insert( std::pair< database_guid, LightConstruct >( nGuid, lConstruct ) );
	}
	else
	{
		// Cleanup since this light failed insertion
		delete pDescriptor;

		// Return failure
		return UNDEFINED_GUID;
	}

	// Return the guid
	return nGuid;
}

// Get information about a light
LightDescriptor SiegeLightDatabase::GetLightDescriptor( const database_guid light_id )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		return (*(*i).second.m_pDescriptor);
	}

	gperror( "Requested information about light that doesn't exist" );
	return LightDescriptor();
}

bool SiegeLightDatabase::HasLightDescriptor( const database_guid light_id )
{
	return ( m_Lights.find( light_id ) != m_Lights.end() );
}

vector_3 SiegeLightDatabase::GetLightWorldSpacePosition( const database_guid light_id )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get the node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			if( node.IsOwnedByAnyFrustum() )
			{
				// Get the local nodal position of this light
				if( (*i).second.m_pDescriptor->m_Subtype == LS_DYNAMIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_DynamicLights );
					if( l != node.m_DynamicLights.end() )
					{
						return node.NodeToWorldSpace( (*l).m_Position );
					}
				}
				else if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_StaticLights );
					if( l != node.m_StaticLights.end() )
					{
						return node.NodeToWorldSpace( (*l).m_Position );
					}
				}
			}
		}

		return vector_3::ZERO;
	}

	gperror( "Requested information about light that doesn't exist" );
	return vector_3::ZERO;
}

vector_3 SiegeLightDatabase::GetLightWorldSpaceDirection( const database_guid light_id )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get the node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			if( node.IsOwnedByAnyFrustum() )
			{
				// Get the local nodal position of this light
				if( (*i).second.m_pDescriptor->m_Subtype == LS_DYNAMIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_DynamicLights );
					if( l != node.m_DynamicLights.end() )
					{
						return( node.GetCurrentOrientation() * (*l).m_Direction );
					}
				}
				else if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_StaticLights );
					if( l != node.m_StaticLights.end() )
					{
						return( node.GetCurrentOrientation() * (*l).m_Direction );
					}
				}
			}
		}

		return vector_3::ZERO;
	}

	gperror( "Requested information about light that doesn't exist" );
	return vector_3::ZERO;
}

// Get the best node space position of this light that we can find
SiegePos SiegeLightDatabase::GetLightNodeSpacePosition( const database_guid light_id )
{
	SiegePos sPosition( vector_3::ZERO, UNDEFINED_GUID );
	float sCost( FLOAT_MAX );

	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get the node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Get the local nodal position of this light
			if( (*i).second.m_pDescriptor->m_Subtype == LS_DYNAMIC )
			{
				SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_DynamicLights );
				if( l != node.m_DynamicLights.end() )
				{
					if( node.IsPointInNode( (*l).m_Position ) )
					{
						sPosition.node	= node.GetGUID();
						sPosition.pos	= (*l).m_Position;
						return sPosition;
					}
					else
					{
						float nCost	= Length2( (*l).m_Position );
						if( nCost < sCost )
						{
							sPosition.node	= node.GetGUID();
							sPosition.pos	= (*l).m_Position;
							sCost			= nCost;
						}
					}
				}
			}
			else if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_StaticLights );
				if( l != node.m_StaticLights.end() )
				{
					if( node.IsPointInNode( (*l).m_Position ) )
					{
						sPosition.node	= node.GetGUID();
						sPosition.pos	= (*l).m_Position;
						return sPosition;
					}
					else
					{
						float nCost	= Length2( (*l).m_Position );
						if( nCost < sCost )
						{
							sPosition.node	= node.GetGUID();
							sPosition.pos	= (*l).m_Position;
							sCost			= nCost;
						}
					}
				}
			}
		}
	}

	gpassertm( !(sPosition.node == UNDEFINED_GUID), "Could not obtain valid node space for light!" );
	return sPosition;
}

// Get the best node space direction of this light that we can find
SiegePos SiegeLightDatabase::GetLightNodeSpaceDirection( const database_guid light_id )
{
	SiegePos sDirection( vector_3::ZERO, UNDEFINED_GUID );

	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get the node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			if( node.IsOwnedByAnyFrustum() )
			{
				// Get the local nodal position of this light
				if( (*i).second.m_pDescriptor->m_Subtype == LS_DYNAMIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_DynamicLights );
					if( l != node.m_DynamicLights.end() )
					{
						sDirection.node	= node.GetGUID();
						sDirection.pos	= (*l).m_Direction;
						return sDirection;
					}
				}
				else if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
				{
					SiegeLightList::iterator l = node.FindLightInList( light_id, node.m_StaticLights );
					if( l != node.m_StaticLights.end() )
					{
						sDirection.node	= node.GetGUID();
						sDirection.pos	= (*l).m_Direction;
						return sDirection;
					}
				}
			}
		}

		return sDirection;
	}

	gperror( "Requested information about light that doesn't exist" );
	return sDirection;
}

// Set the information
void SiegeLightDatabase::SetLightDescriptor( const database_guid light_id, const LightDescriptor& desc )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Update light info
		(*(*i).second.m_pDescriptor)	= desc;

		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get this node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Update this light
			node.UpdateLightDescriptor( light_id, (*i).second.m_pDescriptor );

			// If static, then we need to reaccumulate
			if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				// Reaccumulate the static lights
				node.RequestLightingAccum( true );
			}
		}
	}
}

// Set light position
void SiegeLightDatabase::SetLightPosition( const database_guid light_id, const SiegePos& position )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		if( (*i).second.m_pDescriptor->m_Type == LT_DIRECTIONAL )
		{
			return;
		}

		if( (*i).second.m_pDescriptor->m_Type == LT_POINT )
		{
			// Get the current light position
			if( !gSiegeEngine.IsNodeInTransition( position.node ) &&
				position.WorldPos().IsEqual( GetLightWorldSpacePosition( light_id ), 0.01f ) )
			{
				return;
			}
		}

		SiegePos currentDirection;
		if( (*i).second.m_pDescriptor->m_Type == LT_SPOT )
		{
			currentDirection	= GetLightNodeSpaceDirection( light_id );
		}

		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get this node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Destroy the light
			node.DestroyLight( light_id, false );

			// Accumulate the buffers
			if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				node.RequestLightingAccum( true );
			}
		}

		// Clear the list of nodes
		(*i).second.m_Nodes.clear();

		// Re-insert the light
		if( (*i).second.m_pDescriptor->m_Type == LT_POINT )
		{
			RecursivelyInsertLightSource( light_id, position, SiegePos(), (*i).second );
		}
		else if( (*i).second.m_pDescriptor->m_Type == LT_SPOT )
		{
			RecursivelyInsertLightSource( light_id, position, currentDirection, (*i).second );
		}

		if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
		{
			// Reaccumulate the static buffers
			for( o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
			{
				// Get this node
				SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

				// Accumulate the buffers
				node.RequestLightingAccum( true );
			}
		}
	}
}

// Set light direction
void SiegeLightDatabase::SetLightDirection( const database_guid light_id, const SiegePos& direction )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		if( (*i).second.m_pDescriptor->m_Type == LT_POINT )
		{
			return;
		}

		SiegePos currentPosition;
		if( (*i).second.m_pDescriptor->m_Type == LT_SPOT )
		{
			currentPosition	= GetLightNodeSpacePosition( light_id );
		}

		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get this node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Destroy the light
			node.DestroyLight( light_id, false );

			// Accumulate the buffers
			if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				node.RequestLightingAccum( true );
			}
		}

		// Clear the list of nodes
		(*i).second.m_Nodes.clear();

		// Re-insert the light
		if( (*i).second.m_pDescriptor->m_Type == LT_DIRECTIONAL )
		{
			RecursivelyInsertLightSource( light_id, SiegePos(), direction, (*i).second );
		}
		else if( (*i).second.m_pDescriptor->m_Type == LT_SPOT )
		{
			RecursivelyInsertLightSource( light_id, currentPosition, direction, (*i).second );
		}

		if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
		{
			// Reaccumulate the static buffers
			for( o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
			{
				// Get this node
				SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

				// Accumulate the buffers
				node.RequestLightingAccum( true );
			}
		}
	}
}

// Set light position and direction
void SiegeLightDatabase::SetLightPositionAndDirection( const database_guid light_id,
													   const SiegePos& position,
													   const SiegePos& direction )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes affected by this light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get this node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Destroy the light
			node.DestroyLight( light_id, false );

			// Accumulate the buffers
			if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				node.RequestLightingAccum( true );
			}
		}

		// Clear the list of nodes
		(*i).second.m_Nodes.clear();

		// Re-insert the light
		if( (*i).second.m_pDescriptor->m_Type == LT_DIRECTIONAL )
		{
			RecursivelyInsertLightSource( light_id, SiegePos(), direction, (*i).second );
		}
		else if( (*i).second.m_pDescriptor->m_Type == LT_SPOT )
		{
			RecursivelyInsertLightSource( light_id, position, direction, (*i).second );
		}
		else if( (*i).second.m_pDescriptor->m_Type == LT_POINT )
		{
			RecursivelyInsertLightSource( light_id, position, SiegePos(), (*i).second );
		}

		if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC )
		{
			// Reaccumulate the static buffers
			for( o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
			{
				// Get this node
				SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

				// Accumulate the buffers
				node.RequestLightingAccum( true );
			}
		}
	}
}

// Destroy a light that is currently in the database
void SiegeLightDatabase::DestroyLight( const database_guid light_id )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes of this light and remove the light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			// Get this node
			SiegeNode& node = (*o).RequestObject( gSiegeEngine.NodeCache() );

			// Update this light
			node.DestroyLight( light_id, false );

			// If static, then we need to reaccumulate
			if( (*i).second.m_pDescriptor->m_Subtype == LS_STATIC &&
				node.GetMeshGUID() != UNDEFINED_GUID )
			{
				// Reaccumulate the static lights
				node.RequestLightingAccum( true );
			}
		}

		// Destroy the light
		delete (*i).second.m_pDescriptor;

		// Clean it out of the map
		m_Lights.erase( i );
	}
}

// Get the lights that can affect the given position
LightList SiegeLightDatabase::GetActiveLightsAtPosition( const SiegePos& position )
{
	UNREFERENCED_PARAMETER( position );

	// TODO::  Do something with this function
	LightList lList;
	return lList;
}

// Manually update the lighting for a given node
void SiegeLightDatabase::UpdateLighting( const database_guid node_id )
{
	// Get the node
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( node_id ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

	gpassert( node.GetMeshGUID() != UNDEFINED_GUID );

	for( LightMap::iterator l = m_Lights.begin(); l != m_Lights.end(); ++l )
	{
		if( !(*l).second.m_Nodes.empty() )
		{
			if( (*l).second.m_pDescriptor->m_Subtype == LS_STATIC )
			{
				SiegeLightList::iterator s = node.FindLightInList( (*l).first, node.m_StaticLights );
				if( s == node.m_StaticLights.end() )
				{
					vector_3 WorldSpacePosition		= GetLightWorldSpacePosition( (*l).first );
					vector_3 WorldSpaceDirection	= GetLightWorldSpaceDirection( (*l).first );

					if( LightAffectsNode( node, WorldSpacePosition, WorldSpaceDirection, *(*l).second.m_pDescriptor ) )
					{
						// Put the light in the node
						node.InsertLight( (*l).first, node.WorldToNodeSpace( WorldSpacePosition ), node.GetTransposeOrientation() * WorldSpaceDirection, (*l).second.m_pDescriptor );

						// Tell the main database about the light
						(*l).second.m_Nodes.push_back( handle );
					}
				}
				else
				{
					node.CalculateLight( (*l).first, (*s) );
				}
			}
			else if( (*l).second.m_pDescriptor->m_Subtype == LS_DYNAMIC )
			{
				SiegeLightList::iterator d = node.FindLightInList( (*l).first, node.m_DynamicLights );
				if( d == node.m_DynamicLights.end() )
				{
					vector_3 WorldSpacePosition		= GetLightWorldSpacePosition( (*l).first );
					vector_3 WorldSpaceDirection	= GetLightWorldSpaceDirection( (*l).first );

					if( LightAffectsNode( node, WorldSpacePosition, WorldSpaceDirection, *(*l).second.m_pDescriptor ) )
					{
						// Put the light in the node
						node.InsertLight( (*l).first, node.WorldToNodeSpace( WorldSpacePosition ), node.GetTransposeOrientation() * WorldSpaceDirection, (*l).second.m_pDescriptor );

						// Tell the main database about the light
						(*l).second.m_Nodes.push_back( handle );
					}
				}
				else
				{
					node.CalculateLight( (*l).first, (*d) );
				}
			}
		}
	}
}

// Clear the database
void SiegeLightDatabase::Clear()
{
	// Go through the database and make sure all lights have been cleared
	while( !m_Lights.empty() )
	{
		DestroyLight( (*m_Lights.begin()).first );
	}
}

// Set the directional light color
void SiegeLightDatabase::SetTimedLightColor( DWORD color )
{
	// Save the timed light color
	m_TimedLightColor = color;

	// Go through all existing lights and look for timed ones to update
	for( LightMap::iterator l = m_Lights.begin(); l != m_Lights.end(); ++l )
	{
		if( (*l).second.m_pDescriptor->m_bOnTimer )
		{
			if( (*l).second.m_pDescriptor->m_Color != color )
			{
				(*l).second.m_pDescriptor->m_Color	= color;
				SetLightDescriptor( (*l).first, *(*l).second.m_pDescriptor );
			}
		}
	}
}

// Find an empty slot in the cache
database_guid SiegeLightDatabase::GenerateRandomLightGuid()
{
	// Seed the random number generator
	Randomize();

	// Generate the random guid and assure its uniqueness
	database_guid guid;
	do
	{
		guid = database_guid( RandomDword() );
	} while( m_Lights.find( guid ) != m_Lights.end() );

	// Return GUID
	return guid;
}

// Recursively insert this light source
bool SiegeLightDatabase::RecursivelyInsertLightSource( const database_guid guid, const SiegePos& position, const SiegePos& direction, LightConstruct& lConstruct, const unsigned int recurseLevel )
{
	database_guid node_guid	= UNDEFINED_GUID;
	if( lConstruct.m_pDescriptor->m_Type == LT_POINT	||
		lConstruct.m_pDescriptor->m_Type == LT_SPOT		)
	{
		node_guid	= position.node;
	}
	else if( lConstruct.m_pDescriptor->m_Type == LT_DIRECTIONAL )
	{
		node_guid	= direction.node;
	}

	// Get the node
	SiegeNode* pNode	= gSiegeEngine.IsNodeValid( node_guid );
	if( !pNode )
	{
		return false;
	}

	// Setup the walk counter
	unsigned int currentRecurse		= 0;
	unsigned int visitCount			= RandomDword();
	unsigned int i					= 0;
	unsigned int size				= 0;

	pNode->SetLastVisited( visitCount );

	// Get the world space position of this light
	vector_3 WorldSpaceLightPosition	= pNode->NodeToWorldSpace( position.pos );
	vector_3 WorldSpaceLightDirection	= pNode->GetCurrentOrientation() * direction.pos;

	// Setup the processing list
	SiegeNodeColl& nodeStack	= gSiegeEngine.m_nodeStack;
	gpassert( nodeStack.empty() );

	nodeStack.push_back( pNode );

	do
	{
		// Get the current size of the stack
		size = nodeStack.size();

		// Go through the nodes in the current list
		for( ; i < size; ++i )
		{
			// Get the current node
			siege::SiegeNode& node	= *nodeStack[i];
			bool bInsertNeighbors	= false;

			if( LightAffectsNode( node, WorldSpaceLightPosition, WorldSpaceLightDirection, *lConstruct.m_pDescriptor ) )
			{
				// Insert light into node
				node.InsertLight( guid, node.WorldToNodeSpace( WorldSpaceLightPosition ), node.GetTransposeOrientation() * WorldSpaceLightDirection, lConstruct.m_pDescriptor );

				// Put this node into the list of affected nodes
				lConstruct.m_Nodes.push_back( gSiegeEngine.NodeCache().UseObject( node.GetGUID() ) );
				bInsertNeighbors	= true;
			}

			// Only put on neighbors if we are below the recursion limit
			if( bInsertNeighbors || ((currentRecurse < recurseLevel) || (lConstruct.m_pDescriptor->m_Type == LT_SPOT)) )
			{
				// Go through the neighbors and put them on the new list
				const SiegeNodeDoorList& ndoors = node.GetDoors();
				for( SiegeNodeDoorConstIter d = ndoors.begin(); d != ndoors.end(); ++d )
				{
					SiegeNode* pNeighborNode	= gSiegeEngine.IsNodeInAnyFrustum( (*d)->GetNeighbour() );
					if( !pNeighborNode || pNeighborNode->GetLastVisited() == visitCount )
					{
						continue;
					}

					pNeighborNode->SetLastVisited( visitCount );

					// Push the node onto the list
					nodeStack.push_back( pNeighborNode );
				}
			}
		}

		// Set our current list to the new list and increment our recursion counter
		currentRecurse++;
	}
	while( size < nodeStack.size() );

	// Clear the stack
	nodeStack.clear();

	if( lConstruct.m_Nodes.empty() )
	{
		return false;
	}

	return true;
}

// Register a light with the database
void SiegeLightDatabase::RegisterLight( const database_guid node_id, const database_guid light_id, const SiegeNodeLight& light )
{
#if !GP_RETAIL

	if( light.m_Descriptor.m_Type == LT_DIRECTIONAL && light.m_Direction.IsZero() )
	{
		gperrorf( ("Directional light %s was inserted from node %s, but it has no direction.  Please fix.", light_id.ToString().c_str(), node_id.ToString().c_str()) );
		return;
	}

#endif

	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Found the light, so we need to update
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( node_id ) );
		(*i).second.m_Nodes.push_back( handle );

		if( memcmp( &light.m_Descriptor, (*i).second.m_pDescriptor, sizeof( LightDescriptor ) ) )
		{
			// Get the node
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

			// Update the descriptor
			node.UpdateLightDescriptor( light_id, (*i).second.m_pDescriptor );
		}
	}
	else
	{
		// Construct a new light holder
		LightConstruct lConstruct;
		lConstruct.m_pDescriptor		= new LightDescriptor;
		(*lConstruct.m_pDescriptor)		= light.m_Descriptor;

		// Put this node on the list
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( node_id ) );
		lConstruct.m_Nodes.push_back( handle );

		// Check to see if this light is timed
		if( light.m_Descriptor.m_bOnTimer && light.m_Descriptor.m_Color != m_TimedLightColor )
		{
			lConstruct.m_pDescriptor->m_Color = m_TimedLightColor;

			// Get the node
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

			// Update the descriptor
			node.UpdateLightDescriptor( light_id, lConstruct.m_pDescriptor );
		}

		// Insert into the mapping
		m_Lights.insert( std::pair< database_guid, LightConstruct >( light_id, lConstruct ) );
	}
}

// Unregister a light with the database
void SiegeLightDatabase::UnRegisterLight( const database_guid node_id, const database_guid light_id )
{
	LightMap::iterator i = m_Lights.find( light_id );
	if( i != m_Lights.end() )
	{
		// Go through the nodes of this light and remove the light
		for( SiegeNodeList::iterator o = (*i).second.m_Nodes.begin(); o != (*i).second.m_Nodes.end(); ++o )
		{
			if( (*o).GetGuid() == node_id )
			{
				(*i).second.m_Nodes.erase( o );

				if( (*i).second.m_Nodes.empty() )
				{
					DestroyLight( light_id );
				}
				return;
			}
		}
	}
}

// Determine whether a light affects the given node
bool SiegeLightDatabase::LightAffectsNode( const SiegeNode& node, const vector_3& worldPosition, const vector_3& worldDirection, const LightDescriptor& description )
{
	if( !node.IsOwnedByAnyFrustum() )
	{
		return false;
	}

	if( description.m_Type == LT_POINT )
	{
		if( SphereIntersectsBox( worldPosition, description.m_OuterRadius, node.GetWorldSpaceMinimumBounds(), node.GetWorldSpaceMaximumBounds() ) )
		{
			return true;
		}
	}
	else if( description.m_Type == LT_DIRECTIONAL )
	{
		return true;
	}
	else if( description.m_Type == LT_SPOT )
	{
		const vector_3& min_box	= node.GetWorldSpaceMinimumBounds();
		const vector_3 HalfDiag	= ( node.GetWorldSpaceMaximumBounds() - min_box ) * 0.5f;

		if( SphereIntersectsCone( worldPosition, worldDirection, description.m_OuterRadius, min_box + HalfDiag, HalfDiag.Length() ) )
		{
			return true;
		}
	}

	return false;
}

}
