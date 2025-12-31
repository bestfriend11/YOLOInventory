//////////////////////////////////////////////////////////////////////////////
//
// File     :  Timeline.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "Timeline.h"

//////////////////////////////////////////////////////////////////////////////
// class TimeTarget implementation

const float TimeTarget::DURATION_IMPULSE  =  0;
const float TimeTarget::DURATION_INFINITE = -1;
const int   TimeTarget::REPEAT_INFINITE   = -1;

void TimeTarget :: DecRepeatCount( void )
{
	if ( !IsInfiniteRepeat() )
	{
		gpassert( m_RepeatCount > 0 );
		--m_RepeatCount;
	}
}

#if GP_DEBUG

void TimeTarget :: OnDebugDump( int indent ) const
{
	gpstring indentStr( indent, ' ' );

	gpgenericf(( "%s[t:%s,n:%s]\n",
			indentStr.c_str(),
			OnGetDebugType().c_str(),
			OnGetDebugName().c_str() ));
	gpgenericf(( "%stimeOffset = %f, duration = %s, repeatCount = %s\n",
			indentStr.c_str(),
			m_TimeOffset,
			(m_Duration == DURATION_IMPULSE )
				? "IMPULSE"
				: (m_Duration == DURATION_INFINITE)
					? "INFINITE"
					: gpstringf( "%f", m_Duration ).c_str(),
			(m_RepeatCount == REPEAT_INFINITE)
				? "INFINITE"
				: gpstringf( "%d", m_RepeatCount ).c_str() ));

	gpstring extra = OnDebugGetInfo();
	if ( !extra.empty() )
	{
		gpgenericf(( "%s%s\n", indentStr.c_str(), extra.c_str() ));
	}
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// class CallbackTimeTarget implementation

class CallbackTimeTarget : public TimeTarget
{
public:
	SET_INHERITED( CallbackTimeTarget, TimeTarget );

	CallbackTimeTarget( Timecaster::CbTimeUpdate update )
		: m_Update( update )  {  }
	void OnUpdate( float deltaTime )
		{  m_Update( deltaTime );  }

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const  {  return ( "CallbackTimeTarget" );  }
#	endif // GP_DEBUG

private:
	Timecaster::CbTimeUpdate m_Update;

	SET_NO_COPYING( CallbackTimeTarget );
};

//////////////////////////////////////////////////////////////////////////////
// class ExecuteTimeTarget implementation

class ExecuteTimeTarget : public TimeTarget
{
public:
	SET_INHERITED( ExecuteTimeTarget, TimeTarget );

	ExecuteTimeTarget( Timecaster::CbExecute execute )
		: m_Execute( execute )  {  }
	void OnExecute( float /*deltaTime*/ )
		{  m_Execute();  }

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const  {  return ( "ExecuteTimeTarget" );  }
#	endif // GP_DEBUG

private:
	Timecaster::CbExecute m_Execute;

	SET_NO_COPYING( ExecuteTimeTarget );
};

//////////////////////////////////////////////////////////////////////////////
// class Timecaster implementation

void Timecaster :: Erase( void )
{
	// delete the time targets that we own
	TimeTargetMap::iterator i, ibegin = m_TimeTargets.begin(), iend = m_TimeTargets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->second.Erase();
	}

	// clear the rest
	m_TimeTargets.clear();
	m_CurrentTime = 0;
}

void Timecaster :: AddTarget( TimeTarget* timeTarget, bool owned )
{
	gpassert( (timeTarget != NULL) && (timeTarget->GetParent() == NULL) );

	timeTarget->SetParent( this );
	m_TimeTargets.insert( std::make_pair(					// insert new entry
			timeTarget->GetTimeOffset() + m_CurrentTime,	// now plus offset
			Entry( timeTarget, owned ) ) );					// hand off the target
}

void Timecaster :: AddDurationTarget( TimeTarget* timeTarget, float timeOffset, float duration, bool owned )
{
	gpassert( timeTarget != NULL );
	timeTarget->SetTimeOffset( timeOffset );
	timeTarget->SetDuration( duration );
	AddTarget( timeTarget, owned );
}

void Timecaster :: AddDurationTarget( TimeTarget* timeTarget, float duration, bool owned )
{
	AddDurationTarget( timeTarget, 0, duration, owned );
}

void Timecaster :: AddImpulseTarget( TimeTarget* timeTarget, float timeOffset, bool owned )
{
	gpassert( timeTarget != NULL );
	timeTarget->SetTimeOffset( timeOffset );
	timeTarget->SetDuration( DURATION_IMPULSE );
	AddTarget( timeTarget, owned );
}

void Timecaster :: AddImpulseTarget( TimeTarget* timeTarget, bool owned )
{
	AddImpulseTarget( timeTarget, 0, owned );
}

void Timecaster :: AddUpdateCallback( CbTimeUpdate update, float timeOffset, float duration )
{
	AddDurationTarget( new CallbackTimeTarget( update ), timeOffset, duration, true );
}

void Timecaster :: AddExecuteCallback( CbExecute execute, float timeOffset )
{
	AddImpulseTarget( new ExecuteTimeTarget( execute ), timeOffset, true );
}

void Timecaster :: OnUpdate( float deltaTime )
{
	// reset time to 0 and bail to save accuracy if nothing is going on
	if ( m_TimeTargets.empty() )
	{
		m_CurrentTime = 0;
		return;
	}

	// update time
	gpassert( deltaTime >= 0 );
	m_CurrentTime += deltaTime;

	// update targets
	TimeTargetMap::iterator i, begin = m_TimeTargets.begin(), end = m_TimeTargets.end();
	for ( i = begin ; (i != end) && (i->first <= m_CurrentTime) ; )
	{
		// get some vars
		float timeOffset = i->first;
		Entry& entry = i->second;
		TimeTarget* target = entry.m_TimeTarget;

		// first execution?
		if ( !entry.m_Executed )
		{
			// pass it the offset from when it was supposed to have executed
			entry.m_TimeTarget->OnExecute( m_CurrentTime - timeOffset );
			entry.m_Executed = true;
			++i;
			continue;
		}

		// supposed to continue?
		float endTime = timeOffset + target->GetDuration();
		if ( target->IsInfiniteDuration() || (endTime >= m_CurrentTime) )
		{
			target->OnUpdate( deltaTime );
			++i;
			continue;
		}

		// give it one final update to finish it out (to catch any dangling impulses)
		target->OnLastUpdate( m_CurrentTime - endTime );

		// it's done, time to delete it?
		if ( target->GetRepeatCount() == 0 )
		{
			// ok delete it
			entry.Erase();
		}
		else
		{
			// dec repeat count and requeue after the timeOffset delay
			target->DecRepeatCount();
			m_TimeTargets.insert( std::make_pair(
					endTime + target->GetTimeOffset(),
					Entry( target, entry.m_Owned ) ) );
		}

		// this one is finished, next...
		i = m_TimeTargets.erase( i );
	}
}

#if GP_DEBUG

void Timecaster :: OnDebugDump( int indent ) const
{
	Inherited::OnDebugDump( indent );

	indent += 4;
	gpstring indentStr( indent, ' ' );

	TimeTargetMap::const_iterator i, begin = m_TimeTargets.begin(), end = m_TimeTargets.end();
	int index = 0;
	for ( i = begin ; i != end ; ++i, ++index )
	{
		gpgenericf(( "%s%02d: @ %f, executed = %s\n",
					  indentStr.c_str(),
					  index,
					  i->first,
					  i->second.m_Executed ? "yes" : "no" ));
		i->second.m_TimeTarget->OnDebugDump( indent );
	}
}

gpstring Timecaster :: OnDebugGetInfo( void ) const
{
	return ( gpstringf( "current time = %f, num targets = %d",
								 m_CurrentTime, m_TimeTargets.size() ) );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// class Timeline implementation

// $$$ fix up timeline to do two things: first, give OnFinalUpdate() to
//     always-updates right before they get removed (like timecaster) from the
//     list. second, on the Timeline::OnLastUpdate() do the same thing for any
//     remaining always-updates.

Timeline :: Timeline( float duration )
{
	m_Running    = false;
	m_NextTarget = m_TimeTargets.end();
	SetDuration( duration );
	GPDEBUG_ONLY( m_Iterating = false );
}

Timeline :: ~Timeline( void )
{
	GPDEBUG_ONLY( m_Iterating = false );
	Erase();
}

void Timeline :: Erase( void )
{
	// cannot mess with list while iterating
	gpassert( !m_Iterating );

	Inherited::Erase();
	m_TimeUpdates.clear();
	m_Running = false;
	m_NextTarget = m_TimeTargets.end();
}

void Timeline :: Reset( float baseTime )
{
	// finish off impulse targets that were missed
	ExecuteImpulse( GetDuration() + baseTime );

	// reset self
	m_CurrentTime = baseTime;
	m_TimeUpdates.clear();

	// only repeating entries are left
	TimeTargetMap::iterator i, begin = m_TimeTargets.begin(), end = m_TimeTargets.end();
	for ( i = begin ; i != end ; )
	{
		// get some vars
		Entry& entry       = i->second;
		TimeTarget* target = entry.m_TimeTarget;

		// dec the count
		if ( target->GetRepeatCount() == 0 )
		{
			entry.Erase();
			i = m_TimeTargets.erase( i );
		}
		else
		{
			target->DecRepeatCount();
			++i;
		}
	}

	// reset next iter
	m_Running = false;
	m_NextTarget = m_TimeTargets.end();
}

void Timeline :: AddTarget( TimeTarget* timeTarget, bool owned )
{
	gpassert( (timeTarget != NULL) && (timeTarget->GetParent() == NULL) );

	timeTarget->SetParent( this );
	m_TimeTargets.insert( std::make_pair(			// insert new entry
			timeTarget->GetTimeOffset(),  			// offset absolute from base
			Entry( timeTarget, owned ) ) );			// hand off the target
}

void Timeline :: OnExecute( float deltaTime )
{
	if ( m_Running )
	{
		Reset();
	}
	OnUpdate( deltaTime );
}

void Timeline :: OnUpdate( float deltaTime )
{
	// check for recursion
	gpassert( !m_Iterating );
	GPDEBUG_ONLY( m_Iterating = true );

	// update our running time
	gpassert( deltaTime >= 0 );
	m_CurrentTime += deltaTime;

	// repeat if done and owning timeline (if any) didn't OnExecute() us
	if ( m_CurrentTime > GetDuration() )
	{
		Reset( m_CurrentTime - GetDuration() );
	}

	// check to see if we're just now starting the timeline
	if ( !m_Running )
	{
		// this should be empty at this point
		gpassert( m_TimeUpdates.empty() );

		// update state
		if ( !m_TimeTargets.empty() )
		{
			m_Running = true;
			m_NextTarget = m_TimeTargets.begin();
		}
	}
	else
	{
		// update the always-updates
		TimeTargetColl::iterator i, begin = m_TimeUpdates.begin(), end = m_TimeUpdates.end();
		for ( i = begin ; i != end ; )
		{
			TimeTarget* target = (*i)->second.m_TimeTarget;

			// see if it's supposed to continue
			if (   target->IsInfiniteDuration()
				|| ((target->GetTimeOffset() + target->GetDuration()) >= m_CurrentTime) )
			{
				target->OnUpdate( deltaTime );
				++i;
			}
			else
			{
				// all done, remove it from the always-update list
				i = m_TimeUpdates.erase( i );
			}
		}
	}

	// update the impulse time targets
	ExecuteImpulse( m_CurrentTime );

	// release
	GPDEBUG_ONLY( m_Iterating = false );
}

void Timeline :: OnLastUpdate( float deltaTime )
{
	// finish up the impulse time targets only
	m_CurrentTime += deltaTime;
	ExecuteImpulse( m_CurrentTime );
}

void Timeline :: ExecuteImpulse( float time )
{
	TimeTargetMap::iterator end = m_TimeTargets.end();
	for ( ; m_NextTarget != end ; ++m_NextTarget )
	{
		TimeTarget* target = m_NextTarget->second.m_TimeTarget;

		// bail if it's not supposed to happen yet (neither will any of the rest)
		if ( target->GetTimeOffset() > time )
		{
			break;
		}

		// trigger it with the time passed since it was supposed to fire
		target->OnExecute( time - target->GetTimeOffset() );

		// move it into update list if it's not an impulse
		if ( !target->IsImpulseDuration() )
		{
			m_TimeUpdates.push_back( m_NextTarget );
		}
	}
}

#if GP_DEBUG

void Timeline :: OnDebugDump( int indent ) const
{
	Inherited::OnDebugDump( indent );

	indent += 4;
	gpstring str( indent, ' ' );

	TimeTargetColl::const_iterator i, begin = m_TimeUpdates.begin(), end = m_TimeUpdates.end();
	for ( i = begin ; i != end ; ++i )
	{
		gpgenericf(( "<always> 0x%08X\n", (*i)->second.m_TimeTarget ));
	}
}

gpstring Timeline :: OnDebugGetInfo( void ) const
{
	return (  Inherited::OnDebugGetInfo()
			+ gpstringf( ", running = %s, num continuous updates = %d",
								  m_Running ? "yes" : "no", m_TimeUpdates.size() ) );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
