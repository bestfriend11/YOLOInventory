//////////////////////////////////////////////////////////////////////////////
//
// File     :  Quantify.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"

#if GP_ENABLE_PROFILING

#include "Quantify.h"
#include "Quantify/Pure.h"

//////////////////////////////////////////////////////////////////////////////
// class QuantifyApi implementation

QuantifyApi :: QuantifyApi( void )
{
	m_Profiling    = false;
	m_FrameCount   = 0;
	m_SaveWhenDone = true;
}

bool QuantifyApi :: IsRunning( void )
{
	return ( !!QuantifyIsRunning() );
}

void QuantifyApi :: AddFrame( void )
{
	if ( m_Profiling && (m_FrameCount != -1) && (m_FrameCount-- == 0) )
	{
		StopProfile();
	}
}

void QuantifyApi :: BeginProfile( int frameCount, bool saveWhenDone )
{
	if ( !IsRunning() )
	{
		gpwarning( "Quantify isn't running!\n" );
	}

	m_Profiling = true;
	m_FrameCount = frameCount;
	m_SaveWhenDone = saveWhenDone;

	QuantifyClearData();
	QuantifyStartRecordingData();
}

void QuantifyApi :: StopProfile( void )
{
	if ( m_Profiling )
	{
		m_Profiling = false;
		QuantifyStopRecordingData();

		if ( m_SaveWhenDone )
		{
			QuantifySaveData();
			QuantifyStartRecordingData();
		}
	}
}

void QuantifyApi :: SnapshotProfileData( void )
{
	QuantifySaveData();
}

void QuantifyApi :: ClearProfileData( void )
{
	QuantifyClearData();
}

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_PROFILING

//////////////////////////////////////////////////////////////////////////////
