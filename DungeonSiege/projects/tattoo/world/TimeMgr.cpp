//////////////////////////////////////////////////////////////////////////////
//
// File     :  TimeMgr.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "TimeMgr.h"

#include "SkritEngine.h"
#include "SkritStructure.h"
#include "RapiOwner.h"
#include "World.h"

//////////////////////////////////////////////////////////////////////////////
// class SkritImpulseTarget declaration and implementation

class SkritImpulseTarget : public TimeTarget
{
public:
	SET_INHERITED( SkritImpulseTarget, TimeTarget );

	SkritImpulseTarget( Skrit::HObject skrit, int funcIndex )
	{
		gpassert( !skrit.IsNull() );
		m_Skrit = skrit;
		gSkritEngine.AddRefObject( m_Skrit );

		m_FuncIndex = funcIndex;
	}

	virtual ~SkritImpulseTarget( void )
		{  gSkritEngine.ReleaseObject( m_Skrit );  }
	virtual void OnExecute( float deltaTime )
		{  OnExecute( m_Skrit, m_FuncIndex, scast <float> ( deltaTime ) );  }

	static SKRIT_IMPORT void OnExecute( Skrit::HObject skrit, int funcIndex, float /*deltaTime*/ );

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const
		{  return ( "SkritImpulseTarget" );  }
	virtual gpstring OnGetDebugName( void ) const
		{  return ( gpstringf( "%s:%s",
										m_Skrit->GetImpl()->GetName().c_str(),
										m_Skrit->GetImpl()->GetFunctionSpec( m_FuncIndex )->m_Name ) );  }
#	endif // GP_DEBUG

private:
	Skrit::HObject m_Skrit;
	int            m_FuncIndex;

	SET_NO_COPYING( SkritImpulseTarget );
};

void SkritImpulseTarget :: OnExecute( Skrit::HObject skrit, int funcIndex, float /*deltaTime*/ )
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_CALLV( OnExecute, skrit, funcIndex );  }

//////////////////////////////////////////////////////////////////////////////
// class SkritTimeTarget declaration

DECLARE_EVENT
{
	SKRIT_IMPORT void OnTimeExecute( Skrit::HObject skrit, float /*deltaTime*/, TimeTarget* /*timeTarget*/ )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnTimeExecute, skrit );  }
	SKRIT_IMPORT void OnTimeUpdate( Skrit::HObject skrit, float /*deltaTime*/, TimeTarget* /*timeTarget*/ )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnTimeUpdate, skrit );  }
	SKRIT_IMPORT void OnTimeLastUpdate( Skrit::HObject skrit, float /*deltaTime*/, TimeTarget* /*timeTarget*/ )
		{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnTimeLastUpdate, skrit );  }
}

class SkritTimeTarget : public TimeTarget
{
public:
	SET_INHERITED( SkritTimeTarget, TimeTarget );
	
	SkritTimeTarget( Skrit::HObject skrit )
	{
		gpassert( !skrit.IsNull() );
		m_Skrit = skrit;
		gSkritEngine.AddRefObject( m_Skrit );
	}

	virtual ~SkritTimeTarget( void )
		{  gSkritEngine.ReleaseObject( m_Skrit );  }
	virtual void OnExecute( float deltaTime )
		{  FuBiEvent::OnTimeExecute( m_Skrit, (float)deltaTime, this );  }
	virtual void OnUpdate( float deltaTime )
		{  FuBiEvent::OnTimeUpdate( m_Skrit, (float)deltaTime, this );  }
	virtual void OnLastUpdate( float deltaTime )
		{  FuBiEvent::OnTimeLastUpdate( m_Skrit, (float)deltaTime, this );  }

#	if GP_DEBUG
	virtual gpstring OnGetDebugType( void ) const
		{  return ( "SkritTimeTarget" );  }
	virtual gpstring OnGetDebugName( void ) const
		{  return ( m_Skrit->GetImpl()->GetName() );  }
#	endif // GP_DEBUG

private:
	Skrit::HObject m_Skrit;

	SET_NO_COPYING( SkritTimeTarget );
};


//////////////////////////////////////////////////////////////////////////////
// class RapiFade declaration

class RapiFade : public TimeTarget
{
public:
	RapiFade( float duration, bool bFadeIn )
	{
		m_bFadeIn		= bFadeIn;
		m_CurrentTime	= 0;

		SetDuration( duration );
		OnUpdate( 0 );
	}

	void OnUpdate( float deltaTime )
	{
		m_CurrentTime += deltaTime;
		float norm_time = m_CurrentTime / GetDuration();

		if( m_bFadeIn )
		{
			gDefaultRapi.SetFade( 1.0f - norm_time );
		}
		else
		{
			gDefaultRapi.SetFade( norm_time );
		}
	}

	void OnLastUpdate( float deltaTime )
	{
		UNREFERENCED_PARAMETER( deltaTime );

		if( m_bFadeIn )
		{
			gDefaultRapi.SetFade( 0.0f );
		}
		else
		{
			gDefaultRapi.SetFade( 1.0f );
		}
	}

private:
	float	m_CurrentTime;
	bool	m_bFadeIn;
};

//////////////////////////////////////////////////////////////////////////////
// class TimeMgr implementation

void TimeMgr :: AddRapiFader( bool bFadeIn, const float duration )
{
	m_SysTimecaster.AddTarget( new RapiFade( duration, bFadeIn ), true );
}

//////////////////////////////////////////////////////////////////////////////
// helper implementations

void AddSkritCallback( Timecaster* caster, float timeOffset, Skrit::HObject skrit, int funcIndex )
{
	SkritImpulseTarget* target = new SkritImpulseTarget( skrit, funcIndex );
	target->SetTimeOffset( timeOffset );
	caster->AddTarget( target, true );
}

float AddSkritCallbacks( Timecaster* caster, float timeOffset, Skrit::HObject skrit )
{
	// return the offset of the last callback

	float maxOffset = timeOffset;
	for ( int i = 0, count = skrit->GetImpl()->GetFunctionCount() ; i != count ; ++i )
	{
		const Skrit::Raw::ExportEntry* export = skrit->GetImpl()->GetFunctionSpec( i );
		if ( export->m_AtTime != (DWORD)Skrit::TIME_NONE )
		{
			float offset = timeOffset + export->GetTimeInSeconds();
			maximize( maxOffset, offset );
			AddSkritCallback( caster, offset, skrit, i );
		}
	}
	return ( maxOffset );
}

//////////////////////////////////////////////////////////////////////////////
