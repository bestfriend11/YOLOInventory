#pragma once

#ifndef _SIEGE_LIGHT_STRUCTS_H_
#define _SIEGE_LIGHT_STRUCTS_H_

/**************************************************************************************
**
**								SiegeLightStructs
**
**	Author:		James Loe
**	Date:		03/21/00
**
**************************************************************************************/


#include "siege_defs.h"


namespace siege
{

// Light types
enum LIGHT_TYPE
{
	SET_BEGIN_ENUM( LT_, 0 ),

	LT_POINT		= 0,
	LT_DIRECTIONAL	= 1,
	LT_SPOT			= 2,

	SET_END_ENUM( LT_ ),

	LT_DWORD_ALIGN	= 0x7FFFFFFF,
};

const char* ToString( LIGHT_TYPE var );

enum LIGHT_SUBTYPE
{
	SET_BEGIN_ENUM( LS_, 0 ),

	LS_STATIC		= 0,
	LS_DYNAMIC		= 1,

	SET_END_ENUM( LS_ ),

	LS_DWORD_ALIGN	= 0x7FFFFFFF,
};

const char* ToString( LIGHT_SUBTYPE var );

// Structure to hold information about all lights in the engine
struct LightDescriptor
{
	// Type of light
	LIGHT_TYPE		m_Type;

	// Subtype of light
	LIGHT_SUBTYPE	m_Subtype;

	// Color of light in ARGB format
	DWORD			m_Color;

	// Active status of the light
	bool			m_bActive;

	// Parameters that describe the light's effect
	float			m_InnerRadius;
	float			m_OuterRadius;
	float			m_Intensity;

	// How this light affects geometry
	bool			m_bDrawShadow;
	bool			m_bOccludeGeometry;
	bool			m_bAffectsActors;
	bool			m_bAffectsItems;
	bool			m_bAffectsTerrain;
	bool			m_bOnTimer;

	

};

	// Structure that is used internally to track lights
	struct LightConstruct
	{
		// Information about the light
		LightDescriptor*				m_pDescriptor;

		// List of nodes affected by the light
		SiegeNodeList					m_Nodes;
	};

	// Structure used internally by nodes to track a light's effect on them
	struct SiegeNodeLight
	{
		// Unique identifier of this light
		database_guid					m_Guid;

		// Positional information
		vector_3						m_Position;
		vector_3						m_Direction;

		// Local light descriptor
		LightDescriptor					m_Descriptor;

		// Local light effect
		float*							m_pLightEffect;
	};

	typedef std::map< database_guid, LightConstruct >	LightMap;
	typedef std::list< SiegeNodeLight >					SiegeLightList;
	typedef SiegeLightList::iterator					SiegeLightListIter;
	typedef SiegeLightList::const_iterator				SiegeLightListConstIter;

	typedef std::list< LightDescriptor* >				LightList;

}	// end namespace siege

#endif
