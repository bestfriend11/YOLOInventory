#pragma once
#ifndef _SIEGE_LIGHT_DATABASE_H_
#define _SIEGE_LIGHT_DATABASE_H_

/************************************************************************************
**
**							SiegeLightDatabase
**
**		Responsible for maintaining a central lookup/database for lights in the
**		Siege Engine.  All external light access outside of Siege should come through
**		this interface.
**
**		Author:		James Loe
**		Date:		03/15/00
**
************************************************************************************/

#include "siege_defs.h"
#include "siege_light_structs.h"

namespace siege
{
	
	// Class definition
class SiegeLightDatabase
{
	friend class SiegeNode;

public:

	// Construction and destruction
	SiegeLightDatabase();
	~SiegeLightDatabase();

	// Insertion of a point light source
	database_guid InsertPointLightSource( const SiegePos& position, const vector_3& color, const database_guid guid = UNDEFINED_GUID,
										  const bool bstatic = false, const bool bactive = true, const float iradius = 0.0f,
										  const float oradius = 1.0f, const float intensity = 1.0f,
										  const bool bdrawshadow = false, const bool boccludegeometry = false,
										  const bool baffectsactors = true, const bool baffectsitems = true, const bool baffectsterrain = true,
										  const bool bontimer = false );

	// Insertion of a direction light source
	database_guid InsertDirectionalLightSource( const SiegePos& direction, const vector_3& color, const database_guid guid = UNDEFINED_GUID,
											    const bool bstatic = false, const bool bactive = true, const float intensity = 1.0f, 
											    const bool bdrawshadow = false, const bool boccludegeometry = false,
												const bool baffectsactors = true, const bool baffectsitems = true, const bool baffectsterrain = true,
												const bool bontimer = false );

	// Insertion of a spot light source
	database_guid InsertSpotLightSource( const SiegePos& position, const SiegePos& direction, const vector_3& color, 
										 const database_guid guid = UNDEFINED_GUID, const bool bstatic = false, const bool bactive = true,
										 const float iradius = 0.0f, const float oradius = 1.0f, const float intensity = 1.0f,
										 const bool bdrawshadow = false, const bool boccludegeometry = false,
										 const bool baffectsactors = true, const bool baffectsitems = true, const bool baffectsterrain = true,
										 const bool bontimer = false );

	// Get information about a light
	LightDescriptor		GetLightDescriptor( const database_guid light_id );
	bool				HasLightDescriptor( const database_guid light_id );

	vector_3			GetLightWorldSpacePosition( const database_guid light_id );
	vector_3			GetLightWorldSpaceDirection( const database_guid light_id );

	SiegePos			GetLightNodeSpacePosition( const database_guid light_id );
	SiegePos			GetLightNodeSpaceDirection( const database_guid light_id );

	// Set information about the light.  Note position and direction are in nodespace.
	void				SetLightDescriptor( const database_guid light_id, const LightDescriptor& desc );
	void				SetLightPosition( const database_guid light_id, const SiegePos& position );
	void				SetLightDirection( const database_guid light_id, const SiegePos& direction );
	void				SetLightPositionAndDirection( const database_guid light_id,
													  const SiegePos& position,
													  const SiegePos& direction );

	// Destroy a light that is currently in the database
	void				DestroyLight( const database_guid light_id );

	// Get the lights that can affect the given position
	LightList			GetActiveLightsAtPosition( const SiegePos& position );

	// Get the map of lights in the database
	LightMap&			GetLightDatabaseMap()		{ return m_Lights; }

	// Manually update the lighting for a given node
	void				UpdateLighting( const database_guid node_id );

	// Clear the database
	void				Clear();

	// Set the directional light color
	void				SetTimedLightColor( DWORD color );

private:

	// Array of light information
	LightMap			m_Lights;

	// Saved timed light color
	DWORD				m_TimedLightColor;

	// Find an empty slot in the cache
	database_guid		GenerateRandomLightGuid();

	// Recursively insert this light source
	bool				RecursivelyInsertLightSource( const database_guid guid, const SiegePos& position, const SiegePos& direction, LightConstruct& lConstruct, const unsigned int recurseLevel = 2 );

	// Register a light with the database
	void				RegisterLight( const database_guid node_id, const database_guid light_id, const SiegeNodeLight& light );

	// Unregister a light with the database
	void				UnRegisterLight( const database_guid node_id, const database_guid light_id );

	// Determine whether a light affects the given node
	bool				LightAffectsNode( const SiegeNode& node, const vector_3& worldPosition, const vector_3& worldDirection, const LightDescriptor& description );

};

}

#endif