#pragma once

#ifndef __GPPROFILER_H
#define __GPPROFILER_H


#include "GpStatsDefs.h"
#include "FuBiDefs.h"

#include <vector>


namespace ReportSys
{
	class TextTableSink;
	class LocalContext;
}



#if !GP_RETAIL

#define GPPROFILERSAMPLE( name, flags ) 																\
	_declspec( thread ) static int GPPROFILERSAMPLE_sample = 0; 										\
	if ( (GPPROFILERSAMPLE_sample == 0 ) && gpprofiler::DoesSingletonExist() && gProfiler.IsActive() ) 	\
	{ 																									\
		GPPROFILERSAMPLE_sample = gProfiler.AddSample( name, flags ); 	  								\
	} 																									\
	gpprofiler_timer GPPROFILERSAMPLE_timer( GPPROFILERSAMPLE_sample );									\
	GPSTATS_SYSTEM( flags )

#else

#define GPPROFILERSAMPLE( name, flags )  GPSTATS_SYSTEM( flags );

#endif

//
// Define this if you want to watch the proiler give feedback on each sample taken - in the debug output window
//

//#define DEBUG_PROFILER 1


#if !GP_RETAIL


/*=======================================================================================

  gpprofiler_sample

  purpose:	This is an individual sample which is used in profiling.  This is what the code
			gets instrumented with.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class gpprofiler_sample
{
public:

	// existence

	inline gpprofiler_sample( const char * name, eSystemProperty prop )
	{
		ResetBase();
		ResetMinMax();
		mName		= name;
		mProperty	= prop;
	}

	inline ~gpprofiler_sample(){}

	// the name of this sample
	inline const char * GetName() const			{ return( mName ); }

	inline eSystemProperty GetProperty() const	{ return mProperty; }

	////////////////////////////////////////
	//	query times

	inline unsigned int GetCount() const
	{
		return( mCount );			// how many times this sample was taken during sampling pass
	}

	inline __int64 GetDuration() const
	{
		return( mDuration );		// the running sum duration logged in by this sample during current profiler pass, this takes into account the sample being taken multiple times in the current pass
	}

	inline __int64 GetMaxDuration() const
	{
		return( mMaxDuration );
	}

	inline __int64 GetDurationWithoutChildren() const
	{
		return( mDuration - mChildDuration );
	}

	inline __int64 GetMaxDurationWithoutChildren() const
	{
		return( mMaxDuration - mMaxChildDuration );
	}

	////////////////////////////////////////
	//	query allocs

	inline int GetAllocs() const
	{
		return( mAllocs );
	}

	inline int GetAllocsWithoutChildren() const
	{
		gpassert( mAllocs >= mChildAllocs );
		return( mAllocs - mChildAllocs );
	}

	inline int GetMaxAllocsWithoutChildren() const
	{
		gpassert( mMaxAllocs >= mMaxChildAllocs );
		return( mMaxAllocs - mMaxChildAllocs );
	}

	// other:

	// internal use only - get sample duration for last sample instance, NOT the running total
	inline __int64 GetDurationThisPass() const
	{
		if( mEndTime == 0 )
		{
			return 0;
		}
		else
		{
			gpassert( mEndTime >= mBeginTime );
			return( mEndTime - mBeginTime );
		}
	}

	inline int GetAllocsThisPass() const
	{
		if( mEndAllocId == 0 )
		{
			return 0;
		}
		else
		{
			gpassert( mEndAllocId >= mBeginAllocId );
			return( mEndAllocId - mBeginAllocId );
		}
	}

	//----- write

	inline void ResetBase()
	{
		mDepth         = 0;
		mCount         = 0;

		mBeginTime     = 0;
		mEndTime       = 0;

		mDuration      = 0;
		mChildDuration = 0;

		mBeginAllocId  = 0;
		mEndAllocId	   = 0;

		mAllocs		   = 0;
		mChildAllocs   = 0;
	}

	inline void ResetMinMax()
	{
		mMaxDuration      = 0;
		mMaxChildDuration = 0;

		mMaxAllocs        = 0;
		mMaxChildAllocs   = 0;
	}

	// accumulate sample durations for children
	inline void AddToChildDuration( __int64 duration )
	{
		mChildDuration += duration;
		mMaxChildDuration = max( mChildDuration, mMaxChildDuration );
	}

	inline void AddToChildAllocs( DWORD allocs )
	{
		mChildAllocs += allocs;
		mMaxChildAllocs = max( mChildAllocs, mMaxChildAllocs );
	}

	static void SetMaxDepth( int depth ) { mMaxDepth = depth; }		// sets global static resource pointer

	// tell sample to begin timing
	inline void Begin()
	{
		if( mDepth==0 )
		{
			QueryPerformanceCounter( (LARGE_INTEGER*)&mBeginTime );
			mEndTime       = 0;

#			if GP_DEBUG && !GP_SIEGEMAX
			mBeginAllocId = gpmem::GetLastSerialID();
#			else // GP_DEBUG
			mBeginAllocId = 0;
#			endif // GP_DEBUG

			mEndAllocId	   = 0;

			#ifdef DEBUG_PROFILER
			gpdebuggerf(( "%d - sample beg(%s), children=%I64d\n", mGlobalScope, mName, mChildDuration ));
			#endif
		}

		++mCount;
		++mDepth;

		#ifdef DEBUG_PROFILER
		++mGlobalScope;
		#endif
	}

	// tell sample to end timing
	inline void End()
	{
		gpassert( mDepth > 0 );
		//gpassert( mGlobalScope > 0 );
		//--mGlobalScope;

		--mDepth;

		if( mDepth==0 )
		{
			////////////////////////////////////////
			//	measure time

			QueryPerformanceCounter( (LARGE_INTEGER*)&mEndTime );
			mDuration += ( mEndTime - mBeginTime );
			gpassert( mEndTime >= mBeginTime );
			gpassert( mDuration >= mChildDuration );

			mMaxDuration = max( mDuration, mMaxDuration );

			////////////////////////////////////////
			//	measure allocs

#			if GP_DEBUG && !GP_SIEGEMAX
			mEndAllocId = gpmem::GetLastSerialID();
#			else // GP_DEBUG
			mEndAllocId = 0;
#			endif // GP_DEBUG
			gpassert( mEndAllocId >= mBeginAllocId );

			mAllocs		+= mEndAllocId - mBeginAllocId;
			mMaxAllocs	= max( mAllocs, mMaxAllocs );

			gpassert( mAllocs >= mChildAllocs );
		}

		#ifdef	DEBUG_PROFILER
		gpdebuggerf(( "%d - sample end(%s), me=%I64d childen=%I64d\n", mGlobalScope, mName, mDuration, mChildDuration ));
		#endif
	}


private:

	const char * mName;
	eSystemProperty mProperty;
	static int mMaxDepth;

	// debug
#	ifdef DEBUG_PROFILER
	static unsigned int mGlobalScope;
#	endif

	unsigned int mDepth;
	unsigned int mCount;

	////////////////////////////////////////
	//	time

	__int64 mBeginTime;
	__int64 mEndTime;

	__int64 mDuration;
	__int64 mMaxDuration;

	__int64 mChildDuration;
	__int64 mMaxChildDuration;

	////////////////////////////////////////
	//	allocs

	DWORD	mBeginAllocId;
	DWORD	mEndAllocId;

	int mAllocs;
	int mMaxAllocs;

	int mChildAllocs;
	int mMaxChildAllocs;
};


/*=======================================================================================

  gpprofiler

  purpose:	Master class for doing profiling.  All profiler samples report to this class, so
			this is the class you ask for profile info.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class gpprofiler : public ThreadedSingleton <gpprofiler>
{
public:

	gpprofiler();
	~gpprofiler();

	void SetIsActive( bool flag ) 	{ m_RequestedIsActive = flag; }
	inline bool IsActive()			{ return( m_IsActive ); }

	void SetFirstDisplaySample( unsigned int n );				// when sample output summary is displayed, begin displaying with this sample
  	void SetDisplaySamples( unsigned int n );					// diplay this many samples after above sample
  	void SetFixedScopeTime( unsigned int fps );					// set the scope time to always be equivalent to 1/fps

  	void Begin();												// begin profiling scope

  	void End();													// end profiling scope and don't ask for report
  	void End( ReportSys::ContextRef ctx = NULL );				// end profiling scope and ask for summary report

	int AddSample( const char* name, eSystemProperty prop );	// add sample to list

FEX void Show( const char * properties );
FEX void Hide( const char * properties );
FEX void ShowOnly( const char * properties );

	// look up a sample based on index
	gpprofiler_sample& GetSample( int index )  {  return ( mSamples[ index ] );  }


private:

	bool m_RequestedIsActive;
	bool m_IsActive;

	DWORD mResetMaxCountDown;
	DWORD mFPSTimeLock;

	__int64 mFixedScopeTime;

	DWORD mFirstDisplaySample;
	DWORD mDisplaySample;

	eSystemProperty mShowSampleProperties;

	typedef std::vector<gpprofiler_sample> SampleColl;
	SampleColl mSamples;

	ReportSys::TextTableSink* mSink;
	ReportSys::LocalContext* mContext;

	FUBI_SINGLETON_CLASS( gpprofiler, "Cute little internal profiler." );
};

#define gProfiler gpprofiler::GetSingleton()



/*=======================================================================================

  gpprofiler_timer

  purpose:	This wraps a sample, and provides it the timing information it needs.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class gpprofiler_timer
{
	int mParentSample;
	DECL_THREAD static int sCurrentSample;

public:

	inline gpprofiler_timer( int sampleIndex )
	{
		if( gpprofiler::DoesSingletonExist() && gProfiler.IsActive() )
		{
			mParentSample	= sCurrentSample;
			sCurrentSample	= sampleIndex;
			gProfiler.GetSample( sampleIndex ).Begin();
		}
	}

    inline ~gpprofiler_timer()
	{
		if( gpprofiler::DoesSingletonExist() && gProfiler.IsActive() )
		{
			gpprofiler_sample & sample = gProfiler.GetSample( sCurrentSample );
			sample.End();

			if( mParentSample )
			{
				gProfiler.GetSample( mParentSample ).AddToChildDuration( sample.GetDurationThisPass() );
				gProfiler.GetSample( mParentSample ).AddToChildAllocs( sample.GetAllocsThisPass() );
			}

			sCurrentSample = mParentSample;
		}
	}
};

#endif // !GP_RETAIL




#endif
