/***********************************************************************
**
**							SiegeViewFrustum
**
**	See .h file for details
**
***********************************************************************/


#include "precomp_siege.h"
#include "gpcore.h"
#include "RapiOwner.h"
#include "siege_viewfrustum.h"
#include "gpmath.h"

using namespace siege;


// Construction
SiegeViewFrustum::SiegeViewFrustum( const int width, const int height )
	: m_Visible(true)
{
	// Construct a default frustum
	SetDimensionsWithFOV( width, height, RealPi/3.0f, 0.1f, 200.0f, vector_3(0,0,0), matrix_3x3() );
}

// See if the given object is culled by the frustum
bool SiegeViewFrustum::CullObject( const matrix_3x3& orientation,	const vector_3& position, const vector_3& halfdiag ) const
{
	// Build an accurate box by transforming the existing one
	vector_3 tminBox( -halfdiag );
	vector_3 tmaxBox( halfdiag );

	vector_3 minBox, maxBox;
	TransformAxisAlignedBox( orientation, position, tminBox, tmaxBox, minBox, maxBox );

	// Build an array of points to be used in the cull
	const vector_3 testpoints[8]	= { vector_3( maxBox.x, maxBox.y, minBox.z ),
										vector_3( minBox.x, maxBox.y, minBox.z ),
										vector_3( minBox.x, minBox.y, minBox.z ),
										vector_3( maxBox.x, minBox.y, minBox.z ),
										vector_3( maxBox.x, maxBox.y, maxBox.z ),
										vector_3( minBox.x, maxBox.y, maxBox.z ),
										vector_3( minBox.x, minBox.y, maxBox.z ),
										vector_3( maxBox.x, minBox.y, maxBox.z ) };


	int CombinedOutCode = ANY_OUTCODE;

	for( int p = 0; p < 8; p++ )
	{
		int CurrentOutCode = NULL_OUTCODE;
		const vector_3& testpoint = testpoints[p];

		// if we DON'T find a point inside the frustum then return true

		//	4------------5
		//	|\          /|
		//	| \        / |
		//	|  0 --- 1   |		0-1-5-4 : Bound the TOP CLIP PLANE		0
		//	|  |     |   |		1-2-6-5 : Bound the RIGHT CLIP PLANE	1
		//	|  |     |   |		2-3-7-6 : Bound the BOTTOM CLIP PLANE	2	
		//	|  3 --- 2   |  	3-0-4-7 : Bound the LEFT CLIP PLANE		3
		//	| /       \  |		0-1-2-3 : Bound the NEAR CLIP PLANE		4	
		//	|/         \ |		4-7-6-5 : Bound the FAR CLIP PLANE		5	
		//	7------------6	

		if( (testpoint-m_FrustumPoint[BOTTOM_CLIP_VERTEX]).DotProduct( m_FrustumTangent[BOTTOM_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= BOTTOM_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[TOP_CLIP_VERTEX]).DotProduct( m_FrustumTangent[TOP_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= TOP_OUTCODE;
		}

		if( (testpoint-m_FrustumPoint[NEAR_CLIP_VERTEX]).DotProduct( m_FrustumTangent[NEAR_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= NEAR_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[FAR_CLIP_VERTEX]).DotProduct( m_FrustumTangent[FAR_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= FAR_OUTCODE;
		}

		if( (testpoint-m_FrustumPoint[LEFT_CLIP_VERTEX]).DotProduct( m_FrustumTangent[LEFT_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= LEFT_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[RIGHT_CLIP_VERTEX]).DotProduct( m_FrustumTangent[RIGHT_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= RIGHT_OUTCODE;
		}

		// Combine outcodes
		CombinedOutCode &= CurrentOutCode;

		// Check for NULL, which indicates early success
		if( CombinedOutCode == NULL )
		{
			return false;
		}
	}

	return true;
}

// Axis aligned frustum culling
bool SiegeViewFrustum::CullObject( const vector_3& min_box, const vector_3& max_box ) const
{
	// Build an array of points to be used in the cull
	const vector_3 testpoints[8]	= { vector_3( max_box.x, max_box.y, min_box.z ),
										vector_3( min_box.x, max_box.y, min_box.z ),
										vector_3( min_box.x, min_box.y, min_box.z ),
										vector_3( max_box.x, min_box.y, min_box.z ),
										vector_3( max_box.x, max_box.y, max_box.z ),
										vector_3( min_box.x, max_box.y, max_box.z ),
										vector_3( min_box.x, min_box.y, max_box.z ),
										vector_3( max_box.x, min_box.y, max_box.z ) };

	int CombinedOutCode = ANY_OUTCODE;

	for( int p = 0; p < 8; ++p )
	{
		int CurrentOutCode = NULL_OUTCODE;
		const vector_3& testpoint = testpoints[p];

		if( (testpoint-m_FrustumPoint[BOTTOM_CLIP_VERTEX]).DotProduct( m_FrustumTangent[BOTTOM_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= BOTTOM_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[TOP_CLIP_VERTEX]).DotProduct( m_FrustumTangent[TOP_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= TOP_OUTCODE;
		}

		if( (testpoint-m_FrustumPoint[NEAR_CLIP_VERTEX]).DotProduct( m_FrustumTangent[NEAR_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= NEAR_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[FAR_CLIP_VERTEX]).DotProduct( m_FrustumTangent[FAR_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= FAR_OUTCODE;
		}

		if( (testpoint-m_FrustumPoint[LEFT_CLIP_VERTEX]).DotProduct( m_FrustumTangent[LEFT_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= LEFT_OUTCODE;
		} 
		if( (testpoint-m_FrustumPoint[RIGHT_CLIP_VERTEX]).DotProduct( m_FrustumTangent[RIGHT_CLIP_PLANE] ) > 0.0f )
		{ 
			CurrentOutCode |= RIGHT_OUTCODE;
		}

		// Combine outcodes
		CombinedOutCode &= CurrentOutCode;

		// Check for NULL, which indicates early success
		if( CombinedOutCode == NULL )
		{
			return false;
		}
	}

	return true;
}

// See if the given sphere is culled by the frustum
bool SiegeViewFrustum::CullSphere( const vector_3& position, const float radius )
{
	if( (m_FrustumTangent[BOTTOM_CLIP_PLANE].DotProduct( position ) + m_FrustumD[BOTTOM_CLIP_PLANE]) > radius )
	{
		return true;
	}

	if( (m_FrustumTangent[NEAR_CLIP_PLANE].DotProduct( position ) + m_FrustumD[NEAR_CLIP_PLANE]) > radius )
	{
		return true;
	}

	if( (m_FrustumTangent[LEFT_CLIP_PLANE].DotProduct( position ) + m_FrustumD[LEFT_CLIP_PLANE]) > radius )
	{
		return true;
	}

	if( (m_FrustumTangent[RIGHT_CLIP_PLANE].DotProduct( position ) + m_FrustumD[RIGHT_CLIP_PLANE]) > radius )
	{
		return true;
	}

	if( (m_FrustumTangent[TOP_CLIP_PLANE].DotProduct( position ) + m_FrustumD[TOP_CLIP_PLANE]) > radius )
	{
		return true;
	}

	if( (m_FrustumTangent[FAR_CLIP_PLANE].DotProduct( position ) + m_FrustumD[FAR_CLIP_PLANE]) > radius )
	{
		return true;
	}

	return false;
}

// Construct the frustum using field of view
void SiegeViewFrustum::SetDimensionsWithFOV( const int width, const int height, const float fov,
										 const float nearclip, const float farclip,
										 const vector_3& newPosition, const matrix_3x3& newOrientation )
{
	m_NearClipDistance		= nearclip;
	m_FarClipDistance		= farclip;

	m_ClipWindowWidth		= width;
	m_ClipWindowHeight		= height;

	m_FieldOfView			= fov;
	m_ClipWindowDepth		= (float)m_ClipWindowHeight * tanf( fov * 0.5f );
	
	m_AspectRatio			= (float)width/(float)height;

	m_FrustumPosition		= newPosition;
	m_FrustumOrientation	= newOrientation;
	
	Construct();
}
	
	
// Construct the frustum using depth
void SiegeViewFrustum::SetDimensionsWithDepth( const int width, const int height, const float depth,
										   const float nearclip, const float farclip,
										   const vector_3& newPosition, const matrix_3x3& newOrientation )
{
	m_NearClipDistance		= nearclip;
	m_FarClipDistance		= farclip;

	m_ClipWindowWidth		= width;
	m_ClipWindowHeight		= height;
	
	m_ClipWindowDepth		= depth;
	
	m_AspectRatio			= (float)width/(float)height;

	m_FrustumPosition		= newPosition;
	m_FrustumOrientation	= newOrientation;
	
	Construct();
}

// Construct the frustum using a near box
void SiegeViewFrustum::SetDimensionsWithNearBox( const vector_3& point0, const vector_3& point1,
											 const float nearclip, const float farclip,
											 const vector_3& newPosition, const matrix_3x3& newOrientation )
{

	m_NearClipDistance		= nearclip;
	m_FarClipDistance		= farclip;

	m_FrustumPosition		= newPosition;
	m_FrustumOrientation	= newOrientation;
	
	real MinX				= min_t( point0.x, point1.x ) ;
	real MaxX				= max_t( point0.x, point1.x ) ;
	
	real MinY				= min_t( point0.y, point1.y ) ;
	real MaxY				= max_t( point0.y, point1.y ) ;

	real MinZ				= point0.z;
	
	m_FrustumPoint[0]		= vector_3(MinX,MaxY,MinZ);
	m_FrustumPoint[1]		= vector_3(MaxX,MaxY,MinZ);
	m_FrustumPoint[2]		= vector_3(MaxX,MinY,MinZ);
	m_FrustumPoint[3]		= vector_3(MinX,MinY,MinZ);

	m_FrustumPoint[4]		= m_FrustumPoint[0] * (m_FarClipDistance/m_NearClipDistance);
	m_FrustumPoint[5]		= m_FrustumPoint[1] * (m_FarClipDistance/m_NearClipDistance);
	m_FrustumPoint[6]		= m_FrustumPoint[2] * (m_FarClipDistance/m_NearClipDistance);
	m_FrustumPoint[7]		= m_FrustumPoint[3] * (m_FarClipDistance/m_NearClipDistance);

	for( int p = 0; p < 8; p++ )
	{
		m_FrustumPoint[p] = m_FrustumPosition + (m_FrustumOrientation *  m_FrustumPoint[p]);
	}

	//	4-----------5
	//	|\         /|
	//	| \       / |
	//	|  0 --- 1  |	0-1-5-4 : Bound the TOP CLIP PLANE		0
	//	|  |     |  |	1-2-6-5 : Bound the RIGHT CLIP PLANE	1
	//	|  |     |  |	2-3-7-6 : Bound the BOTTOM CLIP PLANE	2	
	//	|  3 --- 2  |  	3-0-4-7 : Bound the LEFT CLIP PLANE		3
	//	| /       \ |	0-1-2-3 : Bound the NEAR CLIP PLANE		4	
	//	|/         \|	4-7-6-5 : Bound the FAR CLIP PLANE		5	
	//	7-----------6	

	CalcFaceNormal( m_FrustumTangent[TOP_CLIP_PLANE],	 m_FrustumPoint[0], m_FrustumPoint[4], m_FrustumPoint[1] );
	CalcFaceNormal( m_FrustumTangent[RIGHT_CLIP_PLANE],	 m_FrustumPoint[1], m_FrustumPoint[5], m_FrustumPoint[2] );
	CalcFaceNormal( m_FrustumTangent[BOTTOM_CLIP_PLANE], m_FrustumPoint[2], m_FrustumPoint[6], m_FrustumPoint[3] );
	CalcFaceNormal( m_FrustumTangent[LEFT_CLIP_PLANE],	 m_FrustumPoint[3], m_FrustumPoint[7], m_FrustumPoint[0] );
	CalcFaceNormal( m_FrustumTangent[NEAR_CLIP_PLANE],	 m_FrustumPoint[0], m_FrustumPoint[1], m_FrustumPoint[3] );
	CalcFaceNormal( m_FrustumTangent[FAR_CLIP_PLANE],	 m_FrustumPoint[4], m_FrustumPoint[7], m_FrustumPoint[5] );

	m_FrustumTangent[TOP_CLIP_PLANE].Normalize();
	m_FrustumTangent[RIGHT_CLIP_PLANE].Normalize();
	m_FrustumTangent[BOTTOM_CLIP_PLANE].Normalize();
	m_FrustumTangent[LEFT_CLIP_PLANE].Normalize();
	m_FrustumTangent[NEAR_CLIP_PLANE].Normalize();
	m_FrustumTangent[FAR_CLIP_PLANE].Normalize();

	m_FrustumD[TOP_CLIP_PLANE]		= -m_FrustumTangent[TOP_CLIP_PLANE].DotProduct( m_FrustumPoint[0] );
	m_FrustumD[RIGHT_CLIP_PLANE]	= -m_FrustumTangent[RIGHT_CLIP_PLANE].DotProduct( m_FrustumPoint[1] );
	m_FrustumD[BOTTOM_CLIP_PLANE]	= -m_FrustumTangent[BOTTOM_CLIP_PLANE].DotProduct( m_FrustumPoint[2] );
	m_FrustumD[LEFT_CLIP_PLANE]		= -m_FrustumTangent[LEFT_CLIP_PLANE].DotProduct( m_FrustumPoint[3] );
	m_FrustumD[NEAR_CLIP_PLANE]		= -m_FrustumTangent[NEAR_CLIP_PLANE].DotProduct( m_FrustumPoint[0] );
	m_FrustumD[FAR_CLIP_PLANE]		= -m_FrustumTangent[FAR_CLIP_PLANE].DotProduct( m_FrustumPoint[4] );
}

// Clip polygons
void SiegeViewFrustum::ClipTrianglesToFrustum( TriangleColl& triangles )
{
	// Clip the frustum planes with the triangle list
	InvertedClipTriangles( m_FrustumTangent[NEAR_CLIP_PLANE],   m_FrustumD[NEAR_CLIP_PLANE],   triangles );
	InvertedClipTriangles( m_FrustumTangent[RIGHT_CLIP_PLANE],  m_FrustumD[RIGHT_CLIP_PLANE],  triangles );
	InvertedClipTriangles( m_FrustumTangent[LEFT_CLIP_PLANE],   m_FrustumD[LEFT_CLIP_PLANE],   triangles );
	InvertedClipTriangles( m_FrustumTangent[TOP_CLIP_PLANE],    m_FrustumD[TOP_CLIP_PLANE],    triangles );
	InvertedClipTriangles( m_FrustumTangent[BOTTOM_CLIP_PLANE], m_FrustumD[BOTTOM_CLIP_PLANE], triangles );
	InvertedClipTriangles( m_FrustumTangent[FAR_CLIP_PLANE],    m_FrustumD[FAR_CLIP_PLANE],    triangles );
}

void SiegeViewFrustum::ClipTrianglesToNearPlane( TriangleColl& triangles, const SiegeNode& node )
{
	// Calculate local space plane
	vector_3 nearPlaneNormal	= node.GetTransposeOrientation() * m_FrustumTangent[NEAR_CLIP_PLANE];
	float nearPlaneD			= -nearPlaneNormal.DotProduct( node.WorldToNodeSpace( m_FrustumPoint[0] ) );

	// Clip triangles
	InvertedClipTriangles( nearPlaneNormal, nearPlaneD, triangles );
}

// Takes current parameters of the frustum and constructs tangents for culling
void SiegeViewFrustum::Construct()
{
	real Zdist			= m_NearClipDistance;
	real Xdist			= Zdist * tanf( m_FieldOfView * 0.5f );
	real Ydist			= Xdist / m_AspectRatio;
	
	m_FrustumPoint[0]	= vector_3(-Xdist, Ydist,Zdist);
	m_FrustumPoint[1]	= vector_3( Xdist, Ydist,Zdist);
	m_FrustumPoint[2]	= vector_3( Xdist,-Ydist,Zdist);
	m_FrustumPoint[3]	= vector_3(-Xdist,-Ydist,Zdist);

	Xdist				= Xdist * (m_FarClipDistance/m_NearClipDistance);
	Ydist				= Ydist * (m_FarClipDistance/m_NearClipDistance);
	Zdist				= Zdist * (m_FarClipDistance/m_NearClipDistance);
	
	m_FrustumPoint[4]	= vector_3(-Xdist, Ydist,Zdist);
	m_FrustumPoint[5]	= vector_3( Xdist, Ydist,Zdist);
	m_FrustumPoint[6]	= vector_3( Xdist,-Ydist,Zdist);
	m_FrustumPoint[7]	= vector_3(-Xdist,-Ydist,Zdist);
	
	for( int p = 0; p < 8; p++ )
	{
		m_FrustumPoint[p] = m_FrustumPosition + (m_FrustumOrientation *  m_FrustumPoint[p]);
	}

	//	4-----------5
	//	|\         /|
	//	| \       / |
	//	|  0 --- 1  |	0-1-5-4 : Bound the TOP CLIP PLANE		0
	//	|  |     |  |	1-2-6-5 : Bound the RIGHT CLIP PLANE	1
	//	|  |     |  |	2-3-7-6 : Bound the BOTTOM CLIP PLANE	2	
	//	|  3 --- 2  |  	3-0-4-7 : Bound the LEFT CLIP PLANE		3
	//	| /       \ |	0-1-2-3 : Bound the NEAR CLIP PLANE		4	
	//	|/         \|	4-7-6-5 : Bound the FAR CLIP PLANE		5	
	//	7-----------6	

	CalcFaceNormal( m_FrustumTangent[TOP_CLIP_PLANE],	 m_FrustumPoint[0], m_FrustumPoint[4], m_FrustumPoint[1] );
	CalcFaceNormal( m_FrustumTangent[RIGHT_CLIP_PLANE],	 m_FrustumPoint[1], m_FrustumPoint[5], m_FrustumPoint[2] );
	CalcFaceNormal( m_FrustumTangent[BOTTOM_CLIP_PLANE], m_FrustumPoint[2], m_FrustumPoint[6], m_FrustumPoint[3] );
	CalcFaceNormal( m_FrustumTangent[LEFT_CLIP_PLANE],	 m_FrustumPoint[3], m_FrustumPoint[7], m_FrustumPoint[0] );
	CalcFaceNormal( m_FrustumTangent[NEAR_CLIP_PLANE],	 m_FrustumPoint[0], m_FrustumPoint[1], m_FrustumPoint[3] );
	CalcFaceNormal( m_FrustumTangent[FAR_CLIP_PLANE],	 m_FrustumPoint[4], m_FrustumPoint[7], m_FrustumPoint[5] );

	m_FrustumTangent[TOP_CLIP_PLANE].Normalize();
	m_FrustumTangent[RIGHT_CLIP_PLANE].Normalize();
	m_FrustumTangent[BOTTOM_CLIP_PLANE].Normalize();
	m_FrustumTangent[LEFT_CLIP_PLANE].Normalize();
	m_FrustumTangent[NEAR_CLIP_PLANE].Normalize();
	m_FrustumTangent[FAR_CLIP_PLANE].Normalize();

	m_FrustumD[TOP_CLIP_PLANE]		= -m_FrustumTangent[TOP_CLIP_PLANE].DotProduct( m_FrustumPoint[0] );
	m_FrustumD[RIGHT_CLIP_PLANE]	= -m_FrustumTangent[RIGHT_CLIP_PLANE].DotProduct( m_FrustumPoint[1] );
	m_FrustumD[BOTTOM_CLIP_PLANE]	= -m_FrustumTangent[BOTTOM_CLIP_PLANE].DotProduct( m_FrustumPoint[2] );
	m_FrustumD[LEFT_CLIP_PLANE]		= -m_FrustumTangent[LEFT_CLIP_PLANE].DotProduct( m_FrustumPoint[3] );
	m_FrustumD[NEAR_CLIP_PLANE]		= -m_FrustumTangent[NEAR_CLIP_PLANE].DotProduct( m_FrustumPoint[0] );
	m_FrustumD[FAR_CLIP_PLANE]		= -m_FrustumTangent[FAR_CLIP_PLANE].DotProduct( m_FrustumPoint[4] );
}

// Draw the frustum	
void SiegeViewFrustum::DebugDraw( Rapi& renderer )
{
	renderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,0,0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[0], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[4], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 0.0,1.0,0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[1], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[5], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,1.0,0.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[2], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[6], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,0.0,1.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[3], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[7], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,1.0,1.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[0], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[1], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &m_FrustumPoint[2], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &m_FrustumPoint[3], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &m_FrustumPoint[0], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1,1,1 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], &m_FrustumPoint[4], sizeof( float ) * 3 );
	memcpy( &verts[1], &m_FrustumPoint[5], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &m_FrustumPoint[6], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &m_FrustumPoint[7], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &m_FrustumPoint[4], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	// RED
	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,0,0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], ((m_FrustumPoint[0]+m_FrustumPoint[1])/2).GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (((m_FrustumPoint[0]+m_FrustumPoint[1])/2)+Normalize(m_FrustumTangent[0])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	// GREEN
	verts[0].color	= MAKEDWORDCOLOR( vector_3( 0.0,1.0,0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], ((m_FrustumPoint[1]+m_FrustumPoint[2])/2).GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (((m_FrustumPoint[1]+m_FrustumPoint[2])/2)+Normalize(m_FrustumTangent[1])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,1.0,0.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], ((m_FrustumPoint[2]+m_FrustumPoint[3])/2).GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (((m_FrustumPoint[2]+m_FrustumPoint[3])/2)+Normalize(m_FrustumTangent[2])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 1.0,0.0,1.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], ((m_FrustumPoint[3]+m_FrustumPoint[0])/2).GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (((m_FrustumPoint[3]+m_FrustumPoint[0])/2)+Normalize(m_FrustumTangent[3])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 0.0,1.0,1.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], ((m_FrustumPoint[0]+m_FrustumPoint[2])/2).GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (((m_FrustumPoint[0]+m_FrustumPoint[2])/2)+Normalize(m_FrustumTangent[4])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= MAKEDWORDCOLOR( vector_3( 0.0,0.0,1.0 ) );
	verts[1].color	= verts[0].color;
	memcpy( &verts[0], m_FrustumPoint[1].GetElementsPointer(), sizeof( float ) * 3 );
	memcpy( &verts[1], (m_FrustumPoint[1]+Normalize(m_FrustumTangent[5])).GetElementsPointer(), sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
}
