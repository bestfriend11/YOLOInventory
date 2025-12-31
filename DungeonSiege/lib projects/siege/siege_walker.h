#pragma once
#ifndef _SIEGE_WALKER_
#define _SIEGE_WALKER_

/*******************************************************************************************
**
**									SiegeWalker
**
**		Class responsible for maintaining and updating active nodes, valid space, and
**		rendering both nodes and objects with correct sorting.
**
**		Authors:	Mike Biddlecombe
**					James Loe
**
*******************************************************************************************/


#include "siege_defs.h"
#include "Nema_Types.h"
#include <queue>


namespace siege
{
	enum eCollectFlags
	{
		CF_NONE					=      0,		// default
		CF_ACTOR				= 1 << 0,		// is an actor?
		CF_ITEM					= 1 << 1,		// is an item?
		CF_SELECTABLE			= 1 << 2,		// is selectable? (worry about mouse shadow?)
		CF_HIGHLIGHTED			= 1 << 3,		// is highlighted?
		CF_DRAW_SHADOW			= 1 << 4,		// draw a shadow for this?
		CF_AVOID_CAMERA			= 1 << 5,		// does avoid the camera?
		CF_SCREEN_PLAYER_VIS	= 1 << 6,		// can the human player see me?
		CF_GIZMO				= 1 << 7,		// is a gizmo?
		CF_FADEOUT				= 1 << 8,		// should this object be fading out?
		CF_FADEIN				= 1 << 9,		// should this object be fading in?
		CF_PARTYMEMBER			= 1 << 10,		// is a party member?
		CF_SPAWNED				= 1 << 11,		// is spawned?
		CF_PROJECTILE			= 1 << 12,		// is a projectile?
		CF_SELECTED				= 1 << 13,		// is selected?
		CF_INTERESTONLY			= 1 << 14,		// only draw when in interest sphere?
		CF_MM_OVERRIDE			= 1 << 15,		// draw in megamap no matter what?
		CF_MM_ORIENT			= 1 << 16,		// orient megamap icon?
		CF_POSORIENTCHANGED		= 1 << 17,		// has position or orientation changed?
		CF_DYNAMICLIGHT			= 1 << 18,		// is this object affected by dynamic light?
		CF_RENDERALPHAOBJECT	= 1 << 19,		// should we render this object during the alpha pass?
		CF_GHOST				= 1 << 20,		// is a ghost?
		CF_NORENDER				= 1 << 21,		// force no render
		CF_ALIVE				= 1 << 22,		// is this object alive?
		CF_FOCUSED				= 1 << 23,		// is this object the focus go?
	};

	enum eRequestFlags
	{
		RF_NONE					=	   0,		// default, does nothing
		RF_POSITION				= 1 << 0,		// request position
		RF_ORIENT				= 1 << 1,		// request orientation
		RF_RENDERINDEX			= 1 << 2,		// request render indices
		RF_BOUNDS				= 1 << 3,		// request bounding information
		RF_SCALE				= 1 << 4,		// request scale
		RF_FRUSTUMCLIP			= 1 << 5,		// request camera frustum clipping
		RF_SCREEN_PLAYER_ALIGN	= 1 << 6,		// request alignment to screen player
		RF_ALPHA				= 1 << 7,		// request alpha level
		RF_CAMERA_DISTANCE		= 1 << 8,		// request object's distance to the camera
		RF_PROJ_CENTER			= 1 << 9,		// request the object's projection center
	};

	enum eAlignmentFlags
	{
		AF_NONE					=      0,		// No alignment
		AF_FRIEND				= 1 << 0,		// Friendly alignment
		AF_ENEMY				= 1 << 1,		// Enemy alignment
		AF_NEUTRAL				= 1 << 2,		// Neutral alignment
	};

	struct TRICOLLINFO
	{
		// Box to be bounded
		vector_3		m_wminBox;
		vector_3		m_wmaxBox;

		// Clip planes
		vector_3		m_nearPlaneNormal;
		vector_3		m_nearPlanePoint;

		// Color scale
		float			m_colorScale;
	};

	// Object information structure - used during RenderWorldObjects
	struct ASPECTINFO
	{
		// What and where to render
		DWORD				Object;
		const SiegeNode*	pNode;				// siege node in which this object resides
		nema::Aspect*		Aspect;
		unsigned int		MegamapIconTex;
		matrix_3x3			Orientation;
		vector_3			Origin;
		vector_3			ProjCenter;
		short				RenderIndex1;
		short				RenderIndex2;
		short				RenderIndex3;
		DWORD				Alpha;
		DWORD				CameraFadeAlpha;

		// From GetWorldSpaceOrientedBoundingVolume
		vector_3			BoundCenter;
		vector_3			BoundHalfDiag;
		matrix_3x3			BoundOrient;

		// Render tuning
		float				ObjectScale;
		float				BoundingBodyScale;	// bounding volume scale * object scale
		eCollectFlags		Flags;

		// Screen player alignment
		eAlignmentFlags		ScreenPlayerAlign;

		// Distance to camera
		float				CameraDistance;

		// Shadow
		DWORD				ComplexShadowTex;
		TRICOLLINFO			ShadowTriColl;
		D3DMATRIX			ShadowProjMat;

		ASPECTINFO( void )
			: Orientation      ( DoNotInitialize ),
			  Origin           ( DoNotInitialize ),
			  ProjCenter       ( DoNotInitialize ),
			  BoundCenter      ( DoNotInitialize ),
			  BoundHalfDiag    ( DoNotInitialize ),
			  BoundOrient      ( DoNotInitialize ),
			  ObjectScale      ( DoNotInitialize ),
			  BoundingBodyScale( DoNotInitialize )  {  }

		GPDEBUG_ONLY( bool AssertValid( void ) const; )
	};

	struct obj_greater : public std::binary_function< ASPECTINFO*, ASPECTINFO*, bool >
	{
		bool operator()(const ASPECTINFO*& x, const ASPECTINFO*& y) const
		{
			return( x->CameraDistance > y->CameraDistance );
		}
	};

	struct obj_less : public std::binary_function< ASPECTINFO*, ASPECTINFO*, bool >
	{
		bool operator()(const ASPECTINFO*& x, const ASPECTINFO*& y) const
		{
			return( x->CameraDistance < y->CameraDistance );
		}
	};

	typedef stdx::fast_vector< ASPECTINFO* >	WorldObjectList;		// sorted list of pointers to aspectinfo
	typedef stdx::fast_vector< ASPECTINFO >		WorldObjectColl;		// non-sorted list of aspectinfo

	typedef CBFunctor0							EffectRenderCb;			// effects render callback

	// Effect information structure - used during RenderWorldObjects
	struct EFFECTINFO
	{
		// Distance to camera
		float			CameraDistance;

		// Render callback
		EffectRenderCb	RenderCallback;
	};

	struct effect_greater : public std::binary_function< EFFECTINFO, EFFECTINFO, bool >
	{
		bool operator()(const EFFECTINFO& x, const EFFECTINFO& y) const
		{
			return( x.CameraDistance > y.CameraDistance );
		}
	};

	typedef stdx::fast_vector< EFFECTINFO >	WorldEffectColl;			// collection of effectinfo

	struct node_greater : public std::binary_function< SiegeNode*, SiegeNode*, bool >
	{
		bool operator()(const SiegeNode*& x, const SiegeNode*& y) const
		{
			return( x->GetCameraDistance() > y->GetCameraDistance() );
		}
	};

	struct node_less : public std::binary_function< SiegeNode*, SiegeNode*, bool >
	{
		bool operator()(const SiegeNode*& x, const SiegeNode*& y) const
		{
			return( x->GetCameraDistance() < y->GetCameraDistance() );
		}
	};

	MAKE_ENUM_BIT_OPERATORS( eCollectFlags );
	MAKE_ENUM_BIT_OPERATORS( eRequestFlags );
	MAKE_ENUM_BIT_OPERATORS( eAlignmentFlags );

	// Light information structure
	struct LIGHTINFO
	{
		// Light source information
		const LightDescriptor*	pLight;
		DWORD					color;

		// Local light position
		vector_3				localPosition;
		vector_3				nodalPosition;
	};
	typedef std::vector< LIGHTINFO >	LightCache;

	class SiegeWalker
	{
	private:

		// This is the node that we are 'currently looking at'
		database_guid			m_targetNodeGUID;

		// List of nodes to be rendered
		SiegeNodeColl			m_NodeColl;
		SiegeNodeColl			m_AlphaNodeColl;

		// List of objects to be rendered
		WorldObjectColl			m_ObjectColl;
		WorldObjectList			m_ObjectList;
		WorldObjectList			m_ObjectAlphaList;

		// List of effects to be rendered
		WorldEffectColl			m_EffectColl;

		// Light cache
		LightCache				m_StaticLightCache;
		LightCache				m_DynamicLightCache;

		// Light state counters
		DWORD					m_LastLightVisit;
		DWORD					m_CurrentLightVisit;

		// Meters to render the minimap
		float					m_MiniMeters;
		float					m_MiniMaxMeters;
		float					m_MiniMinMeters;
		bool					m_bMiniZoomIn;
		bool					m_bMiniZoomOut;
		float					m_MiniMetersZoomPerSecond;
		float					m_MiniStepZoomMeters;

		// Minimap textures
		unsigned int			m_OrientArrowTex;
		unsigned int			m_SelectionRingTex;
		unsigned int			m_DiscoveryTex;
		unsigned int			m_SelectionTriTex;
		unsigned int			m_TombstoneTex;

		// Object fade information
		float					m_ObjectFadeTime;

		// Discovery position tracking
		DiscoveryPosMap			m_discoveryPosMap;
		float					m_innerFrustumScale;

		// Shadow generation flag
		bool					m_bShadowGenerate;

		// Don't let anyone else copy
		SiegeWalker(SiegeWalker const &);
		SiegeWalker &operator=(SiegeWalker const &);

		// Light a node if needed
		bool LightSiegeNode( SiegeNode& node );

		// Render a node
		void RenderSiegeNode( SiegeNode& node, bool alpha, bool noalphasort = false );

		// Light and deform a single object
		void LightAndDeformObject( ASPECTINFO& info );

		// Render a single object
		void RenderObject( ASPECTINFO& info );

		// Light a SiegeNode for minimap purposes
		bool LightMiniSiegeNode( SiegeNode& node, oriented_bounding_box_3& clip_box,
								 const matrix_3x3& orient, const vector_3& offset );

		// Render a SiegeNode for minimap purposes
		void RenderMiniSiegeNode( SiegeNode& node, bool alpha, const vector_3& mouse_pos );

		// Render the objects for minimap purposes
		void RenderMiniWorldObjects( const matrix_3x3& orient, const float metersPerPixel );

		// Render a single object shadow using the given light source
		void GenerateShadow( ASPECTINFO& object, const LIGHTINFO& light, bool bGenerateShadow );
		void RenderObjectShadow( ASPECTINFO& object );

		// Callback function to assist in the collection of bounded triangle information and shadow drawing
		bool ShadowDrawCallBack( const SiegeNode& node, void* pData );

	public:

		// Existence
		SiegeWalker();
		~SiegeWalker(void);

		// Persistence
		bool Xfer( FuBi::PersistContext& persist );

		// Access
		const database_guid TargetNodeGUID(void) const 				{ return m_targetNodeGUID; }
		database_guid TargetNodeGUID(void)							{ return m_targetNodeGUID; }
		void SetTargetNodeGUID( database_guid targetGUID )			{ m_targetNodeGUID = targetGUID; }

		// Set minimap tolerances
		void SetMiniMapMaxMeters( float maxMeters )					{ m_MiniMaxMeters = maxMeters; }
		void SetMiniMapMinMeters( float minMeters )					{ m_MiniMinMeters = minMeters; }
		void SetMiniMapMetersZoomPerSecond( float metersPerSec )	{ m_MiniMetersZoomPerSecond = metersPerSec; }
		void SetMiniMapStepZoomMeters( float stepZoomMeters )		{ m_MiniStepZoomMeters = stepZoomMeters; }

		// Set minimap scale
		void SetMiniMapMeters( float miniMeters )					{ m_MiniMeters = miniMeters; }
		float GetMiniMapMeters()									{ return m_MiniMeters; }

		// Zoom in/out minimap
		void SetMiniMapZoomIn( bool bZoom )							{ m_bMiniZoomIn = bZoom; }
		bool GetMiniMapZoomIn()										{ return m_bMiniZoomIn; }

		void SetMiniMapZoomOut( bool bZoom )						{ m_bMiniZoomOut = bZoom; }
		bool GetMiniMapZoomOut()									{ return m_bMiniZoomOut; }

		void StepMiniMapZoomIn();
		void StepMiniMapZoomOut();

		// Minimap display textures
		void SetMiniMapOrientArrowTexture( unsigned int tex )		{ m_OrientArrowTex = tex; }
		void SetMiniMapSelectionRingTexture( unsigned int tex )		{ m_SelectionRingTex = tex; }
		void SetMiniMapDiscoveryTexture( unsigned int tex )			{ m_DiscoveryTex = tex; }
		void SetMiniMapSelectionTriTexture( unsigned int tex )		{ m_SelectionTriTex = tex; }
		void SetMiniMapTombstoneTexture( unsigned int tex )			{ m_TombstoneTex = tex; }

		// Minimap scalar
		void SetMinimapInnerFrustumScale( float scale )				{ m_innerFrustumScale = scale; }

		// Set object fade settings
		void SetObjectFadeTime( float fadeTime )					{ m_ObjectFadeTime = fadeTime; }
		float GetObjectFadeTime()									{ return m_ObjectFadeTime; }

		// Force update shadow generation
		void ForceShadowGeneration()								{ m_bShadowGenerate = true; }

		// Clear the walker out
		void Clear();

		// Render all currently visible nodes
		void RenderVisibleNodes();

		// Render all nodes in minimap format
		void RenderVisibleMiniNodes( const int left, const int right, const int top, const int bottom );

		// Submit new discover positions
		void SubmitDiscoveryPosition( const SiegePos& pos, SiegeFrustum* pFrustum );
	};
}


#endif
