#pragma once
#ifndef _SIEGE_COMPASS_H_
#define _SIEGE_COMPASS_H_

/*******************************************************************************
**
**								SiegeCompass
**
**		This class is responsible for maintaining and drawing a compass
**		for the current engine state.
**
**		Author:		James Loe
**		Date:		08/16/00
**
*******************************************************************************/


namespace siege
{
	class SiegeCompass
	{

	public:

		// Construction and destruction
		SiegeCompass();
		~SiegeCompass();

		// Update the compass
		void	Update( const matrix_3x3& cam_orient );

		// See if the given screen point hits the compass
		bool	ScreenPosIntersectsCompass( const int screenx, const int screeny );

		// Move the compass in screen space.
		void	MoveCompass( const int deltax, const int deltay );

		// Set the radius
		void	SetCompassRadius( const int radius );
		int		GetCompassRadius()											{ return m_Radius; }

		// Manually set the position of the compass (center of the compass)
		void	SetCompassPosition( const int screenx, const int screeny )	{ m_ScreenX = screenx; m_ScreenY = screeny; }
		int		GetScreenX() const											{ return m_ScreenX; }
		int		GetScreenY() const											{ return m_ScreenY; }

		// Visibility
		// This option is app controlled, and is meant to be used in code as a context identifier
		void	SetVisible( const bool bVisible )							{ m_bVisible = bVisible; }
		bool	GetVisible() const											{ return m_bVisible; }

		// Activity
		// This option is controlled by the player, and should not be modified by code
		void	SetActive( const bool bActive )								{ m_bActive = bActive; }
		bool	GetActive() const											{ return m_bActive; }

		// Minimize the compass - displays a much small compass
		void	SetMinimized( const bool bMinimize )						{ m_bMinimized = bMinimize; }
		bool	GetMinimized()												{ return m_bMinimized; }

		// Alpha blending
		void	SetAlpha( const BYTE alpha )								{ m_Alpha = alpha; }
		BYTE	GetAlpha() const											{ return m_Alpha; }

		// Cardinal distance
		void	SetCardinalDistance( const int dist )						{ m_CardDist = dist; }
		int		GetCardinalDistance() const									{ return m_CardDist; }

		// Texture
		void	SetNeedleTexture( const gpstring& texname );
		void	SetDirectionTexture( const gpstring& texname );
		void	SetCardinalTextures( const gpstring& northname,
									 const gpstring& eastname,
									 const gpstring& southname,
									 const gpstring& westname );
		void	SetHotpointTexture( const gpstring& texname );
		void	SetMinimizedTexture( const gpstring & texname );

	private:

		// Draw the parts of the compass
		void			DrawNeedle( Rapi& renderer, const matrix_3x3& orient );
		void			DrawDirectionFinder( Rapi& renderer, const matrix_3x3& orient );
		void			DrawCardinals( Rapi& renderer, const matrix_3x3& orient );
		void			DrawHotpoint( Rapi& renderer, const matrix_3x3& orient );
		void			DrawMinimized( Rapi & renderer, const matrix_3x3& orient );

		// Get the orientation matrix using the current camera and targetnode north vector
		matrix_3x3		GetNorthDirectionMatrix( const matrix_3x3& cam_orient );
		matrix_3x3		GetActiveHotpointDirectionMatrix( const matrix_3x3& cam_orient );

		// Screen position
		int				m_Radius;
		int				m_ScreenX;
		int				m_ScreenY;
		int				m_CardDist;

		// Texture information
		unsigned int	m_NeedleTexture;
		unsigned int	m_NeedleWidth;
		unsigned int	m_NeedleHeight;

		unsigned int	m_DirectionTexture;
		unsigned int	m_DirectionWidth;
		unsigned int	m_DirectionHeight;

		unsigned int	m_NorthTexture;
		unsigned int	m_EastTexture;
		unsigned int	m_SouthTexture;
		unsigned int	m_WestTexture;
		unsigned int	m_CardinalWidth;
		unsigned int	m_CardinalHeight;

		unsigned int	m_HotpointTexture;
		unsigned int	m_HotpointWidth;
		unsigned int	m_HotpointHeight;

		unsigned int	m_MinimizedTexture;
		unsigned int	m_MinimizedWidth;
		unsigned int	m_MinimizedHeight;

		// Active
		bool			m_bVisible;
		bool			m_bActive;
		bool			m_bMinimized;

		// Alpha blending
		BYTE			m_Alpha;
	};
}

#endif