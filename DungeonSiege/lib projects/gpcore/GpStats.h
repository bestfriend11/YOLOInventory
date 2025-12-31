//////////////////////////////////////////////////////////////////////////////
//
// File     :  GpStats.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains stat-gathering support for an app.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GPSTATS_H
#define __GPSTATS_H

#include "GpStatsDefs.h"

#if GP_ENABLE_STATS

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"
#include "GpColl.h"
#include "KernelTool.h"

//////////////////////////////////////////////////////////////////////////////

// $ note: the time qwords below are the same units as FILETIME (.1 usec)

struct GpThreadStats
{
	HANDLE m_ThreadHandle;							// duplicated handle that we keep local
	DWORD  m_ThreadId;								// thread that owns this
	eGroup m_CurrentGroup;							// current group we're in
	double m_LastSystemTime;						// last system time we sampled, needed for delta calcs
	QWORD  m_LastThreadTime;						// last thread time we sampled, needed for proportion-of-global calcs
	float  m_TimeAccumulators[ GROUP_COUNT ];		// time accumulated so far for each group

	GpThreadStats( DWORD threadId = 0 );

	void InitTimings( void );
	void SetCurrentGroup( eGroup group, bool force = false );
	void UpdateCurrentGroup( void );
};

struct GpGroupStats
{
	float m_Time[ GROUP_COUNT ];					// time spent in each group in seconds
	float m_TotalTime;								// time spent in this entire frame (minus idle)
	float m_PrimaryThreadRatio;						// ratio of time spent in primary thread (this can be used to see multi-cpu boost)

	GpGroupStats( void )
	{
		Init();
	}

	void Init( void )
	{
		::ZeroObject( *this );
	}
};

//////////////////////////////////////////////////////////////////////////////
// struct GpCounter <BIGT, SMALLT, TYPE> declaration

enum eGpCounterType;

template <typename BIGT, typename SMALLT, enum eGpCounterType TYPE>
struct GpCounter
{
	enum  {  COUNTER_TYPE = TYPE  };

	int    m_Count;					// number of samples taken
	BIGT   m_Last;					// last value
	BIGT   m_Current;				// current value
	BIGT   m_Max;					// max it's ever been
	DWORD  m_LastSpikeCheck;		// last time we checked for spike
	SMALLT m_LastSpike;				// local maximum
	SMALLT m_CurrentSpike;			// local maximum
	SMALLT m_MaxSpike;				// maximum value the spike has been

	GpCounter( void )
	{
		::ZeroObject( *this );
	}

	void Add( BIGT value, DWORD holdMsec, DWORD tickCount )
	{
		++m_Count;
		Set( m_Current + value, holdMsec, tickCount );
	}

	void Sub( BIGT value, DWORD holdMsec, DWORD tickCount )
	{
		--m_Count;
		Set( m_Current - value, holdMsec, tickCount );
	}

	void Set( BIGT value, DWORD holdMsec, DWORD tickCount )
	{
		m_Last = m_Current;
		m_Current = value;
		::maximize( m_Max, m_Current );

		SMALLT delta = (SMALLT)(m_Current - m_Last);
		if ( ((tickCount - m_LastSpikeCheck) > holdMsec) || (delta > m_CurrentSpike) )
		{
			m_LastSpikeCheck = tickCount;
			m_LastSpike = m_CurrentSpike;
			m_CurrentSpike = delta;
			::maximize( m_MaxSpike, m_CurrentSpike );
		}
	}

	void Reset( void )
	{
		m_Count          = 0;
		m_Last           = 0;
		m_Current        = 0;
		m_Max            = 0;
		m_LastSpikeCheck = 0;
		m_LastSpike      = 0;
		m_CurrentSpike   = 0;
		m_MaxSpike       = 0;
	}

	void ResetExtras( void )
	{
		m_Max            = 0;
		m_LastSpikeCheck = 0;
		m_LastSpike      = 0;
		m_CurrentSpike   = 0;
		m_MaxSpike       = 0;
	}

	void ReportFlat( ReportSys::ContextRef ctx ) const
	{
		static gpstring s_Format;
		if ( s_Format.empty() )
		{
			const char* bigstr = NULL, * smallstr = NULL;

			eGpCounterType type = TYPE;
			if ( type == GPCOUNTER_INT )
			{
				bigstr = smallstr = "%d";
			}
			else if ( type == GPCOUNTER_INT64 )
			{
				bigstr = "%I64d";
				smallstr = "%d";
			}
			else if ( type == GPCOUNTER_FLOAT )
			{
				bigstr = smallstr = "%f";
			}
			else
			{
				gpassert( 0 );
			}

			s_Format.assignf( "%%d\t%s\t%s\t%s\t%s", bigstr, bigstr, smallstr, smallstr );
		}

		ReportSys::OutputF( ctx, s_Format, m_Count, m_Current, m_Max, m_CurrentSpike, m_MaxSpike );
	}
};

enum eGpCounterType
{
	GPCOUNTER_INT,
	GPCOUNTER_INT64,
	GPCOUNTER_FLOAT,
};

typedef GpCounter <int,   int  , GPCOUNTER_INT  > GpIntCounter;
typedef GpCounter <INT64, int  , GPCOUNTER_INT64> GpInt64Counter;
typedef GpCounter <float, float, GPCOUNTER_FLOAT> GpFloatCounter;

//////////////////////////////////////////////////////////////////////////////
// struct GpStats declaration

struct GpStats
{

// Timings.

	// value = seconds since last frame rendered
	GpFloatCounter m_FrameDelayTime;

	// value = seconds spent blocking in criticals since last frame
	GpFloatCounter m_CriticalBlockTime;

// Memory.

	// value = bytes, count = num operations
	GpIntCounter   m_MemCurrentAllocs;
	GpInt64Counter m_MemTotalAllocs;
	GpInt64Counter m_MemTotalDeallocs;
	GpInt64Counter m_MemTotalWaste;

// DirectX.

	// value = bytes, count = num textures
	GpIntCounter m_DxCurrentSurfaceAllocs;
	GpIntCounter m_DxTotalSurfaceAllocs;

// FileSys.

	// value = bytes, count = num maps
	GpIntCounter m_FileSysDiyReserves;

	// value = bytes, count = num pages
	GpIntCounter m_FileSysDiyCommits;

	// value = bytes, count = num read operations
	GpIntCounter m_FileSysDiskReads;

	// value = bytes decoded, count = num decode runs
	GpIntCounter m_FileSysDecompression;

// Go's.

	// value = count
	GpIntCounter m_GoGlobalAllocs;
	GpIntCounter m_GoCloneSrcAllocs;
	GpIntCounter m_GoLocalAllocs;
	GpIntCounter m_GoLodfiAllocs;
	GpIntCounter m_GoDeallocs;
};

//////////////////////////////////////////////////////////////////////////////
// class GpStatsMgr declaration

class GpStatsMgr : public OnDemandSingleton <GpStatsMgr>
{
public:
	SET_NO_INHERITED( GpStatsMgr );

	typedef kerneltool::RwCritical RwCritical;
	typedef std::list <GpThreadStats> ThreadStatColl;
	typedef stdx::fast_vector <GpGroupStats> GroupStatColl;

	// cache
	static const GpStats ms_SampleStats;					// this is just used for auto type/offset calcs

// Types.

	enum eOptions
	{
		// one of
		OPTION_ENABLED				= 1 << 0,				// currently collecting data? (defaults to false)
		OPTION_LOG_SAMPLE_HISTORY	= 1 << 1,				// log samples to memory? keep previous x frames
		OPTION_NO_CLEAR_PER_FRAME	= 1 << 2,				// don't clear grow-only counters every frame
		OPTION_DUMP_ON_STAGE_CHANGE = 1 << 3,				// dump the previous stage upon changing stages

		// OR one of
		OPTION_NONE					= 0,					// disable all options
	};

// Setup.

	// ctor/dtor
	GpStatsMgr( void );
   ~GpStatsMgr( void );

	// options
	void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	// query helpers
	void Enable   ( bool enable = true )					{  SetOptions( OPTION_ENABLED, enable );  }
    void Disable  ( void )									{  Enable( false );  }
	bool IsEnabled( void ) const							{  return ( TestOptions( OPTION_ENABLED ) );  }
	bool IsMemBusy( void ) const							{  return ( m_MemCritical.IsReadLocked() );  }

	// sampling
	void SetLogSampleHistory( bool enable, int frameCount );

// Sampling API.

	// stats output
	void Report( ReportSys::ContextRef ctx = NULL ) const;

	// context
	void         SetCurrentStage( eSystemStage stage );
	eSystemStage GetCurrentStage( void ) const				{  return ( m_CurrentStage );  }
	void         SetCurrentGroup( eGroup group );
	void         AddFrame       ( void );

	// query
	float                FillCpuStats        ( GpGroupStats* avgStatsPtr, ReportSys::Context* ctx = NULL );
	const GroupStatColl& GetPausedHistory    ( void )		{  return ( m_CpuStats.m_PausedHistory );  }
	const GroupStatColl& GetUnpausedHistory  ( void )		{  return ( m_CpuStats.m_UnpausedHistory );  }
	int                  GetCurrentFrameIndex( void ) const	{  return ( m_FrameIndex );  }

// Counter API.

	template <typename T, typename COUNTER>
	void AddToCounter( T value, const COUNTER& counter, eSystemProperty system )
	{
		RwCritical::WriteLock writeLock( m_CounterCritical );

		Stage& stage = m_StageColl[ m_CurrentStage ];
		stage.AddToCounter(
				value,
				m_SpikeHoldTimeMsec,
				m_FrameTick,
				(const BYTE*)&counter - (const BYTE*)&ms_SampleStats,
				(COUNTER*)&counter,
				system );
	}

	template <typename T, typename COUNTER>
	void SubFromCounter( T value, const COUNTER& counter, eSystemProperty system )
	{
		RwCritical::WriteLock writeLock( m_CounterCritical );

		Stage& stage = m_StageColl[ m_CurrentStage ];
		stage.SubFromCounter(
				value,
				m_SpikeHoldTimeMsec,
				m_FrameTick,
				(const BYTE*)&counter - (const BYTE*)&ms_SampleStats,
				(COUNTER*)&counter,
				system );
	}

	template <typename T, typename COUNTER>
	void SetCounter( T value, const COUNTER& counter, eSystemProperty system )
	{
		RwCritical::WriteLock writeLock( m_CounterCritical );

		Stage& stage = m_StageColl[ m_CurrentStage ];
		stage.SetCounter(
				value,
				m_SpikeHoldTimeMsec,
				m_FrameTick,
				(const BYTE*)&counter - (const BYTE*)&ms_SampleStats,
				(COUNTER*)&counter,
				system );
	}

	// types of tracking:
	//
	//   system + children
	//   system
	//   global (not necessary, can just add-all)
	//   samples for last xx frames/msec
	//   remember how much total time and frames spent in each stage
	//   sample for an entire period (in msec) a frame at a time, and then average the results for dump to log

	// types of options:
	//
	//   selectively disable profiling
	//   log to file every x msec
	//   attach options/traits to each stat

	// structure:
	//
	// STAGE
	//   -> SYSTEM
	//       -> STATENTRY
	//            -> COUNTER
	// $$$ single + single and children samples (keep separate)

	struct CpuStats
	{
		ThreadStatColl m_Stats;
		GroupStatColl  m_PausedHistory;
		GroupStatColl  m_UnpausedHistory;

	   ~CpuStats( void );

		void           InitLogSampleHistory( int count );
		GpThreadStats& GetStats( DWORD threadId = ::GetCurrentThreadId() );
		GroupStatColl& GetCurrentHistory( void );
	};

	typedef stdx::fast_vector <GpStats> StatColl;

	struct System
	{
		GpStats  m_Stats;
		StatColl m_History;
		bool     m_Touched;

		System( void )
		{
			m_Touched = false;
		}

		template <typename T, typename COUNTER>
		void AddToCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* )
		{
			m_Touched = true;
			COUNTER* counter = (COUNTER*)((BYTE*)&m_Stats + offset);
			counter->Add( value, holdMsec, tickCount );
		}

		template <typename T, typename COUNTER>
		void SubFromCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* )
		{
			m_Touched = true;
			COUNTER* counter = (COUNTER*)((BYTE*)&m_Stats + offset);
			counter->Sub( value, holdMsec, tickCount );
		}

		template <typename T, typename COUNTER>
		void SetCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* )
		{
			m_Touched = true;
			COUNTER* counter = (COUNTER*)((BYTE*)&m_Stats + offset);
			counter->Set( value, holdMsec, tickCount );
		}

		// handle memory allocs for sample history
		void InitLogSampleHistory( int count )
		{
			if ( count > 0 )
			{
				m_History.uninitialized_resize( count );
				std::fill( m_History.begin(), m_History.end(), m_Stats );
			}
			else
			{
				m_History.destroy();
			}
		}

		// commit current stats into history buffer at given index
		void CommitFrame( int frameIndex )
		{
			if ( m_Touched )
			{
				m_History[ frameIndex ] = m_Stats;
			}
		}
	};

	struct Stage : stdx::fast_vector <System>
	{
		Stage( void )
		{
			// prealloc all possible systems
			resize( GetZeroShift( SP_LAST ) );
		}

		template <typename T, typename COUNTER>
		void AddToCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* dummy, eSystemProperty system )
		{
			for ( eSystemProperty i = FindFirstEnum( system ) ; i != SP_NONE ; i = FindNextEnum( i, system ) )
			{
				(*this)[ GetZeroShift( i ) ].AddToCounter( value, holdMsec, tickCount, offset, dummy );
			}
		}

		template <typename T, typename COUNTER>
		void SubFromCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* dummy, eSystemProperty system )
		{
			for ( eSystemProperty i = FindFirstEnum( system ) ; i != SP_NONE ; i = FindNextEnum( i, system ) )
			{
				(*this)[ GetZeroShift( i ) ].SubFromCounter( value, holdMsec, tickCount, offset, dummy );
			}
		}

		template <typename T, typename COUNTER>
		void SetCounter( T value, DWORD holdMsec, DWORD tickCount, DWORD offset, COUNTER* dummy, eSystemProperty system )
		{
			for ( eSystemProperty i = FindFirstEnum( system ) ; i != SP_NONE ; i = FindNextEnum( i, system ) )
			{
				(*this)[ GetZeroShift( i ) ].SetCounter( value, holdMsec, tickCount, offset, dummy );
			}
		}
	};

	struct StageColl : stdx::fast_vector <Stage>
	{
		StageColl( void )
		{
			// prealloc all possible stages
			resize( STAGE_COUNT );
		}
	};

private:

	void initLogSampleHistory( void );

	// spec
	eOptions m_Options;							// current options for entire system
	int      m_LogHistoryFrames;				// frames for log history
	DWORD    m_SpikeHoldTimeMsec;				// hold time for detection of spikes

	// state
	RwCritical   m_MemCritical;					// currently doing something that may alloc memory - lock out mem notifies
	RwCritical   m_CounterCritical;				// general counter modification critical
	DWORD        m_FrameTick;					// begin of frame tick
	double       m_FrameTime;					// begin of frame time
	int          m_FrameIndex;					// frame index
	eSystemStage m_CurrentStage;				// stage we're in

	// samples
	CpuStats  m_CpuStats;						// all group data stored here
	StageColl m_StageColl;						// all data stored here

	// reporting
	my ReportSys::Context* m_DumpContext;		// reports go here

	SET_NO_COPYING( GpStatsMgr );
};

#define gGpStatsMgr GpStatsMgr::GetSingleton()

MAKE_ENUM_BIT_OPERATORS( GpStatsMgr::eOptions );

//////////////////////////////////////////////////////////////////////////////

#endif // GP_ENABLE_STATS

#endif  // __GPSTATS_H

//////////////////////////////////////////////////////////////////////////////
