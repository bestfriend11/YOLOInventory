/**********************************************************************************
**
**							CameraAgent
**
**		See .h file for details
**
**********************************************************************************/

#include "Precomp_World.h"
#include "CameraAgent.h"

#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "GoSupport.h"
#include "Services.h"
#include "spline.h"

FUBI_EXPORT_ENUM( eCameraInterpolate, CI_BEGIN, CI_COUNT );
FUBI_EXPORT_ENUM( eCameraOrder, COR_BEGIN, COR_COUNT );

//////////////////////////////////////////////////////////////////////////////
// persistence support

FUBI_DECLARE_ENUM_TRAITS( eCameraInterpolate, CI_INVALID  );
FUBI_DECLARE_ENUM_TRAITS( eCameraOrder,       COR_INVALID );
FUBI_DECLARE_PAIR_TRAITS( CameraAgent::OrderPair          );
FUBI_DECLARE_SELF_TRAITS( CameraAgent::CameraOrder        );
FUBI_DECLARE_SELF_TRAITS( CameraAgent::ActiveOrder        );
FUBI_DECLARE_SELF_TRAITS( CameraPosition                  );

bool CameraAgent::CameraOrder :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "order",		order    );
	persist.Xfer( "time",		time     );
	persist.Xfer( "bPersist",	bPersist );
	persist.Xfer( "ownerScid",	ownerScid );
	return ( true );
}

bool CameraAgent::ActiveOrder :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "cam_order",   cam_order   );
	persist.Xfer( "bIsComplete", bIsComplete );
	persist.Xfer( "timeElapsed", timeElapsed );
	persist.Xfer( "m_CameraPos", m_CameraPos );
	persist.Xfer( "m_TargetPos", m_TargetPos );
	return ( true );
}

bool CameraPosition :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "targetPos", targetPos );
	persist.Xfer( "cameraPos", cameraPos );
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// CameraPosition helpers

CameraQuatPosition& MakeCameraPosition( const SiegePos& siegePos, const Quat& quat )
{
	static CameraQuatPosition s_CamQuatPos;
	s_CamQuatPos.cameraPos		= siegePos;
	s_CamQuatPos.cameraOrient	= quat;
	return ( s_CamQuatPos );
}

CameraPosition& MakeCameraPosition( const SiegePos& siegePos, const Quat& quat, float distance )
{
	// Setup the new position
	static CameraPosition s_CamPos;
	s_CamPos.cameraPos			= siegePos;
	s_CamPos.targetPos.node		= siegePos.node;
	s_CamPos.targetPos.pos		= siegePos.pos + (Normalize( quat.BuildMatrix().GetColumn2_T() ) * distance);
	return ( s_CamPos );
}

void CameraEulerPosition :: MakeCameraPosition( CameraPosition& out )
{
	out.targetPos = targetPos;

	float d;
	SINCOSF( azimuthAngle, out.cameraPos.pos.y, d );
	SINCOSF( orbitAngle, out.cameraPos.pos.x, out.cameraPos.pos.z );

	d					*= distance;
	out.cameraPos.pos.x	*= d;
	out.cameraPos.pos.y	*= distance;
	out.cameraPos.pos.z	*= d;

	out.cameraPos.node	= targetPos.node;
	out.cameraPos.pos	+= targetPos.pos;
}

//////////////////////////////////////////////////////////////////////////////
// enum eCameraInterpolate implementation

static const char* s_CameraInterpolateStrings[] =
{
	"ci_time",
	"ci_smooth",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_CameraInterpolateStrings ) == CI_COUNT );

static stringtool::EnumStringConverter s_CameraInterpolateConverter(
		s_CameraInterpolateStrings, CI_BEGIN, CI_END, CI_INVALID );

const char* ToString( eCameraInterpolate val )
	{  return ( s_CameraInterpolateConverter.ToString( val ) );  }
bool FromString( const char* str, eCameraInterpolate& val )
	{  return ( s_CameraInterpolateConverter.FromString( str, rcast <int&> ( val ) ) );  }

eCameraOrder CameraOrderFromString( const char* str )
{
	eCameraOrder order;
	FromString( str, order );
	return ( order );
}

//////////////////////////////////////////////////////////////////////////////
// enum eCameraOrder implementation

static const char* s_CameraOrderStrings[] =
{
	"cor_snap",
	"cor_pan",
	"cor_offsetpan",
	"cor_spline",
	"cor_begin_spline",
	"cor_cont_spline",
	"cor_end_spline",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_CameraOrderStrings ) == COR_COUNT );

static stringtool::EnumStringConverter s_CameraOrderConverter(
		s_CameraOrderStrings, COR_BEGIN, COR_END, COR_INVALID );

const char* ToString( eCameraOrder val )
	{  return ( s_CameraOrderConverter.ToString( val ) );  }
bool FromString( const char* str, eCameraOrder& val )
	{  return ( s_CameraOrderConverter.FromString( str, rcast <int&> ( val ) ) );  }

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

void GetCurrentCameraPosition( CameraPosition& pos )
{
	pos.targetPos.pos  = gSiegeEngine.GetCamera().GetTargetPosition();
	pos.targetPos.node = gSiegeEngine.NodeWalker().TargetNodeGUID();
	pos.cameraPos.pos  = gSiegeEngine.GetCamera().GetCameraPosition();
	pos.cameraPos.node = gSiegeEngine.NodeWalker().TargetNodeGUID();
}

//////////////////////////////////////////////////////////////////////////////
// class CameraAgent implementation


// Construction
CameraAgent::CameraAgent()
	: m_CameraOffsetMaximum( 10.0f )
	, m_Interpolate( CI_SMOOTH )
	, m_OverTime( 0.0 )
	, m_FixedHistoricAffect( 0.5f )
	, m_FixedLocationAffect( 0.5f )
	, m_CameraHistoricAffect( 0.5f )
	, m_CameraLocationAffect( 0.5f )
	, m_TargetHistoricAffect( 0.5f )
	, m_TargetLocationAffect( 0.5f )
	, m_CameraAffectorTime( 1.0f )
	, m_TargetAffectorTime( 0.25f )
	, m_ActiveSplineIndex( 0 )
	, m_bTeleportRequested( false )
{
}

// Destruction
CameraAgent::~CameraAgent()
{
}

// Persistence
bool CameraAgent::Xfer( FuBi::PersistContext& persist )
{
	persist.XferMap ( "m_PositionCache",        m_PositionCache        );
	persist.Xfer    ( "m_LocalOffset",          m_LocalOffset          );
	persist.Xfer    ( "m_NodalOffset",          m_NodalOffset          );
	persist.XferList( "m_OrderQueue",           m_OrderQueue           );
	persist.Xfer    ( "m_Interpolate",          m_Interpolate          );
	persist.Xfer    ( "m_ActiveOrder",          m_ActiveOrder          );
	persist.Xfer    ( "m_OverTime",             m_OverTime             );
	persist.Xfer    ( "m_CameraHistoricAffect", m_CameraHistoricAffect );
	persist.Xfer    ( "m_CameraLocationAffect", m_CameraLocationAffect );
	persist.Xfer    ( "m_TargetHistoricAffect", m_TargetHistoricAffect );
	persist.Xfer    ( "m_TargetLocationAffect", m_TargetLocationAffect );
	persist.Xfer    ( "m_FixedHistoricAffect",  m_FixedHistoricAffect  );
	persist.Xfer    ( "m_FixedLocationAffect",  m_FixedLocationAffect  );
	persist.Xfer    ( "m_CameraAffectorTime",   m_CameraAffectorTime   );
	persist.Xfer    ( "m_TargetAffectorTime",   m_TargetAffectorTime   );

	return ( true );
}

// Directly submit a two point camera position
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraPosition& point )
{
	DEBUG_FLOAT_VALIDATE( point.cameraPos.pos.x );
	DEBUG_FLOAT_VALIDATE( point.cameraPos.pos.y );
	DEBUG_FLOAT_VALIDATE( point.cameraPos.pos.z );
	DEBUG_FLOAT_VALIDATE( point.targetPos.pos.x );
	DEBUG_FLOAT_VALIDATE( point.targetPos.pos.y );
	DEBUG_FLOAT_VALIDATE( point.targetPos.pos.z );

	PosCache::iterator i = m_PositionCache.find( point_name );
	if( i != m_PositionCache.end() )
	{
		(*i).second	= point;
	}
	else
	{
		// Insert new position
		m_PositionCache.insert( std::make_pair( point_name, point ) );
	}
}

// Convert an euler angle position to a two point and submit
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraEulerPosition& point )
{
	// Setup the new position
	CameraPosition cameraPos;
	cameraPos.targetPos			= point.targetPos;

	// Build the camera position using the angles
	float d;
	SINCOSF( point.azimuthAngle, cameraPos.cameraPos.pos.y, d );
	SINCOSF( point.orbitAngle, cameraPos.cameraPos.pos.x, cameraPos.cameraPos.pos.z );

	d							*= point.distance;
	cameraPos.cameraPos.pos.x	*= d;
	cameraPos.cameraPos.pos.y	*= point.distance;
	cameraPos.cameraPos.pos.z	*= d;

	cameraPos.cameraPos.node	= point.targetPos.node;
	cameraPos.cameraPos.pos		+= point.targetPos.pos;

	// Insert the new pair
	SubmitCameraPosition( point_name, cameraPos );
}

// Convert a matrix position to a two point and submit
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraMatrixPosition& point )
{
	// Setup the new position
	CameraPosition cameraPos;
	cameraPos.cameraPos			= point.cameraPos;
	cameraPos.targetPos.node	= point.cameraPos.node;
	cameraPos.targetPos.pos		= point.cameraPos.pos + point.cameraOrient.GetColumn2_T();

	// Insert the new pair
	SubmitCameraPosition( point_name, cameraPos );
}

// Convert quaternion based position to a two point and submit
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraQuatPosition& point )
{
	// Setup the new position
	CameraPosition cameraPos;
	cameraPos.cameraPos			= point.cameraPos;
	cameraPos.targetPos.node	= point.cameraPos.node;
	cameraPos.targetPos.pos		= point.cameraPos.pos + point.cameraOrient.BuildMatrix().GetColumn2_T();

	// Insert the new pair
	SubmitCameraPosition( point_name, cameraPos );
}

// Convert a vector to target based position to two point and submit
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraVectorToTargetPosition& point )
{
	// Setup the new position
	CameraPosition cameraPos;
	cameraPos.cameraPos			= point.cameraPos;
	cameraPos.targetPos.node	= point.cameraPos.node;
	cameraPos.targetPos.pos		= point.cameraPos.pos + point.vectToTarget;

	// Insert the new pair
	SubmitCameraPosition( point_name, cameraPos );
}

// Convert a vector to camera based position to two point and submit
void CameraAgent::SubmitCameraPosition( const gpstring& point_name, const CameraVectorToCameraPosition& point )
{
	// Setup the new position
	CameraPosition cameraPos;
	cameraPos.targetPos			= point.targetPos;
	cameraPos.cameraPos.node	= point.targetPos.node;
	cameraPos.cameraPos.pos		= point.targetPos.pos + point.vectToCamera;

	// Insert the new pair
	SubmitCameraPosition( point_name, cameraPos );
}

// Removes a position from the internal cache.
void CameraAgent::RemoveCameraPosition( const gpstring& point_name )
{
	PosCache::iterator i = m_PositionCache.find( point_name );
	if( i != m_PositionCache.end() )
	{
		m_PositionCache.erase( i );
	}
}

// Clears the interal cache of points
void CameraAgent::ClearCameraPositionCache()
{
	m_PositionCache.clear();
}

// Submit a list of local camera space offsets
void CameraAgent::SubmitCameraOffset( const vector_3& offset )
{
	// Add offset
	m_LocalOffset	+= offset;

	// Clamp values to max
	m_LocalOffset.x	= max( -m_CameraOffsetMaximum, min( m_LocalOffset.x, m_CameraOffsetMaximum ) );
	m_LocalOffset.y	= max( -m_CameraOffsetMaximum, min( m_LocalOffset.y, m_CameraOffsetMaximum ) );
	m_LocalOffset.z	= max( -m_CameraOffsetMaximum, min( m_LocalOffset.z, m_CameraOffsetMaximum ) );
}

// Submit a list of nodal space offsets
void CameraAgent::SubmitCameraOffset( const SiegePos& offset )
{
	if( m_NodalOffset.node == siege::UNDEFINED_GUID )
	{
		m_NodalOffset	= offset;
	}
	else
	{
		// Calc the world space position of this new offset
		siege::SiegeNodeHandle off_handle	= gSiegeEngine.NodeCache().UseObject( offset.node );
		const siege::SiegeNode& off_node	= off_handle.RequestObject( gSiegeEngine.NodeCache() );

		// Convert it into the nodal space of the existing offset
		siege::SiegeNodeHandle old_handle	= gSiegeEngine.NodeCache().UseObject( m_NodalOffset.node );
		const siege::SiegeNode& old_node	= old_handle.RequestObject( gSiegeEngine.NodeCache() );

		m_NodalOffset.pos					+= old_node.GetTransposeOrientation() * (off_node.GetCurrentOrientation() * offset.pos);
	}

	// Clamp values to max
	m_NodalOffset.pos.x	= max( -m_CameraOffsetMaximum, min( m_NodalOffset.pos.x, m_CameraOffsetMaximum ) );
	m_NodalOffset.pos.y	= max( -m_CameraOffsetMaximum, min( m_NodalOffset.pos.y, m_CameraOffsetMaximum ) );
	m_NodalOffset.pos.z	= max( -m_CameraOffsetMaximum, min( m_NodalOffset.pos.z, m_CameraOffsetMaximum ) );
}

// Attaches the given order to end of the command list.
void CameraAgent::SubmitOrder( const gpstring& point_name, eCameraOrder order, float time, bool bPersist, Scid scid )
{
	CameraOrder cam_order;
	cam_order.order			= order;
	cam_order.time			= time;
	cam_order.bPersist		= bPersist;
	cam_order.ownerScid		= scid;

	m_OrderQueue.push_back( OrderPair( point_name, cam_order ) );
}

// Clears the orders for the camera
void CameraAgent::ClearOrders()
{
	// Send active order completion
	if( !m_ActiveOrder.bIsComplete && m_ActiveOrder.cam_order.second.ownerScid != SCID_INVALID )
	{
		PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, m_ActiveOrder.cam_order.second.ownerScid, 0.0f );
	}

	// Go through order queue and submit done messages
	for( OrderQueue::iterator o = m_OrderQueue.begin(); o != m_OrderQueue.end(); ++o )
	{
		if( (*o).second.ownerScid != SCID_INVALID )
		{
			PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, (*o).second.ownerScid, 0.0f );
		}
	}

	// Clear the list of queued orders
	m_OrderQueue.clear();

	// Tell the current order that it is now complete
	m_ActiveOrder.bIsComplete	= true;
	m_OverTime					= 0.0;

	ClearTeleportRequest();
}

// Resets the affectors for a location change
void CameraAgent::ResetCameraAffectorModification()
{
	m_CameraHistoricAffect	= 1.0f;
	m_CameraLocationAffect	= 0.0f;
}

// Resets the affectors for a location change
void CameraAgent::ResetTargetAffectorModification()
{
	m_TargetHistoricAffect	= 1.0f;
	m_TargetLocationAffect	= 0.0f;
}

// Modify the current interpolation mode
void CameraAgent::SetInterpolationMode( eCameraInterpolate interpolate )
{
	m_Interpolate	= interpolate;
	ClearOrders();
}

// Update the camera.
void CameraAgent::Update( float secondsElapsed )
{
	// If the current order is complete and another order is waiting, reset the order
	if( m_ActiveOrder.bIsComplete )
	{
		if( !m_OrderQueue.empty() )
		{
			// See if the existing active order was a spline
			if( m_ActiveOrder.cam_order.second.order == COR_SPLINE )
			{
				// Go through order queue and remove the other positions
				for( OrderQueue::iterator o = m_OrderQueue.begin(); o != m_OrderQueue.end(); )
				{
					if( (*o).second.order == COR_CONT_SPLINE || (*o).second.order == COR_END_SPLINE )
					{
						if( (*o).second.ownerScid != SCID_INVALID )
						{
							PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, (*o).second.ownerScid, 0.0f );
						}
						o = m_OrderQueue.erase( o );
					}
					else
					{
						++o;
					}
				}
			}

			// Get the new order from the queue
			m_ActiveOrder.cam_order			= m_OrderQueue.front();
			m_OrderQueue.pop_front();

			// Reset the active order components
			m_ActiveOrder.bIsComplete		= false;
			m_ActiveOrder.timeElapsed		= m_OverTime;
			m_OverTime						= 0.0;

			// Store off the current location of the camera
			m_ActiveOrder.m_CameraPos.node	= gSiegeEngine.NodeWalker().TargetNodeGUID();
			m_ActiveOrder.m_CameraPos.pos	= gSiegeEngine.GetCamera().GetCameraPosition();
			m_ActiveOrder.m_TargetPos.node	= m_ActiveOrder.m_CameraPos.node;
			m_ActiveOrder.m_TargetPos.pos	= gSiegeEngine.GetCamera().GetTargetPosition();

			if( m_ActiveOrder.cam_order.second.order == COR_BEGIN_SPLINE ||
				m_ActiveOrder.cam_order.second.order == COR_SPLINE )
			{
				// Build spline information list and total time
				m_ActiveSplineIndex		= 0;

				SplinePair splinePoint;
				PosCache::iterator i	= m_PositionCache.find( m_ActiveOrder.cam_order.first );
				if( i != m_PositionCache.end() )
				{
					splinePoint.minRange	= 0.0f;
					splinePoint.maxRange	= gSiegeEngine.GetDifferenceVector( m_ActiveOrder.m_CameraPos, (*i).second.cameraPos ).Length();

					splinePoint.camPos	= (*i).second;

					m_SplinePoints.push_back( splinePoint );
				}
				else
				{
					gperror( "Could not find COR_BEGIN_SPLINE position!" );
				}

				float totalDistance		= splinePoint.maxRange;
				unsigned int index		= 0;

				// Go through order queue and build up our list
				for( OrderQueue::iterator o = m_OrderQueue.begin(); o != m_OrderQueue.end(); ++o )
				{
					if( (*o).second.order == COR_CONT_SPLINE || (*o).second.order == COR_END_SPLINE )
					{
						i = m_PositionCache.find( (*o).first );
						if( i != m_PositionCache.end() )
						{
							// Calculate our previous distance
							float newDistance		= gSiegeEngine.GetDifferenceVector( m_SplinePoints[ index ].camPos.cameraPos, (*i).second.cameraPos ).Length();
							gpassert( !IsZero( newDistance ) );

							// Update total distance
							totalDistance			+= newDistance;

							// Assign distance ranges
							splinePoint.minRange	= m_SplinePoints[ index ].maxRange;
							splinePoint.maxRange	= splinePoint.minRange + newDistance;

							splinePoint.camPos		= (*i).second;

							m_SplinePoints.push_back( splinePoint );
							index++;
						}
						else
						{
							gperror( "Could not find spline order position!" );
						}
					}
					if( (*o).second.order == COR_END_SPLINE )
					{
						break;
					}
				}

				gpassert( IsPositive( totalDistance ) );

				if( !IsPositive( totalDistance ) )
				{
					gperror( "Total distance for spline order is not greater than 0!  Order does nothing!" );
				}
				else
				{
					// With proper total distance, we can now normalize our ranges
					for( SplineCollection::iterator s = m_SplinePoints.begin(); s != m_SplinePoints.end(); ++s )
					{
						(*s).minRange	/= totalDistance;
						(*s).maxRange	/= totalDistance;
					}
				}
			}
			else if( m_ActiveOrder.cam_order.second.order == COR_CONT_SPLINE ||
					 m_ActiveOrder.cam_order.second.order == COR_END_SPLINE )
			{
				// Increment our active spline point index
				m_ActiveSplineIndex++;
			}
		}
		else
		{
			if( !m_ActiveOrder.cam_order.second.bPersist )
			{
				return;
			}
		}
	}

	// Apply the time offset to the affectors to keep them up to date
	if( m_CameraHistoricAffect > m_FixedHistoricAffect )
	{
		m_CameraHistoricAffect	-= ((1.0f - m_FixedHistoricAffect) * (secondsElapsed / m_CameraAffectorTime));
		if( m_CameraHistoricAffect < m_FixedHistoricAffect )
		{
			m_CameraHistoricAffect	= m_FixedHistoricAffect;
		}
	}

	if( m_CameraLocationAffect < m_FixedLocationAffect )
	{
		m_CameraLocationAffect	+= (m_FixedLocationAffect * (secondsElapsed / m_CameraAffectorTime));
		if( m_CameraLocationAffect > m_FixedLocationAffect )
		{
			m_CameraLocationAffect	= m_FixedLocationAffect;
		}
	}

	if( m_TargetHistoricAffect > m_FixedHistoricAffect )
	{
		m_TargetHistoricAffect	-= ((1.0f - m_FixedHistoricAffect) * (secondsElapsed / m_TargetAffectorTime));
		if( m_TargetHistoricAffect < m_FixedHistoricAffect )
		{
			m_TargetHistoricAffect	= m_FixedHistoricAffect;
		}
	}

	if( m_TargetLocationAffect < m_FixedLocationAffect )
	{
		m_TargetLocationAffect	+= (m_FixedLocationAffect * (secondsElapsed / m_TargetAffectorTime));
		if( m_TargetLocationAffect > m_FixedLocationAffect )
		{
			m_TargetLocationAffect	= m_FixedLocationAffect;
		}
	}

	// This position is the position that we will set the camera to at the end of the update
	CameraPosition	currentPos;
	bool			bApplyOffset	= true;

	// Increase the elapsed time of the active order
	m_ActiveOrder.timeElapsed	+= secondsElapsed;

	// Check the current order
	if( m_ActiveOrder.cam_order.second.order == COR_SNAP )
	{
		// Find the point that we need to snap to
		PosCache::iterator i	= m_PositionCache.find( m_ActiveOrder.cam_order.first );
		if( i != m_PositionCache.end() )
		{
			// Tell the current position where it needs to be
			currentPos		= i->second;

			// Check our current interpolate mode
			if( m_Interpolate == CI_TIME )
			{
				if( !m_ActiveOrder.bIsComplete &&
					m_ActiveOrder.timeElapsed >= m_ActiveOrder.cam_order.second.time )
				{
					m_ActiveOrder.bIsComplete	= true;

					if( !m_OrderQueue.empty() )
					{
						m_OverTime		= m_ActiveOrder.timeElapsed - m_ActiveOrder.cam_order.second.time;
						bApplyOffset	= false;
					}

					if( m_ActiveOrder.cam_order.second.ownerScid != SCID_INVALID )
					{
						PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, m_ActiveOrder.cam_order.second.ownerScid, 0.0f );
					}
				}
			}
			else if( m_Interpolate == CI_SMOOTH )
			{
				m_ActiveOrder.bIsComplete	= true;
			}
		}
	}
	else if( m_ActiveOrder.cam_order.second.order == COR_PAN )
	{
		// Find the point that we need to pan to
		PosCache::iterator i	= m_PositionCache.find( m_ActiveOrder.cam_order.first );
		if( i != m_PositionCache.end() )
		{
			// Check our current interpolate mode
			if( m_Interpolate == CI_TIME )
			{
				// Figure out how far along we are supposed to be
				float interpolateAmount	= 1.0f;
				if( m_ActiveOrder.cam_order.second.time )
				{
					interpolateAmount = m_ActiveOrder.timeElapsed / m_ActiveOrder.cam_order.second.time;
				}

				if( interpolateAmount >= 1.0 )
				{
					currentPos	= i->second;

					if( !m_ActiveOrder.bIsComplete )
					{
						m_ActiveOrder.bIsComplete	= true;

						if( !m_OrderQueue.empty() )
						{
							m_OverTime		= m_ActiveOrder.timeElapsed - m_ActiveOrder.cam_order.second.time;
							bApplyOffset	= false;
						}

						if( m_ActiveOrder.cam_order.second.ownerScid != SCID_INVALID )
						{
							PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, m_ActiveOrder.cam_order.second.ownerScid, 0.0f );
						}
					}
				}
				else
				{
					currentPos.cameraPos		= i->second.cameraPos;
					currentPos.targetPos		= i->second.targetPos;
					currentPos.cameraPos.pos	+= (gSiegeEngine.GetDifferenceVector( i->second.cameraPos, m_ActiveOrder.m_CameraPos ) * (1.0f - interpolateAmount));
					currentPos.targetPos.pos	+= (gSiegeEngine.GetDifferenceVector( i->second.targetPos, m_ActiveOrder.m_TargetPos ) * (1.0f - interpolateAmount));
				}
			}
			else if( m_Interpolate == CI_SMOOTH )
			{
				// Add up all the points to get an average history
				vector_3 histCameraAverage;
				vector_3 histTargetAverage;
				gSiegeEngine.GetCamera().GetHistoricPositions( histCameraAverage, histTargetAverage );

				// Get the world space camera position
				siege::SiegeNodeHandle cam_handle	= gSiegeEngine.NodeCache().UseObject( i->second.cameraPos.node );
				const siege::SiegeNode& cam_node	= cam_handle.RequestObject( gSiegeEngine.NodeCache() );
				vector_3 worldSpaceCamera			= cam_node.NodeToWorldSpace( i->second.cameraPos.pos );

				// Get the world space target position
				siege::SiegeNodeHandle tar_handle	= gSiegeEngine.NodeCache().UseObject( i->second.targetPos.node );
				const siege::SiegeNode& tar_node	= tar_handle.RequestObject( gSiegeEngine.NodeCache() );
				vector_3 worldSpaceTarget			= tar_node.NodeToWorldSpace( i->second.targetPos.pos );

				if( (Length( histCameraAverage - worldSpaceCamera ) + Length( histTargetAverage - worldSpaceTarget )) < 0.2f )
				{
					m_ActiveOrder.bIsComplete	= true;
				}

				// Scale each history component to be 70% of our current location
				histCameraAverage			*= m_CameraHistoricAffect;
				histTargetAverage			*= m_TargetHistoricAffect;

				currentPos.cameraPos.node	= i->second.cameraPos.node;
				currentPos.cameraPos.pos	= cam_node.WorldToNodeSpace( histCameraAverage + (worldSpaceCamera * m_CameraLocationAffect) );
				currentPos.targetPos.node	= i->second.targetPos.node;
				currentPos.targetPos.pos	= tar_node.WorldToNodeSpace( histTargetAverage + (worldSpaceTarget * m_TargetLocationAffect) );
			}
		}
	}
	else if( m_ActiveOrder.cam_order.second.order == COR_OFFSETPAN )
	{
		// Find the point that we need to pan to
		PosCache::iterator i	= m_PositionCache.find( m_ActiveOrder.cam_order.first );
		if( i != m_PositionCache.end() )
		{
			// Check our current interpolate mode
			if( m_Interpolate == CI_TIME )
			{
				// Figure out how far along we are supposed to be
				float interpolateAmount	= 1.0f;
				if( m_ActiveOrder.cam_order.second.time )
				{
					interpolateAmount = m_ActiveOrder.timeElapsed / m_ActiveOrder.cam_order.second.time;
				}

				if( interpolateAmount >= 1.0 )
				{
					currentPos	= i->second;

					if( !m_ActiveOrder.bIsComplete )
					{
						m_ActiveOrder.bIsComplete	= true;

						if( !m_OrderQueue.empty() )
						{
							m_OverTime		= m_ActiveOrder.timeElapsed - m_ActiveOrder.cam_order.second.time;
							bApplyOffset	= false;
						}

						if( m_ActiveOrder.cam_order.second.ownerScid != SCID_INVALID )
						{
							PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, m_ActiveOrder.cam_order.second.ownerScid, 0.0f );
						}
					}
				}
				else
				{
					currentPos.targetPos.node	= i->second.targetPos.node;
					currentPos.targetPos.pos	= gSiegeEngine.GetDifferenceVector( i->second.targetPos, m_ActiveOrder.m_TargetPos ) * interpolateAmount;

					currentPos.cameraPos		= currentPos.targetPos;
					currentPos.cameraPos.pos	+= gSiegeEngine.GetDifferenceVector( i->second.targetPos, i->second.cameraPos );
				}
			}
			else if( m_Interpolate == CI_SMOOTH )
			{
				// Add up all the points to get an average history
				vector_3 histCameraAverage;
				vector_3 histTargetAverage;
				gSiegeEngine.GetCamera().GetHistoricPositions( histCameraAverage, histTargetAverage );

				// Get the world space camera position
				siege::SiegeNodeHandle cam_handle	= gSiegeEngine.NodeCache().UseObject( i->second.cameraPos.node );
				const siege::SiegeNode& cam_node	= cam_handle.RequestObject( gSiegeEngine.NodeCache() );
				vector_3 worldSpaceCamera			= cam_node.NodeToWorldSpace( i->second.cameraPos.pos );

				// Get the world space target position
				siege::SiegeNodeHandle tar_handle	= gSiegeEngine.NodeCache().UseObject( i->second.targetPos.node );
				const siege::SiegeNode& tar_node	= tar_handle.RequestObject( gSiegeEngine.NodeCache() );
				vector_3 worldSpaceTarget			= tar_node.NodeToWorldSpace( i->second.targetPos.pos );

				if( Length( histTargetAverage - worldSpaceTarget ) < 0.2f )
				{
					m_ActiveOrder.bIsComplete	= true;
				}

				// Scale each history component
				histTargetAverage			*= m_TargetHistoricAffect;

				// Get the worldSpaceTarget
				vector_3 cam_diff			= worldSpaceCamera - worldSpaceTarget;
				worldSpaceTarget			= histTargetAverage + (worldSpaceTarget * m_TargetLocationAffect);

				// Setup the target position
				currentPos.targetPos.node	= i->second.targetPos.node;
				currentPos.targetPos.pos	= tar_node.WorldToNodeSpace( worldSpaceTarget );

				currentPos.cameraPos.node	= i->second.cameraPos.node;
				currentPos.cameraPos.pos	= cam_node.WorldToNodeSpace( worldSpaceTarget + cam_diff );
			}
		}
	}
	else if( m_ActiveOrder.cam_order.second.order == COR_SPLINE			||
			 m_ActiveOrder.cam_order.second.order == COR_BEGIN_SPLINE	||
			 m_ActiveOrder.cam_order.second.order == COR_CONT_SPLINE	||
			 m_ActiveOrder.cam_order.second.order == COR_END_SPLINE )
	{
		// Check our current interpolate mode
		// Spline orders can only be requested in CI_TIME mode
		if( m_Interpolate == CI_TIME )
		{
			// Figure out how far along we are supposed to be
			float interpolateAmount	= 1.0f;
			if( m_ActiveOrder.cam_order.second.time )
			{
				interpolateAmount = m_ActiveOrder.timeElapsed / m_ActiveOrder.cam_order.second.time;
			}

			if( interpolateAmount >= 1.0 )
			{
				interpolateAmount	= 1.0;

				if( !m_ActiveOrder.bIsComplete )
				{
					m_ActiveOrder.bIsComplete	= true;

					if( !m_OrderQueue.empty() )
					{
						m_OverTime		= m_ActiveOrder.timeElapsed - m_ActiveOrder.cam_order.second.time;
						bApplyOffset	= false;
					}

					if( m_ActiveOrder.cam_order.second.ownerScid != SCID_INVALID )
					{
						PostWorldMessage( WE_CAMERA_COMMAND_DONE, GOID_INVALID, m_ActiveOrder.cam_order.second.ownerScid, 0.0f );
					}
				}
			}

			// Get the current point that we are interested in
			SplinePair& splinePoint	= m_SplinePoints[ m_ActiveSplineIndex ];

			// Calculate relative offset into the complete spline
			float splineInterp;
			if( m_ActiveOrder.cam_order.second.order != COR_SPLINE )
			{
				splineInterp = min_t( 1.0f, max_t( 0.0f, splinePoint.minRange + (interpolateAmount * (splinePoint.maxRange - splinePoint.minRange)) ) );
			}
			else
			{
				splineInterp = min_t( 1.0f, max_t( 0.0f, interpolateAmount ) );
			}

			// Build spline interpolated positions
			vector_3 splineTargetPos( DoNotInitialize );
			vector_3 splineCameraPos( DoNotInitialize );

			BuildCurrentSplinePoints( splineInterp, splineCameraPos, splineTargetPos );

			// Assign spline interpolated points to the camera position
			currentPos.targetPos.FromWorldPos( splineTargetPos, splinePoint.camPos.targetPos.node );
			currentPos.cameraPos.FromWorldPos( splineCameraPos, splinePoint.camPos.cameraPos.node );
		}
		else
		{
			gperror( "Cannot request spline orders when not using time based interpolation!" );
		}
	}

	// Set the main camera to the current position
	if( currentPos.cameraPos.node == siege::UNDEFINED_GUID )
	{
		currentPos.cameraPos.node	= gSiegeEngine.NodeWalker().TargetNodeGUID();
		currentPos.cameraPos.pos	= gSiegeEngine.GetCamera().GetCameraPosition();
	}
	if( currentPos.targetPos.node == siege::UNDEFINED_GUID )
	{
		currentPos.targetPos.node	= gSiegeEngine.NodeWalker().TargetNodeGUID();
		currentPos.targetPos.pos	= gSiegeEngine.GetCamera().GetTargetPosition();
	}

	SetMainCameraPosition( currentPos, bApplyOffset );
}

// Clear the main camera position
void CameraAgent::ClearPosition()
{
	ClearCameraPositionCache();
	m_ActiveOrder.bIsComplete	= true;
	m_ActiveOrder.cam_order		= OrderPair( gpstring::EMPTY, COR_SNAP );
	m_ActiveOrder.m_CameraPos	= SiegePos::INVALID;
	m_ActiveOrder.m_TargetPos	= SiegePos::INVALID;
	m_ActiveOrder.timeElapsed	= 0.0;
	gSiegeEngine.GetCamera().ClearHistory();
}

// Set the main camera position with the given pos
void CameraAgent::SetMainCameraPosition( const CameraPosition& pos, bool bApplyOffset )
{
	vector_3 worldSpaceCamera( DoNotInitialize );
	vector_3 worldSpaceTarget( DoNotInitialize );

	// Get the world space camera position
	siege::SiegeNodeHandle cam_handle	= gSiegeEngine.NodeCache().UseObject( pos.cameraPos.node );
	const siege::SiegeNode& cam_node	= cam_handle.RequestObject( gSiegeEngine.NodeCache() );
	worldSpaceCamera					= cam_node.NodeToWorldSpace( pos.cameraPos.pos );

	// Get the world space target position
	siege::SiegeNodeHandle tar_handle	= gSiegeEngine.NodeCache().UseObject( pos.targetPos.node );
	const siege::SiegeNode& tar_node	= tar_handle.RequestObject( gSiegeEngine.NodeCache() );
	worldSpaceTarget					= tar_node.NodeToWorldSpace( pos.targetPos.pos );

	if( bApplyOffset )
	{
		// Add in local camera space offset if requested
		if( !m_LocalOffset.IsZero() )
		{
			// Build a world space orientation matrix to get local space basis vectors
			matrix_3x3 orient	= MatrixFromDirection( worldSpaceTarget - worldSpaceCamera );

			// Add in the offsets
			vector_3 baseOffset( DoNotInitialize );

			orient.GetColumn0( baseOffset );
			baseOffset			*= m_LocalOffset.x;
			worldSpaceTarget	+= baseOffset;
			worldSpaceCamera	+= baseOffset;

			orient.GetColumn1( baseOffset );
			baseOffset			*= m_LocalOffset.y;
			worldSpaceTarget	+= baseOffset;
			worldSpaceCamera	+= baseOffset;

			orient.GetColumn2( baseOffset );
			baseOffset			*= m_LocalOffset.z;
			worldSpaceTarget	+= baseOffset;
			worldSpaceCamera	+= baseOffset;

			m_LocalOffset		= vector_3::ZERO;
		}

		// Add in nodal camera space offset if requested
		if( m_NodalOffset.node != siege::UNDEFINED_GUID )
		{
			// Convert the offset from node to world 	// Get the world space camera position
			siege::SiegeNodeHandle off_handle	= gSiegeEngine.NodeCache().UseObject( m_NodalOffset.node );
			const siege::SiegeNode& off_node	= off_handle.RequestObject( gSiegeEngine.NodeCache() );

			vector_3 worldSpaceOffset			= off_node.GetCurrentOrientation() * m_NodalOffset.pos;
			worldSpaceCamera					+= worldSpaceOffset;
			worldSpaceTarget					+= worldSpaceOffset;

			m_NodalOffset.node					= siege::UNDEFINED_GUID;
			m_NodalOffset.pos					= vector_3::ZERO;
		}
	}

	// Build node space positions
	SiegePos worldCamPos, worldTarPos;
	worldCamPos.FromWorldPos( worldSpaceCamera, pos.cameraPos.node );
	worldTarPos.FromWorldPos( worldSpaceTarget, pos.targetPos.node );

	// Set the new camera position information
	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( worldCamPos, worldTarPos );
}

// Builds interpolated spline points using the current m_SplinePoints and using the given t (0.0 - 1.0)
void CameraAgent::BuildCurrentSplinePoints( const float t, vector_3& splineCameraPos, vector_3& splineTargetPos )
{
	std::vector< vector_3 > cameraWorldPoints;
	std::vector< vector_3 > targetWorldPoints;

	// Go through the spline points and add them to the lists
	for( SplineCollection::iterator i = m_SplinePoints.begin(); i != m_SplinePoints.end(); ++i )
	{
		cameraWorldPoints.push_back( (*i).camPos.cameraPos.WorldPos() );
		targetWorldPoints.push_back( (*i).camPos.targetPos.WorldPos() );
	}
	
	DEBUG_FLOAT_VALIDATE( t );

	// Interpolate the spline
	splineCameraPos = GeneralBezier( t, cameraWorldPoints );
	DEBUG_FLOAT_VALIDATE( splineCameraPos.x );
	DEBUG_FLOAT_VALIDATE( splineCameraPos.y );
	DEBUG_FLOAT_VALIDATE( splineCameraPos.z );

	splineTargetPos = GeneralBezier( t, targetWorldPoints );
	DEBUG_FLOAT_VALIDATE( splineTargetPos.x );
	DEBUG_FLOAT_VALIDATE( splineTargetPos.y );
	DEBUG_FLOAT_VALIDATE( splineTargetPos.z );
}

//////////////////////////////////////////////////////////////////////////////
