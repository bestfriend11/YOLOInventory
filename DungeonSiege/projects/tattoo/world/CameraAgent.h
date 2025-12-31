#pragma once
#ifndef _CAMERA_AGENT_
#define _CAMERA_AGENT_

/**********************************************************************************
**
**							CameraAgent
**
**		This class is responsible for managing all camera control within
**		the game, and includes methods for tracking, snapping, and panning
**		to positions within it's local listing.  Orders given to the camera
**		are queued and executed in the order that they are received.
**
**		Author:		James Loe
**		Date:		04/15/00
**
**********************************************************************************/

#include "siege_camera.h"
#include "CameraPosition.h"
#include "FuBiDefs.h"

#include <map>
#include <list>

//////////////////////////////////////////////////////////////////////////////
// enum CAMINTERPOLATE declaration

// Enumeration that defines the different interpolation modes that can be used
enum eCameraInterpolate
{
	CI_INVALID		= -1,			// Invalid mode
	SET_BEGIN_ENUM( CI_, 0 ),

	CI_TIME			= 0,			// Uses time to interpolate.
	CI_SMOOTH		= 1,			// Basic smooth interpolation.  Ignores time.

	SET_END_ENUM( CI_ ),
	CI_DWORDALIGN	= 0x7FFFFFFF,	// Align the enum on dword boundaries for speed
};

FUBI_EXPORT const char* ToString  ( eCameraInterpolate val );
FUBI_EXPORT bool        FromString( const char* str, eCameraInterpolate& val );

//////////////////////////////////////////////////////////////////////////////
// enum eCameraOrder declaration

// Enumeration that defines what actions can be ordered on a given position
enum eCameraOrder
{
	COR_INVALID		= -1,			// Invalid order
	SET_BEGIN_ENUM( COR_, 0 ),

	COR_SNAP,						// Snap directly to position
	COR_PAN,						// Pan (over time) to position
	COR_OFFSETPAN,					// Pan with strict camera offset
	COR_SPLINE,						// Use points in queue to traverse a spline
	COR_BEGIN_SPLINE,				// Use points in queue to traverse a timed spline
	COR_CONT_SPLINE,				// ...
	COR_END_SPLINE,					// ...

	SET_END_ENUM( COR_ ),
	COR_DWORDALIGN	= 0x7FFFFFFF,	// Align the enum on dword boundaries for speed
};

FEX const char*  ToString             ( eCameraOrder val );
    bool         FromString           ( const char* str, eCameraOrder& val );
FEX eCameraOrder CameraOrderFromString( const char* str );

//////////////////////////////////////////////////////////////////////////////
// helper function declarations

// builds a camera position from current data
void GetCurrentCameraPosition( CameraPosition& pos );

//////////////////////////////////////////////////////////////////////////////
// class CameraAgent declaration

// Class that takes orders to set the Siege camera position.
class CameraAgent: public Singleton< CameraAgent >
{
public:

	// Construction
	CameraAgent();

	// Destruction
	~CameraAgent();

	// Persistence
	bool 				Xfer( FuBi::PersistContext& persist );

	// Puts a named position into the local cache.  Attaching a position
	// with the same name as one that already exists in the cache will replace it.
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraPosition& point );
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraEulerPosition& point );
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraMatrixPosition& point );
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraQuatPosition& point );
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraVectorToTargetPosition& point );
	FUBI_EXPORT void	SubmitCameraPosition( const gpstring& point_name, const CameraVectorToCameraPosition& point );

	// Removes a position from the internal cache.
	FUBI_EXPORT void	RemoveCameraPosition( const gpstring& point_name );

	// Clears the interal cache of points
	FUBI_EXPORT void	ClearCameraPositionCache();

	// Offset to add to the current camera position at the end of each frame, in both local and nodal
	// space varieties.  Each offset is accumulated until the Update() applies it.
	FUBI_EXPORT void	SubmitCameraOffset( const vector_3& offset );
	FUBI_EXPORT void	SubmitCameraOffset( const SiegePos& offset );
	void				SetCameraOffsetMaximum( const float max )		{ m_CameraOffsetMaximum = max; }

	// Attaches the given order to end of the command list.
	FUBI_EXPORT void	SubmitOrder( const gpstring& point_name, eCameraOrder order = COR_SNAP, float time = 0.0,
									 bool bPersist = false, Scid scid = SCID_INVALID );

	// Clears the orders for the camera
	FUBI_EXPORT void	ClearOrders();

	// Resets the affectors for a location change
	void				ResetCameraAffectorModification();
	void				ResetTargetAffectorModification();

	// Modify the current interpolation mode
	FUBI_EXPORT void	SetInterpolationMode( eCameraInterpolate interpolate );
	FUBI_EXPORT eCameraInterpolate	GetInterpolationMode() const					{ return m_Interpolate; }

	// Update the camera.
	void				Update( float secondsElapsed );

	// Clear the position
	FUBI_EXPORT void	ClearPosition();

	// Information about historical effect
	void				SetHistoricalAffect( const float affect )		{ gpassert( affect >= 0.0f && affect <= 1.0f );
																		  m_FixedHistoricAffect = affect;
																		  m_FixedLocationAffect = 1.0f - affect; }
	float				GetHistoricalAffect()							{ return m_FixedHistoricAffect; }

	// Affector smoothing time
	void				SetCameraAffectorTime( const float time )		{ m_CameraAffectorTime = time; }
	float				GetCameraAffectorTime()							{ return m_CameraAffectorTime; }

	void				SetTargetAffectorTime( const float time )		{ m_TargetAffectorTime = time; }
	float				GetTargetAffectorTime()							{ return m_TargetAffectorTime; }

	// Request a teleport from the almighty God of code
	void				RequestTeleport( CameraPosition& pos )			{ m_TeleportPos = pos;
																		  m_bTeleportRequested = true; }
	void				ClearTeleportRequest()							{ m_bTeleportRequested = false; }

	bool				IsTeleportRequested()							{ return m_bTeleportRequested; }
	CameraPosition&		GetTeleportPosition()							{ return m_TeleportPos; }

private:

	// Set the main camera position with the given pos
	void				SetMainCameraPosition( const CameraPosition& pos, bool bApplyOffset = true );

	// Builds interpolated spline points using the current m_SplinePoints and using the given t (0.0 - 1.0)
	void				BuildCurrentSplinePoints( const float t, vector_3& splineCameraPos, vector_3& splineTargetPos );

// Types.

	// Structure that defines an individual camera order
	struct CameraOrder
	{
		// Persistence
		bool 			Xfer( FuBi::PersistContext& persist );

		// What action does the client want?
		eCameraOrder	order;

		// How much time should the order be completed in?
		float			time;

		// Should this order persist if there are no orders following it in queue?
		bool			bPersist;

		// Scid of an object that is interested in knowing when this command completes
		Scid			ownerScid;

		CameraOrder( eCameraOrder order_ = COR_INVALID )
		{
			order     = order_;
			time      = 0;
			bPersist  = false;
			ownerScid = SCID_INVALID;
		}
	};

	// Structure that holds positional and timing information about a spline point
	struct SplinePair
	{
		// Camera position for the spline point
		CameraPosition	camPos;

		// Normalized range (0.0-1.0) that represents the span of this
		// point in our overall spline order.  It is determined from the
		// time assigned for its order relative to the total time allowed
		// for the complete spline order
		float			minRange;
		float			maxRange;
	};

	// Internal types
	typedef std::map< gpstring, CameraPosition, istring_less >	PosCache;
	typedef std::pair< gpstring, CameraOrder >					OrderPair;
	typedef std::list< OrderPair >								OrderQueue;
	typedef std::vector< SplinePair >							SplineCollection;

	// Structure that defines the currently active order.
	struct ActiveOrder
	{
		// Persistence
		bool 			Xfer( FuBi::PersistContext& persist );

		// Order information
		OrderPair		cam_order;

		// Is the order complete?
		bool			bIsComplete;

		// Time elapsed since the start of this order
		float			timeElapsed;

		// Starting location of the camera
		SiegePos		m_CameraPos;
		SiegePos		m_TargetPos;

		ActiveOrder( void )
		{
			bIsComplete = true;
			timeElapsed = 0;
		}
	};

// Data.

	// Internal position cache
	PosCache			m_PositionCache;

	// Offset lists
	vector_3			m_LocalOffset;
	SiegePos			m_NodalOffset;
	float				m_CameraOffsetMaximum;

	// Order queue
	OrderQueue			m_OrderQueue;

	// Current interpolation mode
	eCameraInterpolate	m_Interpolate;

	// The currently active order
	ActiveOrder			m_ActiveOrder;
	unsigned int		m_ActiveSplineIndex;

	// Spline information holder
	SplineCollection	m_SplinePoints;

	// Overtime tracker to account for frame to frame differences
	float				m_OverTime;

	// Current track amount
	float				m_CameraHistoricAffect;
	float				m_CameraLocationAffect;

	float				m_TargetHistoricAffect;
	float				m_TargetLocationAffect;

	// Fixed track amount
	float				m_FixedHistoricAffect;
	float				m_FixedLocationAffect;

	// Affector ramping time (amount of time between stop and full speed)
	float				m_CameraAffectorTime;
	float				m_TargetAffectorTime;

	// Teleportation
	CameraPosition		m_TeleportPos;
	bool				m_bTeleportRequested;

// Other.

	FUBI_SINGLETON_CLASS( CameraAgent, "Controls the client screen camera." );
	SET_NO_COPYING( CameraAgent );
};

#define gCameraAgent CameraAgent::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif	// _CAMERA_AGENT_

//////////////////////////////////////////////////////////////////////////////
