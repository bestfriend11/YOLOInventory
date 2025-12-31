#pragma once
#ifndef _SIEGE_VIEWFRUSTUM_
#define _SIEGE_VIEWFRUSTUM_

/***********************************************************************
**
**							SiegeViewFrustum
**
**	Maintains a view frustum and provides methods for determining
**	whether objects are inside or outside of this frustum.
**
**  Author:			Mike Biddlecombe, James Loe
**  Date:			04/21/00
**
***********************************************************************/

#include "plane_3.h"
#include "space_3.h"

class Rapi;

namespace siege
{
	// Enumerations which define frustum members
	enum FRUSTUM_PLANES
	{ 
		TOP_CLIP_PLANE		= 0,
		RIGHT_CLIP_PLANE	= 1,
		BOTTOM_CLIP_PLANE	= 2,
		LEFT_CLIP_PLANE		= 3,
		NEAR_CLIP_PLANE		= 4,
		FAR_CLIP_PLANE		= 5,
		FP_DWORD_ALIGN		= 0x7FFFFFFF,
	};

	enum FRUSTUM_VERTS
	{ 
		TOP_CLIP_VERTEX		= TOP_CLIP_PLANE,
		RIGHT_CLIP_VERTEX	= RIGHT_CLIP_PLANE,
		BOTTOM_CLIP_VERTEX	= BOTTOM_CLIP_PLANE,
		LEFT_CLIP_VERTEX	= LEFT_CLIP_PLANE,
		NEAR_CLIP_VERTEX	= 0,
		FAR_CLIP_VERTEX		= 4,
		FV_DWORD_ALIGN		= 0x7FFFFFFF,
	};

	enum FRUSTUM_OUTCODES
	{
		NEAR_OUTCODE		= 1 << 0,
		FAR_OUTCODE			= 1 << 1,
		TOP_OUTCODE			= 1 << 2,
		BOTTOM_OUTCODE		= 1 << 3,
		LEFT_OUTCODE		= 1 << 4,
		RIGHT_OUTCODE		= 1 << 5,

		NULL_OUTCODE		= 0,
		ANY_OUTCODE			= -1,
	};

	class SiegeViewFrustum
	{
	public:
		
		// Construction
		SiegeViewFrustum( const int width, const int height );
	
		// See if the given object is culled by the frustum
		bool	CullObject( const matrix_3x3& orientation, const vector_3& position, const vector_3& halfdiag ) const;
		bool	CullObject( const vector_3& min_box, const vector_3& max_box ) const;

		// See if the given sphere is culled by the frustum
		bool	CullSphere( const vector_3& position, const float radius );

		// Construct the frustum using field of view
		void	SetDimensionsWithFOV( const int width, const int height, const float fov,
									  const float nearclip, const float farclip,
									  const vector_3& newPosition, const matrix_3x3& newOrientation );

		// Construct the frustum using depth
		void	SetDimensionsWithDepth( const int width, const int height, const float depth,
										const float nearclip, const float farclip,
										const vector_3& newPosition, const matrix_3x3& newOrientation );

		// Construct the frustum using a near box
		void	SetDimensionsWithNearBox( const vector_3& point0, const vector_3& point1,
										  const float nearclip, const float farclip,
										  const vector_3& newPosition, const matrix_3x3& newOrientation );

		// Clip polygons
		void	ClipTrianglesToFrustum( TriangleColl& triangles );
		void	ClipTrianglesToNearPlane( TriangleColl& triangles, const SiegeNode& node );

		// Field of view
		void	SetFieldOfView( const float fov )			{ m_FieldOfView = fov; }
		float	GetFieldOfView() const						{ return m_FieldOfView; }

		void	SetAspectRatio( const float aspect )		{ m_AspectRatio = aspect; }
		float	GetAspectRatio() const						{ return m_AspectRatio; }

		void	SetNearClipDistance( const float nearclip )	{ m_NearClipDistance = nearclip; }
		float	GetNearClipDistance() const					{ return m_NearClipDistance; }

		void	SetFarClipDistance( const float farclip )	{ m_FarClipDistance = farclip; }
		float	GetFarClipDistance() const					{ return m_FarClipDistance; }


		// Debug information
		void	SetColor( vector_3 const& color )			{ m_Color = color; }
		vector_3 const &GetColor() const					{ return m_Color; }

		void	SetVisibility( bool const visible )			{ m_Visible = visible; }
		bool const IsVisible() const						{ return m_Visible; }

		// Draw the frustum	
		void DebugDraw( Rapi& renderer );

	private:

		// Viewport information
		int			m_ClipWindowHeight;
		int			m_ClipWindowWidth;
		float		m_ClipWindowDepth;

		// Camera information
		float		m_FieldOfView;
		float		m_AspectRatio;
		float		m_NearClipDistance;
		float		m_FarClipDistance;

		// Frustum information
		vector_3	m_FrustumPoint[8];
		vector_3	m_FrustumTangent[6];
		float		m_FrustumD[6];
		vector_3	m_FrustumPosition;
		matrix_3x3	m_FrustumOrientation;

		// Debug information
		vector_3	m_Color;
		bool		m_Visible;

		// Takes current parameters of the frustum and constructs tangents for culling
        void Construct();
	};
}

#endif
