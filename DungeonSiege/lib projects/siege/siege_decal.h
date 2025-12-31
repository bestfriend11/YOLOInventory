#pragma once
#ifndef _SIEGE_DECAL_
#define _SIEGE_DECAL_

/************************************************************************************
**
**								SiegeDecal
**
**		This class is responsible for maintaining all information with regards
**		to an in engine decal.
**
**		Author:  James Loe
**		Date:	 10/24/00
**
************************************************************************************/


namespace siege
{
	const unsigned int DECAL_VERSION	= 3;

	struct LightInterpolant
	{
		// Lighting indices
		unsigned int		m_LightIndex1;
		unsigned int		m_LightIndex2;
		unsigned int		m_LightIndex3;

		// Interpolation amounts
		float				m_InterpAmount1;
		float				m_InterpAmount2;
		float				m_InterpAmount3;
	};

	struct DecalVertex
	{
		// Position
		float				x, y, z;

		// Texture coords
		float				u, v;

		// Projection w for perspective correction
		float				wl;

		// Light interpolant
		LightInterpolant	li;
	};

	struct DecalNodeRenderSet
	{
		// Triangle information for this decal
		unsigned int		m_numVertices;

		sVertex*			m_pVertices;
		float*				m_pPerspCorrect;
		LightInterpolant*	m_pLightInterp;
		bool				m_bRequestLighting;
	};

	struct GeometryCollector
	{
		// Frustum position and orientation (in world space)
		vector_3			m_frustPos;
		matrix_3x3			m_frustOrient;

		// Axis aligned volume in world space
		vector_3			m_wminBox;
		vector_3			m_wmaxBox;

		// Clip planes
		vector_3			m_nearPlaneNormal;
		vector_3			m_nearPlanePoint;

		vector_3			m_farPlaneNormal;
		vector_3			m_farPlanePoint;

		vector_3			m_leftPlaneNormal;
		vector_3			m_leftPlanePoint;

		vector_3			m_rightPlaneNormal;
		vector_3			m_rightPlanePoint;

		vector_3			m_topPlaneNormal;
		vector_3			m_topPlanePoint;

		vector_3			m_bottomPlaneNormal;
		vector_3			m_bottomPlanePoint;
	};

	struct RenderInfo
	{
		database_guid		node;
		DecalNodeRenderSet	renderSet;
	};

	typedef std::vector< RenderInfo >	DecalRenderColl;

	class SiegeDecal
	{
		friend class SiegeDecalDatabase;
		
	public:

		// Takes a position and uses the face normal at that position to setup a
		// standard default projection.  Uses horizontal and vertical measurements
		// (in meters) to setup the initial scaling.
		SiegeDecal( const SiegePos& pos, const vector_3& normal,
					const float horizMeters, const float vertMeters,
					const gpstring& decal_tex, bool bMipMapping = true,
					bool bLight = true, bool bPerspCorrect = false );

		// Detailed construction.  Takes a projection origin and direction,
		// as well as a horizontal and vertical projection size to setup scale,
		// and both the near plane and far plane to build a proper frustum.
		SiegeDecal( const SiegePos& origin, const matrix_3x3& orient,
					const float horizMeters, const float vertMeters,
					const float nearPlane, const float farPlane,
					const gpstring& decal_tex, bool bMipMapping = true,
					bool bLight = true, bool bPerspCorrect = false );

		// Load construction.  Loads information about this decal from a block
		// of data, usually a mapping into a file.
		SiegeDecal( const char*& pData );

		// Destruction
		~SiegeDecal();

		// Submit the proper loads for upgrading this decal to fully loaded
		void				UpgradeLoad();

		// Cancel loads and unload texture to go back into downgraded mode
		void				DowngradeLoad();

		// Draw the decal.  Since a decal exists in only a single node, but may
		// cross into several others, this must be called after all solid node
		// geometry has been rendered.  Also, the render matrix (with z-bias) is
		// expected to be setup for rendering from the owner node with a call to
		// BuildCombinedTransformationMatrix().  Only the portions of the decal
		// that exist in the passed node will be rendered. Return value is
		// the number of triangles drawn.
		unsigned int		Render( const database_guid node );

		// Update the lighting for this node on this decal
		void				UpdateLighting( const database_guid node, const DWORD* lightingInfo );

		// Update the projection matrix for this decal
		void				UpdateProjection();

		// Get the GUID assigned to this decal
		database_guid		GetGUID()										{ return m_Guid; }
		void				SetGUID( const database_guid& guid )			{ m_Guid = guid; }

		// Get and set the level of detail
		void				SetLevelOfDetail( float detail )				{ m_LevelOfDetail = detail; }
		float				GetLevelOfDetail()								{ return m_LevelOfDetail; }

		// Get and set the projection position
		SiegePos			GetDecalOrigin()								{ return m_Origin; }
		void				SetDecalOrigin( const SiegePos& origin )		{ m_Origin = origin; BuildDecal(); }
		
		// Get and set the projection direction
		matrix_3x3			GetDecalOrientation()							{ return m_Orientation; }
		void				SetDecalOrientation( const matrix_3x3& orient )	{ m_Orientation = orient; BuildDecal(); }

		// Get and set the near plane
		float				GetNearPlane()									{ return m_NearPlane; }
		void				SetNearPlane( const float plane )				{ m_NearPlane = plane; BuildDecal(); }

		// Get and set the far plane
		float				GetFarPlane()									{ return m_FarPlane; }
		void				SetFarPlane( const float plane )				{ m_FarPlane = plane; BuildDecal(); }

		// Get and set the horizontal projection angle (in radians)
		float				GetAngle()										{ return m_Angle; }
		void				SetAngle( const float angle )					{ m_Angle = angle; BuildDecal(); }

		// Get and set the aspect ratio (width/height)
		float				GetAspect()										{ return m_Aspect; }
		void				SetAspect( const float aspect )					{ m_Aspect = aspect; BuildDecal(); }

		// Get and set the active state
		bool				GetActive()										{ return m_bActive; }
		void				SetActive( const bool active )					{ m_bActive = active; }

		// Get and set alpha
		BYTE				GetAlpha()										{ return m_Alpha; }
		void				SetAlpha( const BYTE alpha )					{ m_Alpha = alpha; }

		// Get and set light state
		bool				GetLight()										{ return m_bLight; }
		void				SetLight( const bool light )					{ m_bLight = light; }

		// Accessors for the original meter construction sizes
		float				GetHorizontalMeters();
		float				GetVerticalMeters();

		// Get the decal render mapping
		DecalRenderColl&	GetDecalRenderColl()							{ return m_DecalRenderColl; }

		// Get the decal texture
		unsigned int		GetDecalTexture()								{ return m_DecalTexture; }

		// Hit test with this decal.  Ray is assumed to be in world space.
		bool				HitTest( const vector_3& ray_orig, const vector_3& ray_dir );


	private:

		// Unique GUID that identifies this decal
		database_guid		m_Guid;

		// Level of detail settings
		float				m_LevelOfDetail;

		// Handle to the texture used by this decal
		unsigned int		m_DecalTexture;
		LoadOrderId			m_DecalLoadOrder;

		// Projection parameter storage
		SiegePos			m_Origin;
		matrix_3x3			m_Orientation;
		float				m_Angle;
		float				m_Aspect;
		float				m_NearPlane;
		float				m_FarPlane;

		// Decal state
		bool				m_bActive;
		BYTE				m_Alpha;
		bool				m_bLight;
		bool				m_bPerspCorrect;

		// Triangle information for this decal
		DecalRenderColl		m_DecalRenderColl;

		// Projection matrix
		D3DMATRIX			m_ProjectionMatrix;

		// Destroys the data allocated by this decal
		void				DestroyDecal();

		// Uses the current decal parameters to build the render information
		void				BuildDecal();

		// Callback function for the collection of geometry used by this decal
		bool				GeometryCallBack( const SiegeNode& node, void* pData );

		// Build a list of render interpolants from basic geometry info
		void				BuildRenderInterpolants( const SiegeNode& node, TriangleColl& triList,
													 std::vector< LightInterpolant >& liList );

		// Clip a list of vectors to a plane, maintaining a proper index list with interpolants
		void				ClipTrianglesWithInterpolation( const plane_3& plane, TriangleColl& vectList,
															std::vector< LightInterpolant >& liList );

		// Build a proper light interpolant from the given information
		LightInterpolant	BuildLightInterpolant( const vector_3& vector1, const LightInterpolant& light1,
												   const vector_3& vector2, const LightInterpolant& light2,
												   const vector_3& intersection );
	};
}

#endif