/**********************************************************************************************
**
**									RapiMouse
**
**		See .h file for details
**
**********************************************************************************************/

#include "precomp_rapi.h"
#include "gpcore.h"
#include "winx.h"

#include "GpStatsDefs.h"
#include "RapiOwner.h"
#include "RapiImage.h"
#include "RapiAppModule.h"
#include "RapiMouse.h"


// Rapi mouse thread
class RapiMouseThread : public kerneltool::Thread
{

public:

	SET_INHERITED( RapiMouseThread, kerneltool::Thread );

	enum
	{
		RETURN_SUCCESS,		// asked to exit and succeeded
		RETURN_FATAL,		// fatal occurred inside thread, aborted unexpectedly
	};

	RapiMouseThread( void )
		: Inherited( OPTION_DEF | OPTION_IGNORE_EXCEPTIONS | OPTION_BEGIN_THREAD_WAIT )
	{
		Inherited::BeginThread( "RapiMouse" );
	}

	virtual ~RapiMouseThread( void )
	{
		if ( IsValid() && IsThreadRunning() )
		{
			m_RequestQuit.Set();

			// if something bad happened we may be sitting on a critical, so
			// just nuke the thread otherwise shutdown will hang.
			if ( Wait( 2500 ) == WAIT_TIMEOUT )
			{
				Nuke();
			}
		}
	}

	void Pause   ( void )			{  m_AllowExecute.Reset(); 	}
	void Resume  ( void )			{  m_AllowExecute.Set  ();  }
	bool IsPaused( void ) const		{  return ( !m_AllowExecute.IsSignaled() );  }

	kerneltool::Event m_WaitEvent;			// startup state: false

private:

	virtual DWORD Execute( void )
	{
		DWORD rc = 0;
		__try
		{
			__try
			{
				// continue infinitely until normal exit or an external exception
				while ( !ReportSys::IsFatalOccurring() )
				{
					// wait for "go"
					HANDLE syncObjects[ 2 ];
					syncObjects[ 0 ] = m_RequestQuit.GetValid();
					syncObjects[ 1 ] = m_AllowExecute.GetValid();
					m_WaitEvent.Set();
					DWORD waitCode = ::WaitForMultipleObjects( ELEMENT_COUNT( syncObjects ), syncObjects, FALSE, INFINITE );
					m_WaitEvent.Reset();

					// requested to quit?
					if ( waitCode == WAIT_OBJECT_0 )
					{
						break;
					}

					// only update if we have the focus
					if ( Singleton <RapiAppModule>::DoesSingletonExist() && gRapiAppModule.IsAppActive() )
					{
						gRapiMouse.UpdateCursor();
					}

					Sleep( 30 );
				}
			}
			__except( ::GlobalExceptionFilter( GetExceptionInformation() ) )
			{
				// ignore, but fail
				rc = 'Fatl';
			}
		}
		__except ( ::ExceptionDlgFilter( GetExceptionInformation(), ::GetLastError(), "Siege Loader Thread" ), EXCEPTION_EXECUTE_HANDLER )
		{
			// just report it
			rc = 'SeEx';
		}

		// notify app to shut down if we had an exception
		if ( rc != 0 )
		{
			gAppModule.RequestFatalQuit();
		}

		// quit this thread, but make the main thread think we're still waiting
		m_WaitEvent.Set();
		return ( rc );
	}

	kerneltool::Event m_RequestQuit;		// startup state: false
	kerneltool::Event m_AllowExecute;		// startup state: false
	gpstring          m_ProfileOutput;

	SET_NO_COPYING( RapiMouseThread );
};

// Construction
RapiMouse::RapiMouse( Rapi* pRapi )
	: m_pRapi( pRapi )
	, m_pImage( NULL )
	, m_primaryImageWidth( 0 )
	, m_primaryImageHeight( 0 )
	, m_blendedImage( NULL )
	, m_primaryImage( NULL )
	, m_cursorX( 0 )
	, m_cursorY( 0 )
	, m_hotpointX( 0 )
	, m_hotpointY( 0 )
	, m_bMarkedForResume( false )
	, m_bUpdate( true )
	, m_bManualCopy( true )
	, m_newImage( 0 )
	, m_newHotpointX( 0 )
	, m_newHotpointY( 0 )
	, m_bCursorChange( false )
	, m_waitImage( 0 )
	, m_oldImage( 0 )
	, m_waitHotpointX( 0 )
	, m_oldHotpointX( 0 )
	, m_waitHotpointY( 0 )
	, m_oldHotpointY( 0 )
	, m_bWaitStateAllowed( false )
	, m_waitTime( 0.0 )
{
	// Create mouse thread object
	m_pMouseThread	= new RapiMouseThread;
}

// Destruction
RapiMouse::~RapiMouse()
{
	if( m_pMouseThread )
	{
		// Cleanup mouse thread
		if( !m_pMouseThread->IsPaused() )
		{
			m_pMouseThread->Pause();
		}

		delete m_pMouseThread;
		m_pMouseThread = NULL;
	}

	for( MemImageBucketVector::iterator i = m_pImageBucketVector.begin(); i != m_pImageBucketVector.end(); ++i )
	{
		delete (*i);
	}
	m_pImageBucketVector.clear();
}

// Enable mouse
void RapiMouse::Enable()
{
	// Base image sizes
	m_primaryImageWidth		= 64;
	m_primaryImageHeight	= 64;

	// Create the image that will contain the blended back buffer and cursor images
	if( FAILED( TRYDX( m_pRapi->GetDevice()->CreateImageSurface( 128, 128, D3DFMT_A8R8G8B8, &m_blendedImage ) ) ) )
	{
		gpfatal( "Could not create mouse sub surface!" );
	}

	// Create the image that will be the main image
	if( FAILED( TRYDX( m_pRapi->GetDevice()->CreateImageSurface( 64, 64, D3DFMT_A8R8G8B8, &m_primaryImage ) ) ) )
	{
		gpfatal( "Could not create mouse sub surface!" );
	}

	// Get the mouse thread moving
	PauseMouse( false );
}

// Disable mouse
void RapiMouse::Disable()
{
	// Stop the mouse thread
	PauseMouse( true, true );
	::WaitForSingleObject( m_pMouseThread->m_WaitEvent.GetValid(), INFINITE );

	// Release surfaces
	RELEASE( m_blendedImage );
	RELEASE( m_primaryImage );
	m_pImage	= NULL;
}

// Pause the mouse thread
void RapiMouse::PauseMouse( bool bPause, bool bWait )
{
	if( bPause && !m_pMouseThread->IsPaused() )
	{
		m_bMarkedForResume = false;

		m_pMouseThread->Pause();
		if ( bWait )
		{
			::WaitForSingleObject( m_pMouseThread->m_WaitEvent.GetValid(), INFINITE );
		}
	}

	m_bMarkedForResume	= m_pRapi->GetAsyncCursor() ? !bPause : false;
	m_bUpdate			= true;
}

// Is mouse thread paused?
bool RapiMouse::IsMousePaused() const
{
	return ( m_pMouseThread->IsPaused() );
}

void RapiMouse::SetCursorImage( unsigned int cursor, int hotpointx, int hotpointy )
{
	GPSTATS_GROUP( GROUP_RENDER_MISC );

	kerneltool::Critical::Lock locker( m_imageCritical );

	m_newImage		= cursor;
	m_newHotpointX	= hotpointx;
	m_newHotpointY	= hotpointy;
	m_bCursorChange	= true;
}

void RapiMouse::SetWaitImage( unsigned int cursor, int hotpointx, int hotpointy )
{
	GPSTATS_GROUP( GROUP_RENDER_MISC );

	kerneltool::Critical::Lock locker( m_imageCritical );

	m_waitImage		= cursor;
	m_waitHotpointX	= hotpointx;
	m_waitHotpointY	= hotpointy;
}

// Set the current position of the cursor
void RapiMouse::SetCursorPosition( int x, int y )
{
	kerneltool::Critical::Lock locker( m_mouseCritical );

	// Set the requested cursor position
	if( m_cursorX != x || m_cursorY != y )
	{
		m_cursorX	= x;
		m_cursorY	= y;
		m_bUpdate	= true;
	}
}

// Update the cursor on the screen
void RapiMouse::UpdateCursor( bool bPrimary )
{
	GPSTATS_GROUP( GROUP_RENDER_MISC );

	if( m_bMarkedForResume )
	{
		m_pMouseThread->Resume();
		m_bMarkedForResume = false;
	}

	if( m_pMouseThread->IsPaused() && m_pRapi->GetAsyncCursor() )
	{
		return;
	}

	if( !Singleton <RapiAppModule>::DoesSingletonExist() )
	{
		return;
	}

	if( bPrimary )
	{
		if( (m_bWaitStateAllowed && m_newImage != m_waitImage) &&
			PreciseSubtract( ::GetSystemSeconds(), m_waitTime ) > 0.5 )
		{
			m_oldImage		= m_newImage;
			m_oldHotpointX	= m_newHotpointX;
			m_oldHotpointY	= m_newHotpointY;
			SetCursorImage( m_waitImage, m_waitHotpointX, m_waitHotpointY );
		}
	}
	else
	{
		m_waitTime	= ::GetSystemSeconds();
		if( m_newImage == m_waitImage )
		{
			SetCursorImage( m_oldImage, m_oldHotpointX, m_oldHotpointY );
		}
	}

	int cursorX	= m_cursorX;
	int cursorY = m_cursorY;

	if( gRapiAppModule.FrameUpdateCursor( &cursorX, &cursorY, false ) )
	{
		kerneltool::Critical::Lock locker( m_mouseCritical );

		// Check to see if we need to commit cursor changes
		m_imageCritical.Enter();
		if( m_bCursorChange )
		{
			// Create a mem image
			if( m_pImageBucketVector.IsValid( m_newImage ) )
			{
				if( m_pImage != m_pImageBucketVector[ m_newImage ] ||
					(m_hotpointX != m_newHotpointX || m_hotpointY != m_newHotpointY) )
				{
					m_pImage	= m_pImageBucketVector[ m_newImage ];
					SetCursorImage( m_pImage );

					// Save hotpoint location
					m_hotpointX	= m_newHotpointX;
					m_hotpointY = m_newHotpointY;

					m_bUpdate	= true;
				}
			}
			else
			{
				if( m_pImage != NULL )
				{
					m_pImage	= NULL;
					m_bUpdate	= true;
				}
			}
			m_bCursorChange		= false;
		}
		m_imageCritical.Leave();

		if( m_cursorX != cursorX || m_cursorY != cursorY )
		{
			m_cursorX	= cursorX;
			m_cursorY	= cursorY;
			m_bUpdate	= true;
		}

		// Return if there is no valid image
		if( (bPrimary && !m_bUpdate) || !m_pImage || !m_blendedImage || !m_primaryImage )
		{
			return;
		}

		// Build old screen rect
		GRect oldScreenRect		= m_primaryRect;
		ClientToScreen( gRapiAppModule.GetMainWnd(), (POINT*)&oldScreenRect.left );
		ClientToScreen( gRapiAppModule.GetMainWnd(), (POINT*)&oldScreenRect.right );

		// Create a rect that represents the region that the mouse covers
		m_primaryRect.left		= m_cursorX - m_hotpointX;
		m_primaryRect.top		= m_cursorY - m_hotpointY;
		m_primaryRect.right		= min_t( m_pRapi->GetWidth() - 1, m_primaryRect.left + m_pImage->GetWidth() );
		m_primaryRect.bottom	= min_t( m_pRapi->GetHeight() - 1, m_primaryRect.top + m_pImage->GetHeight() );

		// Build image rect for blending
		GRect imageRect;
		if( m_primaryRect.left < 0 )
		{
			imageRect.left		= -m_primaryRect.left;
			m_primaryRect.left	= 0;
		}
		else
		{
			imageRect.left		= 0;
		}
		imageRect.right			= (m_primaryRect.right - m_primaryRect.left) + imageRect.left;

		if( m_primaryRect.top < 0 )
		{
			imageRect.top		= -m_primaryRect.top;
			m_primaryRect.top	= 0;
		}
		else
		{
			imageRect.top		= 0;
		}
		imageRect.bottom		= (m_primaryRect.bottom - m_primaryRect.top) + imageRect.top;

		// Build screen rect
		GRect screenRect		= m_primaryRect;
		ClientToScreen( gRapiAppModule.GetMainWnd(), (POINT*)&screenRect.left );
		ClientToScreen( gRapiAppModule.GetMainWnd(), (POINT*)&screenRect.right );

		if( oldScreenRect.Intersects( screenRect ) && bPrimary )
		{
			// Build union
			GRect screenUnion		= Union( oldScreenRect, screenRect );
			GRect localUnion		= screenUnion;
			ScreenToClient( gRapiAppModule.GetMainWnd(), (POINT*)&localUnion.left );
			ScreenToClient( gRapiAppModule.GetMainWnd(), (POINT*)&localUnion.right );

			// Build local rect
			GRect blendedUnion;
			blendedUnion.left		= 0;
			blendedUnion.top		= 0;
			blendedUnion.right		= screenUnion.right - screenUnion.left;
			blendedUnion.bottom		= screenUnion.bottom - screenUnion.top;

			// Copy the primary union to the blended image
			Blit( m_blendedImage, &blendedUnion, m_pRapi->GetPrimarySurface(), &screenUnion );

			// Copy the old backup into the blended image
			GRect blendedCursorRect	= oldScreenRect;
			blendedCursorRect.Offset( -screenUnion.left, -screenUnion.top );

			Blit( m_blendedImage, &blendedCursorRect, m_primaryImage, &m_cursorRect );

			// Make a local rect for the new cursor position
			GRect localCursorRect	= m_primaryRect;
			localCursorRect.Offset( -localUnion.left, -localUnion.top );

			// Build cursor rect
			m_cursorRect.left		= 0;
			m_cursorRect.top		= 0;
			m_cursorRect.right		= m_primaryRect.right - m_primaryRect.left;
			m_cursorRect.bottom		= m_primaryRect.bottom - m_primaryRect.top;

			// Copy off the image as the new saved image
			Blit( m_primaryImage, &m_cursorRect, m_blendedImage, &localCursorRect );

			// Blend the cursor into the image
			Blend( m_blendedImage, &localCursorRect, m_pImage, &imageRect );

			// Copy blended image onto the primary
			Blit( m_pRapi->GetPrimarySurface(), &screenUnion, m_blendedImage, &blendedUnion );
		}
		else
		{
			if( bPrimary )
			{
				// Copy the existing primary image back into the old rect
				Blit( m_pRapi->GetPrimarySurface(), &oldScreenRect, m_primaryImage, &m_cursorRect );
			}

			// Build cursor rect
			m_cursorRect.left		= 0;
			m_cursorRect.top		= 0;
			m_cursorRect.right		= m_primaryRect.right - m_primaryRect.left;
			m_cursorRect.bottom		= m_primaryRect.bottom - m_primaryRect.top;

			if( bPrimary )
			{
				// Copy the primary image to the sub surface
				Blit( m_primaryImage, &m_cursorRect, m_pRapi->GetPrimarySurface(), &screenRect );
			}
			else
			{
				// Copy the back buffer image to the sub surface
				Blit( m_primaryImage, &m_cursorRect, m_pRapi->GetBackBufferSurface(), &m_primaryRect );
			}

			// Copy primary image to blended image
			Blit( m_blendedImage, &m_cursorRect, m_primaryImage, &m_cursorRect );

			// Blend the cursor into the image
			Blend( m_blendedImage, &m_cursorRect, m_pImage, &imageRect );

			if( bPrimary )
			{
				// Copy blended image onto the primary
				Blit( m_pRapi->GetPrimarySurface(), &screenRect, m_blendedImage, &m_cursorRect );
			}
			else
			{
				// Copy blended image onto the primary
				Blit( m_pRapi->GetBackBufferSurface(), &m_primaryRect, m_blendedImage, &m_cursorRect );
			}
		}
	}

	// Clear update request
	m_bUpdate	= false;
}

// Create an image cursor from file
unsigned int RapiMouse::CreateCursorImage( const gpstring& filename )
{
	// $$$ Should premultiply the alpha into the mem image here
	RapiMemImage* pNewImage	= RapiMemImage::CreateMemImage( filename, false, false );
	pNewImage->FlipVertical();
	return m_pImageBucketVector.Alloc( pNewImage );
}

unsigned int RapiMouse::CreateCursorImage( RapiMemImage* pImage )
{
	// $$$ Should premultiply the alpha into the mem image here
	pImage->FlipVertical();
	return m_pImageBucketVector.Alloc( pImage );
}

// Destroy the given cursor image
void RapiMouse::DestroyCursorImage( unsigned int cursor )
{
	kerneltool::Critical::Lock locker( m_mouseCritical );

	if( m_pImageBucketVector.IsValid( cursor ) )
	{
		if( m_pImage == m_pImageBucketVector[ cursor ] )
		{
			m_pImage	= NULL;
		}

		delete m_pImageBucketVector[ cursor ];
		m_pImageBucketVector.Free( cursor );
	}
}

// Submit an image to be used as the cursor
void RapiMouse::SetCursorImage( RapiMemImage* pCursor )
{
	kerneltool::Critical::Lock locker( m_mouseCritical );

	if( pCursor )
	{
		// Save the new cursor image
		m_pImage	= pCursor;
		if( m_primaryImageWidth < m_pImage->GetWidth() ||
			m_primaryImageHeight < m_pImage->GetHeight() )
		{
			gperror( "Cursor size has changed, bad!" );

			// Release old image
			RELEASE( m_primaryImage );
			RELEASE( m_blendedImage );

			// Save new sizes
			m_primaryImageWidth		= m_pImage->GetWidth();
			m_primaryImageHeight	= m_pImage->GetHeight();

			// Create the image that will contain the blended back buffer and cursor images
			if( FAILED( TRYDX( m_pRapi->GetDevice()->CreateImageSurface( m_pImage->GetWidth() * 2, m_pImage->GetWidth() * 2,
																		 D3DFMT_A8R8G8B8, &m_blendedImage ) ) ) )
			{
				gpfatal( "Could not create mouse sub surface!" );
			}

			// Create the image that will be the main image
			if( FAILED( TRYDX( m_pRapi->GetDevice()->CreateImageSurface( m_pImage->GetWidth(), m_pImage->GetWidth(),
																		 D3DFMT_A8R8G8B8, &m_primaryImage ) ) ) )
			{
				gpfatal( "Could not create mouse sub surface!" );
			}
		}
	}
}

// Blit helper
void RapiMouse::Blit( LPDIRECT3DSURFACE8 destSurface, RECT* pDestRect, LPDIRECT3DSURFACE8 srcSurface, RECT* pSrcRect )
{
	if( ::GetForegroundWindow() != gRapiAppModule.GetMainWnd() )
	{
		return;
	}

	if( ((pDestRect->right == pDestRect->left) || (pDestRect->bottom == pDestRect->top)) ||
		((pSrcRect->right == pSrcRect->left) || (pSrcRect->bottom == pSrcRect->top)) )
	{
		return;
	}

	// Copy the primary image to the sub surface
	if( m_bManualCopy || FAILED( m_pRapi->GetDevice()->CopyRects( srcSurface, pSrcRect, 1, destSurface, (POINT*)pDestRect ) ) )
	{
		// Lock both surfaces
		D3DLOCKED_RECT srcLockedRect;
		if( FAILED( srcSurface->LockRect( &srcLockedRect, pSrcRect, D3DLOCK_READONLY ) ) )
		{
			return;
		}

		D3DLOCKED_RECT dstLockedRect;
		if( FAILED( destSurface->LockRect( &dstLockedRect, pDestRect, 0 ) ) )
		{
			srcSurface->UnlockRect();
			return;
		}

		CopyPixels( dstLockedRect.pBits, GetPixelFormat( D3DFMT_A8R8G8B8 ), pDestRect->right - pDestRect->left, pDestRect->bottom - pDestRect->top, dstLockedRect.Pitch,
					srcLockedRect.pBits, GetPixelFormat( D3DFMT_A8R8G8B8 ), pSrcRect->right - pSrcRect->left, pSrcRect->bottom - pSrcRect->top, srcLockedRect.Pitch,
					false, false );

		destSurface->UnlockRect();
		srcSurface->UnlockRect();
	}
}

// Blend helper.  It is assumed by this function that while the offsets may vary between
// the source and destination rectangles, the size is the same.
void RapiMouse::Blend( LPDIRECT3DSURFACE8 destSurface, RECT* pDestRect, RapiMemImage* pSrcImage, RECT* pSrcRect )
{
	if( ((pDestRect->right == pDestRect->left) || (pDestRect->bottom == pDestRect->top)) ||
		((pSrcRect->right == pSrcRect->left) || (pSrcRect->bottom == pSrcRect->top)) )
	{
		return;
	}

	if( !destSurface || !pSrcImage )
	{
		return;
	}

	gpassert( (pDestRect->right - pDestRect->left) == (pSrcRect->right - pSrcRect->left) );
	gpassert( (pDestRect->bottom - pDestRect->top) == (pSrcRect->bottom - pSrcRect->top) );

	// Copy proper cursor bits into the blended surface
	D3DLOCKED_RECT dstLockedRect;
	if( SUCCEEDED( destSurface->LockRect( &dstLockedRect, pDestRect, 0 ) ) )
	{
		// Calculate destination formats and size
		ePixelFormat dstFormat	= GetPixelFormat( D3DFMT_A8R8G8B8 );
		int dstWidth			= pDestRect->right - pDestRect->left;
		int dstHeight			= pDestRect->bottom - pDestRect->top;

		// Create source pointer at the beginning of data as indicated by src rectangle
		const DWORD* i			= pSrcImage->GetBits() + (pSrcRect->top * pSrcImage->GetWidth()) + pSrcRect->left;

		// Calculate stride steps
		int dstStrideStep		= (dstLockedRect.Pitch / GetSizeBytes( dstFormat )) - dstWidth;
		int srcStrideStep		= pSrcImage->GetWidth() - dstWidth;

		switch( dstFormat )
		{
			case PIXEL_ARGB_4444:
			{
				WORD* o			= (WORD*)dstLockedRect.pBits;
				for ( int y = 0 ; y != dstHeight ; ++y )
				{
					for ( int x = 0 ; x != dstWidth ; ++o, ++i, ++x )
					{
						DWORD alpha	= (*i) >> 24;
						if( alpha )
						{
							*o = MAKE4444FROM8888( InterpolateColor( (*i), MAKE8888FROM4444( (*o) ), alpha ) );
						}
					}

					i	+= srcStrideStep;
					o	+= dstStrideStep;
				}
				break;
			}

			case PIXEL_ARGB_1555:
			{
				WORD* o = (WORD*)dstLockedRect.pBits;
				for ( int y = 0 ; y != dstHeight ; ++y )
				{
					for ( int x = 0 ; x != dstWidth ; ++o, ++i, ++x )
					{
						DWORD alpha	= (*i) >> 24;
						if( alpha )
						{
							*o = MAKE1555FROM8888( InterpolateColor( (*i), MAKE8888FROM1555( (*o) ), alpha ) );
						}
					}

					i	+= srcStrideStep;
					o	+= dstStrideStep;
				}
				break;
			}

			case PIXEL_RGB_565:
			{
				WORD* o = (WORD*)dstLockedRect.pBits;
				for ( int y = 0 ; y != dstHeight ; ++y )
				{
					for ( int x = 0 ; x != dstWidth ; ++o, ++i, ++x )
					{
						DWORD alpha	= (*i) >> 24;
						if( alpha )
						{
							*o = MAKE565FROM8888( InterpolateColor( (*i), MAKE8888FROM565( (*o) ), alpha ) );
						}
					}

					i	+= srcStrideStep;
					o	+= dstStrideStep;
				}
				break;
			}

			case PIXEL_ARGB_8888:
			{
				DWORD* o = (DWORD*)dstLockedRect.pBits;
				for ( int y = 0 ; y != dstHeight ; ++y )
				{
					for ( int x = 0 ; x != dstWidth ; ++o, ++i, ++x )
					{
						DWORD alpha	= (*i) >> 24;
						if( alpha )
						{
							*o = InterpolateColor( (*i), (*o), alpha );
						}
					}

					i	+= srcStrideStep;
					o	+= dstStrideStep;
				}
				break;
			}
		}

		// Unlock surface
		TRYDX( destSurface->UnlockRect() );
	}
}
