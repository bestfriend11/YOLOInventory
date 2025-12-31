#pragma once
#ifndef _SIEGE_NODE_
#define _SIEGE_NODE_

/******************************************************************************************
**
**								SiegeNode
**
**		Main atomic unit that comprises the engine.
**
**		Author:		Mike Biddlecombe, James Loe
**
******************************************************************************************/

#include "siege_defs.h"
#include "vector_3.h"
#include "matrix_3x3.h"
#include "space_3.h"
#include "filesys.h"

#include "siege_cache_handle.h"
#include "siege_light_database.h"
#include "siege_mesh.h"
#include "siege_logical_node.h"
#include "siege_decal_database.h"


namespace siege
{
	// Node flag information
	struct NODEFLAGS
	{
		bool			m_bOccludesCamera;
		bool			m_bOccludesChanged;

		bool			m_bBoundsCamera;
		bool			m_bBoundsChanged;

		bool			m_bCameraFade;
		bool			m_bCameraChanged;
	};

	// Node texture state information
	struct NODETEXSTATE
	{
		// Texture index
		int				m_textureIndex;

		// State
		bool			m_bStateChanged;
		bool			m_bState;

		// Speed
		bool			m_bSpeedChanged;
		float			m_bSpeed;

		// Replacement
		bool			m_bReplaced;
		gpstring		m_replacementTexture;
	};


	class SiegeNode
	{
		friend class SiegeEngine;
		friend class SiegeLightDatabase;
		friend class SiegeDecalDatabase;
		friend class SiegeWalker;
		friend class SiegeTransition;

	public:

		// Construction/destruction
		SiegeNode();
		~SiegeNode(void);

#if		!GP_RETAIL
		// Checksum verification
		bool						VerifyChecksum();

		// Set the spatial parent
		void						SetSpatialParent( database_guid parentGuid ){ m_SpatialParent = parentGuid; }
#endif

		// Loading
		void						Load( const char *pData );
		void						UpgradeLoad();
		void						DowngradeLoad();

		// Identifier
		const database_guid&		GetGUID() const								{ return m_Guid; }
		void						SetGUID( const database_guid& g )			{ m_Guid = g; }
		void						SetFileHandle( FileSys::FileHandle handle )	{ m_fileHandle = handle; }

		const database_guid&		GetMeshGUID() const							{ return m_MeshGUID; }
		SiegeMeshHandle&			GetMeshHandle()	const						{ return *m_pMeshHandle; }
		void						SetMeshGUID( const database_guid& g, bool bFullLoad = true );

		// Texture information
		const NodeTexColl&			GetTextureListing() const					{ return m_TextureListing; }
		int							FindTextureIndex( const gpstring& texName );
		void						ReplaceTexture( const int tex_index, const gpstring& newTexName, bool bFullLoad );

		// Does this node contain alpha stages?
		bool						HasAlphaStages()							{ return m_bHasAlphaStages; }
		bool						HasNoAlphaSortStages()						{ return m_bNoAlphaSortStages; }

		// Set environment map for this node
		void						SetEnvironmentMap( DWORD em )				{ m_EnvironmentMap = em; }
		DWORD						GetEnvironmentMap() const					{ return m_EnvironmentMap; }

		// Fading identification
		int							GetRegionID() const							{ return m_RegionID; }
		void						SetRegionID( int regionid )					{ m_RegionID = regionid; }

		int							GetSection() const							{ return m_Section; }
		void						SetSection( int section )					{ m_Section = section; }

		int							GetLevel() const							{ return m_Level; }
		void						SetLevel( int level )						{ m_Level = level; }

		int							GetObject() const							{ return m_Object; }
		void						SetObject( int object )						{ m_Object = object; }

		DWORD						GetNodeAlpha() const						{ return min_t( m_Alpha, m_CameraAlpha ); }

		void						SetNodeAlpha( DWORD alpha )					{ m_Alpha = alpha; }
		void						SetCameraNodeAlpha( DWORD alpha )			{ m_CameraAlpha = alpha; }

		float						GetCameraDistance() const					{ return m_CameraDistance; }
		void						SetCameraDistance( float dist )				{ m_CameraDistance = dist; }

		// Door info
		const SiegeNodeDoorList&	GetDoors() const							{ return m_DoorList; }
		SiegeNodeDoor*				GetDoorByID( const DWORD id );

		// Request the addition of a door
		void						AddDoor( const DWORD init_id, const vector_3& init_center, const matrix_3x3& init_orient,
											 const database_guid& init_neighbour, const DWORD init_neighbour_id, bool bConnect );

		void						AddDoor( const DWORD init_id, const database_guid& init_neighbour, const DWORD init_neighbour_id, bool bConnect );

		// Request the removal of a door
		void						RemoveDoor( const DWORD id );

		// Keep track of node was visit counter
		void						SetLastVisited( DWORD v )					{ m_LastVisited = v; }
		DWORD						GetLastVisited() const						{ return m_LastVisited; }

		// Keep track of an all purpose visit counter
		void						SetAPVisited( DWORD v )						{ m_APVisited = v; }
		DWORD						GetAPVisited() const						{ return m_APVisited; }

		// Set terrain lit ambient value
		void						SetAmbientColor( DWORD color )				{ m_AmbientColor = color; }

		// Set object lit ambient value
		void						SetActorAmbientColor( DWORD color )			{ m_ActorAmbientColor = color; }

		// Set object lit ambient value
		void						SetObjectAmbientColor( DWORD color )		{ m_ObjectAmbientColor = color; }

		// Relative to the target node, used to compute world space and draw
		const vector_3&				GetCurrentCenter() const					{ return m_CurrentCenter; }
		void						SetCurrentCenter( const vector_3& c )		{ m_CurrentCenter = c; }
		const matrix_3x3&			GetCurrentOrientation()	const				{ return m_CurrentOrientation; }
		const matrix_3x3&			GetTransposeOrientation() const				{ return m_TransposeOrientation; }
		void						SetCurrentOrientation( const matrix_3x3& o ){ m_CurrentOrientation = o; 
																				  o.Transpose( m_TransposeOrientation );
																				  NodeToWorldBox( m_minBox, m_maxBox, m_wminBox, m_wmaxBox );
																				  m_wSphereCenter = (m_wmaxBox + m_wminBox) * 0.5f; }

		bool						HasBoundaryMesh()							{ return m_MeshGUID.IsValid(); }

		void						SetVisible( bool bVisible )					{ m_bVisible = bVisible; }
		bool						GetVisible() const							{ return m_bVisible; }

		void						SetVisibleThisFrame( bool bVisible)			{ m_bVisibleThisFrame = bVisible; }
		bool						GetVisibleThisFrame(void) const				{ return m_bVisibleThisFrame; }

		void						SetLitThisFrame( bool bLit )				{ m_bLit = bLit; }
		bool						GetLitThisFrame() const						{ return m_bLit; }

		// Hit testing
		bool						HitTest( const vector_3& RayPos, const vector_3& RayDir,
											 float& RayT, vector_3& FaceNormal, const eLogicalNodeFlags& flags ) const;
		bool						HitTestTri( const vector_3& RayPos, const vector_3& RayDir,
												float& RayT, TriNorm& Triangle, const eLogicalNodeFlags& flags ) const;
		bool						HitTestFlags( const vector_3& RayPos, const vector_3& RayDir,
												  float& RayT, eLogicalNodeFlags& flags ) const;

		// FRAME OF REFERENCE CONVERSION
		inline						vector_3 WorldToNodeSpace(const vector_3& world_position) const
									{
										return( m_TransposeOrientation * ( world_position - m_CurrentCenter ) );
									}

		inline						vector_3 NodeToWorldSpace(const vector_3& node_position) const
									{
										vector_3 worldPos( DoNotInitialize );
										m_CurrentOrientation.Transform( worldPos, node_position );
										worldPos	+= m_CurrentCenter;
										return( worldPos );
									}

		inline void					WorldToNodeBox( const vector_3& minBox, const vector_3& maxBox,
													vector_3& rMinBox, vector_3& rMaxBox ) const
									{
										TransformAxisAlignedBox( m_TransposeOrientation, vector_3::ZERO,
																 minBox - m_CurrentCenter, maxBox - m_CurrentCenter,
																 rMinBox, rMaxBox );
									}

		inline void					NodeToWorldBox( const vector_3& minBox, const vector_3& maxBox,
													vector_3& rMinBox, vector_3& rMaxBox ) const
									{
										TransformAxisAlignedBox( m_CurrentOrientation, m_CurrentCenter, minBox, maxBox, rMinBox, rMaxBox );
									}


		// Calculate whether or not the given position is in this node
		// Note that the position you pass must be in this node's space
		bool						IsPointInNode( const vector_3& position ) const;
		bool						IsWorldPointInNode( const vector_3& position ) const;

		// Request a static accumulate
		void						RequestLightingAccum( bool req )			{ m_bAccumulateRequested = req; }
		bool						IsLightAccumRequested() const				{ return m_bAccumulateRequested; }

		// Get lighting information in it's current state
		const DWORD*				GetLightingData() const						{ return m_pVertexColors; }
		const DWORD*				GetStaticLightingData() const				{ return m_pStaticVertexColors; }

		// Get the number of vertices that are lit
		const DWORD					GetNumLitVertices() const					{ return m_numVertices; }

		// Set the number of lit vertices manually, for dealing with buffer overruns
		void						SetNumLitVertices( DWORD numVertices )		{ m_numVertices = numVertices; }

		// Logical node accessors
		SiegeLogicalNode**			GetLogicalNodes() const						{ return m_pLogicalNodes; }
		DWORD						GetNumLogicalNodes() const					{ return m_numLogicalNodes; }

		// Setting logical information for dynamic building
		void						SetNumLogicalNodes( DWORD nl )				{ m_numLogicalNodes = nl; }
		void						SetLogicalNodes( SiegeLogicalNode** pNodes ){ m_pLogicalNodes = pNodes; }

		// Loaded state
		bool						IsLoadedPhysical() const					{ return m_bLoadedPhysical; }
		void						SetIsLoadedPhysical( bool bFlag )			{ m_bLoadedPhysical = bFlag; }

		bool						LoadedLastUpdate() const					{ return( m_bLoadedLastUpdate ); }
		void						SetLoadedLastUpdate( bool bFlag )			{ m_bLoadedLastUpdate = bFlag; }

		bool						IsRequestAutoStitch() const					{ return m_bAutoStitch; }
		void						SetRequestAutoStitch( bool bFlag )			{ m_bAutoStitch = bFlag; }

		bool						IsLightLocked() const						{ return m_bLightLocked; }
		void						SetLightLocked( bool bFlag )				{ m_bLightLocked = bFlag; }

		// Frustum state
		void						SetFrustumOwnership( DWORD frustId, bool bOwned );
		bool						IsOwnedByFrustum( DWORD frustId ) const		{ return( (m_FrustumOwned & frustId) != 0 ); }
		bool						IsOwnedByAnyFrustum() const					{ return( m_FrustumOwned != 0 ); }
		DWORD						GetFrustumOwnedBitfield() const				{ return m_FrustumOwned; }
		void						SetFrustumOwnedBitfield( unsigned int fo )	{ m_FrustumOwned = fo; }

		void						SetLoadedOwnership( DWORD frustId, bool bLoaded );
		bool						IsLoadedByFrustum( DWORD frustId ) const	{ return( (m_LoadedOwned & frustId) != 0 ); }
		bool						IsLoadedByAnyFrustum() const				{ return( m_LoadedOwned != 0 ); }
		DWORD						GetLoadedAndOwnedBitfield() const			{ return( m_FrustumOwned | m_LoadedOwned ); }

		void						SetInterestOwnership( DWORD frustId, bool bInterest );
		bool						IsInterestByFrustum( DWORD frustId ) const	{ return( (m_InterestOwned & frustId) != 0 ); }
		bool						IsInterestByAnyFrustum() const				{ return( m_InterestOwned != 0 ); }

		void						SetUpgradedOwnership( DWORD frustId, bool bUpgraded );
		bool						IsUpgradedByFrustum( DWORD frustId ) const	{ return( (m_UpgradedOwned & frustId) != 0 ); }
		bool						IsUpgradedByAnyFrustum() const				{ return( m_UpgradedOwned != 0 ); }

		// Space
		void						SetSpaceFrustum( DWORD frustId )			{ m_SpaceFrustum = frustId; }
		DWORD						GetSpaceFrustum() const						{ return m_SpaceFrustum; }
		bool						HasValidSpace() const						{ return( m_SpaceFrustum != 0 ); }

		// Bounding volumes
		const vector_3&				GetMinimumBounds() const					{ return m_minBox; }
		const vector_3&				GetMaximumBounds() const					{ return m_maxBox; }

		void						SetMinimumBounds( const vector_3& minBox )	{ m_minBox = minBox; }
		void						SetMaximumBounds( const vector_3& maxBox )	{ m_maxBox = maxBox; }

		const vector_3&				GetWorldSpaceMinimumBounds() const			{ return m_wminBox; }
		const vector_3&				GetWorldSpaceMaximumBounds() const			{ return m_wmaxBox; }

		const vector_3&				GetWorldSpaceSphereCenter() const			{ return m_wSphereCenter; }
		float						GetSphereRadius() const						{ return m_sphereRadius; }

		// Lights
		const SiegeLightList&		GetStaticLights() const						{ return m_StaticLights; }
		const SiegeLightList&		GetDynamicLights() const					{ return m_DynamicLights; }

		// Find node in the passed list with the corresponding guid
		SiegeLightListConstIter		FindLightInList( const database_guid guid, const SiegeLightList& list );

		DWORD						GetAmbientColor() const						{ return m_AmbientColor; }
		DWORD						GetActorAmbientColor() const				{ return m_ActorAmbientColor; }
		DWORD						GetObjectAmbientColor() const				{ return m_ObjectAmbientColor; }

		bool						ComputeLighting( SiegeMesh const& mesh );
		void						AccumulateStaticLighting();

		bool						GetOccludesLight() const					{ return m_bOccludesLight; }
		void						SetOccludesLight( bool occlude )			{ m_bOccludesLight = occlude; }

		bool						GetOccludesCamera() const					{ return m_bOccludesCamera; }
		void						SetOccludesCamera( bool occlude )			{ m_bOccludesCamera = occlude; }

		bool						GetBoundsCamera() const						{ return m_bBoundsCamera; }
		void						SetBoundsCamera( bool bound )				{ m_bBoundsCamera = bound; }

		bool						GetCameraFade() const						{ return m_bCameraFade; }
		void						SetCameraFade( bool fade )					{ m_bCameraFade = fade; }

		//----- shadow assistance
		void						GetBoundedTriCollection( const vector_3& minBox, const vector_3& maxBox, TriangleColl& retColl ) const;
		
		//----- Generic textures
		const gpstring&				GetTextureSetAbbr() const					{ return m_sTextureSetAbbr;	}
		const gpstring&				GetTextureSetVersion() const				{ return m_sTextureSetVersion; }

		void						SetTextureSetAbbr( const gpstring& s )		{ m_sTextureSetAbbr = s; }
		void						SetTextureSetVersion( const gpstring& s )	{ m_sTextureSetVersion = s; }

		//----- Hotpoints
		const vector_3&				GetHotpointDirection( const DWORD id );
		void						SetHotpointDirection( const DWORD id, const vector_3& dir );

		void						ClearHotpoints();
		void						InsertHotpoint( const vector_3& dir, const DWORD id );
		void						RemoveHotpoint( const DWORD id );

		bool						IsHotpointUpdateRequested()					{ return m_bUpdateHotpoints; }
		void						RequestHotpointUpdate( bool bUpdate )		{ m_bUpdateHotpoints = bUpdate; }

		// !!IMPORTANT NOTE!!
		// Access to these lists is not threadsafe and should only be done for editing use!
		const SiegeHotpointMap&		GetHotpointMap() const						{ return m_HotpointMap; }
		const DecalIndexList&		GetDecalIndexList() const					{ return m_DecalList; }

	private:

		// IO bullshit
		typedef SiegeNodeIO io_handler;
		friend SiegeNodeIO;

		// Don't let copies occur
		SiegeNode &operator=(SiegeNode const &source);
		SiegeNode(SiegeNode const &);

#if		!GP_RETAIL
		// Mesh checksum
		DWORD						m_meshChecksum;

		// Spatial parent
		database_guid				m_SpatialParent;
#endif

		// Logical Nodes
		SiegeLogicalNode**			m_pLogicalNodes;
		DWORD						m_numLogicalNodes;

		// Mesh that defines up the boundary of the node
		SiegeNodeDoorList			m_DoorList;

		// Mesh guid and handle
		database_guid				m_MeshGUID;
		SiegeMeshHandle*			m_pMeshHandle;

		// Texture list
		NodeTexColl					m_TextureListing;
		LoadIdColl					m_TextureLoadListing;
		bool						m_bHasAlphaStages;
		bool						m_bNoAlphaSortStages;

		// Environment map
		DWORD						m_EnvironmentMap;

		// Lighting
		DWORD*						m_pStaticVertexColors;
		DWORD*						m_pVertexColors;
		DWORD						m_numVertices;

		// Store the global counter for the last time we visted this node
		bool						m_bVisible;
		bool						m_bVisibleThisFrame;
		bool						m_bLit;

		DWORD						m_LastVisited;
		DWORD						m_APVisited;

		// Store the current position relative to the camera target
		// these parameters are updated when we traverse the nodes
		vector_3					m_CurrentCenter;
		matrix_3x3					m_CurrentOrientation;
		matrix_3x3					m_TransposeOrientation;

		// Bounding volume of this SiegeNode, calculated from the Logical nodes
		vector_3					m_minBox;
		vector_3					m_maxBox;

		vector_3					m_wminBox;
		vector_3					m_wmaxBox;

		vector_3					m_wSphereCenter;
		float						m_sphereRadius;

		// Distance from the camera
		float						m_CameraDistance;

		// Store the GUID in the node, we'll need it to check the doors
		// for link errors
		database_guid				m_Guid;
		FileSys::FileHandle			m_fileHandle;

		// What is the current global alpha level for this node
		DWORD						m_Alpha;
		DWORD						m_CameraAlpha;

		// Node properties
		int							m_RegionID;
		int							m_Section;
		int							m_Level;
		int							m_Object;

		// World booleans
		bool						m_bLoadedPhysical;
		bool						m_bValidSpace;
		bool						m_bLoadedLastUpdate;
		bool						m_bUpdateHotpoints;
		bool						m_bAutoStitch;
		bool						m_bLightLocked;

		// Frustum ownership
		DWORD						m_FrustumOwned;
		DWORD						m_SpaceFrustum;
		DWORD						m_LoadedOwned;
		DWORD						m_InterestOwned;
		DWORD						m_UpgradedOwned;

		// Intensity values for default lighting info
		DWORD						m_AmbientColor;
		DWORD						m_ActorAmbientColor;
		DWORD						m_ObjectAmbientColor;

		// Lighting information
		SiegeLightList				m_LoadedStaticLights;
		SiegeLightList				m_StaticLights;
		SiegeLightList				m_DynamicLights;
		bool						m_bOccludesLight;
		bool						m_bOccludesCamera;
		bool						m_bBoundsCamera;
		bool						m_bCameraFade;
		bool						m_bAccumulateRequested;
		bool						m_bLightingChanged;

		// Generic texture set info
		gpstring					m_sTextureSetAbbr;	
		gpstring					m_sTextureSetVersion;

		// Hotpoints
		SiegeHotpointMap			m_HotpointMap;

		// Decals
		DecalIndexList				m_DecalList;

		// Update the light's description info
		void				UpdateLightDescriptor( database_guid guid, LightDescriptor* pDescriptor );

		// Insert the light into this node
		void				InsertLight( const database_guid guid, const vector_3& position, const vector_3& direction, const LightDescriptor* pDescriptor );

		// Insert light into passed listing, sorting by intensity
		void				InsertLightInList( SiegeNodeLight& light, SiegeLightList& list );

		// Find node in the passed list with the corresponding guid
		SiegeLightListIter	FindLightInList( const database_guid guid, SiegeLightList& list );

		// Destroy the light
		void				DestroyLight( database_guid guid, bool bUnRegister = true );

		// Calculate the intensity map for a given light
		void				CalculateLight( const database_guid guid, SiegeNodeLight& light );

		// Get a possible list of occluding nodes based on this light
		void				GetPossibleOccludingNodes( const database_guid guid, const SiegeNodeLight& light, SiegeNodeList& slist );

		// See if the area between two points is occluded by some geometry
		bool				RayIsBlocked( const vector_3& position, const vector_3& point, const SiegeNodeList& slist );
	};


	// SiegeNode door
	class SiegeNodeDoor
	{
	public:

		// Construction
		SiegeNodeDoor(const UINT32& init_id, 
					  const vector_3& init_center,
					  const matrix_3x3& init_orient,
					  const database_guid& init_neighbour,
					  const UINT32 init_neighbour_id,
					  bool bConnect
					 ) 
			: m_Id(init_id)
			, m_Center(init_center)
			, m_Orientation(init_orient)
			, m_Neighbour(init_neighbour)
			, m_Neighbour_Id(init_neighbour_id)
			, m_bConnect( bConnect )
#if		!GP_RETAIL
			, m_bInaccurate( false )
#endif
		{};

		SiegeNodeDoor(const UINT32& init_id,
					  const database_guid& init_neighbour,
					  const UINT32 init_neighbour_id,
					  bool bConnect
					 )
			: m_Id(init_id)
			, m_Neighbour(init_neighbour)
			, m_Neighbour_Id(init_neighbour_id)
			, m_bConnect( bConnect )
#if		!GP_RETAIL
			, m_bInaccurate( false )
#endif
		{};

		// Accessors
		const DWORD				GetID(void)								{ return m_Id; }

		const vector_3&			GetCenter(void)							{ return m_Center; }
		const matrix_3x3&		GetOrientation(void)					{ return m_Orientation; }
		const database_guid&	GetNeighbour(void)						{ return m_Neighbour; }
		const DWORD				GetNeighbourID(void)					{ return m_Neighbour_Id; }

		// Set center/orient
		void					SetCenter( const vector_3& center )		{ m_Center = center; }
		void					SetOrient( const matrix_3x3& orient )	{ m_Orientation = orient; }

		void					SetNeighbour( const database_guid& g )	{ m_Neighbour = g; }
		void					SetNeighbourID( const DWORD i )			{ m_Neighbour_Id = i; }

#if		!GP_RETAIL
		void					SetAsInaccurate( const bool bInacc )	{ m_bInaccurate	= bInacc; }
		bool					IsInaccurate()							{ return m_bInaccurate; }
#endif

		bool					GetConnection()							{ return m_bConnect; }

	private:

		// Identifier of this door
		const DWORD				m_Id;

		// Positional information
		vector_3				m_Center;
		matrix_3x3				m_Orientation;

		// Neighbor information
		database_guid			m_Neighbour;
		UINT32					m_Neighbour_Id;

		// States
		bool					m_bConnect;

#if		!GP_RETAIL
		bool					m_bInaccurate;
#endif


		// Protected construction
		SiegeNodeDoor( SiegeNodeDoor const & );
	    SiegeNodeDoor &operator=( SiegeNodeDoor const & );
	};
}


#endif


