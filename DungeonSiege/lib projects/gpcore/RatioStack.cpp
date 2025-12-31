//////////////////////////////////////////////////////////////////////////////
//
// File     :  RatioStack.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "RatioStack.h"

#include "GpMath.h"

//////////////////////////////////////////////////////////////////////////////
// class RatioStack implementation

RatioStack :: RatioStack( double maxValue )
{
	// init members
	m_RefreshPeriod = 50;			// 20hz by default
	m_LockOutCount  = 0;
	m_MaxValue      = maxValue;
	m_LastUpdate    = 0;

	// always need a top level
	m_Levels.push_back();
}

void RatioStack :: RegisterSample( RatioSample* sample )
{
	GetNextLevel()->SetNewSample( sample );

	// if this asserts, then there is an issue with weight balancing
	gpassert( GetCurrentLevel()->m_RatioRemaining > 0 );

	NotifyChanged( true );
}

void RatioStack :: UnRegisterSample( RatioSample* sample )
{
	// reset the sample
	Level* level = GetCurrentLevel();
	level->ResetSample( sample );

	// now see if we're two-off
	if ( level != &m_Levels.back() )
	{
		gpassert( level == &*(m_Levels.end() - 2) );
		m_Levels.pop_back();
	}
}

void RatioStack :: Advance( const char* newStatus, bool force )
{
	Level* level = GetCurrentLevel();
	if ( level != NULL )
	{
		level->m_CustomStatus = newStatus ? newStatus : "";
	}
	NotifyChanged( force );
}

void RatioStack :: Finish( const char* newStatus )
{
	gpassert( GetCurrentLevel() == NULL );
	m_Levels.front().m_RatioRemaining = 0;

	Advance( newStatus, true );
}

RatioStack::Level* RatioStack :: GetCurrentLevel( void )
{
	Level* level = &m_Levels.back();
	if ( level->m_Sample == NULL )
	{
		if ( m_Levels.size() >= 2 )
		{
			--level;
		}
		else
		{
			return ( NULL );
		}
	}

	gpassert( level->m_Sample != NULL );
	return ( level );
}

RatioStack::Level* RatioStack :: GetNextLevel( void )
{
	Level* level = &m_Levels.back();
	if ( level->m_Sample != NULL )
	{
		level = &*m_Levels.push_back();
	}
	return ( level );
}

void RatioStack :: NotifyChanged( bool force )
{
	// never update more frequently than 20hz unless forced
	DWORD ticks = ::GetTickCount();
	if ( !force && ((ticks - m_LastUpdate) < m_RefreshPeriod) )
	{
		return;
	}
	m_LastUpdate = ticks;

	// status
	gpstring status, newStatus;

	// calc the ratio we need
	double ratio = 0, total = 1;
	int indent = 0;
	LevelColl::const_iterator i, ibegin = m_Levels.begin(), iend = m_Levels.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// grab out level
		const Level* level = &*i;

		// figure out how much is done
		double localUsed, localNext;
		level->GetRatios( localUsed, localNext );

		// update our runners
		ratio += localUsed * total;
		total = (localNext - localUsed) * total;

		// update status
#		if !GP_RETAIL
		newStatus.clear();
		if ( level->m_Sample != NULL )
		{
			newStatus = level->m_Sample->GetName();
		}
		if ( !level->m_CustomStatus.empty() )
		{
			if ( !newStatus.empty() )
			{
				newStatus += ": ";
			}
			newStatus += level->m_CustomStatus;
		}

		// print out if anything there
		if ( !newStatus.empty() )
		{
			if ( indent != 0 )
			{
				status += '\n';
			}

			status.append( indent * 3, ' ' );
			status += newStatus;

			++indent;
		}
#		endif // !GP_RETAIL
	}

	// notify callback
	OnRatioChanged( ratio * m_MaxValue, status );
}

//////////////////////////////////////////////////////////////////////////////
// class RatioStack::Locker implementation

RatioStack::Locker :: Locker( bool lock )
{
	m_Lock = lock;
	if ( m_Lock )
	{
		g_RatioStack.LockOut( true );
	}
}

RatioStack::Locker :: ~Locker( void )
{
	if ( m_Lock )
	{
		g_RatioStack.LockOut( false );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class RatioStack::Level implementation

RatioStack::Level :: Level( void )
{
	m_Sample = NULL;
	m_RatioRemaining = 1.0;
}

void RatioStack::Level :: SetNewSample( RatioSample* sample )
{
	m_Sample = sample;
	m_CustomStatus.clear();
}

void RatioStack::Level :: ResetSample( RatioSample* sample )
{
	gpassert( m_Sample == sample );
	m_Sample = NULL;

	double weight = sample->GetWeight();
	if ( weight == 0 )
	{
		m_RatioRemaining = 0;
	}
	else
	{
		m_RatioRemaining -= weight;
	}
	gpassert( m_RatioRemaining > -FLOAT_TOLERANCE );
}

void RatioStack::Level :: GetRatios( double& used, double& next ) const
{
	used = 1 - m_RatioRemaining;

	double weight = m_Sample ? m_Sample->GetWeight() : 0;
	if ( weight == 0 )
	{
		next = 1;
	}
	else
	{
		next = used + weight;
	}

	if ( m_Sample != NULL )
	{
		used += (next - used) * m_Sample->GetRatio();
	}
}

//////////////////////////////////////////////////////////////////////////////
// class RatioSample implementation

RatioSample :: RatioSample( const gpstring& name, double ratioMax, double weight )
	: m_Name( name )
{
	gpassert( ratioMax >= 0 );

	m_Weight     = weight;
	m_Ratio      = 0;
	m_RatioMax   = ratioMax;
	m_Registered = RatioStack::DoesSingletonExist() && !g_RatioStack.IsLockedOut();

	if ( m_Registered )
	{
		g_RatioStack.RegisterSample( this );
	}
}

RatioSample :: ~RatioSample( void )
{
	if ( m_Registered )
	{
		g_RatioStack.UnRegisterSample( this );
	}
}

void RatioSample :: Advance( double amount, const char* newStatus )
{
	gpassert( amount >= 0 );

	m_Ratio += amount;
	::clamp_max( m_Ratio, m_RatioMax );

	if ( m_Registered )
	{
		g_RatioStack.Advance( newStatus );
	}
}

//////////////////////////////////////////////////////////////////////////////
