#pragma once
/*=======================================================================================

  WorldTerrain and related classes

  date		:	Nov. 17, 1999

  author(s)	:	James Loe, Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __WORLDTERRAIN_H
#define __WORLDTERRAIN_H



#include "KernelTool.h"
#include "World.h"

#include <map>


class WorldTerrain;
class WorldMap;
class FastFuelHandle;
enum eAxisHint;


namespace siege
{
	class SiegeNode;
	class SiegeLogicalNode;
	enum eLogicalNodeFlags;
}


struct SiegePos;


struct TTMessages	//	terrain transition messages
{
	FUBI_POD_CLASS( TTMessages );

	TTMessages()
	{
		Init();
	}

	void Init();

	FUBI_VARIABLE		( eWorldEvent,	m_,		BeginEvent,			"Event to send at the beginning of terrain transition." );
	FUBI_VARIABLE		( Scid,			m_,		BeginSendTo,		"Scid of content to send begin message to." );
	FUBI_VARIABLE		( float,		m_,		BeginDelay,			"Delay after begin tansition after which to send beign event." );

	FUBI_VARIABLE		( eWorldEvent,	m_,		EndEvent,			"Event to send at the end of terrain transition." );
	FUBI_VARIABLE		( Scid,			m_,		EndSendTo,			"Scid of content to send end message to." );
	FUBI_VARIABLE		( float,		m_,		EndDelay,			"Delay after end tansition after which to send end event." );
};

FEX TTMessages & MakeTTMessages();


/*=======================================================================================

  WorldTerrain

  author:	James Loe, Bartosz Kijanka

  date:		Nov. 17, 1999

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class WorldTerrain : public Singleton <WorldTerrain>
{

public:

	WorldTerrain();

	~WorldTerrain();

	bool Xfer( FuBi::PersistContext& persist );
	
	// Synchronize terrain database information to machine
	void SSyncOnMachine( DWORD machineId );

	//////////////////////////////////////////////////////////////////////
	//	frusta

	// Register frustum
FEX FuBiCookie RCRegisterWorldFrustumOnMachine( unsigned int machineId, PlayerId playerId, FrustumId frustumId );
	bool       RegisterWorldFrustum( PlayerId playerId, FrustumId frustumId );

	FUBI_MEMBER_SET( RCRegisterWorldFrustumOnMachine, +SERVER );

	// Registration helper for the server
	FuBiCookie SRegisterWorldFrustum( FrustumId& frustumId, Player* player );

	// Performs actual destruction
	void DestroyWorldFrustum( FrustumId frustumId );

	// Performs an instantaneous complete world load
	void ForceWorldLoad( bool forceThreadStart = false, bool queuedLoad = false );

	//////////////////////////////////////////////////////////////////////
	//	node scripting functions

	// Initiate a nodal transition (elevators, platforms, any type of moving node)
FEX	void SRequestNodeTransition( SiegeId targetNode, const DWORD targetDoor,
								 SiegeId connectNode, const DWORD connectDoor,
								 const float duration, const eAxisHint ahint, Goid owner,
								 bool bConnect, bool bForceComplete, TTMessages * msg );

FEX	void SRequestNodeTransition( SiegeId targetNode, const DWORD targetDoor,
								 SiegeId connectNode, const DWORD connectDoor,
								 const float duration, const eAxisHint ahint, Goid owner,
								 bool bConnect, bool bForceComplete )
	{
		SRequestNodeTransition( targetNode, targetDoor,
								connectNode, connectDoor,
								duration, ahint, owner,
								bConnect, bForceComplete, NULL );
	}

FEX	FuBiCookie RCRequestNodeTransition(	SiegeId targetNode, const DWORD targetDoor,
										SiegeId connectNode, const DWORD connectDoor,
										const double startTime, const double endTime,
										const eAxisHint ahint, Goid owner,
										bool bConnect, bool bForceComplete, TTMessages * msg );

FEX	FuBiCookie RCRequestNodeTransition(	SiegeId targetNode, const DWORD targetDoor,
										SiegeId connectNode, const DWORD connectDoor,
										const double startTime, const double endTime,
										const eAxisHint ahint, Goid owner, bool bConnect,
										bool bForceComplete )
	{
		return RCRequestNodeTransition(	targetNode, targetDoor,
										connectNode, connectDoor,
										startTime, endTime,
										ahint, owner,
										bConnect, bForceComplete, NULL );
	}

	// Request the connection of two nodes
FEX void		SRequestNodeConnection( SiegeId targetNode, const DWORD targetDoor,
										SiegeId connectNode, const DWORD connectDoor,
										bool bConnect, bool bForceComplete );

FEX FuBiCookie	RCRequestNodeConnection( SiegeId targetNode, const DWORD targetDoor,
										 SiegeId connectNode, const DWORD connectDoor,
										 bool bConnect, bool bForceComplete );

	// Query path connection information
FEX bool		IsNodalPathingConnected( SiegeId targetNode, SiegeId connectNode );

	// Request that an object block the area it sits on
	void		RequestObjectUnblock( Goid object );
	void		RequestObjectBlock( Goid object );

	// Find the local index of a nodal based texture
FEX int			FindNodalTextureIndex( SiegeId node, const gpstring& texName );

	// Request nodal texture state change
FEX void		SSetNodalTextureAnimationState( SiegeId node, int tex_index, bool bState );
FEX FuBiCookie	RCSetNodalTextureAnimationState( SiegeId node, int tex_index, bool bState );
FEX bool		GetNodalTextureAnimationState( SiegeId node, int tex_index );

	// Request nodal texture speed change
FEX void		SSetNodalTextureAnimationSpeed( SiegeId node, int tex_index, float speed );
FEX FuBiCookie	RCSetNodalTextureAnimationSpeed( SiegeId node, int tex_index, float speed );
FEX float		GetNodalTextureAnimationSpeed( SiegeId node, int tex_index );

	// Request nodal texture replacement
FEX void		SReplaceNodalTexture( SiegeId node, int tex_index, const gpstring& newTexName );
FEX FuBiCookie	RCReplaceNodalTexture( SiegeId node, int tex_index, const gpstring& newTexName );

	void		SNodeFade( const siege::database_guid& nodeGuid, FADETYPE ft );
FEX	FuBiCookie	RCNodeFade( const UINT32 nodeGuid, UINT8 ft );

	void		SGlobalNodeFade( int regionID, int section, int level, int object, FADETYPE ft );
FEX	FuBiCookie	RCGlobalNodeFade( int regionID, int section, int level, int object, UINT8 ft );
FEX FuBiCookie	RCSyncGlobalNodeFade( DWORD machineId, int regionID, int section, int level, int object, UINT8 ft );

	// Managed node flags
	void		SSetNodeOccludesCamera( const siege::database_guid nodeGuid, bool bOccludes );
FEX FuBiCookie  RCSetNodeOccludesCamera( const UINT32 nodeGuid, bool bOccludes );
 
	void		SSetNodeBoundsCamera( const siege::database_guid nodeGuid, bool bBounds );
FEX FuBiCookie  RCSetNodeBoundsCamera( const UINT32 nodeGuid, bool bBounds );

	void		SSetNodeCameraFade( const siege::database_guid nodeGuid, bool bFade );
FEX FuBiCookie  RCSetNodeCameraFade( const UINT32 nodeGuid, bool bFade );


	//////////////////////////////////////////////////////////////////////

private:

	// Synchronize the nodal transitions between server and given machine
	void		SSyncTransitionsOnMachine( DWORD machineId );
FEX FuBiCookie	RCSyncTransitionOnMachine( DWORD machineId, const_mem_ptr data );

	// Synchronize the global fades between server and given machine
	void		SSyncGlobalFadesOnMachine( DWORD machineId );

	// Synchronize the nodal texture states between server and given machine
	void		SSyncNodalTexStatesOnMachine( DWORD machineId );
FEX FuBiCookie	RCSyncNodalTexStatesOnMachine( DWORD machineId, const_mem_ptr data, const_mem_ptr index );

	// Synchronize the nodal flag states between server and given machine
	void		SSyncNodalFlagStatesOnMachine( DWORD machineId );
FEX FuBiCookie	RCSyncNodalFlagStatesOnMachine( DWORD machineId, const_mem_ptr data, const_mem_ptr index );
	
	//////////////////////////////////////////////////////////////////////
	//	node connectivity

	// Callback for completion of a node transition
	void NodeTransitionComplete( siege::database_guid targetNode, DWORD owner );

	// Callback for blocking volume requests (doors, static object, etc)
	bool BlockRequestCallBack( const siege::SiegeNode& node, void* pAppDefined );

	//////////////////////////////////////////////////////////////////////
	//	data

	WorldMap*							m_pMap;

	FUBI_SINGLETON_CLASS( WorldTerrain, "System responsible for handling world loading and caching" );
	SET_NO_COPYING( WorldTerrain );
};

#define gWorldTerrain WorldTerrain::GetSingleton()

#endif
