// ========================================================================
//   Source File: siege_lines.cpp
//   Description: Implements siege_worldspace_lines,
//                siege_screenspace_lines
// ========================================================================

// ========================================================================
//   Explicit Dependencies
// ========================================================================
   
#include "precomp_siege.h"
#include "gpcore.h"
#include "siege_lines.h"
#include "siege_engine.h"
#include "siege_camera.h"

#include "RapiOwner.h"

using namespace siege;

// ========================================================================
//   Function: DrawLine
//   Description: Stores a worldspace line for retained rendering
// ========================================================================

void worldspace_lines::
DrawLine(vector_3 const &Endpoint0, vector_3 const &Endpoint1, const DWORD Color)
{
	line Line;
	Line.Endpoints[0] = Endpoint0;
	Line.Endpoints[1] = Endpoint1;
	Line.Color = Color;
	Lines.push_back(Line);

}

// ========================================================================
//   Function: Render
//   Description: Renders the lines in world space
// ========================================================================
void worldspace_lines::
Render(void) const
{
	Rapi& renderer = gSiegeEngine.Renderer();

	// Setup state
	renderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_BLENDDIFFUSEALPHA,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	// Clear matrix
	renderer.SetWorldMatrix( matrix_3x3::IDENTITY, vector_3::ZERO );

	bool bPrevFogState = renderer.SetFogActivatedState( false );

	{for(lines::const_iterator LineIterator = Lines.begin();
		 LineIterator != Lines.end();
		 ++LineIterator)
	{
		sVertex line[2];
		memset( line, 0, sizeof( sVertex ) * 2 );

		line[0].x		= (*LineIterator).Endpoints[0].x;
		line[0].y		= (*LineIterator).Endpoints[0].y;
		line[0].z		= (*LineIterator).Endpoints[0].z;
		line[0].color	= (*LineIterator).Color;

		line[1].x		= (*LineIterator).Endpoints[1].x;
		line[1].y		= (*LineIterator).Endpoints[1].y;
		line[1].z		= (*LineIterator).Endpoints[1].z;
		line[1].color	= line[0].color;

		renderer.DrawPrimitive( D3DPT_LINELIST, line, 2, SVERTEX, NULL, 0 );
	}}

	renderer.SetFogActivatedState( bPrevFogState );

	Lines.erase(Lines.begin(), Lines.end());
}

// ========================================================================
void worldspace_lines::Reset()
{
	Lines.clear();
}




// ========================================================================
//   Function: DrawLine
//   Description: Stores a screen-space line for retained rendering
// ========================================================================
void screenspace_lines::
DrawLine( const float StartScreenX, const float StartScreenY, const float EndScreenX, const float EndScreenY, const DWORD Color )
{
	line Line;
	Line.Endpoints[0][0] = StartScreenX;
	Line.Endpoints[0][1] = StartScreenY;
	Line.Endpoints[1][0] = EndScreenX;
	Line.Endpoints[1][1] = EndScreenY;
	Line.Color = Color;
	Lines.push_back(Line);
}

// ========================================================================
//   Function: Render
//   Description: Renders lines in screen-space
// ========================================================================

void screenspace_lines::
Render(void) const
{
	Rapi& renderer = gSiegeEngine.Renderer();

	// Setup state
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

	bool bPrevFogState = renderer.SetFogActivatedState( false );

	{for(lines::const_iterator LineIterator = Lines.begin();
		 LineIterator != Lines.end();
		 ++LineIterator)
	{
		tVertex line[2];
		memset( line, 0, sizeof( tVertex ) * 2 );

		line[0].x		= (*LineIterator).Endpoints[0][0];
		line[0].y		= (*LineIterator).Endpoints[0][1];
		line[0].z		= 0.0f;
		line[0].rhw		= 1.0f;
		line[0].color	= (*LineIterator).Color;

		line[1].x		= (*LineIterator).Endpoints[1][0];
		line[1].y		= (*LineIterator).Endpoints[1][1];
		line[1].z		= 0.0f;
		line[1].rhw		= 1.0f;
		line[1].color	= line[0].color;

		renderer.DrawPrimitive( D3DPT_LINELIST, line, 2, TVERTEX, NULL, 0 );
	}}

	renderer.SetFogActivatedState( bPrevFogState );

	Lines.clear();
}

// ========================================================================
void screenspace_lines::Reset()
{
	Lines.clear();
}

// ========================================================================
//   Function: DrawLine
//   Description: Stores a nodespace line for retained rendering
// ========================================================================

void nodespace_lines::
DrawLine(vector_3 const &Endpoint0, vector_3 const &Endpoint1, const DWORD Color, const database_guid& node_guid)
{
	if( node_guid == UNDEFINED_GUID )
	{
		gpassertm( 0, "Cannot draw a nodespace line with an invalid node" );
		return;
	}

	line Line;
	Line.Endpoints[0]	= Endpoint0;
	Line.Endpoints[1]	= Endpoint1;
	Line.Color			= Color;
	Line.Node			= node_guid;
	Lines.push_back(Line);
}


void nodespace_lines::
DrawCircle(const vector_3 &center, const database_guid& node_guid, const float radius, 
		   const float ang, float arclen, const DWORD color, const bool  filled)

{
	if( node_guid == UNDEFINED_GUID )
	{
		gpassertm( 0, "Cannot draw a nodespace line with an invalid node" );
		return;
	}

	float alen = IsNegative(arclen) ? -arclen : arclen;

	alen = min_t(PI2,alen);

	circle Circle;
	Circle.Radius		= radius;
	Circle.StartAngle	= ang + (alen/2);
	
	const float maxslices = 50;
	const float slicesize = PI2/maxslices;

	Circle.NumSlices	= max_t((int)(floor(alen/slicesize)),1);	// At least one slice, max 50
	Circle.SliceStep	= -alen/Circle.NumSlices;

	Circle.Color	= color;
	Circle.Filled	= filled;
	Circle.Node		= node_guid;

	vector_3 c  = center;

	while (Circles.find(c.y) != Circles.end())
	{		
		c.y  += 0.001f;
	}
	Circle.Center = c;

	Circles[c.y] = Circle;

}


void nodespace_lines::Reset()
{
	Lines.clear();
	Circles.clear();
}




// ========================================================================
//   Function: Render
//   Description: Renders the lines in world space
// ========================================================================
void nodespace_lines::
Render(void) const
{
	Rapi& renderer = gSiegeEngine.Renderer();

	// Setup state
	renderer.SetTextureStageState(	0,
									D3DTOP_DISABLE,
									D3DTOP_BLENDDIFFUSEALPHA,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_WRAP,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DTEXF_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									false );

	bool bPrevFogState = renderer.SetFogActivatedState( false );

	{for(lines::const_iterator LineIterator = Lines.begin();
		 LineIterator != Lines.end();
		 ++LineIterator)
	{
		sVertex line[2];
		memset( line, 0, sizeof( sVertex ) * 2 );

		line[0].x		= (*LineIterator).Endpoints[0].x;
		line[0].y		= (*LineIterator).Endpoints[0].y;
		line[0].z		= (*LineIterator).Endpoints[0].z;
		line[0].color	= (*LineIterator).Color;

		line[1].x		= (*LineIterator).Endpoints[1].x;
		line[1].y		= (*LineIterator).Endpoints[1].y;
		line[1].z		= (*LineIterator).Endpoints[1].z;
		line[1].color	= line[0].color;

		// Get the node and set the matrix
		SiegeNode* node = gSiegeEngine.IsNodeValid( (*LineIterator).Node );
		if ( node != NULL )
		{
			renderer.SetWorldMatrix( node->GetCurrentOrientation(), node->GetCurrentCenter() );
			renderer.DrawPrimitive( D3DPT_LINELIST, line, 2, SVERTEX, NULL, 0 );
		}
	}}

	Lines.clear();

	{for(circles::const_iterator CircleIterator = Circles.begin();
		 CircleIterator != Circles.end();
		 ++CircleIterator)
	{

		const circle Circle = (*CircleIterator).second;
		
		SiegeNode* node = gSiegeEngine.IsNodeValid( Circle.Node );
		if ( node != NULL )
		{
			sVertex pts[53];

			DWORD numslices = min_t(50,(int)Circle.NumSlices);

			memset( pts, 0, sizeof( sVertex ) * (numslices+3) );

			pts[0].color	= Circle.Color;

			float theta = 0;

			theta = Circle.StartAngle;
			
			DWORD i = 1;
			while ( i <= (numslices+1) )
			{
				pts[i].x		= Cos(theta);
				pts[i].z		= Sin(theta);
				pts[i].color	= Circle.Color;
				theta			+= Circle.SliceStep;
				i++;
			}

			// Get the node and set the matrix
			renderer.SetWorldMatrix( node->GetCurrentOrientation(), node->GetCurrentCenter() );
			renderer.TranslateWorldMatrix(Circle.Center);
			renderer.ScaleWorldMatrix(vector_3(Circle.Radius,Circle.Radius,Circle.Radius) );

			pts[numslices+2] = pts[0];	

			if (Circle.Filled)
			{
				renderer.DrawPrimitive( D3DPT_TRIANGLEFAN, pts, (numslices+2), SVERTEX, NULL, 0 );

				vector_3 cv = GETDWORDCOLORV(Circle.Color);
				cv *= 2;

				if (cv.x > 1.0f) cv.x = 1.0f;
				if (cv.y > 1.0f) cv.y = 1.0f;
				if (cv.z > 1.0f) cv.z = 1.0f;

				DWORD brighter = (Circle.Color & (0xFF000000) | MAKEDWORDCOLOR(cv)) ;

				for (i = 0; i < (numslices+3); ++i )
				{
					pts[i].color	= brighter;
					pts[i].y		+= 0.0005f;
				}
			}
			renderer.DrawPrimitive( D3DPT_LINESTRIP, pts, (numslices+3), SVERTEX, NULL, 0 );
		}
	}}

	renderer.SetFogActivatedState( bPrevFogState );

	Circles.clear();
}
