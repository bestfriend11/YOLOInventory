//////////////////////////////////////////////////////////////////////////////
//
// File     :  RatioStack.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a recursive-ratio tracker thingy. For progress bars
//             and such. Named in honor of Rick Saenz.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __RATIOSTACK_H
#define __RATIOSTACK_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class RatioStack;
class RatioSample;

//////////////////////////////////////////////////////////////////////////////
// class RatioStack declaration

class RatioStack : public Singleton <RatioStack>
{
public:
	SET_NO_INHERITED( RatioStack );

// Setup.

	RatioStack( double maxValue = 1.0f );
	virtual ~RatioStack( void )  {  }

	void LockOut         ( bool lock = true )		{  m_LockOutCount += (lock ? 1 : -1);  gpassert( m_LockOutCount >= 0 );  }
	bool IsLockedOut     ( void ) const				{  return ( m_LockOutCount > 0 );  }
	void SetRefreshPeriod( DWORD msec )				{  m_RefreshPeriod = msec;  }

// Events (automatic).

	// registration of samples (done automatically)
	void RegisterSample  ( RatioSample* sample );
	void UnRegisterSample( RatioSample* sample );

	// registration of progress (done automatically)
	void Advance( const char* newStatus, bool force = false );

	// call this when you're all done to force 100% (if needed)
	void Finish( const char* newStatus );

// Special.

	class Locker
	{
	public:
		Locker( bool lock = true );
	   ~Locker( void );

	private:
		bool m_Lock;
	};

protected:

// Events.

	// this is called when the ratio/current status changes
	virtual void OnRatioChanged( double newValue, const char* newStatus ) = 0;

private:
	struct Level;

	Level* GetCurrentLevel( void );
	Level* GetNextLevel   ( void );
	void   NotifyChanged  ( bool force = false );

	struct Level
	{
		RatioSample* m_Sample;				// current sample for this level
		gpstring     m_CustomStatus;		// custom status to use at this level (overrides name)
		double       m_RatioRemaining;		// how much of the (normalized, 0.0-1.0) full ratio is remaining?

		Level( void );

		void SetNewSample( RatioSample* sample );
		void ResetSample ( RatioSample* sample );
		void GetRatios   ( double& used, double& next ) const;
	};

	typedef stdx::fast_vector <Level> LevelColl;

	DWORD     m_RefreshPeriod;				// period between updates in msec
	int       m_LockOutCount;				// ref-counted lockout
	LevelColl m_Levels;						// current progress
	double    m_MaxValue;					// max value we're working up to (generally width in pixels)
	DWORD     m_LastUpdate;					// last time we updated in ticks

	SET_NO_COPYING( RatioStack );
};

#define g_RatioStack RatioStack::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class RatioSample declaration

class RatioSample
{
public:
	SET_NO_INHERITED( RatioSample );

	// ctor/dtor
	RatioSample( const gpstring& name, double ratioMax = 0, double weight = 0 );
   ~RatioSample( void );

	// const query
	const gpstring& GetName  ( void ) const			{  return ( m_Name );  }
	double          GetWeight( void ) const			{  return ( m_Weight );  }

	// register fractional delta progressing towards max
	void Advance( double amount, const char* newStatus = "" );
	void Advance( const char* newStatus = "" )		{  Advance( 1.0, newStatus );  }

	// return 0.0 - 1.0 ratio representing progress
	double GetRatio( void ) const
	{
		return ( (m_RatioMax > 0) ? (m_Ratio / m_RatioMax) : 0 );
	}

private:
	bool     m_Registered;
	gpstring m_Name;
	double   m_Weight;
	double   m_Ratio;
	double   m_RatioMax;

	SET_NO_COPYING( RatioSample );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __RATIOSTACK_H

//////////////////////////////////////////////////////////////////////////////
