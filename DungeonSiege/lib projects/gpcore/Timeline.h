//////////////////////////////////////////////////////////////////////////////
//
// File     :  Timeline.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes for handing out time to objects.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TIMELINE_H
#define __TIMELINE_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"

#include <map>
#include <list>

//////////////////////////////////////////////////////////////////////////////
// future

/*	$$ future: do we need an "update frequency" parameter in the spec? right now
	all non-impulse targets update every frame.
*/

class Timecaster;

//////////////////////////////////////////////////////////////////////////////
// class TimeTarget declaration

// purpose: derive from this class if you want to receive time.

class TimeTarget
{
public:
	SET_NO_INHERITED( TimeTarget );

	// constants
	static const float DURATION_IMPULSE;		// update occurs exactly once (until repeated)
	static const float DURATION_INFINITE;		// update occurs every frame
	static const int   REPEAT_INFINITE;			// repeat this event forever

// Setup.

	// ctor/dtor
	TimeTarget( void )												{  m_Parent = NULL;  Set( 0, DURATION_IMPULSE, 0 );  }
	virtual ~TimeTarget( void )										{  }

	// setup
	void SetParent     ( Timecaster* parent      )					{  m_Parent      = parent;       }
	void SetTimeOffset ( float       timeOffset  )					{  m_TimeOffset  = timeOffset;   }
	void SetDuration   ( float       duration    )					{  m_Duration    = duration;     }
	void SetRepeatCount( int         repeatCount )					{  m_RepeatCount = repeatCount;  }
	void DecRepeatCount( void );

	// all
	void Set( float timeOffset, float duration, int repeatCount )	{  m_TimeOffset = timeOffset;  m_Duration = duration;  m_RepeatCount = repeatCount;  }

// Operation.

	// query
	Timecaster* GetParent     ( void ) const						{  return ( m_Parent      );  }
	float       GetTimeOffset ( void ) const						{  return ( m_TimeOffset  );  }
	float       GetDuration   ( void ) const						{  return ( m_Duration    );  }
	int         GetRepeatCount( void ) const						{  return ( m_RepeatCount );  }

	// other
	bool IsImpulseDuration ( void ) const							{  return ( m_Duration    == DURATION_IMPULSE );  }
	bool IsInfiniteDuration( void ) const							{  return ( m_Duration    == DURATION_INFINITE );  }
	bool IsInfiniteRepeat  ( void ) const							{  return ( m_RepeatCount == REPEAT_INFINITE );  }

	// helpers
	void Finish( void )												{  m_Duration = 0;  m_RepeatCount = 0;  }

// Overrides.

	// this function is called the first time of each repetition with the delta
	// from when it was supposed to have been called. note that this delta may
	// be up to 99% of a full frame during normal execution.
	virtual void OnExecute( float /*deltaTime*/ )					{  }

	// this function is called each frame after OnExecute() to pass the delta
	// from the last update to the derivative class.
	virtual void OnUpdate( float /*deltaTime*/ )					{  }

	// this function is called last after we've gone past the duration. use it
	// to finish up any final work.
	virtual void OnLastUpdate( float /*deltaTime*/ )				{  }

#	if GP_DEBUG
	virtual void     OnDebugDump   ( int indent = 0 ) const;
	virtual gpstring OnGetDebugType( void ) const					{  return ( "TimeTarget" );  }
	virtual gpstring OnGetDebugName( void ) const					{  return ( gpstringf( "0x%08X", this ) );  }
	virtual gpstring OnDebugGetInfo( void ) const					{  return ( gpstring() );  }
#	endif // GP_DEBUG

private:
	Timecaster* m_Parent;				// timecaster parent (who is giving this time) if there is one
	float       m_TimeOffset;			// absolute offset in seconds from base of parent that this will begin receiving time
	float       m_Duration;				// total duration in seconds for this to get time - set to DURATION_IMPULSE or DURATION_INFINITE for special behavior
	int         m_RepeatCount;			// how many times to repeat this event - set to REPEAT_INFINITE for special behavior

	SET_NO_COPYING( TimeTarget );
};

//////////////////////////////////////////////////////////////////////////////
// class Timecaster declaration

// purpose: allows hanging of callbacks off an infinite duration timeline with
//          no end for sequenced and guaranteed execution. timecasters are
//          themselves time targets and can be nested.
//
//          all child time targets are repeated after they complete by adding
//          the completion time to their original timeOffset. timeOffset is
//          relative to the current time of the timecaster.

class Timecaster : public TimeTarget
{
public:
	SET_INHERITED( Timecaster, TimeTarget );
	typedef CBFunctor1 <float> CbTimeUpdate;
	typedef CBFunctor0 CbExecute;

// Setup.

	// ctor/dtor
	Timecaster( void )						{  m_CurrentTime = 0;  }
	virtual ~Timecaster( void )				{  Erase();  }

	// erase all triggers
	virtual void Erase( void );

	// query
	float GetCurrentTime( void ) const		{  return ( m_CurrentTime );  }

// Time targets.

	// general TimeTarget triggers
	virtual void AddTarget( TimeTarget* timeTarget, bool owned );

	// helpers
	void AddDurationTarget( TimeTarget* timeTarget, float timeOffset, float duration, bool owned );
	void AddDurationTarget( TimeTarget* timeTarget, float duration, bool owned );
	void AddImpulseTarget ( TimeTarget* timeTarget, float timeOffset, bool owned );
	void AddImpulseTarget ( TimeTarget* timeTarget, bool owned );

	// special callback target
	void AddUpdateCallback ( CbTimeUpdate update, float timeOffset, float duration );
	void AddExecuteCallback( CbExecute execute, float timeOffset );

// Internal API.

	virtual void OnUpdate( float deltaTime );

#	if GP_DEBUG
	virtual void     OnDebugDump   ( int indent = 0 ) const;
	virtual gpstring OnGetDebugType( void ) const  {  return ( "Timecaster" );  }
	virtual gpstring OnDebugGetInfo( void ) const;
#	endif // GP_DEBUG

protected:
	struct Entry
	{
		TimeTarget* m_TimeTarget;				// pointer to (possibly owned) time target
		bool        m_Owned;					// true if this is owned by timecaster and should be deleted when done
		bool        m_Executed;					// true after OnExecute(), reset on each repeat

		Entry( TimeTarget* target, bool owned )
			: m_TimeTarget( target ), m_Owned( owned ), m_Executed( false )
		{
			gpassert( target != NULL );
		}

		void Erase( void )
		{
			if ( m_Owned )
			{
				Delete( m_TimeTarget );
			}
			else
			{
				m_TimeTarget->SetParent( NULL );
			}
		}
	};

	typedef std::multimap <float, Entry> TimeTargetMap;
	typedef TimeTargetMap::iterator TimeTargetIter;

	float         m_CurrentTime;				// running total
	TimeTargetMap m_TimeTargets;				// impulse time targets

private:
	SET_NO_COPYING( Timecaster );
};

//////////////////////////////////////////////////////////////////////////////
// class Timeline declaration

// purpose: allows hanging of callbacks off a timeline for sequenced and
//          guaranteed execution. timelines are themselves time targets and can
//          be nested.
//
//          all child time targets are repeated from their original timeOffset
//          when the entire timeline repeats. timeOffset is absolute from the
//          start of the timeline, and if the current time has already passed
//          their offset when they are added to the timeline, they will have to
//          wait until the timeline repeats.

class Timeline : public Timecaster
{
public:
	SET_INHERITED( Timeline, Timecaster );

// Setup.

	// ctor/dtor.
	Timeline( float duration = 0 );
	virtual ~Timeline( void );

	// erase all triggers
	virtual void Erase( void );

	// reset the timeline - finish it up, force a repeat at baseTime
	void Reset( float baseTime = 0 );

// Time targets.

	// general TimeTarget triggers
	virtual void AddTarget( TimeTarget* timeTarget, bool owned );

	// $ see inherited for more utility Add() functions.

// Internal API.

	virtual void OnExecute   ( float deltaTime );
	virtual void OnUpdate    ( float deltaTime );
	virtual void OnLastUpdate( float deltaTime );

#	if GP_DEBUG
	virtual void     OnDebugDump   ( int indent = 0 ) const;
	virtual gpstring OnGetDebugType( void ) const  {  return ( "Timeline" );  }
	virtual gpstring OnDebugGetInfo( void ) const;
#	endif // GP_DEBUG

private:
	void ExecuteImpulse( float time );

	typedef std::list <TimeTargetIter> TimeTargetColl;

	bool           m_Running;					// true if we've started the timeline
	TimeTargetColl m_TimeUpdates;				// continuous time targets - from m_TimeTargets after impulse
	TimeTargetIter m_NextTarget;				// next target to update

#	if GP_DEBUG
	bool           m_Iterating;					// true while iterating over targets
#	endif // GP_DEBUG

	SET_NO_COPYING( Timeline );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __TIMELINE_H

//////////////////////////////////////////////////////////////////////////////
