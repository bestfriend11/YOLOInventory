//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpLzo.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains wrappers for the lzo compression algorithm.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPLZO_H
#define __GPLZO_H

//////////////////////////////////////////////////////////////////////////////

#include "GPCore.h"

namespace lzo  {  // begin of namespace lzo

//////////////////////////////////////////////////////////////////////////////
// helper function declarations

// Performance.

#	if !GP_RETAIL

	struct PerfCounter
	{
		PerfCounter(void)  {  ::ZeroObject( *this );  }

		int    m_BytesDeflatedIn;
		int    m_BytesDeflatedOut;
		double m_SecondsDeflating;

		int    m_BytesInflatedIn;
		int    m_BytesInflatedOut;
		double m_SecondsInflating;
	};

	// for performance monitoring
	void               ResetPerfCounters( void );
	void               SetPerfCounters  ( const PerfCounter& counter );
	const PerfCounter& GetPerfCounters  ( void );

	// helper
	struct AutoPerf
	{
		bool   m_Deflating;
		double m_StartTime;

		AutoPerf( bool deflating );
	   ~AutoPerf( void );
	};

#	endif // !GP_RETAIL

// Algorithm API.

	// $ note that LZO does not do block compression, so you must have enough
	//   space allocated for the deflation/inflation buffer, else the algorithm
	//   will fail.

	// gets worst-case size for a call to Deflate() (assuming zero compression
	// ratio + overhead). if compressing an entire buffer at once, call this
	// first to get the output size to guarantee the call succeeds.
	int GetMaxDeflateSize( int inSize );

	// decompresses data all at once, returns -1 on failure, size of
	// uncompressed data otherwise. be sure to include 4 extra bytes for
	// overruns due to unrolled loop optimizations.
	int Inflate( mem_ptr out, const_mem_ptr in );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace lzo

#endif  // __GPLZO_H

//////////////////////////////////////////////////////////////////////////////
