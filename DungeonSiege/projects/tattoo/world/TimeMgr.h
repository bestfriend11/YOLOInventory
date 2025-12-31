//////////////////////////////////////////////////////////////////////////////
//
// File     :  TimeMgr.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a global time manager for the game.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __TIMEMGR_H
#define __TIMEMGR_H

//////////////////////////////////////////////////////////////////////////////

#include "Timeline.h"
#include "SkritDefs.h"

//////////////////////////////////////////////////////////////////////////////
// helper declarations

// Special Skrit callbacks.

	// set a particular void skrit function to be called at the given offset.
	void AddSkritCallback( Timecaster* caster, float timeOffset, Skrit::HObject skrit, int funcIndex );

	// search for Skrit "at" functions and add them to the caster when they are
	// supposed to occur based at 'timeOffset'.
	float AddSkritCallbacks( Timecaster* caster, float timeOffset, Skrit::HObject skrit );

	// use the skrit to create a time target that sends OnTimeExecute() and
	// OnTimeUpdate() events to the skrit.
	TimeTarget* CreateSkritTimeTarget( Skrit::HObject skrit );

//////////////////////////////////////////////////////////////////////////////
// class TimeMgr declaration

// purpose: this object is a global and generic Timeline.

class TimeMgr : public Timecaster, public Singleton <TimeMgr>
{
public:
	SET_INHERITED( TimeMgr, Timecaster );

	TimeMgr( void )  {  }
	virtual ~TimeMgr( void )  {  }

	FUBI_EXPORT void AddRapiFader( bool bFadeIn, const float duration );
	FUBI_MEMBER_DOC( AddRapiFader,     "bFadeIn,"			"duration",
					"Add a fade effect to screen that will last for 'duration' seconds." );

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const					{  return ( "TimeMgr" );  }
	virtual gpstring OnGetDebugName( void ) const					{  return ( "gTimeMgr" );  }
#	endif // GP_DEBUG

	Timecaster& GetSysTimecaster( void )							{  return ( m_SysTimecaster );  }

private:
	Timecaster m_SysTimecaster;

	FUBI_SINGLETON_CLASS( TimeMgr, "General purpose timecaster." );
	SET_NO_COPYING( TimeMgr );
};

#define gTimeMgr TimeMgr::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class DrawTimeMgr declaration

// purpose: same as gTimeMgr except it goes between BeginScene and EndScene.

class DrawTimeMgr : public Timecaster, public Singleton <DrawTimeMgr>
{
public:
	SET_INHERITED( DrawTimeMgr, Timecaster );

	DrawTimeMgr( void )  {  }
	virtual ~DrawTimeMgr( void )  {  }

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const  {  return ( "DrawTimeMgr" );  }
	virtual gpstring OnGetDebugName( void ) const  {  return ( "gDrawTimeMgr" );  }
#	endif // GP_DEBUG

private:
	SET_NO_COPYING( DrawTimeMgr );
};

#define gDrawTimeMgr DrawTimeMgr::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __TIMEMGR_H

//////////////////////////////////////////////////////////////////////////////
