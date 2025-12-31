/*************************************************************************************
**
**									RapiFont
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	06/14/99
**		Author: James Loe
**
*************************************************************************************/

#include "precomp_rapi.h"
#include "gpcore.h"

#include "RapiOwner.h"
#include "RapiImage.h"
#include "WinX.h"
#include "LocHelp.h"


// Existence
RapiFont::RapiFont(	Rapi*	pDevice,
					HDC		hdc,
					HFONT	font,
					bool	forceBitmap )
					: m_Texture( 0 )
					, m_pDevice( pDevice )					
					, m_characterHeightInPixels( 0 )
					, m_textureWidth( 0 )
					, m_textureHeight( 0 )
					, m_startRange( 0 )
					, m_endRange( 255 )
					, m_bCoordinateFlip( false )
					, m_hFont( 0 )
					, m_dwBgColor( 0xffffffff )
					, m_pImage( NULL )
{
	memset( (void *)m_characterWidthInPixels, 0, NumberOfCharacters * sizeof( int ) );

	if (( LocMgr::DoesSingletonExist() && gLocMgr.IsAnsi() ) || !LocMgr::DoesSingletonExist() || forceBitmap )
	{
		BuildFromWin32Font( hdc, font );
	}
	else
	{
		m_hFont = font;
	}
}

RapiFont::RapiFont( Rapi*			pDevice,
				    unsigned int	texture,
					UINT32			characterHeight,
					UINT32			startRange,
					UINT32			endRange,
					gpstring		sTexture )
					: m_Texture( 0 )
					, m_pDevice( pDevice )					
					, m_characterHeightInPixels( characterHeight )
					, m_textureWidth( m_pDevice->GetTextureWidth( texture ) )
					, m_textureHeight( m_pDevice->GetTextureHeight( texture ) )
					, m_startRange( startRange )
					, m_endRange( endRange )
					, m_bCoordinateFlip( true )
					, m_sTexture( sTexture )
					, m_hFont( 0 )
					, m_dwBgColor( 0xffffffff )
					, m_pImage( NULL )
{
	memset( (void *)m_characterWidthInPixels, 0, NumberOfCharacters * sizeof( int ) );
	BuildFromTexture( texture );
}

RapiFont::~RapiFont(void)
{
	if( m_Texture )
	{
		// Destroy our texture
		m_pDevice->DestroyTexture( m_Texture );
	}	

	if ( m_hFont )
	{
		DeleteObject( m_hFont );
	}

	TextCache::iterator i;
	for ( i = m_textCache.begin(); i != m_textCache.end(); ++i )
	{
		TextComponents::iterator j;
		for ( j = (*i).textComponents.begin(); j != (*i).textComponents.end(); ++j )
		{
			gDefaultRapi.DestroySurface( (*j).texture );
		}
	}

	// Remove this font from the internal list
	m_pDevice->RemoveFromFontList( this );

}

// Sizing
void RapiFont::CalculateStringSize(	char const * const String,
									int &Width,
									int &Height)
{
	Width	= 0;
	Height	= m_characterHeightInPixels;
	if( String )
	{
		if ( m_hFont == NULL )
		{
			for( const char *StringPointer = String; *StringPointer; ++StringPointer)
			{
				Width += m_characterWidthInPixels[(unsigned char )(*StringPointer)];
			}
		}
		else
		{
			RECT rect;
			HDC hDC = GetDC( gDefaultRapi.m_hwnd );
			SelectObject( hDC, m_hFont );
			DrawText( hDC, String, -1, &rect, DT_CALCRECT | DT_NOPREFIX );
			Width	= rect.right - rect.left;
			Height	= rect.bottom - rect.top;
			ReleaseDC( gDefaultRapi.m_hwnd, hDC );
		}
	}
	else
	{
		gpassertm( 0, "0 string pointer passed");
	}
}


void RapiFont::CalculateStringSize(	gpwstring sText, int & width, int & height )
{
	CalculateStringSize( ToAnsi( sText ), width, height );
}


void RapiFont::Print(	int StartX, int StartY,
						gpwstring sText,
						DWORD dwColor,
						bool bUseLinear )
{
	bool bAlphaBlend = m_pDevice->EnableAlphaBlending( true );

	if ( m_hFont == NULL )
	{
		Print( StartX, StartY, ToAnsi( sText ), dwColor, bUseLinear );
	}
	else if ( m_hFont && sText.size() != 0 )
	{
		TextCache::iterator i;
		for ( i = m_textCache.begin(); i != m_textCache.end(); ++i )
		{
			if ( sText.same_with_case( (*i).sText ) )
			{
				RenderTrueType( *i, dwColor, StartX, StartY );
				return;
			}
		}

		RapiTrueTypeText ttt;
		ttt.sText = sText;

		int width	= 0;
		int height	= 0;
		CalculateStringSize( sText, width, height );

		std::vector< gpwstring > lines;
		lines.push_back( sText );
		if ( width >= 256 )
		{
			int greatestWidth = width;
			while ( greatestWidth >= 256 )
			{
				greatestWidth = 0;
				std::vector< gpwstring >::iterator iString;
				std::vector< gpwstring > newStrings;
				for ( iString = lines.begin(); iString != lines.end(); ++iString )
				{
					gpwstring sNew;
					int half = (int)((*iString).size()/2);
					sNew = (*iString).substr( half, (*iString).size() );
					(*iString) = (*iString).substr( 0, half );
					newStrings.push_back( *iString );
					newStrings.push_back( sNew );
				}

				lines.clear();
				for ( iString = newStrings.begin(); iString != newStrings.end(); ++iString )
				{
					lines.push_back( *iString );
				}

				for ( iString = lines.begin(); iString != lines.end(); ++iString )
				{
					CalculateStringSize( *iString, width, height );
					if ( width >= greatestWidth )
					{
						greatestWidth = width;
					}
				}
			}
		}

		int currX = StartX;
		std::vector< gpwstring >::iterator iString;
		for ( iString = lines.begin(); iString != lines.end(); ++iString )
		{
			CalculateStringSize( *iString, width, height );

			int originalWidth = width;
			int originalHeight = height;

			height = MakePower2Up( height );
			width = MakePower2Up( width );
			if( width == 0 || height == 0 )
			{
				continue;
			}

			while( (height > width) && ((height / width) > 8)) 
			{
				width = MakePower2Up( width + 1 );
			}			

			while( (width > height) && ((width / height) > 8))
			{
				height = MakePower2Up( height + 1 );
			}
			
			TTFTextComponent ttc;
			ttc.rect.left = currX;
			ttc.rect.top	= StartY;
			ttc.rect.right	= currX + originalWidth;
			ttc.rect.bottom	= StartY + originalHeight;
			currX += originalWidth;

			ttc.uvrect.left		= 0.0f;
			ttc.uvrect.top		= 0.0f;
			ttc.uvrect.right	= (float)originalWidth / (float)MakePower2Up( width );
			ttc.uvrect.bottom	= (float)originalHeight / (float)MakePower2Up( height );						

			// Create system memory texture surface
			gpstring sError;
			ttc.texture = m_pDevice->CreateSurface( MakePower2Up( width ), MakePower2Up( height ), GetBackgroundColor(), TEX_DYNAMIC, false, false, false, false, sError );
/*
			HDC hDC;
			m_pDevice->m_Surfaces[ ttc.texture ]->GetSurface()->GetDC( &hDC );
			SelectObject( hDC, m_hFont );
			SetTextColor( hDC, RGB(255,255,255) );
			SetBkColor( hDC, GetBackgroundColor() );
			SetBkMode ( hDC, TRANSPARENT );
			TextOutW( hDC, 0, 0, (*iString).c_str(), (*iString).size() );
			m_pDevice->m_Surfaces[ ttc.texture ]->GetSurface()->ReleaseDC( hDC );
*/
			ttt.textComponents.push_back( ttc );
		}

		if ( ttt.textComponents.size() != 0 )
		{
			ttt.refCount = 1;
			m_textCache.push_back( ttt );
			RenderTrueType( ttt, dwColor, StartX, StartY );
		}
	}

	m_pDevice->EnableAlphaBlending( bAlphaBlend );
}


void RapiFont::RenderTrueType( RapiTrueTypeText & ttt, DWORD dwColor, int StartX, int StartY )
{
	// Return if no text to render
	if( ttt.textComponents.empty() )
	{
		return;
	}

	dwColor	|= 0xFF000000;

	// Setup proper texture stages
	gDefaultRapi.SetTextureStageState(	0,
										D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_WRAP,
										D3DTADDRESS_WRAP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_INVSRCALPHA,
										D3DBLEND_SRCALPHA,
										false );
	gDefaultRapi.SetSingleStageState(	1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	gDefaultRapi.SetSingleStageState(	1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	// Disable texture transformations
	TRYDX (m_pDevice->GetDevice()->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 ) );
	TRYDX (m_pDevice->GetDevice()->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE ) );

	TextComponents::iterator i;
	int offsetX = StartX - (*ttt.textComponents.begin()).rect.left;
	int offsetY = StartY - (*ttt.textComponents.begin()).rect.top;

	unsigned int startIndex = 0;
	tVertex* pVerts	= NULL;
	if( m_pDevice->LockBufferedTVertices( ttt.textComponents.size() * 4, startIndex, pVerts ) )
	{
		for ( i = ttt.textComponents.begin(); i != ttt.textComponents.end(); ++i, pVerts += 4 )
		{
			TTFTextComponent ttc = *i;

			pVerts[0].x			= ttc.rect.left + offsetX - 0.5f;
			pVerts[0].y			= ttc.rect.top + offsetY - 0.5f;
			pVerts[0].z			= 0.0f;
			pVerts[0].rhw		= 1.0f;
			pVerts[0].uv.u		= ttc.uvrect.left;
			pVerts[0].uv.v		= ttc.uvrect.top;
			pVerts[0].color		= dwColor;

			pVerts[1].x			= ttc.rect.left + offsetX - 0.5f;
			pVerts[1].y			= ttc.rect.bottom + offsetY - 0.5f;
			pVerts[1].z			= 0.0f;
			pVerts[1].rhw		= 1.0f;
			pVerts[1].uv.u		= ttc.uvrect.left;
			pVerts[1].uv.v		= ttc.uvrect.bottom;;
			pVerts[1].color		= dwColor;

			pVerts[2].x			= ttc.rect.right + offsetX - 0.5f;
			pVerts[2].y			= ttc.rect.top + offsetY - 0.5f;
			pVerts[2].z			= 0.0f;
			pVerts[2].rhw		= 1.0f;
			pVerts[2].uv.u		= ttc.uvrect.right;
			pVerts[2].uv.v		= ttc.uvrect.top;
			pVerts[2].color		= dwColor;

			pVerts[3].x			= ttc.rect.right + offsetX - 0.5f;
			pVerts[3].y			= ttc.rect.bottom + offsetY - 0.5f;
			pVerts[3].z			= 0.0f;
			pVerts[3].rhw		= 1.0f;
			pVerts[3].uv.u		= ttc.uvrect.right;
			pVerts[3].uv.v		= ttc.uvrect.bottom;
			pVerts[3].color		= dwColor;
		}

		// Unlock the buffer now that it is filled
		m_pDevice->UnlockBufferedTVertices();

		// Setup for rendering
		TRYDX( m_pDevice->GetDevice()->SetPixelShader( NULL ) );
		TRYDX( m_pDevice->GetDevice()->SetVertexShader( TVERTEX ) );
		TRYDX( m_pDevice->GetDevice()->SetStreamSource( 0, m_pDevice->m_pTVBuffer, sizeof( tVertex ) ) );

		for ( i = ttt.textComponents.begin(); i != ttt.textComponents.end(); ++i, startIndex += 4 )
		{
			TRYDX( m_pDevice->GetDevice()->SetTexture( 0, m_pDevice->m_Surfaces[(*i).texture]->GetSurface() ) );
			TRYDX( m_pDevice->GetDevice()->SetTexture( 1, NULL ) );

			TRYDX( m_pDevice->GetDevice()->DrawPrimitive( D3DPT_TRIANGLESTRIP, startIndex, 2 ) );
		}
	}

	TRYDX (m_pDevice->GetDevice()->SetTexture( 0, NULL ) );
	for ( unsigned int j = 0; j < NUM_TEXTURES; ++j )
	{
		m_pDevice->m_ActiveTex[j] = NULL;
	}
}


void RapiFont::RegisterString( gpwstring sText )
{
	if ( sText.empty() )
	{
		return;
	}

	TextCache::iterator i;
	for ( i = m_textCache.begin(); i != m_textCache.end(); ++i )
	{
		if ( sText.same_with_case( (*i).sText ) )
		{
			(*i).refCount++;
			return;
		}
	}
	return;
}


void RapiFont::UnregisterString( gpwstring sText )
{
	if ( sText.empty() )
	{
		return;
	}

	TextCache::iterator i;
	for ( i = m_textCache.begin(); i != m_textCache.end(); ++i )
	{
		if ( sText.same_with_case( (*i).sText ) )
		{
			(*i).refCount--;
			if ( (*i).refCount == 0 )
			{
				TextComponents::iterator j;
				for ( j = (*i).textComponents.begin(); j != (*i).textComponents.end(); ++j )
				{
					gDefaultRapi.DestroySurface( (*j).texture );
				}

				i = m_textCache.erase( i );
			}
			return;
		}
	}

	return;
}

void RapiFont::ResetSurfaces()
{
	TextCache::iterator i = m_textCache.begin(); 
	while ( i != m_textCache.end() )
	{
		TextComponents::iterator j;
		for ( j = (*i).textComponents.begin(); j != (*i).textComponents.end(); ++j )
		{
			gDefaultRapi.DestroySurface( (*j).texture );
		}

		i = m_textCache.erase( i );
		
		if ( i == m_textCache.end() )
		{
			return;
		}
	}
}

void RapiFont::Print(	int StartX, int StartY,
						char const * const String,
						DWORD dwColor,
						bool bUseLinear )
{
	// Make sure we're not it wireframe mode
	bool bWireframe	= m_pDevice->IsWireframe();
	if( bWireframe ){
		m_pDevice->SetWireframe( false );
	}

	// Draw our text
	if( String )
	{
		if ( !bUseLinear ) {
			m_pDevice->SetTextureStageState(0,
											D3DTOP_MODULATE,
											D3DTOP_SELECTARG1,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_POINT,
											D3DTEXF_POINT,
											D3DTEXF_POINT,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											true );
		}
		else {
			m_pDevice->SetTextureStageState(0,
											D3DTOP_MODULATE,
											D3DTOP_SELECTARG1,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DTEXF_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											true );
		}

		vector_3 const XBasis(1.0f, 0, 0);
		vector_3 const YBasis(0, 1.0f, 0);

		vector_3 const HeightVector( YBasis * real(m_characterHeightInPixels) );
		vector_3 Position(0, 0, 0);
		for( char const *StringPointer = String; *StringPointer; ++StringPointer )
		{
			int xOffset		= m_characterXOffsetInPixels[ (unsigned char)*StringPointer ];
			int yOffset		= m_characterYOffsetInPixels[ (unsigned char)*StringPointer ];
			int charWidth	= m_characterWidthInPixels[(unsigned char)*StringPointer];

#if !GP_RETAIL
			if ( charWidth == 0 && (*StringPointer) != '\r' )
			{
				xOffset		= m_characterXOffsetInPixels[ '@' ];
				yOffset		= m_characterYOffsetInPixels[ '@' ];
				charWidth	= m_characterWidthInPixels[ '@' ];
			}
#endif

			// Figure out the boundary of this character
			vector_3 const NextPosition = Position + XBasis * real(charWidth);

			// Some fonts seem to experience artifacting on their right side, so that is the reason for the
			// additional offset.
			UINT32 const SpaceForBadDrivers = 0;
			real const LeftTextureXCoordinate	=	real( xOffset - SpaceForBadDrivers ) / real( m_textureWidth );
			real const RightTextureXCoordinate	=	real( xOffset +	charWidth - SpaceForBadDrivers ) / real( m_textureWidth );
			real TopTextureYCoordinate, BottomTextureYCoordinate;

			if( m_bCoordinateFlip )
			{
				BottomTextureYCoordinate	=	real( yOffset ) / real( m_textureHeight );
				TopTextureYCoordinate		=	real( yOffset + m_characterHeightInPixels ) / real( m_textureHeight );
			}
			else
			{
				TopTextureYCoordinate		=	real( yOffset ) / real( m_textureHeight );
				BottomTextureYCoordinate	=	real( yOffset +	m_characterHeightInPixels ) / real( m_textureHeight );
			}


			// Draw font character
			tVertex		fontverts[4];
			vector_3	fontvect;

			fontvect				= Position;
			fontverts[0].x			= fontvect.GetX()+StartX - 0.5f;
			fontverts[0].y			= fontvect.GetY()+StartY - 0.5f;
			fontverts[0].z			= fontvect.GetZ();
			fontverts[0].rhw		= 1.0f;
			fontverts[0].uv.u		= LeftTextureXCoordinate;
			fontverts[0].uv.v		= TopTextureYCoordinate;
			fontverts[0].color		= dwColor;

			fontvect				= (Position + HeightVector);
			fontverts[1].x			= fontvect.GetX()+StartX - 0.5f;
			fontverts[1].y			= fontvect.GetY()+StartY - 0.5f;
			fontverts[1].z			= fontvect.GetZ();
			fontverts[1].rhw		= 1.0f;
			fontverts[1].uv.u		= LeftTextureXCoordinate;
			fontverts[1].uv.v		= BottomTextureYCoordinate;
			fontverts[1].color		= dwColor;

			fontvect				= NextPosition;
			fontverts[2].x			= fontvect.GetX()+StartX - 0.5f;
			fontverts[2].y			= fontvect.GetY()+StartY - 0.5f;
			fontverts[2].z			= fontvect.GetZ();
			fontverts[2].rhw		= 1.0f;
			fontverts[2].uv.u		= RightTextureXCoordinate;
			fontverts[2].uv.v		= TopTextureYCoordinate;
			fontverts[2].color		= dwColor;

			fontvect				= (NextPosition + HeightVector);
			fontverts[3].x			= fontvect.GetX()+StartX - 0.5f;
			fontverts[3].y			= fontvect.GetY()+StartY - 0.5f;
			fontverts[3].z			= fontvect.GetZ();
			fontverts[3].rhw		= 1.0f;
			fontverts[3].uv.u		= RightTextureXCoordinate;
			fontverts[3].uv.v		= BottomTextureYCoordinate;
			fontverts[3].color		= dwColor;

			m_pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, fontverts, 4, TVERTEX, &m_Texture, 1 );

			// Advance to the next character
			Position = NextPosition;
		}
	}
	else
	{
		gpassertm( 0, "Bad string passed to Print() in the RapiFont class" );
	}

	if( bWireframe ){
		m_pDevice->SetWireframe( true );
	}
}

// Print text facing the camera at the location specified in 3D space
void RapiFont::Print3D( vector_3& textPos,
						char const * const String, const float textHeightInMeters,
						DWORD dwColor )
{	
	// Make sure we're not it wireframe mode
	bool bWireframe	= m_pDevice->IsWireframe();
	if( bWireframe )
	{
		m_pDevice->SetWireframe( false );
	}

	// Draw our text
	if( String )
	{
		m_pDevice->SetTextureStageState(0,
										D3DTOP_MODULATE,
										D3DTOP_SELECTARG1,
										D3DTADDRESS_WRAP,
										D3DTADDRESS_WRAP,
										D3DTEXF_POINT,
										D3DTEXF_POINT,
										D3DTEXF_POINT,
										D3DBLEND_ONE,
										D3DBLEND_ZERO,
										true );

		// Calculate the basis vectors
		float ratio	= (textHeightInMeters / (float)m_characterHeightInPixels);
		vector_3 const XBasis(ratio, 0, 0);
		vector_3 const YBasis(0, ratio, 0);

		vector_3 const HeightVector( YBasis * real(m_characterHeightInPixels) );
		vector_3 Position( textPos );
		for( char const *StringPointer = String; *StringPointer; ++StringPointer )
		{
			// Figure out the boundary of this character
			vector_3 const NextPosition = Position - XBasis * real(m_characterWidthInPixels[(unsigned char)*StringPointer]);

			real const LeftTextureXCoordinate	=	real( m_characterXOffsetInPixels[ (unsigned char)*StringPointer ] ) /
													real( m_textureWidth );
			real const RightTextureXCoordinate	=	real( m_characterXOffsetInPixels[ (unsigned char)*StringPointer ] +
													m_characterWidthInPixels[ (unsigned char)*StringPointer ] ) /
													real( m_textureWidth );
			real TopTextureYCoordinate, BottomTextureYCoordinate;

			if( m_bCoordinateFlip )
			{
				BottomTextureYCoordinate	=	real( m_characterYOffsetInPixels[ (unsigned char)*StringPointer ] ) /
													real( m_textureHeight );
				TopTextureYCoordinate		=	real( m_characterYOffsetInPixels[ (unsigned char)*StringPointer ] +
													m_characterHeightInPixels ) / real( m_textureHeight );
			}
			else
			{
				TopTextureYCoordinate		=	real( m_characterYOffsetInPixels[ (unsigned char)*StringPointer ] ) /
													real( m_textureHeight );
				BottomTextureYCoordinate	=	real( m_characterYOffsetInPixels[ (unsigned char)*StringPointer ] +
													m_characterHeightInPixels ) / real( m_textureHeight );
			}


			// Draw font character
			sVertex		fontverts[4];
			vector_3	fontvect;

			fontvect				= NextPosition;
			fontverts[0].x			= fontvect.GetX();
			fontverts[0].y			= fontvect.GetY();
			fontverts[0].z			= fontvect.GetZ();
			fontverts[0].uv.u		= RightTextureXCoordinate;
			fontverts[0].uv.v		= BottomTextureYCoordinate;
			fontverts[0].color		= dwColor;

			fontvect				= (NextPosition + HeightVector);
			fontverts[1].x			= fontvect.GetX();
			fontverts[1].y			= fontvect.GetY();
			fontverts[1].z			= fontvect.GetZ();
			fontverts[1].uv.u		= RightTextureXCoordinate;
			fontverts[1].uv.v		= TopTextureYCoordinate;
			fontverts[1].color		= dwColor;

			fontvect				= Position;
			fontverts[2].x			= fontvect.GetX();
			fontverts[2].y			= fontvect.GetY();
			fontverts[2].z			= fontvect.GetZ();
			fontverts[2].uv.u		= LeftTextureXCoordinate;
			fontverts[2].uv.v		= BottomTextureYCoordinate;
			fontverts[2].color		= dwColor;

			fontvect				= (Position + HeightVector);
			fontverts[3].x			= fontvect.GetX();
			fontverts[3].y			= fontvect.GetY();
			fontverts[3].z			= fontvect.GetZ();
			fontverts[3].uv.u		= LeftTextureXCoordinate;
			fontverts[3].uv.v		= TopTextureYCoordinate;
			fontverts[3].color		= dwColor;

			m_pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, fontverts, 4, SVERTEX, &m_Texture, 1 );

			// Advance to the next character
			Position = NextPosition;
		}
	}
	else
	{
		gpassertm( 0, "Bad string passed to Print() in the RapiFont class" );
	}

	if( bWireframe ){
		m_pDevice->SetWireframe( true );
	}
}

// Modification
void RapiFont::BuildFromWin32Font(HDC hdc, HFONT font)
{
	// Free the texture if it exists
	if( m_Texture ){
		m_pDevice->DestroyTexture( m_Texture );
		m_Texture = NULL;
	}

	// Start with the initial values
	m_characterHeightInPixels	= 0;
	m_textureWidth				= 256;
	m_textureHeight				= 256;

	winx::ObjectSelector SelectSystemFont( hdc, font );

	if(!BuildCharacterTables( hdc )){
		gpassertm( 0, "Unable to build character tables");
	}

	if(!BuildCharacterImages( hdc, font )){
		gpassertm( 0, "Unable to build character images");
	}
}


void RapiFont::BuildFromTexture( unsigned int texture )
{
	if ( m_Texture ) 
	{
		m_pDevice->DestroyTexture( m_Texture );
		m_Texture = NULL;
	}

	if ( !BuildCharacterTables( texture ) )
	{
		gpassertm( 0, "Unable to build character tables");
	}	
}

// Build the tables
bool RapiFont::BuildCharacterTables( HDC hdc )
{
	bool TablesBuilt = false;

	// Get the character height
	TEXTMETRIC FontMetric;
	if(GetTextMetrics(hdc, &FontMetric))
	{
		m_characterHeightInPixels = FontMetric.tmHeight + DropShadowY ;

		// Get the character widths
		// TODO: Unsafe cast?  (only if these things can be negative)
		if(GetCharWidth(hdc, 0, NumberOfCharacters - 1, (LPINT)m_characterWidthInPixels))
		{
			int const SpaceForBadDrivers = 1;
			int YOffset = SpaceForBadDrivers;
			int XOffset = 0;
			for( int CharacterIndex = 0; CharacterIndex < NumberOfCharacters; ++CharacterIndex )
			{
				int const CharacterWidth = m_characterWidthInPixels[CharacterIndex] += DropShadowX;
				if( (XOffset + CharacterWidth) > m_textureWidth )
				{
					XOffset = 0;
					YOffset += m_characterHeightInPixels + DropShadowY + SpaceForBadDrivers;
					if( (YOffset + m_characterHeightInPixels) > m_textureHeight )
					{
						m_textureHeight <<= 1;
					}
				}

				m_characterXOffsetInPixels[CharacterIndex] = XOffset;
				m_characterYOffsetInPixels[CharacterIndex] = YOffset;
				XOffset += m_characterWidthInPixels[CharacterIndex];
			}

			TablesBuilt = true;
		}
	}

	return(TablesBuilt);
}
bool RapiFont::BuildCharacterTables( unsigned int texture )
{
	RapiMemImage * pImage = RapiMemImage::CreateMemImage( m_pDevice->GetTexturePathname( texture ), false, false );

	bool TablesBuilt = false;

	int		YOffset			= 0;
	int		XOffset			= 0;
	int		CharacterIndex	= m_startRange;

	YOffset = m_textureHeight - 1;
	while ( ( YOffset > 0 ) && ( CharacterIndex <= m_endRange ) ) 
	{
		for ( int x = 0; x < pImage->GetWidth(); ++x ) 
		{
			UINT32 color = pImage->GetPixel( x, YOffset );
			color &= 0x00FFFFFF;
			if ( ( color == 0xFFFF00FF ) || ( color == 0x00FF00FF ) ) 
			{
				m_characterWidthInPixels[CharacterIndex] = x - XOffset+1;
				m_characterXOffsetInPixels[CharacterIndex] = XOffset;
				m_characterYOffsetInPixels[CharacterIndex] = YOffset-m_characterHeightInPixels+1;
				CharacterIndex++;
				XOffset = x+1;
			}
		}

		YOffset -= m_characterHeightInPixels;
		XOffset = 0;
		TablesBuilt = true;
	}

	delete ( pImage );

	m_Texture = texture;

	return TablesBuilt;
}

// Build the images
bool RapiFont::BuildCharacterImages( HDC hdc, HFONT font )
{
	bool ImagesBuilt = false;

	// Create the RapiMemImage object to hold the image information about the font
	m_pImage = new RapiMemImage( m_textureWidth, m_textureHeight, 0 );

	// Fill in the new image with the font images
	HDC ImageDC = CreateCompatibleDC(hdc);
	if( ImageDC )
	{
		HBITMAP ImageBitmap = CreateCompatibleBitmap( ImageDC, m_textureWidth, m_textureHeight);
		if( ImageBitmap )
		{
			{
				// Select our objects
				winx::ObjectSelector FontSelector( ImageDC, font );
				winx::ObjectSelector BitmapSelector( ImageDC, ImageBitmap);

				// Fill new image with black background
				SetTextColor(ImageDC, RGB(255, 255, 255));
				SetBkColor(ImageDC, RGB(0, 0, 0));
				RECT DCRect;
				DCRect.left		= 0;
				DCRect.top		= 0;
				DCRect.right	= m_textureWidth;
				DCRect.bottom	= m_textureHeight;
				FillRect(ImageDC, &DCRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

				// Use textout calls to fill in the font texture
				for(int unsigned CharacterIndex = 0; CharacterIndex < NumberOfCharacters; ++CharacterIndex){

					char const DrawChar		= (char)CharacterIndex;

					RECT TextRectangle;
					TextRectangle.left		= m_characterXOffsetInPixels[CharacterIndex];
					TextRectangle.right		= TextRectangle.left + m_characterWidthInPixels[CharacterIndex];
					TextRectangle.top		= m_characterYOffsetInPixels[CharacterIndex];
					TextRectangle.bottom	= TextRectangle.top + m_characterHeightInPixels;

					ExtTextOut( ImageDC, TextRectangle.left, TextRectangle.top,
								ETO_OPAQUE, &TextRectangle, &DrawChar, 1, 0);
				}
			}

			// Get the device-independent bits (optimization - faster than
			// GetPixel). Note that for some god damn reason this can fail on
			// certain machines, so we fall back on GetPixel(). %&*$#@($ -sb
			BITMAPINFOHEADER bmi;
			::ZeroObject( bmi );
			bmi.biSize        = sizeof( bmi );
			bmi.biWidth       = m_textureWidth;
			bmi.biHeight      = m_textureHeight;
			bmi.biPlanes      = 1;
			bmi.biBitCount    = 32;
			bmi.biCompression = BI_RGB;
			std::vector <DWORD> buffer( m_textureWidth * m_textureHeight );
			bool success = ::GetDIBits( ImageDC, ImageBitmap, 0, m_textureHeight, buffer.begin(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS ) == m_textureHeight;

			HGDIOBJ oldObject = NULL;
			if( !success )
			{
				oldObject = SelectObject( ImageDC, ImageBitmap );
			}

			// Fill in texture
			for( int Y = 0; Y < (m_textureHeight - DropShadowX); ++Y )
			{
				for( int X = 0; X < (m_textureWidth - DropShadowY); ++X )
				{
					DWORD *BufferLocation		= m_pImage->GetBits( X, Y );
					DWORD *DropShadowLocation	= m_pImage->GetBits( X+1, Y+1 );
					DWORD PixelValue			= success ? buffer[ (m_textureWidth * (m_textureHeight - Y - 1)) + X ] : GetPixel(ImageDC, X, Y);
					*BufferLocation				|= PixelValue;
					if(PixelValue)
					{
						*BufferLocation			|= 0xFF000000;
						*DropShadowLocation		|= 0x80000000;
					}
				}
			}

			if( !success )
			{
				SelectObject( ImageDC, oldObject );
			}

			// Cleanup the mess
			DeleteObject(ImageBitmap);
			ImagesBuilt = true;
		}

		// Continue cleanup
		DeleteDC(ImageDC);
	}

	m_Texture = m_pDevice->CreateAlgorithmicTexture( GetTextureName(), m_pImage, false, 0, TEX_LOCKED, false, false );

	// We're done
	return(ImagesBuilt);
}
