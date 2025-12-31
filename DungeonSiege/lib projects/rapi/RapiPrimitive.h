#pragma once
#ifndef _RAPI_PRIMITIVE_
#define _RAPI_PRIMITIVE_

/***************************************************************************************
**
**								RapiPrimitive
**
**		A collection of functions that use the Rapi graphics abstraction layer to draw
**		a series of geometrical primitives.
**
**		Date:		January 4, 2000
**		Author:		James Loe
**
***************************************************************************************/


class Rapi;


// DrawBox will draw a box at the given coordinates using the current world matrix
void	RP_DrawBox( Rapi& renderer, const vector_3& minBound, const vector_3& maxBound, const DWORD color = 0xffffffff );

// DrawFilledBox will draw a solid color fill box at the given coordinates using the current world matrix
void	RP_DrawFilledBox( Rapi& renderer, const vector_3& minBound, const vector_3& maxBound, const DWORD color = 0xffffffff );

// RP_DrawPyramid will draw a pyramid at the given coordinates using the current world matrix
// Note: a negative height will give an inverted pyramid
void	RP_DrawPyramid( Rapi& renderer, const float baselength, const float height, const DWORD color = 0xffffffff );

// DrawOrientedBox will draw an oriented bounding volume using the current world matrix
void	RP_DrawOrientedBox( Rapi& renderer, const vector_3& position, const matrix_3x3& orientation, const vector_3& half_diag, const DWORD color = 0xffffffff );

// DrawOrigin will draw a coordinate axis using the current world matrix
void	RP_DrawOrigin( Rapi& renderer, const float size );

// DrawSphere will draw a sphere centered at the current origin
void	RP_DrawSphere(  Rapi& renderer, const float radius, const int segments = 10, const DWORD color = 0xffffffff );

// DrawOrigin will draw a grid centered at the current origin
void	RP_DrawReferenceGrid( Rapi& renderer, const float size, const int intervals = 10, const DWORD color = 0xffffffff );

// DrawRay draws a line from the current matrix origin to the endpoint
void	RP_DrawRay( Rapi& renderer, const vector_3& endpoint, const DWORD color = 0xffffffff );

// DrawFilledRect draws a filled rectangle in screen coordinates
void	RP_DrawFilledRect( Rapi& renderer, UINT l, UINT t, UINT r, UINT b, const DWORD color, float z = 0.0f );

// DrawEmptyRace draws an empty rectangle in screen coordinates
void	RP_DrawEmptyRect( Rapi& renderer, UINT l, UINT t, UINT r, UINT b, const DWORD color );

// RP_DrawPolymarker will draw a series of platonic polygon markers
// Defaults to a 32 sided representation of a circle
void	RP_DrawPolymarker( Rapi& renderer, const std::vector<vector_3>& marks, float radius, DWORD color, DWORD sides, float initang);

// RP_DrawPolyline draws a linestrip offset from the current origin
void    RP_DrawPolyline( Rapi& renderer, const std::vector<vector_3>& verts,  DWORD color = 0xFFFFFFFF );

// RP_DrawLine draw a line using the two given points and color
void	RP_DrawLine( Rapi& renderer, const vector_3& start, const vector_3& end, DWORD color = 0xFFFFFFFF );

// DrawFrustum will draw a six-sided representation of a frustum
void	RP_DrawFrustum( Rapi& renderer, const vector_3& origin, const matrix_3x3& orientation,
					    const float angle, const float aspect, const float nearplane, const float farplane,
						DWORD nearPlaneColor = 0xFFFF0000, DWORD farPlaneColor = 0xFF0000FF );

#endif
