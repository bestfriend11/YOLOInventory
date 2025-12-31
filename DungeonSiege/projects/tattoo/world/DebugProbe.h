#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	DebugProbe.h

	Author(s)	: 	Bartosz Kijanka

	Purpose		: 	One place for all the probes.  Sprinkling probe functionality
					throughout all the code has become tedious to maintain, so
					I'm going to try to keep it all in one place.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef __DEBUGPROBE_H
#define __DEBUGPROBE_H


#if !GP_RETAIL


#include "gpcore.h"


enum eDebugProbe
{
	DP_NONE,
	DP_MIND,		// normal mind probe
	DP_MIND2,		// terse mind probe... just the jobs, times, goals etc
	DP_BODY,		// up the ass
	DP_ASPECT,		// 
	DP_NET,			// 
	DP_NET2,		//
	DP_COMBAT,		//
};


/*===========================================================================

	Class		: DebugProbe

	Author(s)	: Bartosz Kijanka

---------------------------------------------------------------------------*/
class DebugProbe : public Singleton < DebugProbe >
{
public:

	DebugProbe();

	~DebugProbe(){};

	
	bool IsProbing();

	FUBI_EXPORT eDebugProbe GetMode() { return m_Mode; }

	FUBI_EXPORT void SetMode( eDebugProbe mode );

	FUBI_EXPORT bool GetVerbose()				{ return m_bVerbose; }
	FUBI_EXPORT void SetVerbose( bool verbose )	{ m_bVerbose = verbose; }

	FUBI_EXPORT void ToggleMode( eDebugProbe mode );

	FUBI_EXPORT void SetProbeObject( Goid object );

	FUBI_EXPORT void SetProbeObject( Scid object );

	void GetProbeOutput( gpstring & out );


private:

	void GetMind( gpstring & out );
	void GetMind2( gpstring & out );
	void GetBody( gpstring & out );
	void GetAspect( gpstring & out );
FEX void GetNet( gpstring & out );
	void GetNet2( gpstring & out );
	void GetMCP( gpstring & out );
	void GetCombat( gpstring & out );

	Go * GetProbedGo();

	void GetBanner( gpstring const & name, gpstring & out );

	//----- data

	eDebugProbe m_Mode;

	Goid m_ProbeGoid;
	Scid m_ProbeScid;

	bool m_bVerbose;
	bool m_fReset;

	FUBI_SINGLETON_CLASS( DebugProbe, "Class for managing debug probes." );
};

#define gDebugProbe DebugProbe::GetSingleton()


#endif // !GP_RETAIL


#endif  // __PROBE_H
