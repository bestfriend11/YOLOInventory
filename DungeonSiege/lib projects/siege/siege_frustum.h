#pragma once
#ifndef _SIEGE_FRUSTUM_
#define _SIEGE_FRUSTUM_

/***********************************************************************
**
**							SiegeFrustum
**
**	Maintains a single engine frustum that controls space, loading,
**  and all interactions with other frustums.
**
**  Author:			James Loe
**  Date:			02/14/01
**
***********************************************************************/

namespace siege
{
	// Node fade information for a group of nodes
	struct NODEFADE
	{
		// Identifier for the fade
		int				m_regionID;
		int				m_section;
		int				m_level;
		int				m_object;

		// Type of fade
		FADETYPE		m_fadeType;

		// Fade storage to sync
		unsigned char	m_alpha;
		double			m_startTime;
	};

	struct fade_less
	{
		bool operator()(const NODEFADE& x, const NODEFADE& y) const
		{
			return( x.m_regionID < y.m_regionID );
		}
		bool operator()(const NODEFADE& x, const int y) const
		{
			return( x.m_regionID < y );
		}
		bool operator()(const int y, const NODEFADE& x) const
		{
			return( y < x.m_regionID );
		}
	};

	class SiegeFrustum
	{
		friend class SiegeEngine;

	public:

		// Construction and destruction
		SiegeFrustum( unsigned int idBitfield = 0 );
		virtual ~SiegeFrustum();

		// Persistence
		virtual bool				Xfer( FuBi::PersistContext& persist );

		// Frustum identification information
		const unsigned int			GetIdBitfield()									{ return m_idBitfield; }
		static const unsigned int	GetNextFreeIdBitfield();
		static const unsigned int	GetReservedIdBitfield();

		// Update frustum
		// Return value is the state of the frustum:
		//		true	= Frustum is still updating/loading itself
		//		false	= Frustum is done updating/loading itself for now
		bool						Update( const float secondsElapsed );
		void						RequestUpdate()									{ m_bUpdateFrustum = true; }

		// See if a space update is currently requested
		bool						IsSpaceUpdate()									{ return m_bUpdateSpace; }

		// Set full load frustum (for MP use only)
		void						SetFullLoad( bool bFullLoad )					{ m_bFullLoad = bFullLoad; }
		bool						IsFullLoad()									{ return m_bFullLoad; }

		// Set discovery position state
		void						SetDiscoverySubmission( bool bSubmit )			{ m_bDiscoverySubmit = bSubmit; }
		bool						IsDiscoverySubmission()							{ return m_bDiscoverySubmit; }

		// Positional information
		void						SetPosition( const SiegePos& pos );
		const SiegePos&				GetPosition()									{ return m_frustumPos; }
		const SiegePos&				GetRequestedPosition()							{ return m_reqFrustumPos; }

		// Size information
		void						SetDimensions( const float width, const float depth, const float height );
		const float					GetWidth()										{ return m_frustumWidth; }
		const float					GetHeight()										{ return m_frustumHeight; }
		const float					GetDepth()										{ return m_frustumDepth; }

		void						SetInterestRadius( const float radius )			{ m_interestRadius = radius; }
		const float					GetInterestRadius()								{ return m_interestRadius; }

		// Frustum ownership information
		void						SetFrustumOwned( const unsigned int idBitfield, bool bOwned );
		const unsigned int			GetFrustumOwned()								{ return m_ownerFrustum; }
		bool						IsFrustumOwned()								{ return( m_ownerFrustum != m_idBitfield ); }

		bool						DoesFrustumOwn( const unsigned int idBitfield );
		bool						DoesFrustumOwnAny()								{ return( m_ownedFrustums != 0 ); }
		const DWORD					GetOwnedFrustums()								{ return m_ownedFrustums; }
		void						ClearOwnedFrustums()							{ m_ownedFrustums = 0; }

		// Node helper functions
		bool						OwnsNode( const database_guid& nodeGuid );
		bool						OwnedFrustumOwnsNode( const database_guid& nodeGuid );

		// Get owned frustum node list
		FrustumNodeColl&			GetFrustumNodeColl()							{ return m_ownedNodeColl; }
		FrustumNodeColl&			GetLoadedNodeColl()								{ return m_loadedNodeColl; }
		DiscoveryPosColl&			GetDiscoveryPosColl()							{ return m_discoveryPosColl; }

		// Setup a node fade, usually as a result of a triggered action
		void						NodeFade( int regionID, int section, int level, int object, FADETYPE ft );

		// Update a node's alpha fade level
		void						UpdateFadedNodeAlpha( SiegeNode& node );

		// Update ownership bits based on differences between current set and previous set
		void						UpdateFrustumOwnership();

	private:

		// See if a node intersects the frustum
		bool						NodeIntersectsFrustum( const SiegeNode& node, vector_3& minBox, vector_3& maxBox );

		// Take/Release ownership of a frustum hierarchy
		void						TakeOwnershipOfFrustumHierarchy( DWORD frustumBitfield );
		void						ReleaseOwnedFrustumHierarchy( DWORD frustumBitfield );

		// Build node space for a node
		bool						BuildNodeSpace( SiegeNode& curr_node, SiegeNodeDoor* near_door,	SiegeNode& far_node, bool bNotify = true );

		// Load/Unload a node
		bool						LoadNode( const database_guid& nodeGuid );
		void						UnloadNode( SiegeNode* pNode );

		// Update the current node fades
		void						UpdateNodeFades();

		// Frustum identification
		DWORD						m_idBitfield;

		// Frustum position
		SiegePos					m_frustumPos;
		SiegePos					m_reqFrustumPos;
		bool						m_bUpdateSpace;
		bool						m_bUpdateFrustum;
		bool						m_bFullLoad;
		bool						m_bDiscoverySubmit;
		SiegePos					m_lastUpdatePos;


		// Frustum dimensions
		float						m_frustumWidth;
		float						m_frustumHeight;
		float						m_frustumDepth;
		float						m_interestRadius;

		vector_3					m_minBox;
		vector_3					m_maxBox;

		// Frustum ownership
		unsigned int				m_ownerFrustum;
		DWORD						m_ownedFrustums;

		// List of nodes owned by this frustum
		FrustumNodeColl				m_loadedNodeColl;
		FrustumNodeColl				m_ownedNodeColl;
		NodeColl					m_prevOwnedNodes;

		// Timed update control
		double						m_currentTime;

		// List of active node fades
		NodeFadeColl				m_nodeFadeColl;

		// Discovery information
		DiscoveryPosColl			m_discoveryPosColl;
		SiegePos					m_lastDiscoveryPos;

		// Used mask
		static unsigned int			ms_usedFrustumMask;
	};
}

#endif
