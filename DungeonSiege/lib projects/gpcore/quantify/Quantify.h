//////////////////////////////////////////////////////////////////////////////
//
// File     :  Quantify.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains wrappers for Quantify profiling fun.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __QUANTIFY_H
#define __QUANTIFY_H

#if GP_ENABLE_PROFILING

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"

//////////////////////////////////////////////////////////////////////////////
// class QuantifyApi declaration

class QuantifyApi : public AutoSingleton <QuantifyApi>
{
public:
	SET_NO_INHERITED( QuantifyApi );

	QuantifyApi( void );
   ~QuantifyApi( void )			{  }

	// running at all?
	bool IsRunning( void );

	// called by main loop
	void AddFrame( void );

	// called by UI functions (set frameCount == -1 for infinite)
FEX	void BeginProfile       ( int frameCount = 1, bool saveWhenDone = true );
FEX	void StopProfile        ( void );
FEX	void SnapshotProfileData( void );
FEX	void ClearProfileData   ( void );

	// meant for a function binder
FEX	bool Profile1  ( void )		{  BeginProfile( 1,   true );  return ( true );  }
FEX	bool Profile10 ( void )		{  BeginProfile( 10,  true );  return ( true );  }
FEX	bool Profile50 ( void )		{  BeginProfile( 50,  true );  return ( true );  }
FEX	bool Profile100( void )		{  BeginProfile( 100, true );  return ( true );  }

private:
	bool m_Profiling;
	int  m_FrameCount;
	bool m_SaveWhenDone;

	FUBI_SINGLETON_CLASS( QuantifyApi, "Interface for Quantify." );
	SET_NO_COPYING( QuantifyApi );
};

#define gQuantifyApi QuantifyApi::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_PROFILING

#endif  // __QUANTIFY_H

//////////////////////////////////////////////////////////////////////////////
