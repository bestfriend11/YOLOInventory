/***********************************************************************************
**
**								SiegeLabel
**
**		This class is responsible for accumulating and rendering labels (3D text).
**
**		Author:		James Loe
**		Date:		01/12/00
**
***********************************************************************************/

#include "precomp_siege.h"

#include "RapiOwner.h"
#include "siege_engine.h"
#include "siege_camera.h"

#include "space_3.h"

#include "siege_label.h"

using namespace siege;

void SiegeLabel::AppendColorChangeEscapeSequence(gpstring& text, unsigned char red, unsigned char grn, unsigned char blu)
{
	if (red == 0) red = 1;
	if (grn == 0) grn = 1;
	if (blu == 0) blu = 1;
	text.appendf("%c%c%c%c%c",SIEGE_ESC_CHAR,'C',red,grn,blu);
}

template<typename T>
void ParseEscapeSequences(T& lbl, gpstring& tstr, int startpos)
{
	unsigned int escpos;
	while ( (escpos = tstr.find(SIEGE_ESC_CHAR,startpos)) != -1 )
	{
		if ( (tstr[escpos+1] == 'C') && (tstr.length() >= escpos+4) )
		{
			// This is a color change escape sequence
			unsigned char red = tstr[escpos+2];
			unsigned char grn = tstr[escpos+3];
			unsigned char blu = tstr[escpos+4];
			lbl.m_Color = MAKEDWORDCOLOR(red/255.0f,grn/255.0f,blu/255.0f);
			tstr = tstr.erase( escpos, escpos+5 );
		}
		else
		{
			// Unknown escape sequence
			tstr = tstr.erase( escpos, escpos+1 );
		}
	}
}

// WorldLabel drawing
void SiegeLabel::DrawLabel( RapiFont* pFont, const SiegePos& position, const gpstring& text,
						    float timeInSeconds, float textHeightInMeters, bool bCenter, DWORD color, bool scroll )
{
	// Setup parsing data
	int pos						= -1;
	gpstring tempString			= text;
	float offsetHeightInMeters	= 0.0f;

	// Setup label info
	WorldLabel Label;
	Label.m_pFont				= pFont;
	Label.m_Position			= position;
	Label.m_TextHeightInMeters	= scroll ? -1 : textHeightInMeters;
	Label.m_bCenter				= bCenter;
	Label.m_Color				= color;
	Label.m_Expiration			= timeInSeconds;
	Label.m_ScrollUp			= scroll;

	while( (pos = tempString.rfind( '\n' )) != -1 )
	{
		// Make the new label
		Label.m_OffsetInMeters	= offsetHeightInMeters;

		// Extract escape sequences in this line of text
		// Extract escape sequences in this line of text
		ParseEscapeSequences(Label,tempString,pos+1);
		Label.m_Text.assign( tempString, pos+1, tempString.length() - pos );

		// Put it in our list
		m_WorldLabels.push_back( Label );

		// Update the position and string information to account for the string
		if (scroll)
		{
			Label.m_TextHeightInMeters -= 1.0f;
		}
		else
		{
			offsetHeightInMeters	+= textHeightInMeters;
		}

		tempString				= tempString.erase( pos, tempString.length() - pos );

		// Restore the default color
		Label.m_Color			= color;	
	}

	// Make a final label with the remaining info
	Label.m_OffsetInMeters		= offsetHeightInMeters;

	// Extract escape sequences in the first line of text
	ParseEscapeSequences(Label,tempString,0);
	Label.m_Text				= tempString;

	m_WorldLabels.push_back( Label );
}

// ScreenLabel drawing
void SiegeLabel::DrawScreenLabel( RapiFont* pFont, const vector_3& position, const gpstring& text,
								  float timeInSeconds, bool bCenter, DWORD color, bool scroll )
{
	// Setup parsing data
	int pos						= -1;
	gpstring tempString			= text;
	float offsetHeightInPixels	= 0.0f;

	// Get the text height of this font
	int textWidthInPixels, textHeightInPixels;
	pFont->CalculateStringSize(	text.c_str(), textWidthInPixels, textHeightInPixels );

	// Setup label info
	ScreenLabel Label;
	Label.m_pFont				= pFont;
	Label.m_Position			= position;
	Label.m_bCenter				= bCenter;
	Label.m_Color				= color;
	Label.m_Expiration			= timeInSeconds;
	Label.m_ScrollUp			= scroll;

	while( (pos = tempString.rfind( '\n' )) != -1 )
	{
		// Make the new label
		Label.m_OffsetInPixels	= offsetHeightInPixels;

		// Extract escape sequences in this line of text
		ParseEscapeSequences(Label,tempString,pos+1);

		Label.m_Text.assign( tempString, pos+1, tempString.length() - pos );

		// Put it in our list
		m_ScreenLabels.push_back( Label );

		// Update the position and string information to account for the string
		offsetHeightInPixels	-= textHeightInPixels;
		tempString				= tempString.erase( pos, tempString.length() - pos );

		// Restore the default color
		Label.m_Color			= color;
	}

	// Make a final label with the remaining info
	Label.m_OffsetInPixels		= offsetHeightInPixels;

	// Extract escape sequences in the first line of text
	ParseEscapeSequences(Label,tempString,0);

	Label.m_Text				= tempString;

	m_ScreenLabels.push_back( Label );
}
				  
// Rendering
void SiegeLabel::Render( matrix_3x3& camOrient ) const
{
	// Get the Rapi renderer
	Rapi& renderer = gSiegeEngine.Renderer();

	// Get the time delta
	double delta_t = gSiegeEngine.GetDeltaFrameTime();

	// Go through all of the WorldLabels
	for( std::list< WorldLabel >::iterator w = m_WorldLabels.begin(); w != m_WorldLabels.end(); )
	{
		if( gSiegeEngine.IsNodeValid( (*w).m_Position.node ) )
		{
			// Get the node and set the matrix
			SiegeNodeHandle handle	= gSiegeEngine.NodeCache().UseObject( (*w).m_Position.node );
			SiegeNode& node			= handle.RequestObject( gSiegeEngine.NodeCache() );

			float text_height = (*w).m_TextHeightInMeters;
			float dist_camera_scale;

			if ((*w).m_ScrollUp)
			{
				// Do we need the distance to the camera scale factor?
				dist_camera_scale = gSiegeEngine.GetDifferenceVector(  gSiegeEngine.GetCamera().GetCameraSiegePos(),(*w).m_Position).Length() ;
			}
			else
			{
				dist_camera_scale = 1.0f;
			}

			vector_3 nDiff;

			if (!IsPositive(text_height))
			{
				// Assume text at viewport projection plane is 1cm high in WCS
				// Value of text_height is -(num of lines)
				float screen_text_height = 0.0125f * dist_camera_scale;
				nDiff.y -= text_height * screen_text_height ;
				text_height = screen_text_height;
			}

			if( (*w).m_bCenter )
			{
				// Get the pixel cost of the string
				int width, height;
				(*w).m_pFont->CalculateStringSize( (*w).m_Text.c_str(), width, height );

				// Calculate, give the text height, the size ratio
				float ratio	= text_height / (float)height;

				// Move the position to the center
				nDiff.x += ((ratio * (float)width) / 2.0f);
				nDiff.y -= (text_height / 2.0f);
			}

			// Set the node matrix
			renderer.SetWorldMatrix( camOrient, node.NodeToWorldSpace( (*w).m_Position.pos ) + ((*w).m_OffsetInMeters * camOrient.GetColumn1_T()) );

			// Draw the text
			(*w).m_pFont->Print3D( nDiff, (*w).m_Text.c_str(), text_height, (*w).m_Color );

			// Destroy the label
			(*w).m_Expiration -= delta_t;

			if( !IsPositive((float)(*w).m_Expiration) )
			{
				w	= m_WorldLabels.erase( w );
			}
			else
			{
				if ((*w).m_ScrollUp)
				{
					float pixels_per_meter = 10;			// In current view space
					float meters_per_second = 0.01f;
					
					float scroll_increment_in_screenspace = 
						pixels_per_meter * meters_per_second * (float)delta_t;
		
					(*w).m_Position.pos.y += scroll_increment_in_screenspace * dist_camera_scale;
				}
				++w;
			}
		}
		else
		{
			w	= m_WorldLabels.erase( w );
		}
	}

	// Go through all of the ScreenLabels
	for( std::list< ScreenLabel >::iterator s = m_ScreenLabels.begin(); s != m_ScreenLabels.end(); )
	{
		vector_3 nDiff( (*s).m_Position );
		if( (*s).m_bCenter )
		{
			// Get the pixel cost of the string
			int width, height;
			(*s).m_pFont->CalculateStringSize( (*s).m_Text.c_str(), width, height );

			// Move the position to the center
			nDiff.x -= (float)(width >> 1);
			nDiff.y -= (float)(height >> 1);
		}

		// Draw the text
		(*s).m_pFont->Print( FTOL( nDiff.x ), FTOL( nDiff.y + (*s).m_OffsetInPixels ), (*s).m_Text.c_str(), (*s).m_Color );

		// Destroy the label
		(*s).m_Expiration -= delta_t;

		if( !IsPositive((float)(*s).m_Expiration) )
		{
			s	= m_ScreenLabels.erase( s );
		}
		else
		{
			++s;
		}
	}
}

// Clear the labels
void SiegeLabel::Reset()
{
	m_WorldLabels.clear();
	m_ScreenLabels.clear();
}
