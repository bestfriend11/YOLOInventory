//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritSupport.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"

#include "DebugHelp.h"
#include "FileSysXfer.h"
#include "Filter_1.h"
#include "FuBi.h"
#include "FuBiSchema.h"
#include "FuBiTraits.h"
#include "Quat.h"
#include "SkritEngine.h"
#include "SkritStructure.h"
#include "SkritSupport.h"
#include "StringTool.h"
#include "RapiOwner.h"
#include "ReportSys.h"
#include "vector_3.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_REPLACE_NAME( "ReportSys", Report );
FUBI_REPLACE_NAME( "String", StringTool );

//////////////////////////////////////////////////////////////////////////////
// class Debug implementation

Debug :: Debug( void )
{
	m_Buffer[ 0 ] = '\0';
}

Debug :: ~Debug( void )
{
	// this space intentionally left blank...
}

const char* Debug :: Format( const char* format, ... )
{
	::dynamic_vsnprintf( m_Buffer, ELEMENT_COUNT( m_Buffer ), NULL, format, va_args( format ) );
	return ( m_Buffer );
}

void Debug :: Assert( int testExpr )
{
	// $$$ make it use file and line from the current executing skrit - be sure
	//     to set 'localStorage' to TRUE in gpassert_box().
	gpassert_raw( testExpr, "", "Assertion (SkritSupport.Debug)" );
}

void Debug :: Assert( int testExpr, const char* msg )
{
	// $$$ make it use file and line from the current executing skrit - be sure
	//     to set 'localStorage' to TRUE in gpassert_box().
	gpassert_raw( testExpr, msg, "Assertion (SkritSupport.Debug)" );
}

void Debug :: AssertF( int testExpr, const char* format, ... )
{
	if ( !testExpr )
	{
		// $$$ make it use file and line from the current executing skrit - be
		//     sure to set 'localStorage' to TRUE in gpassert_box().
		auto_dynamic_vsnprintf printer( format, va_args( format ), false );
		gpassert_raw( testExpr, printer, "Assertion (SkritSupport.Debug)" );
	}
}

void Debug :: Breakpoint( void )
{
	BREAKPOINT();
}

#if GPSTRING_TRACKING

static void WriteString( FileSys::File& file, const char* str )
{
	for ( ; ; )
	{
		const char* begin = str;
		while ( (*str != '\0') && !stringtool::IsEscapeCharacter( *str ) )
		{
			++str;
		}
		if ( str != begin )
		{
			file.Write( begin, str - begin );
		}
		if ( *str != '\0' )
		{
			char esc[ 2 ] = { '\\', stringtool::GetEscapeCharacterLetter( *str ) };
			file.Write( esc, 2 );
			++str;
		}
		else
		{
			break;
		}
	}
}

void Debug :: DumpStrings( void )
{
	FileSys::File file;
	file.CreateAlways( "strings.log" );

	file.WriteText(  "*** Dumping gpstrings... ***\n\n" );
	{
		for ( const gpstring::Header* i = gpstring::Header::GetRoot().m_Next ; i != &gpstring::Header::GetRoot() ; i = i->m_Next )
		{
			file.WriteF( "%04d: ", i->m_RefCount );
			WriteString( file, i->c_str() );
			file.WriteText( "\n" );
		}
	}

	file.WriteText( "\n\n*** Dumping gpwstrings... ***\n\n" );
	{
		for ( const gpwstring::Header* i = gpwstring::Header::GetRoot().m_Next ; i != &gpwstring::Header::GetRoot() ; i = i->m_Next )
		{
			file.WriteF( "%04d: ", i->m_RefCount );
			file.WriteF( "%S\n", i->c_str() );
		}
	}

	file.WriteText( "\n\n*** Dumping gpdumbstrings... ***\n\n" );
	{
		for ( const gpdumbstring::Header* i = gpdumbstring::Header::GetRoot().m_Next ; i != &gpdumbstring::Header::GetRoot() ; i = i->m_Next )
		{
			WriteString( file, i->c_str() );
			file.WriteText( "\n" );
		}
	}

#	if GPSTRING_LOG_DELETES

	file.WriteText( "\n\n*** Dumping retired ANSI strings... ***\n\n" );

	for2 (  std::TrackingStringHeaderBase::StringAColl::const_iterator i = std::TrackingStringHeaderBase::GetRetiredStringsA().begin()
		  ; i != std::TrackingStringHeaderBase::GetRetiredStringsW().end()
		  ; ++i )
	{
		WriteString( file, *i );
		file.WriteText( "\n" );
	}

	file.WriteText( "\n\n*** Dumping retired UNICODE strings... ***\n\n" );

	for2 (  std::TrackingStringHeaderBase::StringAColl::const_iterator i = std::TrackingStringHeaderBase::GetRetiredStringsW().begin()
		  ; i != std::TrackingStringHeaderBase::GetRetiredStringsW().end()
		  ; ++i )
	{
		file.WriteF( "%S\n", *i );
	}

#	endif // GPSTRING_LOG_DELETES
}

#endif // GPSTRING_TRACKING

#if !GP_ERROR_FREE

void Debug :: DumpGameStack( void )
{
	DbgSymbolEngine::DumpGameStack();
}

#endif // !GP_ERROR_FREE

//////////////////////////////////////////////////////////////////////////////
// class Help implementation

#if !GP_RETAIL

Help :: Help( void )
{
	// this space intentionally left blank...
}

Help :: ~Help( void )
{
	// this space intentionally left blank...
}

void Help :: All( void )
{
	gpgeneric( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				"Help for all of SiegeSkrit:\n\n" );
	gFuBiSysExports.GenerateDocs();
	Enums( true );
}

void Help :: Classes( bool includeHidden )
{
	// dump objects into sink
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext ctx( &sink, false );
	ctx.SetSchema( new ReportSys::Schema( 6, "Name", "M", "V", "P", "Flags", "Documentation" ), true );

	// classes
	FuBi::ClassIndex::const_iterator i,
									 begin = gFuBiSysExports.GetClassBegin(),
									 end   = gFuBiSysExports.GetClassEnd();
	for ( i = begin ; i != end ; ++i )
	{
		const FuBi::ClassSpec* cls = *i;
		if ( includeHidden || !(cls->m_Flags & FuBi::ClassSpec::FLAG_HIDDEN) )
		{
			// name
			ReportSys::AutoReport autoReport( ctx );
			if ( cls->m_Flags & FuBi::ClassSpec::FLAG_HIDDEN )
			{
				gpstring clsName( cls->m_Name );
				clsName += '*';
				ctx.OutputField( clsName );
			}
			else
			{
				ctx.OutputField( cls->m_Name );
			}

			// function count
			ctx.OutputFieldObj( cls->m_Functions.size() );

			// variable count
			ctx.OutputFieldObj( cls->m_Variables.size() );

			// property count
			ctx.OutputFieldObj( (cls->m_HeaderSpec != NULL) ? cls->m_HeaderSpec->GetColumnCount() : 0 );

			// flags
			gpstring flags;
			flags += (cls->m_Flags & FuBi::ClassSpec::FLAG_MANAGED    ) ? 'M' : ' ';
			flags += (cls->m_Flags & FuBi::ClassSpec::FLAG_SINGLETON  ) ? 'S' : ' ';
			flags += (cls->m_Flags & FuBi::ClassSpec::FLAG_CANRPC     ) ? 'R' : ' ';
			flags += (cls->m_VarTypeSpec != NULL                      ) ? 'T' : ' ';
			flags += (cls->m_Flags & FuBi::ClassSpec::FLAG_HIDDEN     ) ? 'H' : ' ';
			flags += (cls->m_Flags & FuBi::ClassSpec::FLAG_POD        ) ? 'P' : ' ';
			ctx.OutputField( flags );

			// docs
			ctx.OutputField( cls->m_Docs ? cls->m_Docs : "" );
		}
	}

	// out header
	gpgeneric( "\n" );
	gGenericContext.OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
	gpgeneric( "\n\nClasses:\n\n" );

	// out table
	sink.Sort();
	sink.OutputReport();

	// out legend
	gpgeneric( "\nCounts: M = methods, V = variables, P = properties\n" );
	gpgeneric( "\nFlags: M = managed, S = singleton, R = rpc, T = traits, H = hidden, P = plain old data\n\n" );
}

void Help :: Enums( bool includeConstants )
{
	gpgeneric( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				"Enumerations:\n\n" );

	FuBi::EnumIndex::const_iterator i,
			begin = gFuBiSysExports.GetEnumBegin(), end = gFuBiSysExports.GetEnumEnd();
	for ( i = begin ; i != end ; ++i )
	{
		ReportSys::AutoIndent autoIndent( gGenericContext );
		gpgenericf(( "%s\n", (*i)->m_Name.c_str() ));
		if ( includeConstants )
		{
			ReportSys::AutoIndent autoIndent( gGenericContext );
			Enum( (*i)->m_Name );
		}
	}

	gpgeneric( "\n" );
}

void Help :: Globals( void )
{
	gpgeneric( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				   "Global Functions:\n\n" );

	FuBi::FunctionByNameIndex::const_iterator i,
			ibegin = gFuBiSysExports.GetGlobalsBegin(), iend = gFuBiSysExports.GetGlobalsEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBi::FunctionIndex::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			if ( !((*j)->m_Flags & FuBi::FunctionSpec::FLAG_HIDDEN) )
			{
				gpgenericf(( "    %s\n", (*j)->m_Name.c_str() ));
				break;
			}
		}
	}

	gpgeneric( "\n" );
}

void Help :: Class( const char* className )
{
	FuBi::eVarType type = gFuBiSysExports.FindType( className );
	if ( FuBi::IsUser( type ) )
	{
		const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
		spec->GenerateDocs();
	}
	else
	{
		gpwarningf(( "help.class: '%s' is not a system class\n", className ));
	}
}

void Help :: Global( const char* globalName )
{
	const FuBi::FunctionIndex* globals = gFuBiSysExports.FindGlobalFunction( globalName );
	if ( globals != NULL )
	{
		FuBi::FunctionIndex::const_iterator i, begin = globals->begin(), end = globals->end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( !((*i)->m_Flags & FuBi::FunctionSpec::FLAG_HIDDEN) )
			{
				(*i)->GenerateDocs( NULL, FuBi::DOCS_VERBOSE );
				gpgeneric( "\n" );
			}
		}
	}
	else
	{
		gpwarningf(( "help.global: '%s' is not a global function\n", globalName ));
	}
}

void Help :: Enum( const char* enumName )
{
	FuBi::eVarType type = gFuBiSysExports.FindType( enumName );
	if ( FuBi::IsEnum( type ) )
	{
		const FuBi::EnumSpec* spec = gFuBiSysExports.FindEnum( type );
		if ( spec->m_Exporter.IsContinuous() )
		{
			DWORD i, begin = spec->m_Exporter.m_Begin, end = begin + spec->m_Exporter.m_Count;
			for ( i = begin ; i != end ; ++i )
			{
				gpgenericf(( "%s = %d\n", (*spec->m_Exporter.m_EnumToStringProc)( i ), i ));
			}
		}
		else if ( spec->m_Exporter.HasConstants() )
		{
			gpwarningf(( "help.enum: '%s' is a discontinuous enumeration, dumping not supported yet, sorry!\n", enumName ));
		}
		else
		{
			gpwarningf(( "help.enum: '%s' does not have its constants exported and is unavailable for dumping\n", enumName ));
		}
	}
	else
	{
		gpwarningf(( "help.enum: '%s' is not an enumeration\n", enumName ));
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class VTune implementation

#if GP_ENABLE_PROFILING

VTune :: ~VTune( void )
{
	// this space intentionally left blank...
}

bool VTune :: Pause( void )
{
	if ( !m_VTuneAPI.Load() )  return ( false );
	return ( !!m_VTuneAPI.VtPauseSampling() );
}

bool VTune :: Resume( void )
{
	if ( !m_VTuneAPI.Load() )  return ( false );
	return ( !!m_VTuneAPI.VtResumeSampling() );
}

#endif // GP_ENABLE_PROFILING

//////////////////////////////////////////////////////////////////////////////
// class VTune::VTuneAPI implementation

#if GP_ENABLE_PROFILING

VTune::VTuneAPI :: VTuneAPI( void )
	: DllBinder( "VTUNEAPI.DLL" )
{
#	define ADD_ENTRY( f )  AddProc( &f, #f )

	ADD_ENTRY( VtPauseSampling );
	ADD_ENTRY( VtResumeSampling );

#	undef ADD_ENTRY
}

#endif // GP_ENABLE_PROFILING

//////////////////////////////////////////////////////////////////////////////
// namespace Skrit implementation

void Skrit :: Execute( const char* skritName, const char* funcName )
{
	Skrit::HAutoObject object = gSkritEngine.CreateObject( skritName );
	if ( !object.IsNull() )
	{
		int funcIndex = object->FindFunction( funcName );
		if ( funcIndex != INVALID_FUNCTION_INDEX )
		{
			const Raw::ExportEntry* funcSpec = object->GetImpl()->GetFunctionSpec( funcIndex );
			if ( funcSpec->m_ParamCount == 0 )
			{
				object->Call( funcIndex );
			}
			else
			{
				gpwarningf(( "Cannot execute function '%s' in Skrit '%s' - it takes parameters!\n", funcName, skritName ));
			}
		}
		else
		{
			gpwarningf(( "Could not find function '%s' in Skrit '%s'\n", funcName, skritName ));
		}
	}
	else
	{
		gpwarningf(( "Could not compile object '%s'\n", skritName ));
	}
}

void Skrit :: Execute( const char* skritName )
{
	Skrit::HAutoObject object = gSkritEngine.CreateObject( skritName );
	if ( !object.IsNull() )
	{
		if ( object->HasFunctions() )
		{
			const Raw::ExportEntry* funcSpec = object->GetImpl()->GetFunctionSpec( 0 );
			if ( funcSpec->m_ParamCount == 0 )
			{
				object->Call( 0 );
			}
			else
			{
				gpwarningf(( "Cannot execute function '%s' in Skrit '%s' - it takes parameters!\n", funcSpec->m_Name, skritName ));
			}
		}
		else
		{
			gpwarningf(( "No functions exist in Skrit '%s'\n", skritName ));
		}
	}
	else
	{
		gpwarningf(( "Could not compile object '%s'\n", skritName ));
	}
}

void Skrit :: Command( const char* command )
{
	gSkritEngine.ExecuteCommand( "<skritsupport command>", command );
}

#if !GP_RETAIL

void Skrit :: Dump( const char* skritName )
{
	Skrit::HAutoObject object = gSkritEngine.CreateObject( skritName );
	if ( !object.IsNull() )
	{
		object->GenerateDocs();
	}
	else
	{
		gpwarningf(( "Could not compile object '%s'\n", skritName ));
	}
}

void Skrit :: Disassemble( const char* skritName )
{
	Skrit::HAutoObject object = gSkritEngine.CreateObject( skritName );
	if ( !object.IsNull() )
	{
		object->Disassemble();
	}
	else
	{
		gpwarningf(( "Could not compile object '%s'\n", skritName ));
	}
}

void Skrit :: CheckFile( const char* skritName )
{
	Skrit::HAutoObject object = gSkritEngine.CreateObject( skritName );
	if ( object.IsNull() )
	{
		gpwarningf(( "There were errors compiling object '%s'\n", skritName ));
	}
}

void Skrit :: CheckCommand( const char* command )
{
	std::auto_ptr <Object> object( gSkritEngine.CreateCommand( "Command", command ) );
	if ( object.get() == NULL )
	{
		gpwarning( "There were errors compiling the command\n" );
	}
}

void Skrit :: DumpCommand( const char* command )
{
	std::auto_ptr <Object> object( gSkritEngine.CreateCommand( "Command", command ) );
	if ( object.get() != NULL )
	{
		object->GenerateDocs();
	}
	else
	{
		gpwarning( "There were errors compiling the command\n" );
	}
}

void Skrit :: DisassembleCommand( const char* command )
{
	std::auto_ptr <Object> object( gSkritEngine.CreateCommand( "Command", command ) );
	if ( object.get() != NULL )
	{
		FileSys::MemReader reader( const_mem_ptr( command, ::strlen( command ) ) );
		object->Disassemble( &gGenericContext, &reader );
	}
	else
	{
		gpwarning( "There were errors compiling the command\n" );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// namespace FuBi implementation

#if !GP_RETAIL
void FuBi :: DumpPrototypes( void )
{
	gpgeneric( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
				"Dumping FuBi prototypes:\n\n" );

	for ( int i = 0, count = gFuBiSysExports.GetFunctionCount() ; i != count ; ++i )
	{
		const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( i );
		gpassert( spec != NULL );

		gpgenericf(( "Serial %04X: ", i ));
		spec->GenerateDocs( NULL, FuBi::DOCS_MINIMAL );
		gpgenericf(( "\n             %s\n             %s\n\n",
					 spec->m_UnmangledName.c_str(), spec->m_MangledName ));
	}
}
#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// namespace Math implementation

float Math :: RandomFloat( float minFloat, float maxFloat )
{
	return ( Random( minFloat, maxFloat ) );
}

float Math :: RandomFloat( float maxFloat )
{
	return ( Random( maxFloat ) );
}

int Math :: RandomInt( int minInt, int maxInt )
{
	return ( Random( minInt, maxInt ) );
}

int Math :: RandomInt( int maxInt )
{
	return ( Random( maxInt ) );
}

float Math :: Sin( float f )
{
	return ( SINF( f ) );
}

float Math :: Cos( float f )
{
	return ( COSF( f ) );
}

float Math :: Abs( float f )
{
	return ( FABSF( f ) );
}

int Math :: Round( float f )
{
	return ( ::Round( f ) );
}

float Math :: RoundToFloat( float f )
{
	return ( ::RoundToFloat( f ) );
}

float Math :: Floor( float f )
{
	return ( floorf( f ) );
}

float Math :: Ceil( float f )
{
	return ( ceilf( f ) );
}

float Math :: RadiansToDegrees( float f )
{
	return ( ::RadiansToDegrees( f ) );
}

float Math :: DegreesToRadians( float f )
{
	return ( ::DegreesToRadians( f ) );
}

float Math :: Length( const vector_3& v )
{
	return ( ::Length( v ) );
}

float Math :: Length2( const vector_3& v )
{
	return ( ::Length2( v ) );
}

int Math :: MaxInt( int a, int b )
{
	return( max( a, b ) );
}

int Math :: MinInt( int a, int b )
{
	return( min( a, b ) );
}

float Math :: MaxFloat( float a, float b )
{
	return( max( a, b ) );
}

float Math :: MinFloat( float a, float b )
{
	return( min( a, b ) );
}

float Math :: FilterSmoothStep( float a, float b, float x )
{
	return ( ::FilterSmoothStep( a, b, x ) );
}

float Math :: FilterSmoothStep( float x )
{
	return ( ::FilterSmoothStep( 0, 1, x ) );
}

float Math :: Pi( void )
{
	return ( PI );
}

float Math :: PiHalf( void )
{
	return ( PIHALF );
}

int Math :: ToInt( float f )
{
	return ( (int)f );
}

float Math :: ToFloat( int i )
{
	return ( (float)i );
}

float Math :: FromPercent( float f )
{
	return ( f * 0.01f );
}

//////////////////////////////////////////////////////////////////////////////
// class Report implementation

void Report :: FUBI_RENAME( ReportF )( ReportSys::Context* context, const char* format, ... )
{
#	if !GP_RETAIL
	gpassert( context != NULL );
	ReportSys::OutputArgs( context, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: FUBI_RENAME( Report )( ReportSys::Context* context, const char* msg )
{
#	if !GP_RETAIL
	gpassert( context != NULL );
	ReportSys::Output( context, msg );
#	endif // !GP_RETAIL
}

void Report :: FUBI_RENAME( ReportF )( const char* contextName, const char* format, ... )
{
#	if !GP_RETAIL
	gpassert( contextName != NULL );
	ReportSys::OutputArgs( contextName, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: FUBI_RENAME( Report )( const char* contextName, const char* msg )
{
#	if !GP_RETAIL
	gpassert( contextName != NULL );
	ReportSys::Output( contextName, msg );
#	endif // !GP_RETAIL
}

void Report :: GenericF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gGenericContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: Generic( const char* msg )
{
#	if !GP_RETAIL
	gpgeneric( msg );
#	endif // !GP_RETAIL
}

void Report :: PerfF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gPerfContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: Perf( const char* msg )
{
#	if !GP_RETAIL
	gpperf( msg );
#	endif // !GP_RETAIL
}

void Report :: PerfLogF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gPerfLogContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: PerfLog( const char* msg )
{
#	if !GP_RETAIL
	gpperflog( msg );
#	endif // !GP_RETAIL
}

void Report :: TestLogF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gTestLogContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: TestLog( const char* msg )
{
#	if !GP_RETAIL
	gptestlog( msg );
#	endif // !GP_RETAIL
}

void Report :: MessageBoxF( const char* format, ... )
{
	ReportSys::OutputArgs( gMessageBoxContext, format, va_args( format ) );
}

void Report :: MessageBox( const char* msg )
{
	gpmessagebox( msg );
}

void Report :: ErrorBoxF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gErrorBoxContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: ErrorBox( const char* msg )
{
#	if !GP_RETAIL
	gperrorbox( msg );
#	endif // !GP_RETAIL
}

void Report :: SScreenF( const char* format, ... )
{
	CHECK_SERVER_ONLY;
	RCScreen( RPC_TO_ALL, auto_dynamic_vsnprintf( format, va_args( format ) ) );
}

void Report :: SScreen( const char* msg )
{
	CHECK_SERVER_ONLY;
	RCScreen( RPC_TO_ALL, msg );
}

void Report :: SScreenF( DWORD machineId, const char* format, ... )
{
	CHECK_SERVER_ONLY;
	RCScreen( machineId, auto_dynamic_vsnprintf( format, va_args( format ) ) );
}

void Report :: SScreen( DWORD machineId, const char* msg )
{
	CHECK_SERVER_ONLY;
	RCScreen( machineId, msg );
}

void Report :: RCScreen( DWORD machineId, const char* msg )
{
	FUBI_RPC_CALL( RCScreen, machineId );
	Screen( msg );
}

void Report :: ScreenF( const char* format, ... )
{
	ReportSys::OutputArgs( gScreenContext, format, va_args( format ) );
}

void Report :: Screen( const char* msg )
{
	gpscreen( msg );
}

void Report :: DebuggerF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gDebuggerContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: Debugger( const char* msg )
{
#	if !GP_RETAIL
	gpdebugger( msg );
#	endif // !GP_RETAIL
}

void Report :: WarningF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gWarningContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: Warning( const char* msg )
{
#	if !GP_RETAIL
	gpwarning( msg );
#	endif // !GP_RETAIL
}

void Report :: ErrorF( const char* format, ... )
{
#	if !GP_RETAIL
	ReportSys::OutputArgs( gErrorContext, format, va_args( format ) );
#	endif // !GP_RETAIL
}

void Report :: Error( const char* msg )
{
#	if !GP_RETAIL
	gperror( msg );
#	endif // !GP_RETAIL
}

void Report :: FatalF( const char* format, ... )
{
	ReportSys::OutputArgs( gFatalContext, format, va_args( format ) );
}

void Report :: Fatal( const char* msg )
{
	gpfatal( msg );
}

void Report :: Fatal( void )
{
	gpfatal( "" );
}

void Report :: Enable( const char* context )
{
	ReportSys::EnableContext( context );
}

void Report :: Disable( const char* context )
{
	ReportSys::EnableContext( context, false );
}

void Report :: Toggle( const char* context )
{
	ReportSys::ToggleContext( context );
}

bool Report :: IsEnabled( const char* context )
{
	return ( ReportSys::IsContextEnabled( context ) );
}

void Report :: AddNewSinkToContext( const char* context, const char* name, const char* sinkFactory, const char* sinkParams )
{
#	if !GP_RETAIL
	ReportSys::Context* ctx = gReportSysMgr.FindContext( context );
	if ( ctx != NULL )
	{
		gpstring localName;
		if ( (name != NULL) && (*name != '\0') )
		{
			localName = name;
		}
		else
		{
			localName = ReportSys::Sink::GenerateName();
		}

		if ( sinkParams == NULL )
		{
			sinkParams = "";
		}

		ReportSys::Sink* newSink = gReportSysMgr.CreateSink( localName, sinkFactory, sinkParams );
		if ( newSink != NULL )
		{
			ctx->AddSink( newSink, true );
		}
		else
		{
			gperrorf(( "Sink factory '%s' does not exist\n", sinkFactory ));
		}
	}
	else
	{
		gperrorf(( "Context '%s' does not exist\n", context ));
	}
#	endif // !GP_RETAIL
}

void Report :: AddNewSinkToContext( const char* context, const char* name, const char* sinkFactory )
{
#	if !GP_RETAIL
	AddNewSinkToContext( context, name, sinkFactory, NULL );
#	endif // !GP_RETAIL
}

void Report :: AddNewSinkToContext( const char* context, const char* sinkFactory )
{
#	if !GP_RETAIL
	AddNewSinkToContext( context, NULL, sinkFactory );
#	endif // !GP_RETAIL
}

void Report :: AddNewSinkToSink( const char* sink, const char* name, const char* sinkFactory )
{
#	if !GP_RETAIL
	ReportSys::Sink* snk = gReportSysMgr.FindSink( sink );
	if ( snk != NULL )
	{
		gpstring localName;
		if ( (name != NULL) && (*name != '\0') )
		{
			localName = name;
		}
		else
		{
			localName = ReportSys::Sink::GenerateName();
		}
		ReportSys::Sink* newSink = gReportSysMgr.CreateSink( localName, sinkFactory );
		if ( newSink != NULL )
		{
			snk->AddSink( newSink, true );
		}
		else
		{
			gperrorf(( "Sink factory '%s' does not exist\n", sinkFactory ));
		}
	}
	else
	{
		gperrorf(( "Sink '%s' does not exist\n", sink ));
	}
#	endif // !GP_RETAIL
}

void Report :: AddNewSinkToSink( const char* sink, const char* sinkFactory )
{
#	if !GP_RETAIL
	AddNewSinkToSink( sink, NULL, sinkFactory );
#	endif // !GP_RETAIL
}

const char* Report :: TranslateMsg( const char* message )
{
	return ( ReportSys::TranslateMsgA( message ) );
}

const char* Report :: Translate( const char* message )
{
	return ( ReportSys::TranslateA( message ) );
}

void Report :: DumpFile( const char* fileName )
{
#	if !GP_RETAIL
	FileSys::AutoFileHandle file;
	FileSys::AutoMemHandle mem;

	if ( file.Open( fileName ) && mem.Map( file ) )
	{
		ReportSys::AutoReport autoReport( gGenericContext );
		gGenericContext.OutputF( "\n-==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==- -==-\n\n"
								 "Dumping file '%s':\n\n", fileName );
		gGenericContext.OutputRaw( (const char*)mem.GetData(), mem.GetSize() );
		gGenericContext.Output( "\n" );
	}
	else
	{
		gpwarningf(( "Unable to open file '%s': '%s'\n", fileName, stringtool::GetLastErrorText().c_str() ));
	}
#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
// maker implementations

vector_3& MakeVector( float x, float y, float z )
{
	static vector_3 s_Vector( vector_3::ZERO );
	s_Vector.x = x;
	s_Vector.y = y;
	s_Vector.z = z;
	return ( s_Vector );
}

const vector_3& GetZeroVector( void )
{
	static vector_3 s_Vector( vector_3::ZERO );
	return ( s_Vector );
}

Quat& MakeQuat( float x, float y, float z, float w )
{
	static Quat s_Quat( Quat::IDENTITY );
	s_Quat.m_x = x;
	s_Quat.m_y = y;
	s_Quat.m_z = z;
	s_Quat.m_w = w;
	return ( s_Quat );
}

const char* MakeFourCcString( int fourCc )
{
	static gpstring s_String;
	s_String = stringtool::MakeFourCcString( fourCc );
	return ( s_String );
}

const char* MakeFourCcString( int fourCc, bool reverse )
{
	static gpstring s_String;
	s_String = stringtool::MakeFourCcString( fourCc, reverse );
	return ( s_String );
}

DWORD MakeColor( float red, float green, float blue, float alpha )
{
	return ( MakeColor( vector_3( red, green, blue ), alpha ) );
}

DWORD MakeColor( float red, float green, float blue )
{
	return ( MakeColor( vector_3( red, green, blue ) ) );
}

DWORD MakeColor( const vector_3& color, float alpha )
{
	vector_3 localColor( color );
	clamp_min_max( localColor.x, 0.0f, 1.0f );
	clamp_min_max( localColor.y, 0.0f, 1.0f );
	clamp_min_max( localColor.z, 0.0f, 1.0f );
	return ( MAKEDWORDCOLORA( localColor, alpha ) );
}

DWORD MakeColor( const vector_3& color )
{
	vector_3 localColor( color );
	clamp_min_max( localColor.x, 0.0f, 1.0f );
	clamp_min_max( localColor.y, 0.0f, 1.0f );
	clamp_min_max( localColor.z, 0.0f, 1.0f );
	return ( MAKEDWORDCOLOR( localColor ) );
}

gpstring& MakeStringF( const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	static gpstring s_String;
	s_String.assign( printer, printer.length() );
	return ( s_String );
}

//////////////////////////////////////////////////////////////////////////////
// decomposer implementations

float GetAlpha( DWORD intColor )
{
	return ( GETDWORDCOLORA( intColor ) );
}

float GetRed( DWORD intColor )
{
	return ( GETDWORDCOLORR( intColor ) );
}

float GetGreen( DWORD intColor )
{
	return ( GETDWORDCOLORG( intColor ) );
}

float GetBlue( DWORD intColor )
{
	return ( GETDWORDCOLORB( intColor ) );
}

//////////////////////////////////////////////////////////////////////////////
// renderer support implementations

void StartActiveTextureLogging()
{
	// Start the logging of active textures in the render layer
	gDefaultRapi.SetActiveTextureLogging( true );
}

void StopActiveTextureLogging( const char* fileName )
{
	// Stop logging
	gDefaultRapi.SetActiveTextureLogging( false );

	// Write the log out to the given file
	const TextureSet& logTextureSet = gDefaultRapi.GetActiveTextureLog();

	FileSys::File file;
	if( file.CreateAlways( fileName ) )
	{
		gpstring outString;

		// Write out the header string
		outString	= "[active_texture_log]\n{\n";
		file.WriteText( outString.c_str(), outString.length() );

		// Write out each texture from the active log
		for( TextureSet::const_iterator i = logTextureSet.begin(); i != logTextureSet.end(); ++i )
		{
			file.WriteText( (*i) + "\n", (*i).length() + 1 );
		}

		// Finish and close
		outString	= "}\n";
		file.WriteText( outString.c_str(), outString.length() );

		file.Close();
	}
	else if( strlen( fileName ) > 0 )
	{
		gpwarningf(( "Unable to create/open file '%s': '%s'\n", fileName, stringtool::GetLastErrorText().c_str() ));
	}
}

void SetTextureLogSaturation( const char* fileName, float saturation )
{
	// Read the log in from the given file
	TextureSet logTextureSet;

	FileSys::AutoFileHandle file;
	if ( file.Open( fileName ) )
	{
		gpstring inString;

		// Get header line
		file.ReadLine( inString );
		if( inString.same_no_case( "[active_texture_log]" ) )
		{
			// Step past opening brace
			file.ReadLine( inString );
			inString.clear();

			// Read in each line
			while( file.ReadLine( inString ) && !inString.same_no_case( "}" ) )
			{
				stringtool::RemoveBorderingWhiteSpace( inString );
				logTextureSet.insert( inString );
				inString.clear();
			}
		}
	}

	// Send the textures and their new saturation to the render layer
	gDefaultRapi.ChangeSaturationForTextures( logTextureSet, saturation );
}

void RevertSaturation()
{
	// Revert any textures that have had their saturation changed
	gDefaultRapi.RevertSaturatedTextures();
}

//////////////////////////////////////////////////////////////////////////////
// class String implementation

bool String :: IsEmpty( const char* str )
{
	gpassert( str != NULL );
	return ( *str == '\0' );
}

const gpstring& String :: GetFileNameOnly( const gpstring& str )
{
	static gpstring s_Str;
	s_Str = FileSys::GetFileNameOnly( str );
	return ( s_Str );
}

gpstring& String :: Append( gpstring& str, const char* extra )
{
	return ( str.append( extra ) );
}

gpstring& String :: AppendF( gpstring& str, const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( str.append( printer, printer.length() ) );
}

gpstring& String :: Assign( gpstring& str, const char* extra )
{
	return ( str.assign( extra ) );
}

gpstring& String :: AssignF( gpstring& str, const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( str.assign( printer, printer.length() ) );
}

gpstring& String :: Left( gpstring& str, int len )
{
	static gpstring s_Str;
	s_Str = str.left_safe( len );
	return ( s_Str );
}

gpstring& String :: Mid( gpstring& str, int pos, int len )
{
	static gpstring s_Str;
	s_Str = str.mid_safe( pos, len );
	return ( s_Str );
}

gpstring& String :: Mid( gpstring& str, int pos )
{
	static gpstring s_Str;
	s_Str = str.mid_safe( pos, str.length() - pos );
	return ( s_Str );
}

gpstring& String :: Right( gpstring& str, int len )
{
	static gpstring s_Str;
	s_Str = str.right_safe( len );
	return ( s_Str );
}

bool String :: SameNoCase( const char* l, const char* r )
{
	return ( ::same_no_case( l, r ) );
}

bool String :: SameNoCase( const char* l, const char* r, int len )
{
	return ( ::same_no_case( l, r, len ) );
}

bool String :: SameWithCase( const char* l, const char* r )
{
	return ( ::same_with_case( l, r ) );
}

bool String :: SameWithCase( const char* l, const char* r, int len )
{
	return ( ::same_with_case( l, r, len ) );
}

int String :: GetNumDelimitedValues( const char* in, char delimiter )
{
	return ( stringtool::GetNumDelimitedStrings( in, delimiter ) );
}

gpstring& String :: GetDelimitedString( const char* in, int index, char delimiter, const char* defValue )
{
	static gpstring s_Str;
	if ( !stringtool::GetDelimitedValue( in, delimiter, index, s_Str ) )
	{
		s_Str = defValue ? defValue : "";
	}
	return ( s_Str );
}

bool String :: GetDelimitedBool( const char* in, int index, char delimiter, bool defValue )
{
	bool val = defValue;
	stringtool::GetDelimitedValue( in, delimiter, index, val );
	return ( val );
}

int String :: GetDelimitedInt( const char* in, int index, char delimiter, int defValue )
{
	int val = defValue;
	stringtool::GetDelimitedValue( in, delimiter, index, val );
	return ( val );
}

float String :: GetDelimitedFloat( const char* in, int index, char delimiter, float defValue )
{
	float val = defValue;
	stringtool::GetDelimitedValue( in, delimiter, index, val );
	return ( val );
}


//////////////////////////////////////////////////////////////////////////////

#if !GP_ERROR_FREE
void DumpGameStack( void )
{
	DbgSymbolEngine::DumpGameStack( &gDebuggerContext );
}
#endif // !GP_ERROR_FREE

//////////////////////////////////////////////////////////////////////////////
