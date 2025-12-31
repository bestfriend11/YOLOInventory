#pragma once
#ifndef _SIEGE_LABEL_
#define _SIEGE_LABEL_


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

#include "siege_pos.h"

class RapiFont;

// Define a character that can be used to signal an escape sequence
// in the text of a label --biddle
const char SIEGE_ESC_CHAR = '\x1B';

namespace siege
{
	struct WorldLabel
	{
		RapiFont*	m_pFont;
		SiegePos	m_Position;
		gpstring	m_Text;
		float		m_TextHeightInMeters;
		bool		m_bCenter;
		DWORD		m_Color;
		double		m_Expiration;
		float		m_OffsetInMeters;
		bool		m_ScrollUp;
	};

	struct ScreenLabel
	{
		RapiFont*	m_pFont;
		vector_3	m_Position;
		gpstring	m_Text;
		bool		m_bCenter;
		DWORD		m_Color;
		double		m_Expiration;
		float		m_OffsetInPixels;
		bool		m_ScrollUp;
	};

	class SiegeLabel
	{
		public:

			// Label drawing
			void DrawLabel( RapiFont* pFont, const SiegePos& position, const gpstring& text, 
							float timeInSeconds = 0.0f, float textHeightInMeters = 0.2f, bool bCenter = true,
							DWORD color = 0xFFFFFFFF, bool scroll = false );

			void DrawScreenLabel( RapiFont* pFont, const vector_3& screenPos, const gpstring& text,
								float timeInSeconds = 0.0f, bool bCenter = true, 
								DWORD color = 0xFFFFFFFF, bool scroll = false );
				  
			// Rendering
  			virtual void Render( matrix_3x3& camOrient ) const;

			// Clear the labels
			void Reset();
		
			// Utility function to encode colour changes
			static void AppendColorChangeEscapeSequence(gpstring& text, unsigned char red, unsigned char grn, unsigned char blu);

		private:

			mutable std::list< WorldLabel >		m_WorldLabels;
			mutable std::list< ScreenLabel >	m_ScreenLabels;
	};

}

#endif