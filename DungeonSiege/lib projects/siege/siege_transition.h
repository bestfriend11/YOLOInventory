#pragma once
#ifndef _SIEGE_TRANSITION_H_
#define _SIEGE_TRANSITION_H_

/******************************************************************************************
**
**								SiegeTransition
**
**		Class responsible for maintaining nodal transition information and
**		persisting it.
**
**		Author:		James Loe
**		Date:		03/03/01
**
******************************************************************************************/

#include "blindcallback.h"


// Transition axis hints
enum eAxisHint
{
	SET_BEGIN_ENUM( AH_, 0 ),

	AH_NONE				= 0,
	AH_XAXIS_POSITIVE	= 1,
	AH_XAXIS_NEGATIVE	= 2,
	AH_YAXIS_POSITIVE	= 3,
	AH_YAXIS_NEGATIVE	= 4,
	AH_ZAXIS_POSITIVE	= 5,
	AH_ZAXIS_NEGATIVE	= 6,

	SET_END_ENUM( AH_ ),
	AH_DWORDALIGN		= 0x7FFFFFFF,
};

FUBI_EXPORT const char* ToString  ( eAxisHint val );
FUBI_EXPORT bool        FromString( const char* str, eAxisHint& val );

namespace siege
{
	typedef CBFunctor2< database_guid, DWORD >				TransitionCompleteCb;

	class SiegeTransition
	{
		struct NodeTransition
		{
			database_guid		m_targetGuid;		// Node that is in transition
			DWORD				m_targetDoor;		// Door through which it is transitioning
			DWORD				m_oldTargetDoor;	// Door through which the target used to connect
			database_guid		m_connectGuid;		// Node that it is connecting to
			DWORD				m_connectDoor;		// Door though which it is connecting
			database_guid		m_oldConnectGuid;	// Node that the target is coming from
			DWORD				m_oldConnectDoor;	// Door through which the target used to be connected
			double				m_currentTime;		// Current gametime
			double				m_startTime;		// Gametime for start of transition
			double				m_endTime;			// Gametime for end of transition
			float				m_percentComplete;	// Percentage complete
			eAxisHint			m_axisHint;			// Axis hint for controlling rotation direction
			DWORD				m_owner;			// Owner GO
			bool				m_bConnect;			// Connect pathing information upon transition completion
		};

		typedef std::map< database_guid, NodeTransition >	TransitionMap;
		typedef stdx::fast_vector< NodeTransition >			TransitionColl;

	public:

		struct NodeDoorInfo
		{
			DWORD				m_nearDoor;
			database_guid		m_farNode;
			DWORD				m_farDoor;
			bool				m_bConnect;
		};

		typedef stdx::fast_vector< NodeDoorInfo >			NodeDoorColl;
		typedef stdx::fast_vector< database_guid >			NodeDisconnectColl;

		struct NodeInfo
		{
			NodeDoorColl		m_nodeDoorColl;
			NodeDisconnectColl	m_nodeDisconnectColl;
		};
		
		typedef std::map< database_guid, NodeInfo >			NodeInfoMap;

		// Construction/Destruction
		SiegeTransition();
		~SiegeTransition();

		// Persistence
		bool Xfer( FuBi::PersistContext& persist );

		// Xfer for synchronization on server
		void XferForSync( FuBi::BitPacker& packer );

		// Clear
		void Clear();

		// Update all active transitions
		void Update( float secondsElapsed );

		// Start a transition
		bool StartTransition( const database_guid targetGuid, const DWORD targetDoor,
							  const database_guid connectGuid, const DWORD connectDoor,
							  const double currentTime, const double startTime, const double endTime,
							  const eAxisHint ahint, const DWORD owner, bool bConnect, bool bForceComplete );

		// Connect two nodes
		bool NodeConnection( const database_guid targetGuid, const DWORD targetDoor,
							 const database_guid connectGuid, const DWORD connectDoor,
							 bool bConnect, bool bForceComplete );

		// Rebuild all of the door adds/removes that this node needs
		bool RebuildDoors( const database_guid nodeGuid );

		// Process needed nodal disconnects for this node
		void ProcessDisconnects( const database_guid nodeGuid );

		// Update all active transitions
		void UpdateTransitionPositions();

		// Update the given node's position/orientation if it is in transition
		void UpdateTransitionPosition( const database_guid nodeGuid );

	private:

		// Update the position/orientation for the given transition info
		void UpdateTransitionPosition( const NodeTransition& trans );

		// Complete a transition
		void CompleteTransition( NodeTransition& transition, bool bRemoveAndNotify );

		// Save out the doors for this node if we need to
		void PersistDoors( const database_guid nodeGuid );

		// Register a nodal disconnect
		void RegisterNodeDisconnect( const database_guid nearGuid, const database_guid farGuid );

		// Transition mapping
		TransitionMap			m_TransitionMap;

		// Collection for holding our complete transitions
		TransitionColl			m_CompleteTransitions;

		// Door mapping
		NodeInfoMap				m_NodeInfoMap;

	};
}

#endif