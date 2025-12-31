/***************************************************************************************
**
**								RapiPrimitive
**
**		A collection of functions that use the Rapi graphics abstraction layer to draw
**		a series of geometrical primitives.
**
**	$TODO:: allow two radii on the plymarkers so that we can draw alternating radius star shape
**
***************************************************************************************/

#include "precomp_rapi.h"

#include "RapiOwner.h"
#include "RapiPrimitive.h"
#include "space_3.h"

// DrawBox will draw a box at the given coordinates using the current world matrix
void	RP_DrawBox( Rapi& renderer, const vector_3& minBound, const vector_3& maxBound, const DWORD color )
{
	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	renderer.SetTexture( 0, NULL );
	renderer.SetTexture( 1, NULL );

	vector_3 p0( maxBound.x, maxBound.y, minBound.z );
	vector_3 p1( minBound.x, maxBound.y, minBound.z );
	vector_3 p2( minBound.x, minBound.y, minBound.z );
	vector_3 p3( maxBound.x, minBound.y, minBound.z );
	vector_3 p4( maxBound.x, maxBound.y, maxBound.z );
	vector_3 p5( minBound.x, maxBound.y, maxBound.z );
	vector_3 p6( minBound.x, minBound.y, maxBound.z );
	vector_3 p7( maxBound.x, minBound.y, maxBound.z );

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= color;
	verts[1].color	= color;

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p1, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p1, sizeof( float ) * 3 );
	memcpy( &verts[1], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	memcpy( &verts[1], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p3, sizeof( float ) * 3 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
}

// DrawFilledBox will draw a solid color fill box at the given coordinates using the current world matrix
void	RP_DrawFilledBox( Rapi& renderer, const vector_3& minBound, const vector_3& maxBound, const DWORD color )
{
	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	renderer.SetTexture( 0, NULL );
	renderer.SetTexture( 1, NULL );

	vector_3 p0( maxBound.x, maxBound.y, minBound.z );
	vector_3 p1( minBound.x, maxBound.y, minBound.z );
	vector_3 p2( minBound.x, minBound.y, minBound.z );
	vector_3 p3( maxBound.x, minBound.y, minBound.z );
	vector_3 p4( maxBound.x, maxBound.y, maxBound.z );
	vector_3 p5( minBound.x, maxBound.y, maxBound.z );
	vector_3 p6( minBound.x, minBound.y, maxBound.z );
	vector_3 p7( maxBound.x, minBound.y, maxBound.z );

	sVertex verts[3];
	memset( verts, 0, sizeof( sVertex ) * 3 );

	verts[0].color	= color;
	verts[1].color	= color;
	verts[2].color	= color;

	memcpy( &verts[0], &p1, sizeof( float ) * 3 );
	memcpy( &verts[1], &p2, sizeof( float ) * 3 );
	memcpy( &verts[2], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );	
	memcpy( &verts[0], &p5, sizeof( float ) * 3 );
	memcpy( &verts[1], &p2, sizeof( float ) * 3 );
	memcpy( &verts[2], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );	

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p1, sizeof( float ) * 3 );
	memcpy( &verts[2], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p1, sizeof( float ) * 3 );
	memcpy( &verts[2], &p5, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	memcpy( &verts[2], &p0, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p7, sizeof( float ) * 3 );
	memcpy( &verts[2], &p3, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p2, sizeof( float ) * 3 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	memcpy( &verts[2], &p6, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p6, sizeof( float ) * 3 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	memcpy( &verts[2], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p5, sizeof( float ) * 3 );
	memcpy( &verts[1], &p6, sizeof( float ) * 3 );
	memcpy( &verts[2], &p4, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p4, sizeof( float ) * 3 );
	memcpy( &verts[1], &p6, sizeof( float ) * 3 );
	memcpy( &verts[2], &p7, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );

	memcpy( &verts[0], &p0, sizeof( float ) * 3 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	memcpy( &verts[2], &p1, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &p1, sizeof( float ) * 3 );
	memcpy( &verts[1], &p3, sizeof( float ) * 3 );
	memcpy( &verts[2], &p2, sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, verts, 3, SVERTEX, NULL, 0 );
}

// RP_DrawPyramid will draw a pyramid at the given coordinates using the current world matrix
// Note: a negative height will give an inverted pyramid
void	RP_DrawPyramid( Rapi& renderer, const float baselength, const float height, const DWORD color)
{

	float ground;
	float summit;
	float halflengthX;
	float halflengthZ;

	if (height > 0) {
		ground = 0.0f;
		summit = height;
		halflengthX = baselength * 0.5f;
		halflengthZ = baselength * 0.5f;
	} else {
		ground = -height;
		summit = 0.0f;
		halflengthX = baselength *  0.5f;
		halflengthZ = baselength * -0.5f;
	}


	sVertex p[6];
	memset( p, 0, sizeof( sVertex ) * 6 );

	// Preserve the texture stage 0 COLOROP
	DWORD colorop;
	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG2 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	}

	p[0].color = p[1].color = p[2].color = p[3].color = p[4].color = p[5].color = color;

	//--------- Sides of pyramid

	p[0].x = 0.0f;			p[0].y = summit;	p[0].z =  0.0f;
	p[1].x = -halflengthX;	p[1].y = ground;	p[1].z = -halflengthZ;
	p[2].x = -halflengthX;	p[2].y = ground;	p[2].z =  halflengthZ;
	p[3].x =  halflengthX;	p[3].y = ground;	p[3].z =  halflengthZ;
	p[4].x =  halflengthX;	p[4].y = ground;	p[4].z = -halflengthZ;
	p[5].x = p[1].x;		p[5].y = p[1].y;	p[5].z = p[1].z;

	renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, p, 6, SVERTEX, NULL, 0 );

	//--------- Base of pyramid

	p[0].x = -halflengthX;	p[0].y = ground;	p[0].z = -halflengthZ;
	p[1].x =  halflengthX;
	p[2].x =  halflengthX;
	p[3].x = -halflengthX;

	renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, p, 4, SVERTEX, NULL, 0 );

	// Restore the texture stage 0 COLOROP
	if( colorop != D3DTOP_SELECTARG2 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
}

// DrawOrientedBox will draw an oriented bounding volume using the current world matrix
void	RP_DrawOrientedBox( Rapi& renderer, const vector_3& position, const matrix_3x3& orientation, const vector_3& half_diag, const DWORD color )
{
	// Setup the world matrix
	renderer.SetWorldMatrix( orientation, position );

	// Draw the box
	RP_DrawBox( renderer, -half_diag, half_diag, color );
}

// DrawOrigin will draw a coordinate axis using the current world matrix
void	RP_DrawOrigin( Rapi& renderer, const float size )
{
	sVertex p[6];
	memset( p, 0, sizeof( sVertex ) * 6 );

	// Preserve the texture stage 0 COLOROP
	DWORD colorop;
	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG2 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	}

	//--------- X Axis

	p[0].x = size;
	p[0].y = 0.0f;
	p[0].z = 0.0f;

	p[1].x = p[2].x = p[3].x = p[4].x = p[5].x	=
	p[1].y = p[4].y = p[5].y =
	p[1].z = p[2].z = p[5].z = size*0.05f;

	p[2].y = p[3].y =
	p[3].z = p[4].z = -size*0.05f;

	p[0].color = p[1].color = p[2].color = p[3].color = p[4].color = p[4].color = 0xFFFF0000;

	renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, p, 6, SVERTEX, NULL, 0 );

	//--------- Y Axis

	p[0].x = 0.0f;
	p[0].y = size;
	p[0].z = 0.0f;

	p[1].y = p[2].y = p[3].y = p[4].y = p[5].y	=
	p[1].x = p[2].x = p[5].x =
	p[1].z = p[4].z = p[5].z = size*0.05f;

	p[4].x = p[3].x =
	p[2].z = p[3].z = -size*0.05f;

	p[0].color = p[1].color = p[2].color = p[3].color = p[4].color = p[4].color = 0xFF00FF00;

	renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, p, 6, SVERTEX, NULL, 0 );


	//--------- Z Axis

	p[0].x = 0.0f;
	p[0].y = 0.0f;
	p[0].z = size;

	p[1].z = p[2].z = p[3].z = p[4].z = p[5].z	=
	p[1].x = p[4].x = p[5].x =
	p[1].y = p[2].y = p[5].y = size*0.05f;

	p[2].x = p[3].x =
	p[3].y = p[4].y = -size*0.05f;

	p[0].color = p[1].color = p[2].color = p[3].color = p[4].color = p[4].color = 0xFF0000FF;

	renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, p, 6, SVERTEX, NULL, 0 );

	// Restore the texture stage 0 COLOROP
	if( colorop != D3DTOP_SELECTARG2 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
}

// DrawSphere will draw a sphere centered that the current origin
void	RP_DrawSphere(  Rapi& renderer, const float radius, const int segments, const DWORD color )
{

	if (segments == 0) return;
	if (IsZero(radius)) return;

	DWORD numverts = segments+1;

	float delta = PI2/segments;

	sVertex* p;
	unsigned int startIndex;

	// Preserve the texture stage COLOROPs
	DWORD colorop,colorarg1;
	DWORD alphaop;

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG1 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLORARG1, &colorarg1 );
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_ALPHAOP, &alphaop );
	if( alphaop != D3DTOP_DISABLE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	if (renderer.LockBufferedVertices( numverts, startIndex, p ) ) 
	{
		for (int t = 0; t < segments; t++)
		{
			float s,c;
			
			SINCOSF( (delta * t), s, c );
			p[t].x = c*radius;
			p[t].y = s*radius;
			p[t].z = 0;
			p[t].color = color;

		}

		p[segments].x = p[0].x;
		p[segments].y = p[0].y;
		p[segments].z = 0;
		p[segments].color = color;

		renderer.UnlockBufferedVertices();

		renderer.PushWorldMatrix();

		matrix_3x3 m = YRotationColumns(delta);

		for (int s = 0 ; s < segments ; ++s )
		{
			renderer.DrawBufferedPrimitive( D3DPT_LINESTRIP, startIndex, numverts);
			renderer.RotateWorldMatrix(m);
		}
		renderer.PopWorldMatrix();

	}

	// Restore the texture stage COLOROPs
	if( colorop != D3DTOP_SELECTARG1 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, colorarg1 ) );
	}
	if( alphaop != D3DTOP_DISABLE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, alphaop ) );
	}

}


// DrawRay draws a line from the current matrix origin to the endpoint
void	RP_DrawRay( Rapi& renderer, const vector_3& endpoint, const DWORD color )
{
	sVertex p[2];
	memset( p, 0, sizeof( sVertex ) * 2 );

	// Preserve the texture stage 0 COLOROP
	DWORD colorop;
	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG2 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 ) );
	}

	p[1].x = endpoint.x;
	p[1].y = endpoint.y;
	p[1].z = endpoint.z;

	p[0].color = color;

	renderer.DrawPrimitive( D3DPT_LINELIST, p, 2, SVERTEX, NULL, 0 );

	// Restore the texture stage 0 COLOROP
	if( colorop != D3DTOP_SELECTARG2 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
}

// DrawFilledRect draws a filled rectangle in screen coordinates - 0,0 is top left
void	RP_DrawFilledRect( Rapi& renderer, UINT l, UINT t, UINT r, UINT b, const DWORD color, float z )
{
	tVertex	verts[4];
	memset( verts, 0, sizeof( tVertex ) * 4 );

	verts[0].color = verts[1].color = verts[2].color = verts[3].color = color;

	verts[0].x		= (float)l - 0.5f;
	verts[0].y		= (float)b - 0.5f;
	verts[0].z		= z;
	verts[0].rhw	= 1.0f;

	verts[1].x		= (float)r - 0.5f;
	verts[1].y		= (float)b - 0.5f;
	verts[1].z		= z;
	verts[1].rhw	= 1.0f;

	verts[2].x		= (float)l - 0.5f;
	verts[2].y		= (float)t - 0.5f;
	verts[2].z		= z;
	verts[2].rhw	= 1.0f;

	verts[3].x		= (float)r - 0.5f;
	verts[3].y		= (float)t - 0.5f;
	verts[3].z		= z;
	verts[3].rhw	= 1.0f;

	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_NONE,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	renderer.SetTexture( 0, NULL );
	renderer.SetTexture( 1, NULL );
	bool bAlpha = renderer.EnableAlphaBlending( true );

	if( IsZero( z ) )
	{
		TRYDX (renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS ) );
	}

	renderer.DrawPrimitive( D3DPT_TRIANGLESTRIP, &verts, 4, TVERTEX, NULL, 0 );

	if( IsZero( z ) )
	{
		TRYDX (renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) );
	}
	renderer.EnableAlphaBlending( bAlpha );
}

// DrawEmptyRace draws an empty rectangle in screen coordinates
void	RP_DrawEmptyRect( Rapi& renderer, UINT l, UINT t, UINT r, UINT b, const DWORD color )
{
	tVertex	verts[5];
	memset( verts, 0, sizeof( tVertex ) * 5 );

	verts[0].color = verts[1].color = verts[2].color = verts[3].color = color;

	verts[0].x		= (float)l - 0.5f;
	verts[0].y		= (float)b - 0.5f;
	verts[0].rhw	= 1.0f;

	verts[1].x		= (float)r - 0.5f;
	verts[1].y		= (float)b - 0.5f;
	verts[1].rhw	= 1.0f;

	verts[2].x		= (float)r - 0.5f;
	verts[2].y		= (float)t - 0.5f;
	verts[2].rhw	= 1.0f;

	verts[3].x		= (float)l - 0.5f;
	verts[3].y		= (float)t - 0.5f;
	verts[3].rhw	= 1.0f;

	verts[4]		= verts[0];

	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	renderer.SetTexture( 0, NULL );
	renderer.SetTexture( 1, NULL );

	TRYDX (renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS ) );
	renderer.DrawPrimitive( D3DPT_LINESTRIP, &verts, 5, TVERTEX, NULL, 0 );
	TRYDX (renderer.GetDevice()->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) );
}

// DrawOrigin will draw a grid centered at the current origin
void	RP_DrawReferenceGrid( Rapi& renderer, const float size, const int intervals, DWORD color ) {

	vector_3 p0( -size*0.5f, 0,  size*0.5f);
	vector_3 p1(  size*0.5f, 0,  size*0.5f);
	vector_3 p2( -size*0.5f, 0, -size*0.5f);
	vector_3 p3(  size*0.5f, 0, -size*0.5f);

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= color;
	verts[1].color	= color;

	for (float i = 0; i <= intervals; i+=1.0f) {
		vector_3 v0 = (i/intervals)*p0 + ((intervals-i)/intervals)*p1;
		vector_3 v1 = (i/intervals)*p2 + ((intervals-i)/intervals)*p3;
		memcpy( &verts[0], &v0, sizeof( float ) * 3 );
		memcpy( &verts[1], &v1, sizeof( float ) * 3 );
		renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
		v0 = (i/intervals)*p0 + ((intervals-i)/intervals)*p2;
		v1 = (i/intervals)*p1 + ((intervals-i)/intervals)*p3;
		memcpy( &verts[0], &v0, sizeof( float ) * 3 );
		memcpy( &verts[1], &v1, sizeof( float ) * 3 );
		renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	}

}

// RP_DrawPolymarker will draw a series of platonic polygon markers
void	RP_DrawPolymarker( Rapi& renderer, const std::vector<vector_3>& marks, float radius, DWORD color, DWORD sides, float initang)
{

	// Initial offset angle is in RADIANS
	// Low chord values yield line, triangle, square, pentagon etc...

	if (marks.size() == 0) return;

	if (sides == 0) return;

	if (sides > 256) sides = 256;

	// Preserve the texture stage COLOROPs
	DWORD colorop,colorarg1;
	DWORD alphaop;

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG1 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLORARG1, &colorarg1 );
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_ALPHAOP, &alphaop );
	if( alphaop != D3DTOP_DISABLE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	DWORD numverts = sides+1;

	float delta = PI2/sides;

	sVertex* p;
	unsigned int startIndex;

	if (renderer.LockBufferedVertices( numverts, startIndex, p ) ) 
	{
		for (DWORD t = 0; t < sides; t++)
		{
			float s,c;
			
			SINCOSF( initang + (delta * t), s, c );
			p[t].x = c*radius;
			p[t].y = 0;
			p[t].z = s*radius;
			p[t].color = color;

		}

		p[sides].x = p[0].x;
		p[sides].y = 0;
		p[sides].z = p[0].z;
		p[sides].color = color;

		renderer.UnlockBufferedVertices();

		for (DWORD m = 0;  m < marks.size(); m++) {
			renderer.PushWorldMatrix();
			renderer.TranslateWorldMatrix(marks[m]);
			renderer.DrawBufferedPrimitive( D3DPT_LINESTRIP, startIndex, numverts);
			renderer.PopWorldMatrix();
		}


	}

	// Restore the texture stage COLOROPs
	if( colorop != D3DTOP_SELECTARG1 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, colorarg1 ) );
	}
	if( alphaop != D3DTOP_DISABLE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, alphaop ) );
	}
}

// RP_DrawPolyline will draw a platonic polygon relative to the current origin
void	RP_DrawPolyline( Rapi& renderer, const std::vector<vector_3>& verts,  DWORD color)
{

	// Initial offset angle is in RADIANS
	// Low chord values yield line, triangle, square, pentagon etc...

	if (verts.size() == 0) return;

	// Preserve the texture stage COLOROPs
	DWORD colorop,colorarg1;
	DWORD alphaop;

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG1 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLORARG1, &colorarg1 );
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_ALPHAOP, &alphaop );
	if( alphaop != D3DTOP_DISABLE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	DWORD numverts = verts.size();

	sVertex* p;
	unsigned int startIndex;

	if (renderer.LockBufferedVertices( numverts, startIndex, p ) ) 
	{

		for (DWORD t = 0; t < numverts; t++)
		{
			memcpy( &p[t], &verts[t], sizeof( float ) * 3 );
			p[t].color = color;
		}

		renderer.UnlockBufferedVertices();

		renderer.DrawBufferedPrimitive( D3DPT_LINESTRIP, startIndex, numverts);
	}

	// Restore the texture stage COLOROPs
	if( colorop != D3DTOP_SELECTARG1 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, colorarg1 ) );
	}
	if( alphaop != D3DTOP_DISABLE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, alphaop ) );
	}

}

// RP_DrawLine draw a line using the two given points and color
void	RP_DrawLine( Rapi& renderer, const vector_3& start, const vector_3& end, DWORD color )
{
	// Preserve the texture stage COLOROPs
	DWORD colorop,colorarg1;
	DWORD alphaop;

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &colorop );
	if( colorop != D3DTOP_SELECTARG1 )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_COLORARG1, &colorarg1 );
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	}

	renderer.GetDevice()->GetTextureStageState( 0, D3DTSS_ALPHAOP, &alphaop );
	if( alphaop != D3DTOP_DISABLE )
	{
		renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	sVertex* p;
	unsigned int startIndex;

	if (renderer.LockBufferedVertices( 2, startIndex, p ) ) 
	{
		memcpy( &p[0], &start, sizeof( vector_3 ) );
		p[0].color = color;

		memcpy( &p[1], &end, sizeof( vector_3 ) );
		p[1].color = color;

		renderer.UnlockBufferedVertices();

		renderer.DrawBufferedPrimitive( D3DPT_LINELIST, startIndex, 2 );
	}

	// Restore the texture stage COLOROPs
	if( colorop != D3DTOP_SELECTARG1 )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLOROP, colorop ) );
	}
	if( colorarg1 != D3DTA_DIFFUSE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, colorarg1 ) );
	}
	if( alphaop != D3DTOP_DISABLE )
	{
		TRYDX (renderer.GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, alphaop ) );
	}
}

// DrawFrustum will draw a six-sided representation of a frustum
void	RP_DrawFrustum( Rapi& renderer, const vector_3& origin, const matrix_3x3& orientation,
					    const float angle, const float aspect, const float nearplane, const float farplane,
						DWORD nearPlaneColor, DWORD farPlaneColor )
{
	vector_3 FrustumPoint[8];

	float Zdist			= nearplane;
	float Xdist			= Zdist * tanf( angle * 0.5f );
	float Ydist			= Xdist / aspect;
	
	FrustumPoint[0]		= vector_3(-Xdist, Ydist,Zdist);
	FrustumPoint[1]		= vector_3( Xdist, Ydist,Zdist);
	FrustumPoint[2]		= vector_3( Xdist,-Ydist,Zdist);
	FrustumPoint[3]		= vector_3(-Xdist,-Ydist,Zdist);

	float farnearratio	= farplane / nearplane;
	Xdist				*= farnearratio;
	Ydist				*= farnearratio;
	Zdist				*= farnearratio;
	
	FrustumPoint[4]		= vector_3(-Xdist, Ydist,Zdist);
	FrustumPoint[5]		= vector_3( Xdist, Ydist,Zdist);
	FrustumPoint[6]		= vector_3( Xdist,-Ydist,Zdist);
	FrustumPoint[7]		= vector_3(-Xdist,-Ydist,Zdist);
	
	for( int p = 0; p < 8; p++ )
	{
		FrustumPoint[p] = origin + (orientation *  FrustumPoint[p]);
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



	renderer.SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );
	renderer.SetTexture( 0, NULL );
	renderer.SetTexture( 1, NULL );

	sVertex verts[2];
	memset( verts, 0, sizeof( sVertex ) * 2 );

	verts[0].color	= nearPlaneColor;
	verts[1].color	= farPlaneColor;
	memcpy( &verts[0], &FrustumPoint[0], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[4], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= nearPlaneColor;
	verts[1].color	= farPlaneColor;
	memcpy( &verts[0], &FrustumPoint[1], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[5], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= nearPlaneColor;
	verts[1].color	= farPlaneColor;
	memcpy( &verts[0], &FrustumPoint[2], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[6], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= nearPlaneColor;
	verts[1].color	= farPlaneColor;
	memcpy( &verts[0], &FrustumPoint[3], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[7], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= nearPlaneColor;
	verts[1].color	= nearPlaneColor;
	memcpy( &verts[0], &FrustumPoint[0], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[1], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &FrustumPoint[2], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &FrustumPoint[3], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &FrustumPoint[0], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );

	verts[0].color	= farPlaneColor;
	verts[1].color	= farPlaneColor;
	memcpy( &verts[0], &FrustumPoint[4], sizeof( float ) * 3 );
	memcpy( &verts[1], &FrustumPoint[5], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &FrustumPoint[6], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[1], &FrustumPoint[7], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
	memcpy( &verts[0], &FrustumPoint[4], sizeof( float ) * 3 );
	renderer.DrawPrimitive( D3DPT_LINELIST, verts, 2, SVERTEX, NULL, 0 );
}

