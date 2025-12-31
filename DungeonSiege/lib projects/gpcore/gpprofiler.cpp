#include "precomp_gpcore.h"

#if !GP_RETAIL
	

#include "gpprofiler.h"
#include "ReportSys.h"
#include "FubiSchema.h"
#include "stringtool.h"


FUBI_REPLACE_NAME( "gpprofiler", Profiler );


////////////////////////////////////////////////////////////////////////////////
//	

int gpprofiler_sample::mMaxDepth				= 0;
int gpprofiler_timer::sCurrentSample			= 0;

#ifdef DEBUG_PROFILER
unsigned int gpprofiler_sample::mGlobalScope	= 0;
#endif




gpprofiler::gpprofiler()
{
	m_RequestedIsActive		= false;
	m_IsActive				= false;

	mFPSTimeLock        = 30;   
	mFirstDisplaySample = 0;    
	mDisplaySample      = 20;   
	mResetMaxCountDown  = 180;  

	mShowSampleProperties = SP_ALL;

	if( mFPSTimeLock )
	{
		__int64 begin, end;
		QueryPerformanceCounter( (LARGE_INTEGER*)&begin );
		unsigned int pause_milliseconds = 1000/mFPSTimeLock;
		Sleep( pause_milliseconds );
		QueryPerformanceCounter( (LARGE_INTEGER*)&end );
		mFixedScopeTime = end-begin;
	}

	mSink = new ReportSys::TextTableSink;
	mContext = new ReportSys::LocalContext( mSink, true );

#	if GP_DEBUG
	mContext->SetSchema( new ReportSys::Schema( 8, "num", "cpu", "cpu_c", "m_cpu", "m_cpu_c", "alc", "m_alc", "name" ), true );
#	else
	mContext->SetSchema( new ReportSys::Schema( 6, "num", "cpu", "cpu_c", "m_cpu", "m_cpu_c", "name" ), true );
#	endif
}


gpprofiler::~gpprofiler()
{
	delete ( mContext  );
}


void gpprofiler::Begin()
{
	// reset all the samples
	SampleColl::iterator i, begin = mSamples.begin(), end = mSamples.end();
	for( i = begin ; i != end ; ++i )
	{
		i->ResetBase();
	}
}


void gpprofiler::End()
{
	m_IsActive = m_RequestedIsActive;
}


int gpprofiler::AddSample( const char* name, eSystemProperty prop )
{
	int size = (int)mSamples.size();
	mSamples.push_back( gpprofiler_sample( name, prop ) );
	return ( size );
}


void gpprofiler::Show( const char * properties )
{
	eSystemProperty eProp = SP_NONE;
	if( ::FromFullString( properties, eProp ) )
	{
		mShowSampleProperties |= eProp;
	}
}


void gpprofiler::Hide( const char * properties )
{
	eSystemProperty eProp = SP_NONE;
	if( ::FromFullString( properties, eProp ) )
	{
		mShowSampleProperties = mShowSampleProperties & NOT( eProp );
	}
}


void gpprofiler::ShowOnly( const char * properties )
{
	eSystemProperty eProp = SP_NONE;
	if( ::FromFullString( properties, eProp ) )
	{
		mShowSampleProperties = eProp;
	}
}


void gpprofiler::End( ReportSys::ContextRef _ctx )
{
	if( !mSamples.empty() )
	{
		__int64 timetakenthispass = 0;

		if( mFPSTimeLock )
		{
			timetakenthispass = mFixedScopeTime;
		}
		else
		{
			SampleColl::const_iterator i, begin = mSamples.begin(), end = mSamples.end();
			for( i = begin ; i != end ; ++i )
			{
				timetakenthispass += i->GetDurationWithoutChildren();
			}
		}

		float audit_total = 0.0;

		DWORD samplesReported = 0;

		ReportSys::AutoTable autoTable( mContext );

		{for( unsigned int i=1; i<mSamples.size(); ++i )
		{
			float pct_cpu		= (float)(float(100.0) * double(mSamples[i].GetDurationWithoutChildren())/double(timetakenthispass));
			float pct_cpu_c		= (float)(float(100.0) * double(mSamples[i].GetDuration())/double(timetakenthispass));
			float pct_cpu_max	= (float)(float(100.0) * double(mSamples[i].GetMaxDurationWithoutChildren())/double(timetakenthispass));
			float pct_cpu_max_c	= (float)(float(100.0) * double(mSamples[i].GetMaxDuration())/double(timetakenthispass));

			if( ( mSamples[i].GetProperty() & mShowSampleProperties ) !=  0 )
			{
				ReportSys::AutoReport autoReport( mContext );

				mContext->OutputFieldF( "%d", mSamples[i].GetCount() );
				mContext->OutputFieldF( "%3.2f", pct_cpu );
				mContext->OutputFieldF( "%3.2f", pct_cpu_c );
				mContext->OutputFieldF( "%3.2f", pct_cpu_max );
				mContext->OutputFieldF( "%3.2f", pct_cpu_max_c );

#				if GP_DEBUG
				mContext->OutputFieldF( "%d", mSamples[i].GetAllocsWithoutChildren() );
				mContext->OutputFieldF( "%d", mSamples[i].GetMaxAllocsWithoutChildren() );
#				endif
				mContext->OutputFieldF( "%s", mSamples[i].GetName() );

				++samplesReported;
			}

			audit_total += pct_cpu;
		}}

		if( samplesReported )
		{
#			if GP_DEBUG
			mSink->Sort( 7, true );
#			else
			mSink->Sort( 5, true );
#			endif
		}

		_ctx->Output( "\nGPPROFILER:\n" );

		mSink->OutputReport( _ctx );
		mSink->Clear();

		_ctx->Output(	"===========================================\n" );
		_ctx->OutputF(	"audit total = %03.3f of a %d fps profile\n\n", audit_total, mFPSTimeLock );

		--mResetMaxCountDown;

		if( !mResetMaxCountDown )
		{
			mResetMaxCountDown = 180;
			for( unsigned int i=1; i<mSamples.size(); ++i )
			{
				mSamples[i].ResetMinMax();
			}
		}
	}

	m_IsActive = m_RequestedIsActive;
}



#endif // !GP_RETAIL
