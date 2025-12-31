//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpLzo.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "GpLzo.h"

#include "extern\lzo-free\lzo_free.h"

namespace lzo  {  // begin of namespace lzo

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

#if !GP_RETAIL

static PerfCounter s_PerfCounter;

void ResetPerfCounters( void )
{
	s_PerfCounter = PerfCounter();
}

void SetPerfCounters( const PerfCounter& counter )
{
	s_PerfCounter = counter;
}

const PerfCounter& GetPerfCounters( void )
{
	return ( s_PerfCounter );
}

AutoPerf :: AutoPerf( bool deflating )
{
	m_Deflating = deflating;
	m_StartTime = ::GetSystemSeconds();
}

AutoPerf :: ~AutoPerf( void )
{
	if ( m_Deflating )
	{
		s_PerfCounter.m_SecondsDeflating += ::GetSystemSeconds() - m_StartTime;
	}
	else
	{
		s_PerfCounter.m_SecondsInflating += ::GetSystemSeconds() - m_StartTime;
	}
}

#endif // !GP_RETAIL

int GetMaxDeflateSize( int inSize )
{
	// $ from lzo.faq

	return ( inSize + (inSize / 64) + 16 + 3 );
}

int Inflate( mem_ptr out, const_mem_ptr in )
{
	GPDEV_ONLY( AutoPerf autoPerf( false ) );

	if ( in.size == 0 )
	{
		return ( 0 );
	}

	if ( ::FreeDecompress_LZO1X_999(
			(const BYTE*)in.mem, in.size,
			(BYTE*)out.mem, &out.size ) >= 0 )
	{
#		if !GP_RETAIL
		s_PerfCounter.m_BytesInflatedIn += in.size;
		s_PerfCounter.m_BytesInflatedOut += out.size;
#		endif // !GP_RETAIL

		return ( out.size );
	}
	else
	{
		return ( -1 );
	}
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace zlib

//////////////////////////////////////////////////////////////////////////////
