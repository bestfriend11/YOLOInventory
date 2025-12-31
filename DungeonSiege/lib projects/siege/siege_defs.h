#pragma once
#ifndef _SIEGE_DEFS_
#define _SIEGE_DEFS_

#include "siege_database_guid.h"
#include "siege_cache.h"
#include "vector_3.h"
#include "gpcoll.h"
#include <list>
#include <map>
#include <set>


//////////////////////////////////////////////////////////
// forward
struct	SiegePos;
struct	StaticObjectTex;

// Type of fade
enum FADETYPE
{
	SET_BEGIN_ENUM( FT_, 0 ),

	FT_NONE			= 0,
	FT_ALPHA		= 1,
	FT_BLACK		= 2,
	FT_IN			= 3,
	FT_INSTANT		= 4,
	FT_IN_INSTANT	= 5,

	SET_END_ENUM( FT_ ),
	FT_ALIGN		= 0x7FFFFFFF,
};

namespace siege
{
	//////////////////////////////////////////////////////////
	// forward
	class	SiegeFrustum;
	class	SiegeNode;
	struct	LightDescriptor;
	class	SiegeNodeIO;
	class	SiegeMesh;
	class	SiegeNodeDoor;
	class	SiegeLogicalNode;
	class	SiegeWalker;

	class	database_guid;
	class	SiegeCamera;
	class	SiegeNode;
	class	SiegeViewFrustum;
	struct	LightDescriptor;

	class	SiegeCompass;
	class	SiegeHotpointDatabase;
	class	SiegePathfinder;
	class	SiegeMouseShadow;
	class	SiegeOptions;
	class	LoadMgr;

	struct	NODEFADE;
	struct	SINGLENODEFADE;
	struct  NODEFLAGS;
	struct	NODETEXSTATE;

	struct  LoadOrderId_;
	typedef const LoadOrderId_* LoadOrderId;

	//////////////////////////////////////////////////////////
	// types
	typedef cache_handle< SiegeNode >						SiegeNodeHandle;
	typedef stdx::fast_vector< SiegeNodeHandle >			SiegeNodeList;
	typedef stdx::fast_vector< SiegeNode* >					SiegeNodeColl;

	typedef stdx::fast_vector< SiegeNodeDoor* >				SiegeNodeDoorList;
	typedef SiegeNodeDoorList::iterator						SiegeNodeDoorIter;
	typedef SiegeNodeDoorList::const_iterator				SiegeNodeDoorConstIter;

	typedef std::map< unsigned int, vector_3 >				SiegeHotpointMap;

	typedef stdx::fast_vector< vector_3 >					TriangleColl;
	
	typedef stdx::fast_vector< NODEFADE >					NodeFadeColl;
	typedef std::map< database_guid, SINGLENODEFADE >		SingleNodeFadeMap;

	typedef std::map< database_guid, NODEFLAGS >			NodeFlagMap;
	typedef std::multimap< database_guid, NODETEXSTATE >	NodeTexMap;

	typedef std::list< SiegeNode* >							FrustumNodeColl;
	typedef std::map< database_guid, database_guid >		FrustumNodeReplacementMap;

	typedef stdx::fast_vector< SiegePos >					DiscoveryPosColl;
	typedef std::multimap< database_guid, vector_3 >		DiscoveryPosMap;

	typedef stdx::fast_vector< StaticObjectTex >			NodeTexColl;
	typedef stdx::fast_vector< LoadOrderId >				LoadIdColl;

	typedef std::map< database_guid, database_guid >		NodeReplacementMap;

	typedef stdx::fast_vector< database_guid >				NodeColl;

	// used by the loader thread
	enum eStopLoaderType
	{
		STOP_LOADER_THREAD_ONLY,	// just pause the thread
		STOP_LOADER_AND_COMMIT,		// pause the thread and commit all requests
		STOP_LOADER_AND_CANCEL,		// pause the thread and delete all requests (careful on this one)
	};
}

#endif
