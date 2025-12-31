//////////////////////////////////////////////////////////////////////////////
//
// File     :  ReportSys.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "ReportSys.h"

#include "AppModule.h"
#include "Config.h"
#include "DrWatson.h"
#include "FileSysDefs.h"
#include "FuBiSchema.h"
#include "KernelTool.h"
#include "StdHelp.h"
#include "StringTool.h"

#define DISPLAY_TIME_IN_PREFIX 0
#if DISPLAY_TIME_IN_PREFIX
#include "WorldTime.h"
#endif

#include <numeric>

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////

DECLARE_SINK_FACTORY( File,       FileSink::CreateSink );		// append to file if exists
DECLARE_SINK_FACTORY( FileAppend, FileSink::CreateSink );		// append to file if exists
DECLARE_SINK_FACTORY( FileNew,    FileSink::CreateSink );		// always create new file

DECLARE_SINK_FACTORY( RetryFile,       RetryFileSink::CreateSink );		// append to file if exists
DECLARE_SINK_FACTORY( RetryFileAppend, RetryFileSink::CreateSink );		// append to file if exists
DECLARE_SINK_FACTORY( RetryFileNew,    RetryFileSink::CreateSink );		// always create new file

DECLARE_SINK_FACTORY( Debugger, DebuggerSink::CreateSink );

DECLARE_SINK_FACTORY( Assert,     CallbackSink::CreateSink );
DECLARE_SINK_FACTORY( Fatal,      CallbackSink::CreateSink );
DECLARE_SINK_FACTORY( MessageBox, CallbackSink::CreateSink );
DECLARE_SINK_FACTORY( ErrorBox,   CallbackSink::CreateSink );

//////////////////////////////////////////////////////////////////////////////

struct SpecialCritical : kerneltool::Critical, Singleton <SpecialCritical>  {  };

static kerneltool::Critical* GetReportCritical( void )
{
	static SpecialCritical s_ReportCritical;
	return ( SpecialCritical::GetSingletonPtr() );
}

#define s_ReportCritical GetReportCritical()

//////////////////////////////////////////////////////////////////////////////
// class Sink implementation

gpstring Sink::ms_IndentText;
int      Sink::ms_NextId = 0;

Sink :: Sink( void )
{
	m_Enabled          = true;
	m_Id               = ms_NextId++;
	m_Type             = "Sink";
	m_ChildEnableCount = -1;

	GPDEBUG_ONLY( m_ChildCollIterCount   = 0 );
	GPDEBUG_ONLY( m_ParentCollIterCount  = 0 );
	GPDEBUG_ONLY( m_ContextCollIterCount = 0 );
}

Sink :: ~Sink( void )
{
	kerneltool::Critical::OptionalLock locker( s_ReportCritical );

	// release children
	{
		GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_WRITE ) );

		ChildColl::iterator i, ibegin = m_ChildColl.begin(), iend = m_ChildColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			PrivateRemoveChild( i->m_Sink, i->m_Owned );
		}
	}

	// release parents
	{
		GPDEBUG_ONLY( IterLock locker( &m_ParentCollIterCount, LOCK_WRITE ) );

		ParentColl::iterator i, begin = m_ParentColl.begin(), end = m_ParentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			Sink* parent = *i;
			bool old = parent->IsEnabled();

			{
				GPDEBUG_ONLY( IterLock locker( &parent->m_ChildCollIterCount, LOCK_WRITE ) );
				gpassert( stdx::has_any( parent->m_ChildColl, this ) );

				// release this as child
				parent->m_ChildColl.erase( stdx::find( parent->m_ChildColl, this ) );
			}

			// update enabling state
			parent->PrivateEnable( old );
		}
	}

	// unregister self
	if ( Mgr::DoesSingletonExist() )
	{
		gReportSysMgr.UnregisterSink( this );
	}
}

void Sink :: AddSink( Sink* sink, bool owned, bool pushBack )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_WRITE ) );
	kerneltool::Critical::OptionalLock serializer( s_ReportCritical );

	// verify
	gpassert( sink != NULL );
	gpassert( sink != this );
	gpassert( stdx::find( m_ChildColl, sink ) == m_ChildColl.end() );

	// adjust -1 over
	bool old = IsEnabled();
	if ( m_ChildEnableCount == -1 )
	{
		gpassert( m_ChildColl.empty() );
		m_ChildEnableCount = 0;
	}

	// add child
	if ( pushBack )
	{
		m_ChildColl.push_back( ChildEntry( sink, owned ) );
	}
	else
	{
		m_ChildColl.insert( m_ChildColl.begin(), ChildEntry( sink, owned ) );
	}

	// who's yer daddy
	{
		GPDEBUG_ONLY( IterLock locker( &sink->m_ParentCollIterCount, LOCK_WRITE ) );
		gpassert( stdx::has_none( sink->m_ParentColl, this ) );
		sink->m_ParentColl.push_back( this );
	}

	// add our contexts to it
	{
		GPDEBUG_ONLY( IterLock locker( &m_ContextCollIterCount, LOCK_READ ) );

		ContextDb::iterator i, begin = m_ContextDb.begin(), end = m_ContextDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			sink->AddContextRef( i->first );
		}
	}

	// update enabling cache
	m_ChildEnableCount += sink->IsEnabled() ? 1 : -1;
	PrivateEnable( old );
}

void Sink :: RemoveSink( Sink* sink, bool delIfOwned )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_WRITE ) );
	kerneltool::Critical::OptionalLock serializer( s_ReportCritical );

	// find it
	ChildColl::iterator found = stdx::find( m_ChildColl, sink );
	gpassert( found != m_ChildColl.end() );

	// get state vars
	bool old = IsEnabled();
	bool childEnabled = found->m_Sink->IsEnabled();

	// remove it
	PrivateRemoveChild( found->m_Sink, delIfOwned && found->m_Owned );

	// remove child
	m_ChildColl.erase( found );

	// update cache
	if ( m_ChildColl.empty() )
	{
		m_ChildEnableCount = -1;
	}
	else
	{
		m_ChildEnableCount += childEnabled ? 1 : -1;
	}
	PrivateEnable( old );
}

Sink* Sink :: FindSinkByType( const char* type )
{
	if ( m_Type.same_no_case( type ) )
	{
		return ( this );
	}

	ChildColl::iterator i, ibegin = m_ChildColl.begin(), iend = m_ChildColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Sink* sink = i->m_Sink->FindSinkByType( type );
		if ( sink != NULL )
		{
			return ( sink );
		}
	}

	return ( NULL );
}

void Sink :: AddContextRef( Context* context )
{
	gpassert( context != NULL );
	typedef std::pair <ContextDb::iterator, bool> ContextDbInsertRc;

	// add to self first
	{
		GPDEBUG_ONLY( IterLock locker( &m_ContextCollIterCount, LOCK_WRITE ) );

		ContextDbInsertRc rc = m_ContextDb.insert( std::make_pair( context, ContextEntry() ) );
		if ( rc.second )
		{
			rc.first->second.m_DerivedStateInfo = OnCreateStateInfo( context );
		}
		++rc.first->second.m_RefCount;
	}

	// now add to children
	{
		GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

		ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			i->m_Sink->AddContextRef( context );
		}
	}
}

void Sink :: RemoveContextRef( Context* context )
{
	gpassert( context != NULL );

	// remove from self first
	{
		GPDEBUG_ONLY( IterLock locker( &m_ContextCollIterCount, LOCK_WRITE ) );

		ContextDb::iterator found = m_ContextDb.find( context );
		gpassert( found != m_ContextDb.end() );
		gpassert( found->second.m_RefCount > 0 );

		if ( --found->second.m_RefCount == 0 )
		{
			if ( found->second.m_DerivedStateInfo != NULL )
			{
				OnDestroyStateInfo( found->second.m_DerivedStateInfo );
			}
			m_ContextDb.erase( found );
		}
	}

	// now remove from children
	{
		GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

		ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			i->m_Sink->RemoveContextRef( context );
		}
	}
}

void Sink :: SetName( const char* name )
{
	// unregister old
	if ( Mgr::DoesSingletonExist() )
	{
		gReportSysMgr.UnregisterSink( this );
	}
	m_Name.erase();

	// register new - this will assert if a nonunique name
	if ( (name != NULL) && (*name != '\0') )
	{
		m_Name = name;
		gReportSysMgr.RegisterSink( m_Name, this );
	}
}

gpstring Sink :: GenerateName( void )
{
	return ( gpstringf( "Sink%d", ms_NextId ) );
}

void Sink :: Enable( bool enable )
{
	// update
	bool old = IsEnabled();
	m_Enabled = enable;

	// notify parents if we changed (taking child enabling into account)
	PrivateEnable( old );
}

void Sink :: OnBeginTable( const Context* context )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->m_Sink->OnBeginTable( context );
	}
}

void Sink :: OnEndTable( const Context* context )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->m_Sink->OnEndTable( context );
	}
}

void Sink :: OnBeginReport( const Context* context )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->m_Sink->OnBeginReport( context );
	}
}

void Sink :: OnEndReport( const Context* context )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->m_Sink->OnEndReport( context );
	}
}

void Sink :: OnFormatChange( const Context* context, eChange change )
{
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->m_Sink->OnFormatChange( context, change );
	}
}

bool Sink :: OutputString( const Context* context, const char* text, int len,
						   bool startLine, bool endLine )
{
	gpassert( IsEnabled() );
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	bool success = true;

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Sink->IsEnabled() && !i->m_Sink->OutputString( context, text, len, startLine, endLine ) )
		{
			success = false;
		}
	}

	return ( success );
}

bool Sink :: OutputField( const Context* context, const char* text, int len )
{
	gpassert( IsEnabled() );
	GPDEBUG_ONLY( IterLock locker( &m_ChildCollIterCount, LOCK_READ ) );

	bool success = true;

	ChildColl::iterator i, begin = m_ChildColl.begin(), end = m_ChildColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Sink->IsEnabled() && !i->m_Sink->OutputField( context, text, len ) )
		{
			success = false;
		}
	}

	return ( success );
}

void* Sink :: GetStateInfo( Context* context )
{
	void* info = NULL;

	ContextDb::iterator found = m_ContextDb.find( context );
	if ( found != m_ContextDb.end() )
	{
		info = found->second.m_DerivedStateInfo;
	}

	return ( info );
}

int Sink :: GetIndentSpaces( const Context* context )
{
	return ( (context != NULL) ? (context->GetIndent() * context->GetIndentSpaces()) : 0 );
}

const char* Sink :: MakeIndentText( int indent )
{
	ms_IndentText.resize( indent, ' ' );
	return ( ms_IndentText );
}

void Sink :: PrivateRemoveChild( Sink* child, bool owned )
{
	gpassert( child != NULL );
	gpassert( child != this );

	// release
	{
		GPDEBUG_ONLY( IterLock locker( &child->m_ParentCollIterCount, LOCK_WRITE ) );
		gpassert( stdx::has_any( child->m_ParentColl, this ) );
		child->m_ParentColl.erase( stdx::find( child->m_ParentColl, this ) );
	}

	// release and delete it if owned
	if ( owned )
	{
		delete ( child );
	}
	else
	{
		// release our contexts from it if not deleting it
		GPDEBUG_ONLY( IterLock locker( &m_ContextCollIterCount, LOCK_READ ) );
		ContextDb::iterator i, begin = m_ContextDb.begin(), end = m_ContextDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			child->RemoveContextRef( i->first );
		}
	}
}

void Sink :: PrivateEnable( bool old )
{
	// if our enabling state has changed (for whatever reason) let our parents
	// know about it

	bool enable = IsEnabled();
	if ( enable != old )
	{
		GPDEBUG_ONLY( IterLock locker( &m_ParentCollIterCount, LOCK_READ ) );

		ParentColl::iterator i, begin = m_ParentColl.begin(), end = m_ParentColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			Sink* parent = *i;

			bool parentOld = parent->IsEnabled();
			parent->m_ChildEnableCount += enable ? 1 : -1;
			parent->PrivateEnable( parentOld );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class StreamSink implementation

bool StreamSink :: OutputField( const Context* context, const char* text, int len )
{
	bool success = Inherited::OutputField( context, text, len );

	bool startLine = context->GetFieldIndex() == 0;
	bool endLine   = context->GetFieldIndex() == (context->GetFieldCount() - 1);

	if ( !startLine )
	{
		static const char DELIM[] = ", ";
		if ( !OutputString( context, DELIM, ELEMENT_COUNT( DELIM ) - 1, false, false ) )
		{
			success = false;
		}
	}

	if ( !OutputString( context, text, len, startLine, endLine ) )
	{
		success = false;
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class FileSink implementation

FileSink :: FileSink( HANDLE file, bool ownsHandle, const char* name )
	: Inherited( name )
{
	m_OwnsHandle = ownsHandle;

	SetType( "File" );
	Open( file );
}

FileSink :: FileSink( const char* fileName, bool truncate, const char* name )
	: Inherited( name )
{
	m_OwnsHandle = true;

	SetType( "File" );
	Open( fileName, truncate );
}

FileSink :: ~FileSink( void )
{
	if ( !m_OwnsHandle )
	{
		m_File.Release();
	}
}

bool FileSink :: Open( const char* fileName, bool truncate )
{
	Close();
	if ( m_File.Create( fileName,
						FileSys::File::ACCESS_READ_WRITE,
						FileSys::File::SHARE_READ_WRITE,
						truncate ? FileSys::File::CREATION_ALWAYS
								 : FileSys::File::CREATION_OPEN_ALWAYS ) )
	{
		m_File.SeekEnd();
		return ( true );
	}
	return ( false );
}

bool FileSink :: OutputString( const Context* context, const char* text, int len,
							   bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// return if no file
	if ( !m_File.IsOpen() )
	{
		return ( success );
	}

	// write indent
	int indent;
	if ( startLine && ((indent = GetIndentSpaces( context )) > 0) )
	{
		success = m_File.Write( MakeIndentText( indent ), indent );
	}

	// write text
	if ( success )
	{
		success = m_File.Write( text, len );
	}

	// terminate
	if ( endLine && success )
	{
		success = m_File.Write( "\r\n", 2 );
	}

	// done
	return ( success );
}

Sink* FileSink :: CreateSink( const char* name, const char* type, const char* fileName )
{
	gpassert( (type != NULL) && (*type != '\0') && (fileName != NULL) );
	if ( *fileName == '\0' )
	{
		fileName = name ? name : "";
	}
	
	if ( *fileName == '\0' )
	{
		gpwarningf(( "Unable to create FileSink of type '%s': fileName is empty\n", type ));
		return ( NULL );
	}

	bool truncate;
	if ( (::stricmp( type, "File" ) == 0) || (::stricmp( type, "FileAppend" ) == 0) )
	{
		truncate = false;
	}
	else if ( ::stricmp( type, "FileNew" ) == 0 )
	{
		truncate = true;
	}
	else
	{
		gpassert( 0 );		// this should be impossible...
		return ( NULL );
	}

	FileSink* sink = new FileSink( fileName, truncate );
	sink->SetName( name );
	sink->SetType( type );

	if ( !sink->IsOpen() )
	{
		gpwarningf(( "Unable to open FileSink of type '%s' for fileName '%s': error is 0x%08X ('%s')\n",
				   type, fileName, ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		Delete( sink );
	}

	return ( sink );
}

void FileSink :: OutputGenericHeader( Context* context, const char* categoryName, bool forErrors )
{
	SYSTEMTIME st;
	gpstring appName;

	if ( AppModule::DoesSingletonExist() )
	{
		st = gAppModule.GetStartTime();
		appName = ::ToAnsi( gAppModule.GetAppName() );
	}
	else
	{
		::GetLocalTime( &st );
		appName = "<Unknown App>";
	}

	context->OutputF(
			"\n"
			"-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-\n"
			"-== App          : %s (%s - %s)\n"
			"-== Log category : %s\n"
			"-== Session      : %d/%d/%d %02d:%02d:%02d %s\n"
#			if !GP_RETAIL
			"-== User/Machine : %s/%s (%s)\n"
#			endif // !GP_RETAIL
			"-== Build        : %s (%s)\n",
			appName.c_str(), FileSys::GetModuleFileName().c_str(), COMPILE_MODE_TEXT,
			categoryName,
			st.wMonth, st.wDay, st.wYear,
			(st.wHour > 12) ? (st.wHour - 12) : (st.wHour ? st.wHour : 12), st.wMinute, st.wSecond,
			(st.wHour >= 12) ? "PM" : "AM",
#			if !GP_RETAIL
			SysInfo::GetUserName(), SysInfo::GetComputerName(),
#			endif // !GP_RETAIL
			(SysInfo::GetOsVersionInfo().dwPlatformId == VER_PLATFORM_WIN32_NT) ? "Win2k" : "Win9x",
			SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE ).c_str(), __DATE__ " " __TIME__
		  );

	if ( forErrors )
	{
		context->OutputF(
				"-== Failures     : %d warnings, %d errors, %d SEH's\n",
				ReportSys::GetTotalWarningCount(),
				ReportSys::GetTotalErrorCount(),
				::GlobalGetUnfilteredHandledExceptionCount() );
	}

	context->Output(
			"-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-\n"
			"\n"
		  );
}

//////////////////////////////////////////////////////////////////////////////
// class RetryFileSink implementation

RetryFileSink :: RetryFileSink( const char* name )
	: Inherited( name )
{
	SetType( "RetryFile" );
}

RetryFileSink :: RetryFileSink( const char* fileName, bool truncate, const char* name )
	: Inherited( name )
{
	SetType( "RetryFile" );
	Open( fileName, truncate );
}

RetryFileSink :: ~RetryFileSink( void )
{
	// this space intentionally left blank...
}

void RetryFileSink :: Open( const char* fileName, bool truncate )
{
	Close();
	m_FileName = fileName;
	m_Truncate = truncate;
	stringtool::AppendExtension( m_FileName, ".log" );
}

bool RetryFileSink :: OutputString( const Context* context, const char* text, int len,
									bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// make sure file is open
	if ( !m_File.IsOpen() )
	{
		// return if no file
		if ( m_FileName.empty() )
		{
			return ( success );
		}

		// if it has path info, give that a try first
		gpstring fileName = m_FileName;
		if ( FileSys::HasPathInfo( fileName ) && !TryOpen( m_FileName ) )
		{
			// didn't work, so reduce it to its plain file name
			const char* fileNameOnly = FileSys::GetFileName( m_FileName );
			if ( fileNameOnly != NULL )
			{
				fileName = fileNameOnly;
			}
			else
			{
				return ( false );
			}
		}

		// try again with local path info
		if ( !m_File.IsOpen() )
		{
			fileName = FileSys::MakeWritableFileName( fileName );
			if ( !TryOpen( fileName ) )
			{
				return ( false );
			}
		}
	}

	// write indent
	int indent;
	if ( startLine && ((indent = GetIndentSpaces( context )) > 0) )
	{
		success = m_File.Write( MakeIndentText( indent ), indent );
	}

	// write text
	if ( success )
	{
		success = m_File.Write( text, len );
	}

	// terminate
	if ( endLine && success )
	{
		success = m_File.Write( "\r\n", 2 );
	}

	// done
	return ( success );
}

Sink* RetryFileSink :: CreateSink( const char* name, const char* type, const char* fileName )
{
	gpassert( (type != NULL) && (*type != '\0') && (fileName != NULL) );
	if ( *fileName == '\0' )
	{
		gpwarningf(( "Unable to create RetryFileSink of type '%s': fileName is empty\n", type ));
		return ( NULL );
	}

	bool truncate;
	if ( (::stricmp( type, "RetryFile" ) == 0) || (::stricmp( type, "RetryFileAppend" ) == 0) )
	{
		truncate = false;
	}
	else if ( ::stricmp( type, "RetryFileNew" ) == 0 )
	{
		truncate = true;
	}
	else
	{
		gpassert( 0 );		// this should be impossible...
		return ( NULL );
	}

	RetryFileSink* sink = new RetryFileSink( fileName, truncate );
	sink->SetName( name );
	sink->SetType( type );

	return ( sink );
}

bool RetryFileSink :: TryOpen( const char* fileName )
{
	if ( m_File.Create( fileName,
						FileSys::File::ACCESS_READ_WRITE,
						FileSys::File::SHARE_READ_WRITE,
						m_Truncate ? FileSys::File::CREATION_ALWAYS
								   : FileSys::File::CREATION_OPEN_ALWAYS ) )
	{
		m_File.SeekEnd();
		return ( true );
	}
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// class StdioFileSink implementation

StdioFileSink :: StdioFileSink( FILE* file, const char* name )
	: Inherited( name )
{
	SetType( "StdioFile" );
	Open( file );
}

StdioFileSink :: ~StdioFileSink( void )
{
	// this space intentionally left blank...
}

bool StdioFileSink :: Close( void )
{
	if ( m_File == NULL )
	{
		return ( true );
	}
	return ( !::fclose( m_File ) );
}

bool StdioFileSink :: OutputString( const Context* context, const char* text, int len,
									bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// return if no file
	if ( m_File == NULL )
	{
		return ( success );
	}

	// write indent
	int indent;
	if ( startLine && ((indent = GetIndentSpaces( context )) > 0) )
	{
		success = ::fwrite( MakeIndentText( indent ), indent, 1, m_File ) == 1;
	}

	// write text
	if ( success )
	{
		success = ::fwrite( text, len, 1, m_File ) == 1;
	}

	// terminate
	if ( endLine && success )
	{
		success = ::fwrite( "\r\n", 2, 1, m_File ) == 1;
	}

	// done
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class FileSysStreamerSink implementation

FileSysStreamerSink :: FileSysStreamerSink( FileSys::StreamWriter& writer, const char* name )
	: Inherited( name ), m_Writer( writer )
{
	SetType( "FileSysStreamer" );
}

FileSysStreamerSink :: ~FileSysStreamerSink( void )
{
	// this space intentionally left blank...
}

bool FileSysStreamerSink :: OutputString( const Context* context, const char* text, int len,
										  bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// write indent
	int indent;
	if ( startLine && ((indent = GetIndentSpaces( context )) > 0) )
	{
		success = m_Writer.Write( MakeIndentText( indent ), indent );
	}

	// write text
	if ( success )
	{
		success = m_Writer.Write( text, len );
	}

	// terminate
	if ( endLine && success )
	{
		success = m_Writer.WriteText( "\n", 1 );
	}

	// done
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class Win32ConsoleSink implementation

Win32ConsoleSink :: Win32ConsoleSink( const char* name )
	: Inherited( INVALID_HANDLE_VALUE, true, name )
{
	SetType( "Win32Console" );

	FileSys::File file;
	file.CreateConsoleOut();
	m_Console = file.Release();
	Open( m_Console );
}

Win32ConsoleSink :: ~Win32ConsoleSink( void )
{
	// this space intentionally left blank...
}

bool Win32ConsoleSink :: OutputString( const Context* context, const char* text, int len,
								  bool startLine, bool endLine )
{
	gpassert( IsEnabled() );

	bool success;
	if ( !context->IsColorDefault() )
	{
		// get default color for background
		CONSOLE_SCREEN_BUFFER_INFO info;
		::GetConsoleScreenBufferInfo( m_Console, &info );
		WORD defattr = info.wAttributes;
		WORD attr = (WORD)(defattr & ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));

		// get foreground color
		gpcolor color = context->GetColor();
		if ( color.r > 85 )
		{
			attr |= FOREGROUND_RED;
			if ( color.r > 170 )
			{
				attr |= FOREGROUND_INTENSITY;
			}
		}
		if ( color.g > 85 )
		{
			attr |= FOREGROUND_GREEN;
			if ( color.g > 170 )
			{
				attr |= FOREGROUND_INTENSITY;
			}
		}
		if ( color.b > 85 )
		{
			attr |= FOREGROUND_BLUE;
			if ( color.b > 170 )
			{
				attr |= FOREGROUND_INTENSITY;
			}
		}

		// see if it's the same as the background
		if (   (!(attr & FOREGROUND_RED      ) == !(attr & BACKGROUND_RED      ))
			&& (!(attr & FOREGROUND_GREEN    ) == !(attr & BACKGROUND_GREEN    ))
			&& (!(attr & FOREGROUND_BLUE     ) == !(attr & BACKGROUND_BLUE     ))
			&& (!(attr & FOREGROUND_INTENSITY) == !(attr & BACKGROUND_INTENSITY)) )
		{
			// flip the intensity just so it's different
			if ( attr & FOREGROUND_INTENSITY )
			{
				attr &= ~FOREGROUND_INTENSITY;
			}
			else
			{
				attr |= FOREGROUND_INTENSITY;
			}
		}

		// output the string
		::SetConsoleTextAttribute( m_Console, attr );
		success = Inherited::OutputString( context, text, len, startLine, endLine );
		::SetConsoleTextAttribute( m_Console, defattr );
	}
	else
	{
		success = Inherited::OutputString( context, text, len, startLine, endLine );
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class DebuggerSink implementation

DebuggerSink :: DebuggerSink( const char* name )
	: Inherited( name )
{
	SetType( "Debugger" );
}

DebuggerSink :: ~DebuggerSink( void )
{
	// this space intentionally left blank...
}

bool DebuggerSink :: OutputString( const Context* context, const char* text, int len,
				 			  bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

#if DISPLAY_TIME_IN_PREFIX
	if ( startLine && WorldTime::DoesSingletonExist())
	{
		char stime[60];
		sprintf(stime,"%12.5f:",gWorldTime.GetTime());
		::OutputDebugString( stime );
	}
#endif

	// write indent
	int indent;
	if ( startLine && ((indent = GetIndentSpaces( context )) > 0) )
	{
		::OutputDebugString( MakeIndentText( indent ) );
	}

	// write text
	if ( text[ len ] == '\0' )
	{
		// easy case - null terminated is what ODS expects
		::OutputDebugString( text );
	}
	else
	{
		// damn, have to copy it local
		std::vector <char> buffer( len + 1 );
		::memcpy( buffer.begin(), text, len );
		buffer[ len ] = '\0';
		::OutputDebugString( buffer.begin() );
	}

	// terminate
	if ( endLine )
	{
		::OutputDebugString( "\n" );
	}

	// done
	return ( success );
}

Sink* DebuggerSink :: CreateSink( const char* name, const char* /*type*/, const char* /*unused*/ )
{
	return ( new DebuggerSink( name ) );
}

//////////////////////////////////////////////////////////////////////////////
// class StringSink implementation

StringSink :: StringSink( const char* name )
	: Inherited( name )
{
	SetType( "String" );
	m_Buffer = &m_LocalBuffer;
}

StringSink :: StringSink( gpstring& out, const char* name )
	: Inherited( name )
{
	SetType( "String" );
	m_Buffer = &out;
}

StringSink :: ~StringSink( void )
{
	// this space intentionally left blank...
}

bool StringSink :: OutputString( const Context* context, const char* text, int len,
				 				 bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// write indent
	if ( startLine )
	{
		m_Buffer->append( GetIndentSpaces( context ), ' ' );
	}

	// write text
	m_Buffer->append( text, len );

	// terminate
	if ( endLine )
	{
		m_Buffer->append( "\n" );
	}

	// done
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class CallbackSink implementation

static void AssertCb( const Context* context, CallbackSink* sink )
{
	DWORD reportFrame = GetReportMacroFrame();
	bool dbgBreak = (reportFrame == 0);

	if ( gpassert_box(
				NULL,
				sink->GetBuffer(),
				context->GetLocationFile(),
				context->GetLocationLine(),
				context->GetName(),
				false,
				false,
				false,
				reportFrame,
				dbgBreak ) && !dbgBreak )
	{
		SetNextReportDbgBreak();
	}
}

static LONG gs_IsFatalOccurring = -1;

bool IsFatalOccurring( void )
{
	return ( gs_IsFatalOccurring != -1 );
}

bool SetFatalIsOccurring( void )
{
	return ( InterlockedIncrement( &gs_IsFatalOccurring ) > 0 );
}

static const char ERROR_TEXT[] = $MSG$ "A fatal error has occurred and the app must shut down.";

static void FatalCb( const Context* /*context*/, CallbackSink* sink )
{
	if ( InterlockedIncrement( &gs_IsFatalOccurring ) == 0 )
	{
		// persistent message storage
		static char s_Buffer[ 1000 ];

		// get error
		const char* errorPrefix = gptranslate( ERROR_TEXT );

		// copy error
		if ( !sink->GetBuffer().empty() )
		{
			::_snprintf( s_Buffer, ELEMENT_COUNT( s_Buffer ), "%s\n\n%s", errorPrefix, sink->GetBuffer().c_str() );
			s_Buffer[ ELEMENT_COUNT( s_Buffer ) - 1 ] = '\0';
		}
		else
		{
			::strcpy( s_Buffer, errorPrefix );
		}

		// put up dialog and throw buffer as our exception
		SafeMessageBox( ::ToUnicode( s_Buffer ), gpwtranslate( $MSG$ "Fatal Error" ), MB_ICONERROR );

		// if debugging, just automatically break
		if ( SysInfo::IsDebuggerPresent() && (!AppModule::DoesSingletonExist() || !gAppModule.IsFullScreen()) )
		{
			BREAKPOINT();
		}

		// watson?
		if ( IsFatalAsWatsonEnabled() )
		{
			volatile int* i = NULL;
			*i = 0;
		}

		// throw the exception $$ note - we've been having too many problems
		// with game shutdown on a fatal. too many places the fatal could occur,
		// and the game is not exception-safe. so just nuke it. -sb
		if ( !SysInfo::IsDebuggerPresent() )
		{
			// $$ extra note - apparently many DLL's have issues when we try to
			// kill the game and die during the detaching operation, so let's
			// just nuke it all, screw the DLL's. dplay and some spyware are 
			// some of the DLL's that have problems with ExitProcess() + DS.
			::TerminateProcess( ::GetCurrentProcess(), 'Fatl' );
		}

		::RaiseException( EXCEPTION_GPG_FATAL, EXCEPTION_NONCONTINUABLE, 1, (const DWORD*)s_Buffer );
	}
	InterlockedDecrement( &gs_IsFatalOccurring );
}

static void MessageBoxCb( const Context* /*context*/, CallbackSink* sink )
{
	SafeMessageBox( ::ToUnicode( sink->GetBuffer() ), gpwtranslate( $MSG$ "Message" ), MB_ICONINFORMATION );
}

static void ErrorBoxCb( const Context* /*context*/, CallbackSink* sink )
{
	SafeMessageBox( ::ToUnicode( sink->GetBuffer() ), gpwtranslate( $MSG$ "Serious Error" ), MB_ICONERROR );
}

CallbackSink :: CallbackSink( Callback cb, const char* name, const char* type )
	: Inherited( name ), m_Callback( cb )
{
	SetType( (type != NULL) ? type : "Callback" );
}

CallbackSink :: ~CallbackSink( void )
{
	// this space intentionally left blank...
}

void CallbackSink :: OnBeginReport( const Context* context )
{
	Inherited::OnBeginReport( context );
	if ( context->GetReportLevel() == 0 )
	{
		ClearBuffer();
	}
}

void CallbackSink :: OnEndReport( const Context* context )
{
	Inherited::OnEndReport( context );
	if ( (context->GetReportLevel() == 1) && m_Callback )
	{
		m_Callback( context, this );
	}
}

CallbackSink::Callback CallbackSink :: MakeAssertCallback( void )
{
	return ( makeFunctor( AssertCb ) );
}

CallbackSink::Callback CallbackSink :: MakeFatalCallback( void )
{
	return ( makeFunctor( FatalCb ) );
}

CallbackSink::Callback CallbackSink :: MakeMessageBoxCallback( void )
{
	return ( makeFunctor( MessageBoxCb ) );
}

CallbackSink::Callback CallbackSink :: MakeErrorBoxCallback( void )
{
	return ( makeFunctor( ErrorBoxCb ) );
}

Sink* CallbackSink :: CreateSink( const char* name, const char* type, const char* /*unused*/ )
{
	gpassert( type != NULL );

	Callback cb;

	if ( ::stricmp( type, "Assert" ) == 0 )
	{
		cb = MakeAssertCallback();
	}
	else if ( ::stricmp( type, "Fatal" ) == 0 )
	{
		cb = MakeFatalCallback();
	}
	else if ( ::stricmp( type, "MessageBox" ) == 0 )
	{
		cb = MakeMessageBoxCallback();
	}
	else if ( ::stricmp( type, "ErrorBox" ) == 0 )
	{
		cb = MakeErrorBoxCallback();
	}
	else
	{
		gpassert( 0 );		// this should be impossible...
		return ( NULL );
	}

	CallbackSink* sink = new CallbackSink( cb, name );
	sink->SetType( type );

	return ( sink );
}

//////////////////////////////////////////////////////////////////////////////
// class TextTableSink implementation

TextTableSink :: TextTableSink( const char* name,
								const char* vertDelim,
								const char* horizDelim )
{
	SetName( name );  
	m_VertDelim = vertDelim;
	m_HorizDelim = horizDelim;
}

TextTableSink :: ~TextTableSink( void )
{
	Clear();
}
	
void TextTableSink :: OnBeginReport( const Context* context )
{
	if ( context->GetRowIndex() == 0 )
	{
		// setup collections
		m_Header.clear();
		m_Table.clear();
		m_Widths.clear();

		// setup header
		Schema* schema = context->GetSchema();
		Schema::ConstIter i,
						  begin = schema->GetColumnBegin(),
						  end   = schema->GetColumnEnd();
		for ( i = begin ; i != end ; ++i )
		{
			const gpstring& name = i->GetName();
			m_Header.push_back( name );
			m_Widths.push_back( name.length() );
		}
	}

	Inherited::OnBeginReport( context );
	m_Table.push_back( new Row( context->GetFieldCount() ) );
}

bool TextTableSink :: OutputString( const Context* context, const char* text, int len,
						   bool startLine, bool endLine )
{
	return ( Inherited::OutputString( context, text, len, startLine, endLine ) );
}

bool TextTableSink :: OutputField( const Context* context, const char* text, int len )
{
	bool success = Inherited::OutputField( context, text, len );

	// get access
	gpassert( context->GetRowIndex() < scast <int> ( m_Table.size() ) );
	Row* row = m_Table[ context->GetRowIndex() ];
	int fieldIndex = context->GetFieldIndex();
	gpassert( fieldIndex < scast <int> ( row->size() ) );
	gpstring& field = (*row)[ context->GetFieldIndex() ];

	// store it
	field.assign( text, len );

	// if first field, indent properly
	int indent;
	if ( (context->GetFieldIndex() == 0) && ((indent = GetIndentSpaces( context )) > 0) )
	{
		field.insert( (size_t)0, (size_t)indent, ' ' );
	}

	// update max sizes
	maximize( m_Widths[ fieldIndex ], scast <int> ( field.length() ) );

	// ok
	return ( success );
}

int TextTableSink :: CalcFrontPadWidth( void ) const
{
	// get initial padding (any w/s on the right side of delim)
	size_t frontPad = m_VertDelim.find_last_not_of( stringtool::ALL_WHITESPACE );
	if ( frontPad == gpstring::npos )
	{
		frontPad = 0;
	}
	else
	{
		frontPad = m_VertDelim.length() - frontPad - 1;
	}
	return ( scast <int> ( frontPad ) );
}

int TextTableSink :: CalcDividerWidth( int frontPad ) const
{
	// none? skip it
	if ( m_Header.empty() )
	{
		return ( 0 );
	}

	// include padding
	size_t width = frontPad * 2;

	// delimiters
	width += m_VertDelim.length() * (m_Header.size() - 1);

	// widths of each column
	width = std::accumulate( m_Widths.begin(), m_Widths.end(), width );

	// done
	return ( width );
}

int TextTableSink :: CalcDividerWidth( void ) const
{
	return ( CalcDividerWidth( CalcFrontPadWidth() ) );
}

void TextTableSink :: OutputReport( ContextRef ctx, int maxLines )
{
	// bail early if no header/data
	if ( m_Header.empty() )
	{
		return;
	}

	// get max lines if default
	if ( maxLines == -1 )
	{
		maxLines = m_Table.size();
	}

	AutoReport autoReport( ctx );

// Out the header.

	int frontPad = CalcFrontPadWidth();

	// output header
	OutputRow( ctx, frontPad, &m_Header );

	// out divider
	ctx->OutputRepeatWidth( CalcDividerWidth( frontPad ), m_HorizDelim, m_HorizDelim.length() );
	ctx->OutputEol();

	// output rows
	Table::const_iterator i, begin = m_Table.begin(), end = m_Table.end();
	for ( i = begin ; (i != end) && (maxLines > 0) ; ++i, --maxLines )
	{
		OutputRow( ctx, frontPad, *i );
	}
}

void TextTableSink :: Sort( int column, bool forward )
{
	if ( !m_Table.empty() )
	{
		gpassert( (column >= 0) && (column < scast <int> ( m_Header.size() )) );
		stdx::sort( m_Table, CompareRows( column, forward ) );
	}
}

void TextTableSink :: Clear( void )
{
	stdx::for_each( m_Table, stdx::delete_by_ptr() );

	m_Header.clear();
	m_Table .clear();
	m_Widths.clear();
}

void TextTableSink :: OutputRow( Context* ctx, int frontPad, Row* row )
{
	// pad front
	if ( frontPad > 0 )
	{
		ctx->Output( MakeIndentText( frontPad ), frontPad );
	}

	// for each column...
	Row   ::const_iterator ci, cbegin = row->begin(), cend = row->end();
	IntVec::const_iterator wi, wbegin = m_Widths.begin();
	for ( ci = cbegin, wi = wbegin ; ci != cend ; ++ci, ++wi )
	{
		// add delim if necessary
		if ( (ci != cbegin) && !m_VertDelim.empty() )
		{
			ctx->Output( m_VertDelim );
		}

		// add column
		if ( !ci->empty() )
		{
			ctx->Output( *ci );
		}

		// add padding
		int colPad = *wi - ci->length();
		if ( colPad > 0 )
		{
			ctx->Output( MakeIndentText( colPad ), colPad );
		}
	}
	ctx->OutputEol();
}

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

gpstring MakeLogFileName( const char* fileName )
{
	gpstring logName( fileName );
	if ( Config::DoesSingletonExist() && !FileSys::HasPathInfo( logName ) )
	{
		logName = gConfig.ResolvePathVars( "%log_path%/" + logName );
		if ( logName.empty() )
		{
			logName = fileName;
		}
	}
	return ( logName );
}

//////////////////////////////////////////////////////////////////////////////
// class Context implementation

int Context::ms_NextId = 0;

Context :: Context( Context* prototype, const char* name, const char* type, eTraits traits, bool isLocal )
{
	gpassert( prototype != NULL );

	Init( traits, prototype, isLocal );
	SetSink( new MultiSink, true );
	m_Sink->AddSink( prototype->m_Sink, false );
	SetName( name );
	SetType( type );
}

Context :: Context( Sink* sink, bool owned, const char* name, const char* type, eTraits traits, bool isLocal )
{
	gpassert( sink != NULL );

	Init( traits, NULL, isLocal );
	SetSink( sink, owned );
	SetName( name );
	SetType( type );
}

Context :: Context( const char* name, const char* type, eTraits traits, bool isLocal )
{
	Init( traits, NULL, isLocal );
	SetSink( new MultiSink, true );
	SetName( name );
	SetType( type );
}

Context :: ~Context( void )
{
	SetName( NULL );
	DestroySink();
	if ( m_OwnsSchema )
	{
		delete ( m_Schema );
	}
	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.RemoveContextRef( this );
	}
}

void Context :: SetOptions( eOptions options, bool set )
{
	eOptions oldOptions = m_Options;
	if ( set )
	{
		m_Options |= options;
	}
	else
	{
		m_Options &= NOT( options );
	}

	// special global - register/deregister
	if ( (oldOptions & OPTION_NOGLOBAL) != (m_Options & OPTION_NOGLOBAL) )
	{
		if ( m_Options & OPTION_NOGLOBAL )
		{
			gGlobalSink.RemoveContextRef( this );
		}
		else
		{
			gGlobalSink.AddContextRef( this );
		}
	}
}

void Context :: SetTraits( eTraits traits, bool set )
{
	if ( set )
	{
		m_Traits |= traits;
	}
	else
	{
		m_Traits &= NOT( traits );
	}
}

void Context :: SetSink( Sink* sink, bool owned, bool autoDeleteOld )
{
	gpassert( sink != NULL );

	// release old
	DestroySink( autoDeleteOld );

	// take new
	m_Sink     = sink;
	m_OwnsSink = owned;
	m_Sink->AddContextRef( this );
}

void Context :: HookSink( Sink* hook, bool owned )
{
	gpassert( hook != NULL );

	// release old
	Sink* oldSink = m_Sink;
	bool oldOwned = m_OwnsSink;
	DestroySink( false );

	// take new
	SetSink( hook, owned );
	if ( oldSink != NULL )
	{
		m_Sink->AddSink( oldSink, oldOwned );
	}
}

void Context :: ClearSink( void )
{
	SetSink( new MultiSink, true );
}

Sink* Context :: FindSinkByType( const char* type )
{
	return ( (m_Sink != NULL) ? m_Sink->FindSinkByType( type ) : NULL );
}

void Context :: SetName( const char* name )
{
	// unregister old
	if ( Mgr::DoesSingletonExist() )
	{
		gReportSysMgr.UnregisterContext( this );
	}
	m_Name.erase();

	// register new - this will assert if a nonunique name
	if ( (name != NULL) && (*name != '\0') )
	{
		m_Name = name;
		gReportSysMgr.RegisterContext( m_Name, this );
	}
}

gpstring Context :: GenerateName( void )
{
	return ( gpstringf( "Context%d", ms_NextId ) );
}

void Context :: SetType( const char* type )
{
	if ( type == NULL )
	{
		m_Type = "Generic";
	}
	else
	{
		m_Type = type;
	}
}

void Context :: SetSchema( Schema* schema, bool owned )
{
	Schema* old = m_Schema;

	if ( m_OwnsSchema )
	{
		delete ( m_Schema );
	}
	m_Schema = NULL;

	if ( schema != NULL )
	{
		m_Schema = schema;
		m_OwnsSchema = owned;
	}

	if ( old != schema )
	{
		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OnFormatChange( this, Sink::CHANGE_SCHEMA );
		}
		m_Sink->OnFormatChange( this, Sink::CHANGE_SCHEMA );
	}
}

void Context :: ClearSchema( void )
{
	SetSchema( NULL, false );
}

int Context :: GetFieldCount( void ) const
{
	int count = 0;
	if ( HasSchema() )
	{
		count = GetSchema()->GetColumnCount();
	}
	else if ( IsInReport() )
	{
		count = GetFieldIndex() + 2;	// always one more than the next
	}
	return ( count );
}

void Context :: BeginTable( void )
{
	gpassert( !m_InTable );
	gpassert( HasSchema() );

	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.OnBeginTable( this );
	}
	m_Sink->OnBeginTable( this );

	m_InTable = true;
}

void Context :: EndTable( void )
{
	gpassert( m_InTable );

	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.OnEndTable( this );
	}
	m_Sink->OnEndTable( this );

	// reset the row index
	m_InTable = false;
	m_RowIndex = 0;
}

void Context :: BeginReport( void )
{
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.OnBeginReport( this );
	}
	m_Sink->OnBeginReport( this );

	if ( m_ReportLevel++ == 0 )
	{
		AddReport( true );
	}
}

void Context :: EndReport( void )
{
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( (m_ReportLevel == 1) && IsLineOpen() && TestOptions( OPTION_AUTOREPORT ) )
	{
		// if we're ending the report on an auto-report context and the line
		// hasn't been terminated, terminate it automatically.
		Output( "\n" );
	}

	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.OnEndReport( this );
	}
	m_Sink->OnEndReport( this );

	if ( --m_ReportLevel == 0 )
	{
		// reset the field index if done reporting
		m_FieldIndex = 0;
	}
	gpassert( m_ReportLevel >= 0 );

	// next row
	++m_RowIndex;
}

void Context :: SetIndent( int indent )
{
	gpassert( (indent >= 0) && (indent < 500) );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( m_Indent != indent )
	{
		m_Indent = indent;

		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OnFormatChange( this, Sink::CHANGE_INDENT );
		}
		m_Sink->OnFormatChange( this, Sink::CHANGE_INDENT );
	}
}

void Context :: SetIndentSpaces( int spaces )
{
	gpassert( (spaces >= 0) && (spaces < 500) );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( m_IndentSpaces != spaces )
	{
		m_IndentSpaces = spaces;

		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OnFormatChange( this, Sink::CHANGE_INDENTSPACES );
		}
		m_Sink->OnFormatChange( this, Sink::CHANGE_INDENTSPACES );
	}
}

void Context :: SetColor( DWORD color )
{
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( (m_Color != color) || m_ColorDefault )
	{
		m_ColorDefault = false;
		m_Color = color;

		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OnFormatChange( this, Sink::CHANGE_COLOR );
		}
		m_Sink->OnFormatChange( this, Sink::CHANGE_COLOR );
	}
}

void Context :: SetColorDefault( void )
{
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	if ( !m_ColorDefault )
	{
		m_ColorDefault = true;

		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OnFormatChange( this, Sink::CHANGE_COLORDEFAULT );
		}
		m_Sink->OnFormatChange( this, Sink::CHANGE_COLORDEFAULT );
	}
}

bool Context :: OutputRaw( const char* data, int len )
{
	gpassert( data != NULL );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	// test and return if not needed
	if ( !AddReport() )
	{
		return ( true );
	}

	// get length if not set
	if ( len == -1 )
	{
		len = ::strlen( data );
	}

	// out to sink
	return ( m_Sink->OutputString( this, data, len, false, false ) );
}

bool Context :: Output( const char* data, int len )
{
	gpassert( data != NULL );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	// test and return if not needed
	if ( !AddReport() )
	{
		return ( true );
	}

	// get some vars
	PrivateAutoReport PrivateAutoReport( this );

	// translate the format if requested
	gpstring translate;
	if ( ShouldLocalize( data ) )
	{
		Translate( translate, data );
		data = translate;

		// the translation may have changed the length - make sure that we
		// weren't depending on a particular length
		gpassert( len == -1 );
		len = scast <int> ( translate.size() );
	}
	else
	{
		// get length if not set
		if ( len == -1 )
		{
			len = ::strlen( data );
		}

		// check and warn if supposed to localize!
		if ( TestOptions( OPTION_LOCALIZE ) && (std::find_if( data, data + len, isalpha ) != (data + len)) )
		{
			// debugger context cannot be localize-only!!
			gpassert( !gDebuggerContext.TestOptions( OPTION_LOCALIZE ) );

			// out it
			gpdebuggerf(( "Error: unlocalized string detected on a localizing context: '%s'\n",
						  stringtool::EscapeCharactersToTokens( data ).c_str() ));
		}
	}

	// output it
	return ( OutputPrivate( data, len ) );
}

bool Context :: OutputArgs( const char* format, va_list args )
{
	gpassert( format != NULL );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	// early bail out (to avoid expensive printf) if not enabled
	if ( !IsEnabled() )
	{
		return ( true );
	}

	// get some vars
	PrivateAutoReport PrivateAutoReport( this );

	// translate the format if requested
	gpstring translate;
	if ( ShouldLocalize( format ) )
	{
		Translate( translate, format );
		format = translate;
	}

	// do the printf and output
	auto_dynamic_vsnprintf printer( format, args );
	return ( OutputPrivate( printer, printer.length() ) );
}

bool Context :: OutputField( const char* data, int len )
{
	gpassert( data != NULL );
	gpassert( IsInReport() );
	kerneltool::Critical::OptionalLock locker( TestOptions( OPTION_NOTHREADS ) ? NULL : s_ReportCritical );

	// ignore if not enabled
	if ( !IsEnabled() )
	{
		return ( true );
	}

	// get length if not set
	if ( len == -1 )
	{
		len = ::strlen( data );
	}

	// out field
	bool success = true;
	if ( !TestOptions( OPTION_NOGLOBAL ) )
	{
		gGlobalSink.OutputField( this, data, len );
	}
	if ( !m_Sink->OutputField( this, data, len ) )
	{
		success = false;
	}

	// advance field index, and reset if done. note that if we're in a report,
	// ending the report will reset the field count. this allows arbitrary field
	// counts.
	if ( ++m_FieldIndex >= GetFieldCount() )
	{
		m_FieldIndex = 0;
	}

	// done
	return ( success );
}

bool Context :: OutputFieldArgs( const char* format, va_list args )
{
	gpassert( format != NULL );

	// early bail out (to avoid expensive printf) if not enabled
	if ( !IsEnabled() )
	{
		return ( true );
	}

	// do the printf and output
	auto_dynamic_vsnprintf printer( format, args );
	return ( OutputField( printer, printer.length() ) );
}

bool Context :: OutputRepeatCount( int repeatCount, const char* text, int len )
{
	gpassert( text != NULL );
	gpassert( repeatCount >= 0 );

	if ( (*text == '\0') || (repeatCount == 0) )
	{
		return ( true );
	}

	if ( len == -1 )
	{
		len = ::strlen( text );
	}

	gpstring str;
	if ( len == 1 )
	{
		str.assign( repeatCount, text[ 0 ] );
	}
	else
	{
		for ( int i = 0 ; i < repeatCount ; ++i )
		{
			str.append( text, len );
		}
	}
	return ( Output( str ) );
}

bool Context :: OutputRepeatWidth( int width, const char* text, int len )
{
	gpassert( text != NULL );
	gpassert( width >= 0 );

	if ( (*text == '\0') || (width == 0) )
	{
		return ( true );
	}

	if ( len == -1 )
	{
		len = ::strlen( text );
	}

	gpstring str;
	if ( len == 1 )
	{
		str.assign( width, text[ 0 ] );
	}
	else
	{
		// out repeating part
		while ( width > len )
		{
			str.append( text, len );
			width -= len;
		}

		// out remainder
		str.append( text, width );
	}
	return ( Output( str ) );
}

bool Context :: OutputPrivate( const char* data, int len )
{
	bool success = true;

	// write it out a line at a time
	const char* found;
	const char* iter = data;
	gpstring local;
	while ( (found = ::strnchr( iter, '\n', len )) != NULL )
	{
		// remember where we found it
		const char* end = found;

		// it may be preceded by \r
		if ( (found != iter) && (found[ -1 ] == '\r') )
		{
			--end;
		}

		// advance now before iter changes
		++found;
		len -= found - iter;

		// find and replace \t characters - only if necessary
		if ( !TestOptions( OPTION_IGNORETABS ) )
		{
			const char* tab = ::strnchr( iter, '\t', end - iter );
			if ( tab != NULL )
			{
				// damn - have to do local replacement
				local.assign( iter, end - iter );
				stringtool::ConvertTabsToSpaces( local, GetIndentSpaces(), m_Column );
				iter = local.begin();
				end  = local.end();
			}
		}

		// write out line with terminator
		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OutputString( this, iter, end - iter, !IsLineOpen(), true );
		}
		if ( !m_Sink->OutputString( this, iter, end - iter, !IsLineOpen(), true ) )
		{
			success = false;
		}
		m_Column = 1;

		// next
		iter = found;
	}

	// write out remainder if any
	if ( *iter != '\0' )
	{
		// find and replace \t characters - only if necessary
		if ( !TestOptions( OPTION_IGNORETABS ) )
		{
			const char* tab = ::strnchr( iter, '\t', len );
			if ( tab != NULL )
			{
				// damn - have to do local replacement
				local.assign( iter, len );
				stringtool::ConvertTabsToSpaces( local, GetIndentSpaces(), m_Column );
				iter = local.begin();
				len  = scast <int> ( local.size() );
			}
		}

		// write out portion
		if ( !TestOptions( OPTION_NOGLOBAL ) )
		{
			gGlobalSink.OutputString( this, iter, len, !IsLineOpen(), false );
		}
		if ( !m_Sink->OutputString( this, iter, len, !IsLineOpen(), false ) )
		{
			success = false;
		}
		m_Column += len;
	}

	// done
	return ( success );
}

bool Context :: AddReport( bool fromBegin )
{
	// only inc the count if:
	//
	// (a) we're a non-autoreporting context and we're not inside of BeginReport
	// (b) we're an autoreporting context and we're in the first BeginReport

	if ( TestOptions( OPTION_AUTOREPORT ) == fromBegin )
	{
		++m_ReportCount;
		if ( TestTraits( TRAIT_WARNING ) )
		{
			IncWarningCount();
		}
		else if ( TestTraits( TRAIT_ERROR ) )
		{
			IncErrorCount();
		}
	}

	return ( IsEnabled() );
}

void Context :: Init( eTraits traits, Context* prototype, bool isLocal )
{
	// formatting
	m_Schema       = NULL;
	m_OwnsSchema   = false;
	m_Indent       = prototype ? prototype->m_Indent       : 0;
	m_IndentSpaces = prototype ? prototype->m_IndentSpaces : DEFAULT_INDENT_SPACES;
	m_Color        = prototype ? prototype->m_Color        : 0;
	m_ColorDefault = prototype ? prototype->m_ColorDefault : true;
	m_LocationFile = prototype ? prototype->m_LocationFile : NULL;
	m_LocationLine = prototype ? prototype->m_LocationLine : 0;

	// state
	m_Options      = OPTION_NONE;							 // $ options are not copied over
	m_Traits       = prototype ? prototype->m_Traits       : traits;
	m_Id           = ms_NextId++;
	m_Column       = prototype ? prototype->m_Column       : 1;
	m_InTable      = false;
	m_RowIndex     = 0;
	m_ReportLevel  = prototype ? prototype->m_ReportLevel  : 0;
	m_ReportCount  = 0;
	m_FieldIndex   = 0;
	m_Type         = prototype ? prototype->m_Type         : "";

	// schema
	if ( (prototype != NULL) && (prototype->m_Schema != NULL) )
	{
		if ( prototype->m_OwnsSchema )
		{
			m_Schema = new Schema( *prototype->m_Schema );
			m_OwnsSchema = true;
		}
		else
		{
			m_Schema = prototype->m_Schema;
		}
	}

	// output
	m_Sink = NULL;
	m_OwnsSink = false;

	// enabling by build
#	if GP_DEBUG
	m_Enabled = true;
#	endif
#	if GP_RELEASE
	m_Enabled = !TestTraits( TRAIT_DEBUG );
#	endif // GP_RELEASE
#	if GP_RETAIL
	m_Enabled = !TestTraits( TRAIT_DEBUG | TRAIT_DEV );
#	endif

	// global
	if ( isLocal )
	{
		m_Options |= OPTION_NOGLOBAL;
	}
	else
	{
		gGlobalSink.AddContextRef( this );
	}
}

void Context :: DestroySink( bool autoDeleteOld )
{
	if ( m_Sink != NULL )
	{
		m_Sink->RemoveContextRef( this );
		if ( m_OwnsSink && autoDeleteOld )
		{
			delete ( m_Sink );
		}
		m_Sink = NULL;
	}
}

static const char* s_ContextOptionStrings[] =
{
	"AutoReport",
	"NoGlobal",
	"Localize",
};

static const char* s_ContextTraitStrings[] =
{
	"Debug",
	"Dev",
	"Warning",
	"Error",
};

template <typename ENUM>
gpstring ToString( ENUM e, const char* delim, const char** strings, int size )
{
	UNREFERENCED_PARAMETER( size );
	gpassert( delim != NULL );

	gpstring str;

	for (  ENUM i = FindFirstEnum( e ) ; i != 0 ; i = FindNextEnum( i, e ) )
	{
		if ( !str.empty() )
		{
			str += delim;
		}
		int si = GetShift( i );
		gpassert( (si >= 0) && (si < size) );
		str += strings[ si ];
	}
	return ( str );
}

gpstring ToString( Context::eOptions options, const char* delim )
{
	return ( ToString( options, delim, s_ContextOptionStrings, ELEMENT_COUNT( s_ContextOptionStrings ) ) );
}

gpstring ToString( Context::eTraits traits, const char* delim )
{
	return ( ToString( traits, delim, s_ContextTraitStrings, ELEMENT_COUNT( s_ContextTraitStrings ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class Mgr implementation

Mgr :: Mgr( void )
{
	m_RegisteredDefaults = false;
}

Mgr :: ~Mgr( void )
{
	// this space intentionally left blank...
}

void Mgr :: RegisterSink( const char* name, Sink* sink )
{
	m_SinkMap.Register( name, sink );
}

Sink* Mgr :: FindSink( const char* name ) const
{
	return ( m_SinkMap.Find( name ) );
}

void Mgr :: UnregisterSink( const char* name )
{
	m_SinkMap.Unregister( name );
}

void Mgr :: UnregisterSink( Sink* sink )
{
	m_SinkMap.Unregister( sink );
}

void Mgr :: RegisterSinkFactory( const char* type, SinkFactoryProc proc )
{
	gpassert( proc != NULL );
	gpassert( (type != NULL) && (*type != '\0') );
	gpassert( m_SinkFactoryProcMap.find( type ) == m_SinkFactoryProcMap.end() );

	m_SinkFactoryProcMap[ type ] = proc;
}

void Mgr :: UnregisterSinkFactory( const char* type )
{
	gpassert( (type != NULL) && (*type != '\0') );

	SinkFactoryProcMap::iterator found = m_SinkFactoryProcMap.find( type );
	gpassert( found != m_SinkFactoryProcMap.end() );
	m_SinkFactoryProcMap.erase( found );
}

void Mgr :: EnableSink( const char* name, bool enable )
{
	gpassert( name != NULL );

	Sink* sink = FindSink( name );
	if ( sink != NULL )
	{
		sink->Enable( enable );
	}
}

Sink* Mgr :: CreateSink( const char* name, const char* type, const char* params )
{
	gpassert( (type != NULL) && (*type != '\0') );

	Sink* sink = NULL;

	// auto register if not done already
	if ( !m_RegisteredDefaults )
	{
		m_RegisteredDefaults = true;
		for ( SinkFactoryRegistrar* i = SinkFactoryRegistrar::ms_Root ; i != NULL ; i = i->m_Next )
		{
			RegisterSinkFactory( i->m_Name, i->m_Data );
		}
	}

	// ok for it not to exist
	SinkFactoryProcMap::iterator found = m_SinkFactoryProcMap.find( type );
	if ( found != m_SinkFactoryProcMap.end() )
	{
		sink = (*found->second)( name, found->first, params );
	}

	// done
	return ( sink );
}

void Mgr :: RegisterContext( const char* name, Context* context )
{
	m_ContextMap.Register( name, context );
}

Context* Mgr :: FindContext( const char* name ) const
{
	return ( m_ContextMap.Find( name ) );
}

void Mgr :: UnregisterContext( const char* name )
{
	m_ContextMap.Unregister( name );
}

void Mgr :: UnregisterContext( Context* context )
{
	m_ContextMap.Unregister( context );
}

void Mgr :: EnableContext( const char* name, bool enable )
{
	gpassert( name != NULL );

	Context* context = FindContext( name );
	if ( context != NULL )
	{
		context->Enable( enable );
	}
}

int Mgr :: QueryContextByTraits( ContextColl& coll, Context::eTraits traits )
{
	ContextMap::ForwardMap::const_iterator i,
										   begin = m_ContextMap.m_ForwardMap.begin(),
										   end   = m_ContextMap.m_ForwardMap.end();
	int found = 0;
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->second->TestTraits( traits ) )
		{
			coll.push_back( i->second );
			++found;
		}
	}
	return ( found );
}

int Mgr :: QueryContextByName( ContextColl& coll, const char* nameWildcard )
{
	gpassert( nameWildcard != NULL );

	ContextMap::ForwardMap::const_iterator i,
										   begin = m_ContextMap.m_ForwardMap.begin(),
										   end   = m_ContextMap.m_ForwardMap.end();
	int found = 0;
	for ( i = begin ; i != end ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( nameWildcard, i->second->GetName() ) )
		{
			coll.push_back( i->second );
			++found;
		}
	}
	return ( found );
}

int Mgr :: QueryContextByType( ContextColl& coll, const char* typeWildcard )
{
	gpassert( typeWildcard != NULL );

	ContextMap::ForwardMap::const_iterator i,
										   begin = m_ContextMap.m_ForwardMap.begin(),
										   end   = m_ContextMap.m_ForwardMap.end();
	int found = 0;
	for ( i = begin ; i != end ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( typeWildcard, i->second->GetType() ) )
		{
			coll.push_back( i->second );
			++found;
		}
	}
	return ( found );
}

int Mgr :: QuerySinkByName( SinkColl& coll, const char* nameWildcard )
{
	gpassert( nameWildcard != NULL );

	SinkMap::ForwardMap::const_iterator i,
										begin = m_SinkMap.m_ForwardMap.begin(),
										end   = m_SinkMap.m_ForwardMap.end();
	int found = 0;
	for ( i = begin ; i != end ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( nameWildcard, i->second->GetName() ) )
		{
			coll.push_back( i->second );
			++found;
		}
	}
	return ( found );
}

int Mgr :: QuerySinkByType( SinkColl& coll, const char* typeWildcard )
{
	gpassert( typeWildcard != NULL );

	SinkMap::ForwardMap::const_iterator i,
										begin = m_SinkMap.m_ForwardMap.begin(),
										end   = m_SinkMap.m_ForwardMap.end();
	int found = 0;
	for ( i = begin ; i != end ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( typeWildcard, i->second->GetType() ) )
		{
			coll.push_back( i->second );
			++found;
		}
	}
	return ( found );
}


const char* Mgr :: TranslateA( const char* seaText )
{
	gpassert( seaText != NULL );
	if( *seaText == '\0' )
	{
		return seaText;
	}

	// perform translation
	TranslateMap::const_iterator found = m_TranslateMap.find( seaText );
	if ( found != m_TranslateMap.end() )
	{
		return ( found->second );
	}
	else if ( m_TranslateMap.empty() )
	{
		return ( seaText );
	}

	// hmm, not found, perhaps it has embedded plaintext \n?
	char* substr = ::strstr( seaText, "\\n" );
	if ( substr != NULL )
	{
		// ok copy local
		int len = ::strlen( seaText );
		char* localText = (char*)_alloca( len + 1 );
		::memcpy( localText, seaText, len + 1 );

		// update \\n to be \r\n
		substr = localText + (substr - seaText);
		while ( substr != NULL )
		{
			substr[ 0 ] = '\r';
			substr[ 1 ] = '\n';
			substr = ::strstr( substr + 1, "\\n" );
		}

		// find again
		return ( TranslateA( localText ) );
	}

	// what? no entry?
	gperrorf(( "Error translating string - could not find foreign language "
			   "version of '%s'!\n", stringtool::EscapeCharactersToTokens( seaText ).c_str() ));

	// just return our original text, screwit
	return ( seaText );
}

gpwstring Mgr :: TranslateW( const char* seaText )
{
	return ( ::ToUnicode( TranslateA( seaText ) ) );
}

gpwstring Mgr :: TranslateW( const wchar_t* seaText )
{
	return ( TranslateW( ::ToAnsi( seaText ) ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
