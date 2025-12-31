//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiPersist.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiPersist.h"

#include "FuBi.h"
#include "FuBiSchema.h"
#include "ReportSys.h"
#include "StringTool.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////

const char* PersistContext::VALUE_STR = "_value";

//////////////////////////////////////////////////////////////////////////////
// class PersistContext implementation

PersistContext :: PersistContext( PersistWriter* writer, bool isFullXfer, eXMode xmode, gpstring* reportOutput )
{
	gpassert( writer != NULL );

	m_Writer               = writer;
	m_Reader               = NULL;
	m_IsFullXfer           = isFullXfer;
	m_IsText               = writer->AsText();
	m_XferMode             = xmode;
	m_Version              = SysInfo::GetExeFileVersion();
	m_IsFatalError         = false;
	m_IsWatsonError        = false;
	m_SeriousErrorCount    = 0;
	m_ReportOutput         = NULL;
	m_ReportOutputExternal = false;
	m_ReportContext        = NULL;

	if ( isFullXfer )
	{
		InitContext( reportOutput );
	}

#	if !GP_RETAIL
	m_NestDepth = 0;
#	endif // !GP_RETAIL
}

PersistContext :: PersistContext( PersistReader* reader, bool isFullXfer, eXMode xmode, gpstring* reportOutput )
{
	gpassert( reader != NULL );

	m_Writer               = NULL;
	m_Reader               = reader;
	m_IsFullXfer           = isFullXfer;
	m_IsText               = reader->AsText();
	m_XferMode             = xmode;
	m_Version              = SysInfo::GetExeFileVersion();
	m_IsFatalError         = false;
	m_IsWatsonError        = false;
	m_SeriousErrorCount    = 0;
	m_ReportOutput         = NULL;
	m_ReportOutputExternal = false;
	m_ReportContext        = NULL;

	if ( isFullXfer )
	{
		InitContext( reportOutput );
	}

#	if !GP_RETAIL
	m_NestDepth = 0;
#	endif // !GP_RETAIL
}

PersistContext :: ~PersistContext( void )
{
#	if ( !GP_RETAIL )

	// check for bad alignment, but only if everything else succeeded
	if ( (m_ReportOutput != NULL) && m_ReportOutput->empty() )
	{
		if ( !m_BadNest.empty() )
		{
			gperror( "Something must be seriously wrong with save game... the 'bad block' collection is not empty!!\n" );
		}
		if ( m_NestDepth != 0 )
		{
			gperror( "Something must be seriously wrong with save game... the nested depth is not zero!!\n" );
		}
	}

	// output errors if nobody else handling it
	if ( !m_ReportOutputExternal )
	{
		ReportPersistErrors();
	}

#	endif // GP_RETAIL

	// free memory
	delete ( m_ReportContext );
}

bool PersistContext :: EnterBlock( const char* name )
{
	gpassert( (name != NULL) && (*name != '\0') );
	GPDEV_ONLY( ++m_NestDepth );

	if ( IsSaving() )
	{
		return ( m_Writer->EnterBlock( name ) );
	}
	else if ( m_BadNest.empty() && m_Reader->EnterBlock( name ) )
	{
		return ( true );
	}
	else
	{
		m_BadNest.push( name );

#		if !GP_RETAIL
		if ( m_BadNest.size() > 5 )
		{
			gperror( "Something must be seriously wrong with save game... the 'bad block' nesting level is over 5!!\n" );
		}
#		endif // !GP_RETAIL

		return ( false );
	}
}

bool PersistContext :: LeaveBlock( void )
{
#	if !GP_RETAIL
	if ( --m_NestDepth < 0 )
	{
		gperror( "Something must be seriously wrong with save game... we've popped the stack past empty!!\n" );
	}
#	endif // !GP_RETAIL

	if ( IsSaving() )
	{
		return ( m_Writer->LeaveBlock() );
	}
	else if ( m_BadNest.empty() )
	{
		return ( m_Reader->LeaveBlock() );
	}
	else
	{
		m_BadNest.pop();
		return ( false );
	}
}

bool PersistContext :: EnterColl( const char* name )
{
	gpassert( (name != NULL) && (*name != '\0') );

	bool rc = EnterBlock( name );
	m_CollStack.push( CollEntry() );
	return ( rc );
}

bool PersistContext :: LeaveColl( void )
{
	gpassert( !m_CollStack.empty() );

	if ( m_CollStack.top().m_Entered )
	{
		LeaveBlock();
	}
	m_CollStack.pop();

	LeaveBlock();
	return ( true );
}

bool PersistContext :: AdvanceCollIter( void )
{
	if ( m_CollStack.top().m_Entered )
	{
		LeaveBlock();
	}
	else
	{
		m_CollStack.top().m_Entered = true;
	}

	char buffer[ 20 ];
	::_itoa( m_CollStack.top().m_Index++, buffer, 10 );

	return ( EnterBlock( buffer ) );
}

void PersistContext :: ReportPersistErrors( void )
{
	if ( (m_ReportOutput != NULL) && !m_ReportOutput->empty() )
	{
		gperrorf(( "%d serious errors occurred during persist operation:\n\n%s\n",
				   m_SeriousErrorCount, m_ReportOutput->c_str() ));
	}
}

#if !GP_RETAIL

void PersistContext :: ReportConversionError( const gpstring& str, const char* key )
{
	if ( ShouldReportConversionErrors( m_XferMode ) )
	{
		gpreportf( gXferErrors, ( "Error: unable to convert value '%s' for key '%s'\n",
				   stringtool::EscapeCharactersToTokens( str ).c_str(), key ));
	}
}

#endif // !GP_RETAIL

bool PersistContext :: XferRecord( eXfer xfer, const char* key, Record& record, const Record* defRecord )
{
	gpassert( (defRecord == NULL) || (*record.GetHeaderSpec() == *defRecord->GetHeaderSpec()) );

	bool rc = (key != NULL) ? EnterBlock( key ) : true;

	const HeaderSpec* spec = record.GetHeaderSpec();
	HeaderSpec::ConstIter i, begin = spec->GetColumnBegin(), end = spec->GetColumnEnd();
	int index = 0;
	for ( i = begin ; i != end ; ++i, ++index )
	{
		// skip this one if we aren't supposed to xfer it
		if ( i->m_Extra->m_NoXfer )
		{
			continue;
		}

		// get the type and the param itself
		const ColumnSpec& col = *i;
		void* obj = record.OnGetFieldPtr( index );
		int objSize = 0;

		// only supporting by-value or pointerclasses for now
		gpassert( col.m_Type.IsPassByValue() || col.m_Type.IsPointerClass() );

		// see which mode to play in
		bool asText = AsText();
		if ( !asText )
		{
			if ( col.m_Type.IsPointerClass() )
			{
				objSize = sizeof( void* );
			}
			else
			{
				const VarTypeSpec* vspec = gFuBiSysExports.GetVarTypeSpec( col.m_Type.m_Type );
				if (    (vspec != NULL)
					 && !(vspec->m_Flags & Trait::FLAG_FORCETEXT)
					 && (vspec->m_Flags & Trait::FLAG_POD) )
				{
					objSize = vspec->m_SizeBytes;
				}
				else
				{
					asText = true;
				}
			}
		}

		// do the xfer
		bool success = true;
		if ( asText )
		{
			gpstring temp;

			if ( IsSaving() )
			{
				if ( ::ToString( col.m_Type, temp, obj, xfer ) )
				{
					eXfer xfer = XFER_NORMAL;
					if ( record.GetQuotedStrings() && (col.m_Type.m_Type == VAR_STRING) )
					{
						xfer = XFER_QUOTED;
					}
					XferString( xfer, col.GetName(), temp, col.m_Type.m_Type );
				}
			}
			else
			{
				if (   !XferString( col.GetName(), temp, col.m_Type.m_Type )
					|| !::FromString( col.m_Type, temp, obj ) )
				{
					success = false;
				}
			}
		}
		else
		{
			gpassert( objSize > 0 );
			success = !XferBinary( col.GetName(), mem_ptr( obj, objSize ) );
		}

		// copy default if failed
		if ( !success && (defRecord != NULL) )
		{
			gFuBiSysExports.CopyVar( col.m_Type, obj, defRecord->OnGetFieldPtr( index ) );
		}
	}

	if ( key != NULL )
	{
		LeaveBlock();
	}
	return ( rc );
}

void PersistContext :: InitContext( gpstring* reportOutput )
{
	ReportSys::StringSink* stringSink
			= reportOutput
			? new ReportSys::StringSink( *reportOutput )
			: new ReportSys::StringSink;

	m_ReportOutputExternal = reportOutput != NULL;
	m_ReportOutput         = m_ReportOutputExternal ? reportOutput : &stringSink->GetBuffer();
	m_ReportContext        = new ReportSys::LocalContext( stringSink, true );

	struct AutoAddCount : public ReportSys::Sink
	{
		PersistContext* m_Persist;

		AutoAddCount( PersistContext* persist )
		{
			m_Persist = persist;
		}

		virtual void OnBeginReport( const ReportSys::Context* /*context*/ )
		{
			m_Persist->IncSeriousErrorCount();
		}

		virtual bool OutputString( const ReportSys::Context* /*context*/, const char* /*text*/, int /*len*/, bool /*startLine*/, bool /*endLine*/ )  {  return ( true );  }
		virtual bool OutputField ( const ReportSys::Context* /*context*/, const char* /*text*/, int /*len*/ )  {  return ( true );  }
	};

	m_ReportContext->SetOptions( ReportSys::Context::OPTION_AUTOREPORT );
	m_ReportContext->AddSink( new AutoAddCount( this ), true );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
