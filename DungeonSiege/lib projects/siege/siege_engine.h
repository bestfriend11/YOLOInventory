#pragma once
#ifndef _SIEGE_ENGINE_
#define _SIEGE_ENGINE_

/*******************************************************************************************
**
**									SiegeEngine
**
**		Class that maintains many of the active systems required for SiegeEngine
**		functionality and also provides numerous support functions to aid clients.
**
**		Authors:	Mike Biddlecombe
**					James Loe
**
*******************************************************************************************/

// Siege includes
#include "siege_defs.h"
#include "siege_database_guid.h"
#include "siege_pos.h"
#include "siege_node.h"
#include "siege_mesh.h"
#include "siege_lines.h"
#include "siege_label.h"
#include "siege_cache.h"
#include "siege_database.h"
#include "siege_transition.h"
#include "siege_walker.h"

// Core includes
#include "timeline.h"

// Set the numbers of samples we are keep for the histogram
#define MAX_FRAME_RATE_SAMPLES 512

namespace siege
{
	// Callback for SiegeNode transitions
	typedef CBFunctor0												PRCALLBACK;		// PR = process requestor
	typedef CBFunctor3< const SiegeNode&, unsigned int, bool >		NWMCCALLBACK;	// NWMC = node world membership changed
	typedef CBFunctor2< const SiegeNode&, unsigned int >			NUMCCALLBACK;	// NUMC = node upgraded membership changed
	typedef CBFunctor1< const database_guid >						NLCALLBACK;		// NL = node load
	typedef CBFunctor2< const database_guid, bool >					NSCCALLBACK;	// NSC = node space changed
	typedef CBFunctor0												PORCALLBACK;	// POR = post opaque render

	// Collections of callbacks
	typedef std::vector< NLCALLBACK >								NLCALLBACKCOLL;

	// Callback for door collection
	typedef CBFunctor2< const database_guid, SiegeTransition::NodeDoorColl& >	CollectDoorsCb;

	// Callback for object collection
	typedef CBFunctor4< WorldObjectColl&, const SiegeNode&, eCollectFlags, eRequestFlags >	CollectObjectsCb;

	// Callback for effect collection
	typedef CBFunctor2< WorldEffectColl&, const SiegeNode& >		CollectEffectsCb;

	// Callback for frustum creation
	typedef CBFunctor1wRet< unsigned int, siege::SiegeFrustum* >	FrustumFactoryCb;

	// Collection of frustums
	typedef std::vector< siege::SiegeFrustum* >						SiegeFrustumColl;

	// Node fade information for a single node
	struct SINGLENODEFADE
	{
		// Type of fade
		FADETYPE		m_fadeType;

		// Fade storage to sync
		unsigned char	m_alpha;
		double			m_startTime;
	};

	class SiegeEngine : public Singleton< SiegeEngine >
	{
		friend class SiegeWalker;
		friend class SiegeFrustum;
		friend class SiegeLightDatabase;

	public:

		// Construction and destruction
		SiegeEngine();
		~SiegeEngine();

		// Persistence
		bool Xfer( FuBi::PersistContext& persist );
		bool XferNodeTexMap( FuBi::PersistContext& persist );
		bool XferNodeFlagMap( FuBi::PersistContext& persist );
		void Shutdown( bool destroyAllFrusta = true );

		// Access to the renderer
		const Rapi& Renderer(void) const											{ return *m_pRenderer; }
		Rapi& Renderer(void)														{ return *m_pRenderer; }

		// Access to the node cache
		const siege::cache<siege::SiegeNode>& NodeCache(void) const					{ return *m_pNodeCache; }
		siege::cache<siege::SiegeNode>& NodeCache(void)								{ return *m_pNodeCache; }

		// Access to the mesh cache
		const siege::cache<siege::SiegeMesh>& MeshCache(void) const					{ return *m_pMeshCache; }
		siege::cache<siege::SiegeMesh>& MeshCache(void)								{ return *m_pMeshCache; }

		// Access to the node database
		const siege::NodeDatabase& NodeDatabase(void) const							{ return m_nodeDatabase; }
		siege::NodeDatabase& NodeDatabase(void)										{ return m_nodeDatabase; }

		// Access to the mesh database
		const siege::MeshDatabase& MeshDatabase(void) const							{ return m_meshDatabase; }
		siege::MeshDatabase& MeshDatabase(void)										{ return m_meshDatabase; }

		// Access to the walker
		const siege::SiegeWalker& NodeWalker(void) const							{ return *m_pWalker; }
		siege::SiegeWalker& NodeWalker(void)										{ return *m_pWalker; }

		// Access to the camera
	FEX	const siege::SiegeCamera& GetCamera(void) const								{ return *m_pCamera; }
		siege::SiegeCamera& GetCamera(void)											{ return *m_pCamera; }

		// Access to the compass
		const siege::SiegeCompass& GetCompass(void) const							{ return *m_pCompass; }
		siege::SiegeCompass& GetCompass(void)										{ return *m_pCompass; }

		// Access to the hotpoint database
		const siege::SiegeHotpointDatabase& HotpointDatabase(void) const			{ return *m_pHotpointDatabase; }
		siege::SiegeHotpointDatabase& HotpointDatabase(void)						{ return *m_pHotpointDatabase; }

		// Access to the light database
		const siege::SiegeLightDatabase& LightDatabase(void) const					{ return *m_pLightDatabase; }
		siege::SiegeLightDatabase& LightDatabase(void)								{ return *m_pLightDatabase; }

		// Access to the decal database
		const siege::SiegeDecalDatabase& DecalDatabase(void) const					{ return *m_pDecalDatabase; }
		siege::SiegeDecalDatabase& DecalDatabase(void)								{ return *m_pDecalDatabase; }

		// Access to the pathfinder
		const siege::SiegePathfinder& GetPathfinder(void) const						{ return *m_pPathfinder; }
		siege::SiegePathfinder& GetPathfinder(void)									{ return *m_pPathfinder; }

		// Access to the transition
		const siege::SiegeTransition& GetTransition(void) const						{ return *m_pTransition; }
		siege::SiegeTransition& GetTransition(void)									{ return *m_pTransition; }

		// Access to the mouse shadow
		const siege::SiegeMouseShadow& GetMouseShadow(void) const					{ return *m_pMouseShadow; }
		siege::SiegeMouseShadow& GetMouseShadow(void)								{ return *m_pMouseShadow; }

		// Access to the world space lines
		const siege::worldspace_lines& GetWorldSpaceLines(void) const				{ return m_WorldSpaceLines; }
		siege::worldspace_lines& GetWorldSpaceLines(void)							{ return m_WorldSpaceLines; }

		// Access to the screen space lines
		const siege::screenspace_lines& GetScreenSpaceLines(void) const				{ return m_ScreenSpaceLines; }
		siege::screenspace_lines& GetScreenSpaceLines(void)							{ return m_ScreenSpaceLines; }

		// Access to the node space lines
		const siege::nodespace_lines& GetNodeSpaceLines(void) const					{ return m_NodeSpaceLines; }
		siege::nodespace_lines& GetNodeSpaceLines(void)								{ return m_NodeSpaceLines; }

		// Access to the labels
		const siege::SiegeLabel& GetLabels(void) const								{ return m_Labels; }
		siege::SiegeLabel& GetLabels(void)											{ return m_Labels; }

		// Access to the options
		const siege::SiegeOptions& GetOptions(void) const							{ return *m_pOptions; }
		siege::SiegeOptions& GetOptions(void)										{ return *m_pOptions; }

		// Access to the loader
		const siege::LoadMgr& GetLoadMgr(void) const								{ return *m_loadMgr; }
		siege::LoadMgr& GetLoadMgr(void)											{ return *m_loadMgr; }

		// Load thread control
		bool					IsLoadingThreadRunning() const;
		void					StartLoadingThread();
		void					StopLoadingThread( eStopLoaderType stopType = STOP_LOADER_AND_COMMIT );

		// Returns whether or not the function was called from the main thread
		bool					IsPrimaryThread();

		// Critical section access
		kerneltool::Critical&	GetLogicalCritical()								{ return m_logicalCritical; }
		kerneltool::Critical&	GetHotpointCritical()								{ return m_hotpointCritical; }
		kerneltool::Critical&	GetDecalCritical()									{ return m_decalCritical; }
		kerneltool::Critical&	GetLightCritical()									{ return m_lightCritical; }
		kerneltool::Critical&	GetSpaceCritical()									{ return m_spaceCritical; }
		kerneltool::Critical&	GetMeshCritical()									{ return m_meshCritical; }
		kerneltool::Critical&	GetNodeCreateCritical()								{ return m_nodeCreateCritical; }
		kerneltool::Critical&	GetNodeDestroyCritical()							{ return m_nodeDestroyCritical; }
		kerneltool::Critical&	GetTransitionCritical()								{ return m_transitionCritical; }

		// Tell the engine whether or not it is in a multithreaded environment
		void					SetMultithreaded( bool threaded )					{ m_bMultithreaded = threaded; }
		bool					GetMultithreaded()									{ return m_bMultithreaded; }

		// Attach a renderer to the engine
		void					AttachRenderer( Rapi& pRapi );

		// Engine-owned texture access
		unsigned int			GetCheapShadowTexture() const						{ return m_CheapShadowTex; }
		unsigned int			GetEnvironmentTexture() const						{ return m_EnvironmentTex; }

		// Begin/End a frame
		bool					BeginRender();
		bool					EndRender( bool bCopyToPrimary = true, bool bCopyImmediately = false );

		// Set/Get the frame delta (time between last frame)
		void					SetDeltaFrameTime( double dt )						{ m_DeltaFrameTime = dt; }
		double					GetDeltaFrameTime()									{ return m_DeltaFrameTime; }

		void					SetAbsoluteDeltaFrameTime( double dt )				{ m_AbsoluteDeltaFrameTime = dt; }
		double					GetAbsoluteDeltaFrameTime()							{ return m_AbsoluteDeltaFrameTime; }

		// Culled/Rendered node counts
		int						GetCulledNodeCount()								{ return m_CulledNodeCount; }
		int						GetRenderedNodeCount()								{ return m_RenderedNodeCount; }

		// Get a node space difference vector in the space of pos_orig.node.
		// RESULT = pos_dest.pos - pos_orig.pos
		vector_3				GetDifferenceVector( const SiegePos& pos_orig, const SiegePos& pos_dest );

		// Get the orientation difference in the space of the orig_node
		// orig_orientation * RESULT == dest_orientation
		matrix_3x3				GetDifferenceOrientation( const matrix_3x3& orig_orientation, const database_guid& orig_node,
														  const matrix_3x3& dest_orientation, const database_guid& dest_node );

		// Get the node space difference vector AND orientation in the space of orig_node
		// diff_pos = dest_pos - orig_pos
		// orig_rot * diff_rot == dest_rot
		bool					GetDifferenceVectorOrientation(	const database_guid& orig_node, const vector_3& orig_pos, const Quat& orig_rot,
																const database_guid& dest_node,	const vector_3& dest_pos, const Quat& dest_rot,
																vector_3& diff_pos, Quat& diff_rot );

		// Get the orientation of the requested node
		void					GetNodeOrientation( const database_guid &node, matrix_3x3 &orient ) const;

		// Update the given position by seeing if there is another nearby node that is a better fit
		bool					UpdateNodePosition( SiegePos& siegepos );

		// Get the height and logical flags of liquid at the specified location
		bool					GetLiquidInfo( const SiegePos& pos, float& liquidHeight, eLogicalNodeFlags& liquidFlags );

		// Adjust the given position to the terrain, updating the position and node in the process
		bool					AdjustPointToTerrain( SiegePos& pos, eLogicalNodeFlags flags = LF_IS_FLOOR, DWORD recurseLevel = 1, vector_3* pNormal = NULL );

		// Performs the functionality of AdjustPointToTerrain with the addition of render indices
		bool					AdjustPointToTerrainWithIndex( SiegePos& pos, short& index1, short& index2, short& index3, eLogicalNodeFlags flags = LF_IS_FLOOR, DWORD recurseLevel = 1 );

		// Performs priority based adjustment
		bool					AdjustPointToTerrainPriority( SiegeNode* pStartNode, SiegePos& pos, eLogicalNodeFlags flags = LF_IS_FLOOR, eLogicalNodeFlags secondary_flags = LF_IS_WALL, DWORD recurseLevel = 3, bool bEarlyOut = true );

		// Given a triangles from the BSP, this functions returns the mesh indices that correspond
		void					GetRenderGeometryIndicesFromTriangle( TriNorm& tri, database_guid& node_guid, short& index1, short& index2, short& index3 );

		// Get a triangle index match
		bool					GetTriangleIndexMatch( const vector_3& tri, unsigned int& retIndex, sVertex* pVerts,
													   unsigned int index1, unsigned int index2, unsigned int index3 );

		// Given a ray, see if it intersects any nodes and objects that match the given obj_flags
		bool					RayIntersectsRenderedTerrainAndObjects( const SiegePos& origin, const SiegePos& destination,
																		eCollectFlags obj_flags, bool bCameraBlock );

		// Determines whether the local space around start and end is obstruction free.
		// Returns true if there was an obstruction, false if no obstruction
		bool					CheckForLocalTerrainObstruction( const SiegePos& start, const SiegePos& end );

		// Does large scale terrain obstruction determination at increased cost.
		// Returns true if there was an obstruction, false if no obstruction
		bool					CheckForGlobalTerrainObstruction( const SiegePos& start, const SiegePos& end );

		// Does large scale terrain hit test at increased cost.
		// Returns true if there was an obstruction, false if no obstruction
		// Sets the intersect pos and intersect normal if an intersection was found
		bool					HitTestGlobalTerrain( const SiegePos& start, const SiegePos& end,
													  SiegePos& intersect_pos, vector_3& intersect_norm,
													  const eLogicalNodeFlags flags = LF_IS_ANY );

		// Get the logical node that resides at the given position with the given flags
		SiegeLogicalNode*		GetLogicalNodeAtPosition( const SiegePos& pos, const eLogicalNodeFlags flags );

		// See if this position is blocked
		bool					IsPositionBlocked( SiegeLogicalNode* pLogicalNode, const SiegePos& pos );

		// See if position is allowed for given position and flags.  Position is adjusted to
		// valid position if it is within the height delta given.
		bool					IsPositionAllowed( SiegePos& pos, const eLogicalNodeFlags flags, const float maxHeightDelta );

		// Helpful function that will recursively traverse the known nodal tree the requested
		// number of levels and call the given callback function for each unique node.
		void					NodeTraversalCallback( const database_guid startGuid,
													   CBFunctor2wRet< const SiegeNode&, void*, bool > callBack,
													   unsigned int numRecurse		= 1,
													   void* pAppDefinedData		= NULL );

		// Transition a node to a new position.  Handles the mathematical interpolation, but not
		// any state tracking.  Takes a blind callback which is notified of completion of the transition.
		bool					NodeTransition( const database_guid targetGuid, const DWORD targetDoor,
												const database_guid connectGuid, const DWORD connectDoor,
												const double current_time, const double start_time, const double end_time,
												const eAxisHint ahint, const DWORD owner, bool bConnect, bool bForceComplete );

		// Add the specified door connection and connect the two nodes
		bool					NodeConnection( const database_guid targetGuid, const DWORD targetDoor,
												const database_guid connectGuid, const DWORD connectDoor,
												bool bConnect, bool bForceComplete );

		// Query to see if the pathing is connected between two nodes
		bool					IsNodalPathingConnected( const database_guid& near_node, const database_guid& far_node );

		// Node logical connections
		bool					ConnectSiegeNodes( const database_guid& near_node, const database_guid& far_node );
		bool					DisconnectSiegeNodes( const database_guid& near_node, const database_guid& far_node );
		bool					ConnectLogicalNodes( SiegeLogicalNode* pNearNode, bool bNearToFarConnection,
													 SiegeLogicalNode* pFarNode, bool bFarToNearConnection );

		// Rebuild the doors
		bool					RebuildDoors( const database_guid nodeGuid );

		// Process any disconnects for this node that need to be setup as the result of a transition
		void					ProcessDisconnects( const database_guid nodeGuid );

		// Registration of door collection callback
		void					RegisterCollectDoorsCb( CollectDoorsCb cb )				{ m_collectDoorsCb = cb; }

		// Collect doors using the registered callback
		void					CollectDoors( const database_guid& nodeGuid, SiegeTransition::NodeDoorColl& doorColl );

		// Registration of collection callback
		void					RegisterCollectObjectsCb( CollectObjectsCb cb )			{ m_collectObjectsCb = cb; }

		// Collect object using registered callback
		void					CollectObjects( WorldObjectColl& objects, const SiegeNode& node, eCollectFlags cFlags, eRequestFlags rFlags );

		// Registration of effect collection callback
		void					RegisterCollectEffectsCb( CollectEffectsCb cb )			{ m_collectEffectsCb = cb; }

		// Collect effects using registered callback
		void					CollectEffects( WorldEffectColl& effects, const SiegeNode& node );

		// Registration of transition completion callback
		void					RegisterTransitionCompleteCb( TransitionCompleteCb cb )	{ m_transitionCompleteCb = cb; }

		// Notify callback of node transition completion
		void					TransitionComplete( database_guid targetNode, DWORD owner );

		// Node ownership change callback
		void					RegisterNodeWorldMembershipChangedCb( NWMCCALLBACK cb )	{ m_nwmcCb = cb; }
		void					NotifyNodeWorldMembershipChanged( const SiegeNode& node, unsigned int oldMembership, bool bDelete );

		// Node upgraded ownership change callback
		void					RegisterNodeUpgradedMembershipChangedCb( NUMCCALLBACK cb ) { m_numcCb = cb; }
		void					NotifyNodeUpgradedMembershipChanged( const SiegeNode& node, unsigned int oldMembership );

		// Node load notification callback
		void					RegisterNotifyNodeLoadCb( NLCALLBACK cb );
		void					UnregisterNotifyNodeLoadCb( NLCALLBACK cb );
		void					NotifyNodeLoad( const database_guid nodeGuid );

		// Node space change callback
		void					RegisterNodeSpaceChangedCb( NSCCALLBACK cb )			{ m_nscCb = cb; }
		void					NotifyNodeSpaceChanged( const database_guid nodeGuid, bool bIsElevator = false );

		// Post opaque render callback
		void					RegisterPostOpaqueRenderCb( PORCALLBACK cb )			{ m_porCb = cb; }
		void					NotifyPostOpaqueRender();

		// Debug console output
		void					PrintFrameRate( float gameFPS, float gamespeed, RapiFont& font );
		void					PrintPolyStats( RapiFont& font );
		int						GetTotalTrianglesRendered() { return m_TotalTrisRendered; }

		// Creation/Destruction of frustums
		void					RegisterFrustumFactoryCb( FrustumFactoryCb cb )			{ m_frustumFactoryCb = cb; }
		siege::SiegeFrustum*	CreateNewFrustum( DWORD idBitfield = 0 );
		DWORD					CreateFrustum( DWORD idBitfield = 0 );
		void					DestroyFrustum( DWORD idBitfield, bool bUpdate = true );
		void					DestroyAllFrusta();

		// Frustum update.  Returns true if it is completely loaded, false if it is not.
		bool					UpdateFrustums( float secondsElapsed, bool completeUpdate = false );
		void					UpdateFrustumOwnership();

		// Frustum access
		siege::SiegeFrustum*	GetFrustum( const DWORD idBitfield );
		DWORD					GetNumberOfFrustums()									{ return m_frustumColl.size(); }

		// Render active frustum
		void					SetRenderFrustum( const DWORD idBitfield );
		void					ClearRenderFrustum()									{ SetRenderFrustum( 0 ); }
		DWORD					GetRenderFrustum()										{ return m_renderFrustum; }

		// Frustum node replacment (for transition support)
		void					AddFrustumNodeReplacement( const database_guid& nodeGuid, const database_guid& repGuid );
		void					RemoveFrustumNodeReplacement( const database_guid& nodeGuid );
		bool					IsNodeInTransition( const database_guid& nodeGuid );

		// Utility functions
		unsigned int			GetNodeFrustumOwnedBitfield( const database_guid& nodeGuid );
		SiegeNode*				IsNodeInAnyFrustum( const database_guid& nodeGuid );
		SiegeNode*				IsNodeValid( const database_guid& nodeGuid );
		SiegeNode*				DoesNodeExist( const database_guid& nodeGuid );
		bool					SpaceDiffers( const database_guid& nearGuid, const database_guid& farGuid );

		// Submit a node fade
		void					NodeFade( const database_guid& nodeGuid, FADETYPE ft );
		void					GlobalNodeFade( int regionID, int section, int level, int object, FADETYPE ft );
		const NodeFadeColl&		GetGlobalFadeColl()										{ return m_globalNodeFadeColl; }

		// Fade updating
		void					UpdateFades();

		// Update a node's alpha fade level
		void					UpdateFadedNodeAlpha( SiegeNode& node );

		// Change managed node flags
		void					SetNodeOccludesCamera( const database_guid& nodeGuid, bool bOccludes );
		void					SetNodeBoundsCamera( const database_guid& nodeGuid, bool bBounds );
		void					SetNodeCameraFade( const database_guid& nodeGuid, bool bFade );

		// Fill in the managed flags for this node
		void					GetNodeFlags( SiegeNode* pNode );

		// Change nodal texture animation state
		void					SetNodalTextureAnimationState( const database_guid& nodeGuid, int texIndex, bool bState );
		bool					GetNodalTextureAnimationState( const database_guid& nodeGuid, int texIndex );

		// Change nodal texture animation speed
		void					SetNodalTextureAnimationSpeed( const database_guid& nodeGuid, int texIndex, float speed );
		float					GetNodalTextureAnimationSpeed( const database_guid& nodeGuid, int texIndex );

		// Change nodal texture
		void					ReplaceNodalTexture( const database_guid& nodeGuid, int texIndex, const gpstring& newTexName );

		// Process the texture changes for this node
		void					ProcessNodalTextureStates( SiegeNode& node, bool bFullLoad );

		// Set node fade settings
		void					SetNodeFadeTime( float fadeTime )						{ m_NodeFadeTime = fadeTime; }
		void					SetNodeFadeAlpha( BYTE fadeAlpha )						{ m_NodeFadeAlpha = fadeAlpha; }
		void					SetNodeFadeBlack( BYTE fadeBlack )						{ m_NodeFadeBlack = fadeBlack; }

		// Discovery fog settings
		void					SetDiscoveryRadius( float radius )						{ m_DiscoveryRadius = radius; }
		void					SetDiscoverySampleLength( float length )				{ m_DiscoverySampleLength = length; }

		// Level of detail setting
		void					SetLevelOfDetail( float detail )						{ m_LevelOfDetail = detail; }
		float					GetLevelOfDetail()										{ return m_LevelOfDetail; }

		// Mouse drag select pixel threshold
		void					SetDragSelectThreshold( float dragSelectThreshold )		{ m_DragSelectThreshold = dragSelectThreshold; }
		float					GetDragSelectThreshold()								{ return m_DragSelectThreshold; }

	private:

		// Databases to file mappings
		siege::NodeDatabase					m_nodeDatabase;
		siege::MeshDatabase					m_meshDatabase;

		// A cache of all the SpaceNodes within Siege
		siege::cache<siege::SiegeNode>*		m_pNodeCache;

		// A cache of all the Meshes within Siege
		siege::cache<siege::SiegeMesh>*		m_pMeshCache;

		// The node traversal object
		siege::SiegeWalker*					m_pWalker;

		// Camera
		siege::SiegeCamera*					m_pCamera;

		// Compass
		siege::SiegeCompass*				m_pCompass;

		// The current pipeline down which all rendering commands are issued
		Rapi*								m_pRenderer;

		// Mouse information
		siege::SiegeMouseShadow*			m_pMouseShadow;

		// Line drawing lists
		siege::worldspace_lines				m_WorldSpaceLines;
		siege::screenspace_lines			m_ScreenSpaceLines;
		siege::nodespace_lines				m_NodeSpaceLines;

		// Label drawing list
		siege::SiegeLabel					m_Labels;

		// Options
		siege::SiegeOptions*				m_pOptions;

		// Lights
		siege::SiegeLightDatabase*			m_pLightDatabase;

		// Hotpoints
		siege::SiegeHotpointDatabase*		m_pHotpointDatabase;

		// Decals
		siege::SiegeDecalDatabase*			m_pDecalDatabase;

		// Pathfinder
		siege::SiegePathfinder*				m_pPathfinder;

		// Transition
		siege::SiegeTransition*				m_pTransition;

		// Frustums
		DWORD								m_renderFrustum;
		siege::SiegeFrustumColl				m_frustumColl;
		bool								m_bUpdatingFrustums;

		// Frustum update
		float								m_FrustumUpdateDistance;

		// Loader
		siege::LoadMgr*						m_loadMgr;

		// Whether or not the engine is running in a multithreaded environment
		bool								m_bMultithreaded;

		// Critical section for logical information
		kerneltool::Critical				m_logicalCritical;

		// Critical section for hotpoint information
		kerneltool::Critical				m_hotpointCritical;

		// Critical section for decal information
		kerneltool::Critical				m_decalCritical;

		// Critical section for light information
		kerneltool::Critical				m_lightCritical;

		// Critical section for space information
		kerneltool::Critical				m_spaceCritical;

		// Critical section for mesh information
		kerneltool::Critical				m_meshCritical;

		// Critical sections for node information
		kerneltool::Critical				m_nodeCreateCritical;
		kerneltool::Critical				m_nodeDestroyCritical;

		// Critical section for transition information
		kerneltool::Critical				m_transitionCritical;

		// Critical section for node texture information
		kerneltool::Critical				m_nodeTexCritical;

		// Primary thread ID
		DWORD								m_primaryThreadId;

		// Number of second from the last time time a frame refresh occured
		// used to keep track of varying frame rates
		double								m_DeltaFrameTime;
		double								m_AbsoluteDeltaFrameTime;

		// Texture used for cheap shadow rendering
		unsigned int						m_CheapShadowTex;

		// Texture used for environment mapping
		unsigned int						m_EnvironmentTex;

		// Callback function for door collectin
		CollectDoorsCb						m_collectDoorsCb;

		// Callback function for object collection
		CollectObjectsCb					m_collectObjectsCb;

		// Callback function for effect collection
		CollectEffectsCb					m_collectEffectsCb;

		// Callback function for transition completion
		TransitionCompleteCb				m_transitionCompleteCb;

		// Callback function for frustum creation
		FrustumFactoryCb					m_frustumFactoryCb;

		// Callback function for node world membership changing
		NWMCCALLBACK						m_nwmcCb;

		// Callback function for node upgraded membership changing
		NUMCCALLBACK						m_numcCb;

		// Callback function for node load notification
		NLCALLBACKCOLL						m_nlCbColl;

		// Callback function for node space change notification
		NSCCALLBACK							m_nscCb;

		// Callback function for post opaque render notification
		PORCALLBACK							m_porCb;

		// Single node fade mapping
		SingleNodeFadeMap					m_singleNodeFadeMap;
		NodeFadeColl						m_globalNodeFadeColl;

		// Node flag mapping
		NodeFlagMap							m_nodeFlagMap;

		// Node texture mapping
		NodeTexMap							m_nodeTexMap;

		// Node replacement map
		NodeReplacementMap					m_nodeReplacementMap;

		// Stack cache
		SiegeNodeColl						m_nodeStack;

		// Node fade information
		float								m_NodeFadeTime;
		BYTE								m_NodeFadeAlpha;
		BYTE								m_NodeFadeBlack;

		// Discovery settings
		float								m_DiscoveryRadius;
		float								m_DiscoverySampleLength;

		// Level of detail setting
		float								m_LevelOfDetail;

		// Mouse drag select pixel threshold
		float								m_DragSelectThreshold;

		// Rendered/Culled node counts
		int									m_CulledNodeCount;
		int									m_RenderedNodeCount;

		// Cost Evaluation
		float								m_NemaCostRendered;
		
		// Triangle counts
		int									m_TotalTrisRendered;
		int									m_SiegeTrisRendered;
		int									m_NemaTrisRendered;
		int									m_DecalTrisRendered;

		// Vertex counts
		int									m_TotalVertsRendered;
		int									m_SiegeVertsRendered;
		int									m_NemaVertsRendered;

		// Selected triangle counts
		int									m_SelectedSiegeTrisRendered;
		int									m_SelectedNemaTrisRendered;

		// Selected vertex counts
		int									m_SelectedSiegeVertsRendered;
		int									m_SelectedNemaVertsRendered;

		// Selected SiegeNode name
		gpstring							m_SelectedSiegeNodeName;

		// Don't let anyone else copy
		SiegeEngine(SiegeEngine const &);
		SiegeEngine &operator=(SiegeEngine const &);

		FUBI_SINGLETON_CLASS( siege::SiegeEngine, "The mighty Siege Engine." );
	};
}


#define gSiegeEngine siege::SiegeEngine::GetSingleton()

#	if GP_DEBUG
#	define CHECK_SIEGE_PRIMARY_THREAD_ONLY \
	if ( !gSiegeEngine.IsPrimaryThread() ) \
	{ \
		gperror( "This function can only be called from the primary thread!\n" ); \
	}
#	else // GP_DEBUG
#	define CHECK_SIEGE_PRIMARY_THREAD_ONLY
#	endif // GP_DEBUG


#endif
