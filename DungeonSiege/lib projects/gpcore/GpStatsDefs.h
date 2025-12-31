//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpStatsDefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains forward declarations and macros for using the stats
//             system. Always include this instead of gpstats.h.
//
//             Note: implementations for any of these functions are
//             in GpStats.cpp.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPSTATSDEFS_H
#define __GPSTATSDEFS_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"

//////////////////////////////////////////////////////////////////////////////
// macro definitions

#if GP_ENABLE_STATS

#define GPSTATS_GROUP( GROUP ) \
		gpstats::AutoGroup GPSTATS_AutoGroup( GROUP )
#define GPSTATS_SYSTEM( SYSTEM ) \
		gpstats::AutoSystem GPSTATS_AutoSystem( SYSTEM )
#define GPSTATS_SAMPLE( FUNC )				\
		if ( gpstats::AreStatsEnabled() )	\
		{									\
			gpstats::FUNC;					\
		}
#define GPSTATS_ONLY( X ) X

#else // GP_ENABLE_STATS

#define GPSTATS_GROUP( GROUP )
#define GPSTATS_SYSTEM( SYSTEM )
#define GPSTATS_SAMPLE( FUNC )
#define GPSTATS_ONLY( X )

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////

#if !GP_RETAIL || GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////
// enum eGroup declaration

enum eGroup
{
	SET_BEGIN_ENUM( GROUP_, 0 ),

	GROUP_IDLE,						// idle time not to be counted
	GROUP_MISC,						// anything not covered by other types
	GROUP_RENDER_UI,				// rendering the ui
	GROUP_UPDATE_UI,				// updating the ui
	GROUP_RENDER_FX,				// rendering effects and weather
	GROUP_UPDATE_FX,				// updating effects and weather
	GROUP_PHYSICS,					// physics simulation
	GROUP_RENDER_TERRAIN,			// rendering the terrain
	GROUP_LIGHT_TERRAIN,			// lighting the terrain
	GROUP_RENDER_MODELS,			// rendering the models
	GROUP_LIGHT_MODELS,				// lighting the models
	GROUP_DEFORM_MODELS,			// deforming the vertices of the models
	GROUP_ANIMATE_MODELS,			// animating the bones of the models
	GROUP_RENDER_MISC,				// rendering misc stuff (wait for flip, etc.)
	GROUP_AI_QUERY,					// doing any kind of spatial query
	GROUP_AI_MISC,					// misc ai related tasks
	GROUP_GODB,						// misc godb logic
	GROUP_PATH_FIND,				// finding paths
	GROUP_PATH_FOLLOW,				// following paths
	GROUP_LOAD_GOS,					// loading game objects
	GROUP_LOAD_TERRAIN,				// loading terrain
	GROUP_LOAD_TEXTURE,				// loading textures
	GROUP_NET,						// any net related activity
	GROUP_SKRIT,					// any skrit running and its overhead
	GROUP_FUEL,						// any fuel related activity (loading, unloading, walking)
	GROUP_TRIGGERS,					// trigger evaluation & execution

	SET_END_ENUM( GROUP_ ),
};

const char* ToScreenString( eGroup e );

//////////////////////////////////////////////////////////////////////////////
// enum eSystemProperty declaration

enum eSystemProperty
{
	// special properties
	SP_ALL						= 0xFFFFFFFF,
	SP_NONE						= 0,
	SP_GLOBAL					= 1 <<  0,

	// ai
	SP_AI_MIND					= 1 <<  1,
	SP_AI_QUERY					= 1 <<  2,
	SP_AI						= 1 <<  3,

	// net
	SP_NET_SEND					= 1 <<  4,
	SP_NET_RECEIVE				= 1 <<  5,
	SP_NET						= 1 <<  6,

	// logic
	SP_HANDLE_MESSAGE			= 1 <<  7,
	SP_PHYSICS					= 1 <<  8,

	// effects
	SP_EFFECTS_WEATHER			= 1 <<  9,
	SP_EFFECTS_OBJECT			= 1 << 10,
	SP_EFFECTS					= 1 << 11,

	// rendering
	SP_DRAW_TERRAIN				= 1 << 12,
	SP_DRAW_GO					= 1 << 13,
	SP_DRAW_CALC_TERRAIN		= 1 << 14,
	SP_DRAW_CALC_GO				= 1 << 15,
	SP_DRAW						= 1 << 16,

	// loading
	SP_LOAD_TERRAIN				= 1 << 17,
	SP_LOAD_GO					= 1 << 18,
	SP_LOAD						= 1 << 19,

	// other
	SP_SKRIT					= 1 << 20,
	SP_FUEL						= 1 << 21,
	SP_UI						= 1 << 22,
	SP_STATIC_CONTENT			= 1 << 23,
	SP_MISC						= 1 << 24,
	SP_MISC_NOGROUP				= 1 << 25,

	// triggers
	SP_TRIGGER_EVAL				= 1 << 26,
	SP_TRIGGER_EXEC				= 1 << 27,
	SP_TRIGGER_MISC				= 1 << 28,

	SP_GODB						= 1 << 29,


	// this one must always be last, and all must be in sequence with no gaps
	SP_LAST						= 1 << 30,
};

MAKE_ENUM_BIT_OPERATORS( eSystemProperty );

const char* ToString      ( eSystemProperty e );
gpstring    ToFullString  ( eSystemProperty e );
bool        FromString    ( const char* str, eSystemProperty& e );
bool        FromFullString( const char* str, eSystemProperty& e );

inline eSystemProperty MakeSystemPropertyFromIndex( int d )  {  return ( (eSystemProperty)( 1 << d ) );  }

//////////////////////////////////////////////////////////////////////////////
// enum eSystemStage declaration

enum eSystemStage
{
	STAGE_INVALID = -1,			// default stage (never valid)

	SET_BEGIN_ENUM( STAGE_, 0 ),

	// init/shutdown pipe
	STAGE_STARTUP,				// starting up app
	STAGE_SHUTDOWN,				// shutting down app
	STAGE_LOAD_GAME,			// loading a saved game
	STAGE_SAVE_GAME,			// saving a game
	STAGE_LOAD_MAP,				// loading a map

	// runtime stages
	STAGE_FRONT_END,			// playing in the front end
	STAGE_IN_GAME,				// playing in the game itself
	STAGE_USER_SAMPLE,			// special user-requested sampling period

	SET_END_ENUM( STAGE_ ),
};

const char* ToString  ( eSystemStage e );
bool        FromString( const char* str, eSystemStage& e );

//////////////////////////////////////////////////////////////////////////////

#endif // !GP_RETAIL || GP_ENABLE_STATS

#if GP_ENABLE_STATS

namespace gpstats  {  // begin of namespace gpstats

//////////////////////////////////////////////////////////////////////////////
// GpStats API declaration

// Helper structs.

	struct AutoSystem
	{
		eSystemProperty m_System;

		AutoSystem( eSystemProperty system );
	   ~AutoSystem( void );
	};

	struct AutoGroup
	{
		eGroup m_Group;

		AutoGroup( eGroup group );
	   ~AutoGroup( void );
	};

// Helper functions.

	bool            AreStatsEnabled ( void );
	void            SetCurrentStage ( eSystemStage stage );
	eSystemStage    GetCurrentStage ( void );
	void            EnterGroup      ( eGroup group );
	void            LeaveGroup      ( eGroup group );
	eGroup          GetCurrentGroup ( void );
	void            EnterSystem     ( eSystemProperty system );
	void            LeaveSystem     ( eSystemProperty system );
	eSystemProperty GetCurrentSystem( void );

	void AddFrame           ( void );

	void AddMemAlloc        ( eSystemProperty system, size_t size );
	void AddMemDealloc      ( eSystemProperty system, size_t size, size_t waste );
	void AddDxCreateSurface ( eSystemProperty system, int width, int height, int bytesPerPixel, bool hasMips );
	void AddDxReleaseSurface( eSystemProperty system, int width, int height, int bytesPerPixel, bool hasMips );
	void AddDiyReserve      ( eSystemProperty system, int size );
	void AddDiyFree         ( eSystemProperty system, int size );

	void AddDiyCommit       ( int size );
	void AddFileRead        ( int size );
	void AddFileDecompress  ( int size );
	void AddGoGlobalAlloc   ( void );
	void AddGoCloneSrcAlloc ( void );
	void AddGoLocalAlloc    ( void );
	void AddGoLodfiAlloc    ( void );
	void AddGoDealloc       ( void );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace gpstats

#endif // GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////

#endif  // __GPSTATSDEFS_H

//////////////////////////////////////////////////////////////////////////////
