
//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_line.cpp
// Author(s):  Jessica Tams
//
// Summary  :  ui class to draw a line from a list of numbers
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "precomp_gamegui.h"
#include "ui_shell.h"
#include "ui_window.h"
#include "ui_line.h"


UILine::UILine( FastFuelHandle fhWindow, gpstring parent_interface, UIWindow *parent )
: UIWindow( fhWindow, parent_interface, parent )
, m_lineThickness( 1 )
{
	SetType( UI_TYPE_LINE );
	SetInputType( TYPE_INPUT_NONE );
	m_color = 0xff0000ff;
	fhWindow.Get( "linethickness", m_lineThickness );
}

UILine::UILine()
: UIWindow()
, m_lineThickness( 1 )
{
	SetType( UI_TYPE_LINE );
	SetInputType( TYPE_INPUT_NONE );
	m_color = 0xff0000ff;
}

UILine::~UILine()
{
}


void UILine::GetMaxGroupValue( double& xMax, double& yMax ) 
{
	xMax		= 0.0;
	yMax		= 0.0;

	double xCurrMax;
	double yCurrMax;
	
	UIWindowVec graphsInGroup = gUIShell.ListWindowsOfGroup( GetGroup() );
	for( std::vector< UIWindow* >::iterator graphs = graphsInGroup.begin(); graphs != graphsInGroup.end(); ++graphs )
	{
		if(  (*graphs)->GetType() == UI_TYPE_LINE )
		{
			((UILine*)(*graphs))->GetMaxLineValue(xCurrMax, yCurrMax);
			xMax = max( xCurrMax, xMax);
			yMax = max( yCurrMax, yMax);
		}
	}
}

void UILine::GetMaxLineValue( double& xMax, double& yMax ) 
{
	xMax = 0.0;
	yMax = 0.0;

	for( LineValues::const_iterator i = m_lineValues.begin(); i != m_lineValues.end(); ++i )
	{
		yMax = max( i->y, yMax );
		xMax = max( i->x, xMax );
	}
}

void UILine::Draw () 
{
	if( GetVisible() )
	{
		UIWindow::Draw();

		//
		//	If we don't have enough for a line, just make one :)
		//
		if( m_lineValues.size() < 2  )
		{
			m_lineValues.push_back( StatEntry(0,0) );
			m_lineValues.push_back( StatEntry(1,0) );
		}

		if( m_lineValues.size() > 0 )
		{
			//
			//	find the max of all the lines in the group, so we can scale the values
			//
			double yMax;
			double xGroupMax;

			GetMaxGroupValue(xGroupMax, yMax);

			//
			//	determine the scaling for the lines based on the size of the rect, and the max value
			//
			float yScaling = yMax > 0 ? (float) PreciseDivide( GetRect().bottom - GetRect().top, yMax) : 0;
			float xScaling = xGroupMax > 0 ? (float) PreciseDivide( (GetRect().right - GetRect().left), (xGroupMax) ): 0;

			//
			//	get ready to draw the graph
			//
			gDefaultRapi.GetDevice()->SetRenderState( D3DRS_ZFUNC,	D3DCMP_ALWAYS );

			gUIShell.GetRenderer().SetTextureStageState(	0,
				D3DTOP_DISABLE,
				D3DTOP_MODULATE,
				D3DTADDRESS_CLAMP,
				D3DTADDRESS_CLAMP,
				D3DTEXF_POINT,
				D3DTEXF_POINT,
				D3DTEXF_POINT,
				D3DBLEND_SRCALPHA,
				D3DBLEND_INVSRCALPHA,
				true );

			tVertex Verts[4];
			memset( Verts, 0, sizeof( tVertex ) * 4 );

			tVertex oldVerts[4];
			memset( oldVerts, 0, sizeof( tVertex ) * 4 );

			//
			//	draw each section of the graph!
			//
			float yValue = 0;
			float yValuePrevious = 0;

			float xValue = 0;
			float xValuePrevious = 0;

			LineValues::iterator lineIter = m_lineValues.begin();

			if( lineIter != m_lineValues.end() )
			{
				// this is the first value!
				yValuePrevious	= (float) GetRect().bottom;
				yValue			= (float) (GetRect().bottom - ((lineIter->y) * yScaling));

				xValuePrevious	= (float) GetRect().left;
				xValue			= (float) (GetRect().left + PreciseMultiply((lineIter->x), xScaling));

				++lineIter;

				float yNormalized = 0.0f;
				float xNormalized = 0.0f;

				oldVerts[2].x = xValuePrevious;
				oldVerts[2].y = yValuePrevious;
				oldVerts[3].x = xValuePrevious;
				oldVerts[3].y = yValuePrevious;

				for( ;lineIter != m_lineValues.end(); ++lineIter )
				{		
					yValuePrevious = yValue;
					yValue	= (float) (GetRect().bottom - ((lineIter->y) * yScaling));

					xValuePrevious	= xValue;
					xValue			= (float) (GetRect().left + PreciseMultiply((lineIter->x), xScaling));

					yNormalized = (float) (m_lineThickness * (yValuePrevious-yValue) / sqrt( (yValuePrevious-yValue)*(yValuePrevious-yValue) + PreciseSubtract(xValuePrevious,xValue)*PreciseSubtract(xValuePrevious,xValue) ) );
					xNormalized = (float) (m_lineThickness * (xValue-xValuePrevious) / sqrt( (yValuePrevious-yValue)*(yValuePrevious-yValue) + PreciseSubtract(xValuePrevious,xValue)*PreciseSubtract(xValuePrevious,xValue) ) );

					gpassert( yValue >= GetRect().top && yValuePrevious >= GetRect().top && yValue <= GetRect().bottom && yValuePrevious <= GetRect().bottom );

					//
					//	Draw the connecting section
					//

					Verts[0].x			= oldVerts[2].x;
					Verts[0].y			= oldVerts[2].y;
					Verts[0].z			= 0.0f;
					Verts[0].rhw		= 1.0f;
					Verts[0].uv.u		= 0.0f;
					Verts[0].uv.v		= 0.0f;
					Verts[0].color		= m_color;
									
					Verts[1].x			= oldVerts[3].x;
					Verts[1].y			= oldVerts[3].y;
					Verts[1].z			= 1.0f;
					Verts[1].rhw		= 1.0f;
					Verts[1].uv.u		= 0.0f;
					Verts[1].uv.v		= 1.0f;
					Verts[1].color		= m_color;
					
					Verts[2].x			= xValuePrevious - yNormalized;
					Verts[2].y			= yValuePrevious - xNormalized;
					Verts[2].z			= 0.0f;
					Verts[2].rhw		= 1.0f;
					Verts[2].uv.u		= 1.0f;
					Verts[2].uv.v		= 0.0f;
					Verts[2].color		= m_color;
					
					Verts[3].x			= xValuePrevious + yNormalized;
					Verts[3].y			= yValuePrevious + xNormalized;
					Verts[3].z			= 0.0f;
					Verts[3].rhw		= 1.0f;
					Verts[3].uv.u		= 1.0f;
					Verts[3].uv.v		= 1.0f;
					Verts[3].color		= m_color;
					gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );


					//
					//	Draw the line!
					//
					Verts[0].x			= xValuePrevious - yNormalized;
					Verts[0].y			= yValuePrevious - xNormalized;

					Verts[1].x			= xValuePrevious + yNormalized;
					Verts[1].y			= yValuePrevious + xNormalized;

					Verts[2].x			= xValue - yNormalized;
					Verts[2].y			= yValue - xNormalized;

					Verts[3].x			= xValue + yNormalized;
					Verts[3].y			= yValue + xNormalized;

					gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

					oldVerts[2].x = Verts[2].x;
					oldVerts[2].y = Verts[2].y;
					oldVerts[3].x = Verts[3].x;
					oldVerts[3].y = Verts[3].y;		
				}
/*
				//
				//	Make sure we connect to the end!!
				//
				// the connecting section
				Verts[0].x			= oldVerts[2].x;
				Verts[0].y			= oldVerts[2].y;
				
				Verts[1].x			= oldVerts[3].x;
				Verts[1].y			= oldVerts[3].y;

				Verts[2].x			= xValue;
				Verts[2].y			= yValue - m_lineThickness;
				
				Verts[3].x			= xValue;
				Verts[3].y			= yValue + m_lineThickness;
				gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

				// the line to the end.
				Verts[0].x			= xValue;
				Verts[0].y			= yValue - m_lineThickness;

				Verts[1].x			= xValue;
				Verts[1].y			= yValue + m_lineThickness;
		
				Verts[2].x			= GetRect().left + (float) PreciseMultiply(xGroupMax, xScaling) - m_lineThickness;
				Verts[2].y			= yValue - m_lineThickness;

				Verts[3].x			= GetRect().left + (float) PreciseMultiply(xGroupMax, xScaling) + m_lineThickness;
				Verts[3].y			= yValue + m_lineThickness;
				gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );

*/
			}
		}
	}
}
