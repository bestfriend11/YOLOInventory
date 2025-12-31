//////////////////////////////////////////////////////////////////////////////
//
// File     :  Skrit.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "Skrit.h"

#include "FileSys.h"
#include "FileSysUtils.h"
#include "FuBiSchema.h"
#include "NamingKey.h"
#include "SkritEngine.h"
#include "SkritObject.h"
#include "SkritStructure.h"
#include "SkritByteCode.h"
#include "StdHelp.h"
#include "StringTool.h"

#include <limits>

namespace Skrit  {  // begin of namespace Skrit

FUBI_EXPORT_KEYWORDS( GetKeyword, GetKeywordCount() );

//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

static ReportSys::Context* gSkritContextPtr   = NULL;

ReportSys::Context& GetSkritContext( void )
{
	if ( gSkritContextPtr == NULL )
	{
		static ReportSys::Context s_SkritContext( &gGenericContext, "Skrit" );
		gSkritContextPtr = &s_SkritContext;

		s_SkritContext.Enable( false );
	}
	return ( *gSkritContextPtr );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

DWORD g_PRODUCT_ID = 0;
const DWORD& PRODUCT_ID = g_PRODUCT_ID;

void SetProductId( DWORD id )
{
	g_PRODUCT_ID = id;
}

//////////////////////////////////////////////////////////////////////////////
// class HObject implementation

bool HObject :: IsValid( void ) const
{
	return ( gSkritEngine.IsObjectValid( *this ) );
}

Object* HObject :: Get( void )
{
	return ( gSkritEngine.GetObject( *this ) );
}

const Object* HObject :: GetConst( void ) const
{
	return ( gSkritEngine.GetObject( *this ) );
}

bool HObject :: CreateSkrit( const CreateReq& createReq )
{
	*this = gSkritEngine.CreateObject( createReq );
	return ( !IsNull() );
}

bool HObject :: CloneSkrit( const CloneReq& cloneReq )
{
	*this = gSkritEngine.CloneObject( cloneReq );
	return ( !IsNull() );
}

void HObject :: ReleaseSkrit( void )
{
	if ( !IsNull() )
	{
		gSkritEngine.ReleaseObject( *this );
	}
}

const gpstring& HObject :: GetName( void ) const
{
	return ( gSkritEngine.GetObjectName( *this ) );
}

//////////////////////////////////////////////////////////////////////////////
// class HAutoObject implementation

bool HAutoObject :: CreateSkrit( const CreateReq& createReq )
{
	ReleaseSkrit();
	return ( Inherited::CreateSkrit( createReq ) );
}

bool HAutoObject :: CloneSkrit( const CloneReq& cloneReq )
{
	ReleaseSkrit();
	return ( Inherited::CloneSkrit( cloneReq ) );
}

void HAutoObject :: ReleaseSkrit( void )
{
	Inherited::ReleaseSkrit();
	scast <Inherited&> ( *this ) = HObject();
}

HObject HAutoObject :: ReleaseHandle( void )
{
	HObject old = *this;
	scast <Inherited&> ( *this ) = HObject();
	return ( old );
}

HAutoObject& HAutoObject :: operator = ( HObject h )
{
	if ( *this != h )
	{
		ReleaseSkrit();
		scast <Inherited&> ( *this ) = h;
	}
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// struct CreateReq implementation

// $$$ plug naming key in here

CreateReq& CreateReq :: Init( const char* fileName, bool createAlways )
{
	m_FileName            = fileName;
	m_CreateAlways        = createAlways;
	m_IgnoreFileErrors    = false;
	m_IgnoreCompileErrors = false;
	m_ForceSummary        = false;
	m_DeferConstruction   = false;
	m_ForceFloatConstants = false;
	m_PersistType         = PERSIST_YES;
	m_OwnerType           = FuBi::VAR_UNKNOWN;
	m_Owner               = NULL;

	return ( *this );
}

CreateReq& CreateReq :: SetOwner( eVarType type, void* owner )
{
	m_OwnerType = type;
	m_Owner = owner;
	return ( *this );
}

CreateReq& CreateReq :: SetOwner( const char* typeName, void* owner )
{
	m_OwnerType = gFuBiSysExports.FindType( typeName );
	gpassert( m_OwnerType != FuBi::VAR_UNKNOWN );
	m_Owner = owner;
	return ( *this );
}

#if GP_DEBUG

bool CreateReq :: AssertValid( void ) const
{
	gpassert( (m_FileName != NULL) && (*m_FileName != '\0') );
	return ( true );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// struct CloneReq implementation

CloneReq :: CloneReq( HObject cloneSource )
{
	m_CloneSource       = cloneSource;
	m_DeferConstruction = false;
	m_PersistType       = PERSIST_YES;
	m_CloneGlobals      = false;
	m_OwnerType         = FuBi::VAR_UNKNOWN;
	m_Owner             = NULL;
}

CloneReq :: CloneReq( HObject cloneSource, const CreateReq& createReq )
{
	m_CloneSource       = cloneSource;
	m_DeferConstruction = createReq.m_DeferConstruction;
	m_PersistType       = createReq.m_PersistType;
	m_CloneGlobals      = false;
	m_OwnerType         = createReq.m_OwnerType;
	m_Owner             = createReq.m_Owner;
}

CloneReq& CloneReq :: SetOwner( eVarType type, void* owner )
{
	m_OwnerType = type;
	m_Owner = owner;
	return ( *this );
}

CloneReq& CloneReq :: SetOwner( const char* typeName, void* owner )
{
	m_OwnerType = gFuBiSysExports.FindType( typeName );
	gpassert( m_OwnerType != FuBi::VAR_UNKNOWN );
	m_Owner = owner;
	return ( *this );
}

#if GP_DEBUG

bool CloneReq :: AssertValid( void ) const
{
	gpassert( m_CloneSource );
	return ( true );
}

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// eResult implementation

const char* s_ResultStrings[] =
{
	"success",
	"no event",
	"warning",
	"error",
	"fatal",
	"no function",
	"bad params",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ResultStrings ) == RESULT_COUNT );

static stringtool::EnumStringConverter s_ResultConverter(
		s_ResultStrings, RESULT_BEGIN, RESULT_END );

const char* ToString( eResult result )
	{  return ( s_ResultConverter.ToString( result ) );  }
bool FromString( const char* str, eResult& result )
	{  return ( s_ResultConverter.FromString( str, rcast <int&> ( result ) ) );  }

//////////////////////////////////////////////////////////////////////////////
// Skrit API implementations

// these are just redirectors

Result ExecuteCommand( const char* name, const char* command, int len )
	{  gpassert( name != NULL );  return ( gSkritEngine.ExecuteCommand( name, command, len ) );  }
bool EvalFloatExpression( const char* expr, float& result )
	{  return ( gSkritEngine.EvalFloatExpression( expr, result ) );  }
bool EvalIntExpression( const char* expr, int& result )
	{  return ( gSkritEngine.EvalIntExpression( expr, result ) );  }
bool EvalBoolExpression( const char* expr, bool& result )
	{  return ( gSkritEngine.EvalBoolExpression( expr, result ) );  }
Result SendEvent( Object* object, UINT eventSerial, const void* params )
	{  gpassert( object != NULL );  return ( object->Event( eventSerial, params ) );  }
Result SendEvent( HObject hobject, UINT eventSerial, const void* params )
	{  return ( SendEvent( hobject.Get(), eventSerial, params ) );  }
Result SendEvent( Object* object, const FuBi::FunctionSpec* spec, const void* params )
	{  gpassert( (object != NULL) && (spec != NULL) );  return ( object->Event( spec->m_SerialID, params ) );  }
Result SendEvent( HObject hobject, const FuBi::FunctionSpec* spec, const void* params )
	{  gpassert( spec != NULL );  return ( SendEvent( hobject.Get(), spec->m_SerialID, params ) );  }
Result CallSkrit( HObject hobject, int funcIndex, const FuBi::FunctionSpec* spec, const void* params )
	{  return ( CallSkrit( hobject.Get(), funcIndex, spec, params ) );  }
Result CallSkrit( HObject hobject, const char* funcName, const FuBi::FunctionSpec* spec, const void* params )
	{  return ( CallSkrit( hobject.Get(), funcName, spec, params ) );  }

Result CallSkrit( Object* object, int funcIndex, const FuBi::FunctionSpec* spec, const void* params )
{
	gpassert( object != NULL );
	gpassert( spec != NULL );
	int paramSize = (spec->m_ParamSpecs.size() - 2) * 4;		// strip off leading params not used by skrit
	return ( object->Call( funcIndex, spec, const_mem_ptr( params, paramSize ) ) );
}

Result CallSkrit( Object* object, const char* funcName, const FuBi::FunctionSpec* spec, const void* params )
{
	gpassert( object != NULL );
	gpassert( spec != NULL );
	int paramSize = (spec->m_ParamSpecs.size() - 2) * 4;		// strip off leading params not used by skrit
	return ( object->Call( funcName, spec, const_mem_ptr( params, paramSize ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class ParseNode implementations

void ParseNode :: DeleteChain( void )
{
	if ( m_Next != NULL )
	{
		m_Next->DeleteChain();
		Delete( m_Next );
	}
}

void ParseNode :: AddLinkEnd( ParseNode* node )
{
	// $$ this is inefficient, could use some help if chains get large (probably won't)

	// find the end
	for ( ParseNode* i = this ; i->m_Next != NULL ; i = i->m_Next )
	{
		;	// just loop
	}

	// add link
	i->m_Next = node;
	node->m_Next = NULL;
}

int ParseNode :: GetLinkCount( void ) const
{
	int count = 0;
	for ( ParseNode* i = m_Next ; i != NULL ; i = i->m_Next )
	{
		++count;
	}
	return ( count );
}

void ParseNode :: SetNonConstantToEval( void )
{
	switch ( m_Token )
	{
		case ( T_BOOLCONSTANT   ):
		case ( T_INTCONSTANT    ):
		case ( T_FLOATCONSTANT  ):
		case ( T_STRINGCONSTANT ):
		case ( T_TEMPEVAL ):
		case ( T_NULL ):
		{
			// $ do nothing
		}	break;

		default:
		{
			m_Token = T_EVAL;
		}
	}
}

const char* ParseNode :: ResolveString( void ) const
{
	gpassert( m_Location.m_Scanner != NULL );
	return ( m_Location.m_Scanner->ResolveString( m_Int ) );
}

//////////////////////////////////////////////////////////////////////////////
// class Reporter implementations

#if !GP_ERROR_FREE

Reporter :: Reporter( void )
{
	m_DelayedReporting = false;
	m_LocalContext.AddSink( &m_StringSink, false );

	Reset();
}

void Reporter :: Reset( void )
{
	m_Warnings = 0;
	m_Errors   = 0;
	m_Fatals   = 0;
	m_Name     = NULL;

	m_StringSink.ClearBuffer();
}

void Reporter :: Message( Location location, eMessageType type, const char* msg, va_list args )
{
#	if !GP_DEBUG
	if ( type == MESSAGE_INFO )
	{
		return;
	}
#	endif // !GP_DEBUG

// Store message.

	const char* level = "";
	switch ( type )
	{
		case ( MESSAGE_INFO    ):  level = "info"   ;                 break;
		case ( MESSAGE_WARNING ):  level = "warning";  ++m_Warnings;  break;
		case ( MESSAGE_ERROR   ):  level = "error"  ;  ++m_Errors  ;  break;
		case ( MESSAGE_FATAL   ):  level = "fatal"  ;  ++m_Fatals  ;  break;
	}

// Out message.

	// choose context
	ReportSys::Context* ctx = &gDebuggerContext;
	if ( m_DelayedReporting )
	{
		ctx = &m_LocalContext;
	}
	else
	{
		switch ( type )
		{
			case ( MESSAGE_WARNING ):
			{
				ctx = &gWarningContext;
			}	break;

			case ( MESSAGE_ERROR ):
			case ( MESSAGE_FATAL ):
			{
				ctx = &gErrorContext;
			}	break;
		}
	}

	// out
	ReportSys::AutoReport autoReport( ctx );
	gpassert( location.m_Scanner != NULL );
	const char* fileName = FileSys::GetFileName( location.m_Scanner->GetName() );
	ctx->OutputF( "%s(%d,%d) %s: ", fileName ? fileName : "<unnamed>", location.m_Line, location.m_Col, level );
	ctx->OutputArgs( msg, args );
	ctx->OutputEol();
}

void Reporter :: Summary( bool force )
{
	if ( HasMessage() || force )
	{
		ReportSys::Context* ctx = NULL;
		if ( m_DelayedReporting )
		{
			ctx = &m_LocalContext;
		}
		else if ( HasError() )
		{
			ctx = &gErrorContext;
		}
		else
		{
			ctx = &gWarningContext;
		}

		ctx->OutputF(
				"\n%s - %d fatal%s, %d error%s, %d warning%s\n",
				*GetName() ? GetName() : "<unnamed>",
				m_Fatals,   (m_Fatals   == 1) ? "" : "s",
				m_Errors,   (m_Errors   == 1) ? "" : "s",
				m_Warnings, (m_Warnings == 1) ? "" : "s" );
	}
}

void Reporter :: Report( bool forceSummary )
{
	if ( HasMessage() )
	{
		ReportSys::Context* ctx = NULL;
		if ( HasError() )
		{
			ctx = &gErrorContext;
		}
		else
		{
			ctx = &gWarningContext;
		}

		ctx->Output( m_StringSink.GetBuffer() );
	}
	else if ( forceSummary )
	{
		Summary( true );
	}
}

bool Reporter :: Success( void )
{
	return ( (m_Errors == 0) && (m_Fatals == 0) );
}

const char* Reporter :: GetName( void ) const
{
	if ( m_Name == NULL )
	{
		return ( "<unknown>" );
	}
	else
	{
		return ( m_Name );
	}
}

#endif // !GP_ERROR_FREE

//////////////////////////////////////////////////////////////////////////////
// class Scanner implementation

void Scanner :: Message( eMessageType type, const char* msg, ... )
{
#	if !GP_ERROR_FREE
	m_Compiler->GetReporter()->Message( GetStartLocation(), scast <Reporter::eMessageType> ( type ), msg, va_args( msg ) );
#	endif // !GP_ERROR_FREE
}

void Scanner :: Message( Location location, eMessageType type, const char* msg, ... )
{
#	if !GP_ERROR_FREE
	m_Compiler->GetReporter()->Message( location, scast <Reporter::eMessageType> ( type ), msg, va_args( msg ) );
#	endif // !GP_ERROR_FREE
}

//////////////////////////////////////////////////////////////////////////////
// class Compiler implementation

Compiler :: Compiler( void )
{
	// alloc default scanner
	m_Scanners.push_back( new Scanner( this ) );

	// setup
	m_Pack = new Raw::Pack;
	m_EventClass = gFuBiSysExports.FindEventNamespace();
	m_SkritObjectType = gFuBiSysExports.FindSkritObject()->m_Type;
	m_SkritVmType = gFuBiSysExports.FindSkritVm()->m_Type;
#	if !GP_RETAIL
	m_DebugMode = !GP_RETAIL;
#	endif // !GP_RETAIL

	// install some defaults
	RegisterDefaultOnlyConditions();

	// clear state info
	Reset();
}

Compiler :: ~Compiler( void )
{
	Reset();
	m_Scanners.back().Delete();

	delete ( m_Pack );
}

void Compiler :: Reset( void )
{
	Inherited::Reset();

	while ( m_Scanners.size() > 1 )
	{
		m_Scanners.back().Delete();
		m_Scanners.pop_back();
	}

	while ( !m_OldScanners.empty() )
	{
		m_OldScanners.back().Delete();
		m_OldScanners.pop_back();
	}

	m_NextScannerIndex       = 1;
	m_OptExplicitStatesOnly  = true;
	m_OptForceFloatConstants = false;

	m_OverrideLocation    . Reset();
	m_Scanners.back()     . Reset();
	m_Pack               -> Reset();
	m_CurrentFunction     . Reset();
	m_Symbols             . clear();
	m_GlobalShared        = false;
	m_GlobalProperty      = false;
	m_GlobalHidden        = false;
	m_TempStringCount     = 0;
	m_OwnerType           = FuBi::VAR_UNKNOWN;
	m_OwnerUsedLine       = 0;
	m_AtTimeFrames        = 0;
	m_AtTimeMsec          = 0;
	m_StartOfLocalSymbols = 0;
	m_FunctionCount       = 0;

	stdx::for_each( m_LocalScopes, stdx::delete_by_ptr() );
	m_LocalScopes.clear();
	m_GlobalScope.Reset();

	m_GlobalState.Reset();
	m_States     .clear();

	// always keep a null string in there
	AddStringConstant( "", 0 );
}

bool Compiler :: Compile( const char* name, const_mem_ptr mem, const CreateReq* defOptions )
{
	gpassert( name != NULL );
	Reset();

	// set default options
	if ( defOptions != NULL )
	{
		m_OptForceFloatConstants = defOptions->m_ForceFloatConstants;
	}

#	if !GP_ERROR_FREE
	bool ignoreErrors =  ((defOptions != NULL) && defOptions->m_IgnoreCompileErrors)
					  || gSkritEngine.TestOptions( Engine::OPTION_DEFER_ERRORS );
	m_Reporter.Reset();
#	endif // !GP_ERROR_FREE

	// init
	m_Scanners.back().Init( name, mem );
#	if !GP_ERROR_FREE
	m_Reporter.SetName( name );
	m_Reporter.SetDelayed( ignoreErrors );
#	endif // !GP_ERROR_FREE

	// debug info
#	if !GP_RETAIL
	if ( m_DebugMode )
	{
		Raw::DebugSourceFile dbgSourceFile;
		gpstring localName( name );
		dbgSourceFile.m_Size = scast <WORD> ( dbgSourceFile.m_Size + localName.length() + 1 );
		AppendToBuffer( m_Pack->m_Debug, dbgSourceFile );
		AppendToBuffer( m_Pack->m_Debug, localName );
	}
#	endif // !GP_RETAIL

	// compile it and report a summary
	bool rc = !Parse();
#	if !GP_ERROR_FREE
	m_Reporter.Summary();
#	endif // !GP_ERROR_FREE

	// parse
	return ( rc GPDEV_ONLY( && m_Reporter.Success() ) );
}

mem_ptr Compiler :: Save( void ) const
{
#	if 0 //$$$$$$$$$$$$$$$$
	gpassert( m_SourceCode.mem != NULL );

	// get size
	size_t size = m_Pack->GetSize();

	// alloc mem
	mem_ptr mem;
	mem.size = size;
	if ( m_SourceCode.size != 0 )
	{
		mem.size += m_SourceCode.size + 1;
	}
	mem.mem = new BYTE[ mem.size ];

	// copy data in
	m_Pack->Save( mem );

	// process source code if any
	if ( m_SourceCode.size != 0 )
	{
		// append it
		BYTE* runner = rcast <BYTE*> ( mem.mem ) + size;
		::memcpy( runner, m_SourceCode.mem, m_SourceCode.size );
		runner[ m_SourceCode.size ] = '\0';

		// adjust sizes
		Raw::Header* header = rcast <Raw::Header*> ( mem.mem );
		header->m_SourceOffset = size;
		header->m_TotalSize = mem.size;

		// other header info
		header->m_OwnerType = m_OwnerType;
	}

	// return new'd memory for storage to disk or whatever
	return ( mem );
#	endif // 0

	return ( mem_ptr() );
}

void Compiler :: MoveTo( ObjectImpl& impl )
{
	impl.Swap( *m_Pack, m_OwnerType );
	Reset();
}

void Compiler :: IncludeDirective( ParseNode& node )
{
	// get the file
	const char* fileName = ResolveStringConstant( node.m_Int );
	m_OverrideLocation = node.m_Location;

	// file goes here
	FileSys::AutoFileHandle file;
	gpstring localFileName;

	// attempt to open file through naming key
	if ( NamingKey::DoesSingletonExist() && gNamingKey.BuildSkritLocation( fileName, localFileName ) )
	{
		// take it
		fileName = localFileName;
		file.Open( fileName );
	}
	else if ( !file.Open( fileName ) )		// try to open absolutely
	{
		// try again using "current dir" from most recent scanner
		const char* path = GetScanner()->GetName();
		const char* pathEnd = FileSys::GetPathEnd( path );
		localFileName.assign( path, pathEnd );
		stringtool::AppendTrailingBackslash( localFileName );
		localFileName += fileName;
		fileName = localFileName;
		file.Open( fileName );
	}

	// check for recursion
	Scanners::iterator i, ibegin = m_Scanners.begin(), iend = m_Scanners.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->m_Scanner->GetName().same_no_case( fileName ) )
		{
			// $ error and early bailout
			Message( MESSAGE_ERROR, "recursive #include of '%s' detected, ignoring directive", (fileName && *fileName) ? FileSys::GetFileName( fileName ) : "<empty>" );
			return;
		}
	}

	// open it
	if ( file )
	{
		// get and fill memory
		FileSys::AutoMemHandle mem( file );
		mem_ptr data( new BYTE[ mem.GetSize() ], mem.GetSize() );
		::memcpy( data.mem, mem.GetData(), data.size );

		// build entry
		ScannerEntry scannerEntry( new Scanner( this ), m_NextScannerIndex++ );
		scannerEntry.Init( fileName, data, true );

		// debug info
#		if !GP_RETAIL
		if ( m_DebugMode )
		{
			Raw::DebugSourceFile dbgSourceFile;
			gpstring localName( fileName );
			dbgSourceFile.m_Size = scast <WORD> ( dbgSourceFile.m_Size + localName.length() + 1 );
			dbgSourceFile.m_IncluderIndex = scast <WORD> ( m_Scanners.back().m_ScannerIndex );
			dbgSourceFile.m_IncludeLine = scast <WORD> ( node.m_Location.m_Line );
			AppendToBuffer( m_Pack->m_Debug, dbgSourceFile );
			AppendToBuffer( m_Pack->m_Debug, localName );
		}
#		endif // !GP_RETAIL

		// add it
		m_Scanners.push_back( scannerEntry );
	}
	else
	{
		Message( MESSAGE_ERROR, "unable to open file '%s'", fileName );
	}
}

void Compiler :: OptionDirective( ParseNode& node, bool enable )
{
	const char* option = node.ResolveString();
	if ( same_no_case( option, "explicit_states" ) )
	{
		m_OptExplicitStatesOnly = enable;
	}
	else if ( same_no_case( option, "force_float_constants" ) )
	{
		m_OptForceFloatConstants = enable;
	}
	else
	{
		m_OverrideLocation = node.m_Location;
		Message( MESSAGE_WARNING, "'#option': unrecognized option '%s'", option );
	}
}

void Compiler :: OnlyDirective( ParseNode& node )
{
	bool skipCode = true;

	// figure out skip type
	if ( node.m_Token == T_SYSID )
	{
		// accept anything here, and if it's unrecognized then skip
		ConditionMap::iterator found = m_ConditionMap.find( node.ResolveString() );
		if ( found != m_ConditionMap.end() )
		{
			skipCode = !found->second;
		}
	}
	else if ( (node.m_Token == T_INTCONSTANT) || (node.m_Token == T_BOOLCONSTANT) )
	{
		skipCode = node.m_Int == 0;
	}
	else
	{
		m_OverrideLocation = node.m_Location;
		Message( MESSAGE_WARNING, "'#only': unrecognized type" );
	}

	// attempt skip
	if ( !GetScanner()->SetSkipMode( skipCode ) )
	{
		m_OverrideLocation = node.m_Location;
		Message( MESSAGE_WARNING, "'#only': this directive cannot be nested" );
	}
}

void Compiler :: Message( eMessageType type, const char* msg, ... )
{
#	if ( !GP_ERROR_FREE )
	{
		// allowed to override default message location
		if ( !m_OverrideLocation.IsValid() )
		{
			m_OverrideLocation = m_ValueStackTop[ 1 ].m_Location;
		}

		// out the error using $1 for the location
		m_Reporter.Message( m_OverrideLocation, scast <Reporter::eMessageType> ( type ), msg, va_args( msg ) );

		// clear it for next time
		m_OverrideLocation.Reset();
	}
#	endif // !GP_ERROR_FREE
}

void Compiler :: InternalError( eMessageType type, const char* msg )
{
	m_OverrideLocation = GetScanner()->GetStartLocation();
	Inherited::InternalError( type, msg );
}

int Compiler :: Scan( void )
{
	// clear out old value
	m_OverrideLocation.Reset();

	// scan
	int rc = GetScanner()->Scan();
	while ( (rc == 0) && (m_Scanners.size() > 1) )
	{
		// move the scanner into the retired set
		m_OldScanners.push_back( m_Scanners.back() );
		m_Scanners.pop_back();

		// re-scan
		rc = GetScanner()->Scan();
	}

	// done
	return ( rc );
}

void Compiler :: ReportLastCompile( bool forceSummary )
{
#	if !GP_ERROR_FREE
	m_Reporter.Report( forceSummary );
#	endif // !GP_ERROR_FREE
}

int Compiler :: AddStringConstant( const char* name, int len )
{
	//$$$ collect and sort strings to remove dups

	if ( len == -1 )
	{
		len = (name == NULL) ? 0 : ::strlen( name );
	}

	UINT index = AppendToBuffer( m_Pack->m_Strings, const_mem_ptr( name, len ) );
	m_Pack->m_Strings.push_back( '\0' );
	return ( index );
}

const char* Compiler :: ResolveStringConstant( int offset )
{
	gpassert( (offset >= 0) && (offset < (int)m_Pack->m_Strings.size()) );
	return ( rcast <const char*> ( &*m_Pack->m_Strings.begin() + offset ) );
}

void Compiler :: RegisterDefaultOnlyConditions( void )
{
#	if !GP_RETAIL
	if ( gSkritEngine.TestOptions( Engine::OPTION_RETAIL_MODE ) )
	{
		RegisterOnlyCondition( "dev",     0 );
		RegisterOnlyCondition( "debug",   0 );
		RegisterOnlyCondition( "release", 0 );
		RegisterOnlyCondition( "retail",  1 );
	}
	else
#	endif // !GP_RETAIL
	{
		RegisterOnlyCondition( "dev",     !GP_RETAIL );
		RegisterOnlyCondition( "debug",   GP_DEBUG   );
		RegisterOnlyCondition( "release", GP_RELEASE );
		RegisterOnlyCondition( "retail",  GP_RETAIL  );
	}
}

Raw::ExportEntry* Compiler :: AddExport( const char* funcName, Location location, const char* docs, int paramCount, int eventSerial )
{
	bool ok = true;

// Verification.

	// check func name length
	int nameLen = ::strlen( funcName );
	if ( nameLen > std::numeric_limits <BYTE>::max() )
	{
		Message( MESSAGE_ERROR, "exported function names cannot be longer than %d characters ('%s' too long)",
				 std::numeric_limits <BYTE>::max(), funcName );
		funcName = "<out of range>";
		ok = false;
	}

	// check doc length
	int docsLen = docs ? ::strlen( docs ) : 0;
	if ( docsLen > std::numeric_limits <WORD>::max() )
	{
		Message( MESSAGE_ERROR, "exported function docs cannot be longer than %d characters ('%s')",
				 std::numeric_limits <WORD>::max(), funcName );
		ok = false;
	}

	// check param count
	if ( paramCount > std::numeric_limits <BYTE>::max() )
	{
		Message( MESSAGE_ERROR, "exported function cannot take more than %d parameters ('%s')",
				 std::numeric_limits <BYTE>::max(), funcName );
		ok = false;
	}

	// fail
	if ( !ok )
	{
		return ( NULL );
	}

// Add to symbol table.

	// add symbol to global scope
	gpassert( m_LocalScopes.empty() );
	Symbol* symbol = AddSymbol( funcName, location, SYMTYPE_FUNCTION );
	if ( symbol == NULL )
	{
		return ( NULL );
	}

	// postprocess for event type
	if ( eventSerial != INVALID_FUNCTION_INDEX )
	{
		symbol->m_SymType = SYMTYPE_EVENT;
	}

// Out to exports table.

	// build prototype
	Raw::ExportEntry entry;
	entry.m_FunctionOffset = GetCodeOffset();
	entry.m_NameLength     = scast <BYTE>  ( nameLen );
	entry.m_DocsLength     = scast <WORD>  ( docsLen );
	entry.m_ParamCount     = scast <BYTE>  ( paramCount );
	entry.m_EventBinding   = scast <WORD>  ( eventSerial );
	entry.m_StateMember    = scast <WORD>  ( m_GlobalState.m_Current );
	entry.m_AtTime         = scast <DWORD> ( TIME_NONE );

	// add it
	UINT offset = m_Pack->m_Exports.size();
	symbol->m_Index = m_FunctionCount++;
	symbol->m_Offset = offset;
	UINT size = entry.GetSize();
	m_Pack->m_Exports.resize( offset + size );
	Raw::ExportEntry* newEntry = rcast <Raw::ExportEntry*> ( &*m_Pack->m_Exports.begin() + offset );
	::memcpy( newEntry, &entry, min( size, sizeof( entry ) ) );

	// fill out rest of members
	::memcpy( newEntry->m_Name, funcName, nameLen + 1 );
	if ( docs != NULL )
	{
		::memcpy( newEntry->GetDocs(), docs, docsLen + 1 );
	}

	// done
	return ( newEntry );
}

Raw::ExportEntry* Compiler :: AddExportParam( Raw::ExportEntry* oldEntry, const ParseNode& param )
{
	// check
	gpassert( oldEntry != NULL );

	// remember where we were (vector may realloc and invalidate ptrs)
	UINT offset = rcast <BYTE*> ( oldEntry ) - &*m_Pack->m_Exports.begin();

	// add it
	++oldEntry->m_ParamCount;
	AppendToBuffer( m_Pack->m_Exports, param.m_Type.m_Type );

	// readjust and return
	oldEntry = rcast <Raw::ExportEntry*> ( &*m_Pack->m_Exports.begin() + offset );
	return ( oldEntry );
}

bool Compiler :: SetGlobalType( ParseNode& type )
{
	// figure out its type
	m_GlobalVarType = type.m_Type;
	if ( FuBi::IsUser( m_GlobalVarType.m_Type ) )
	{
		// UDT pointer or handle
		const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( m_GlobalVarType.m_Type );
		gpassert( spec != NULL );

		// managed class?
		if ( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
		{
			// handle
			m_GlobalSymType = SYMTYPE_HANDLE;
		}
		else
		{
			// pointer
			m_GlobalSymType = SYMTYPE_VARIABLE;
		}
	}
	else
	{
		if ( m_GlobalVarType.IsString() )
		{
			m_GlobalSymType = SYMTYPE_STRING;
		}
		else
		{
			gpassert( !m_GlobalVarType.IsCString() );		// shouldn't be a const char* variable in skrit!
			m_GlobalSymType = SYMTYPE_VARIABLE;
		}
	}

	return ( m_GlobalVarType != FuBi::VAR_UNKNOWN );
}

bool Compiler :: AddVariable( ParseNode& name, ParseNode* value, ParseNode* doco )
{
	// can't have any temp strings outstanding
	gpassert( m_TempStringCount == 0 );

// Constant type conversions.

	// $ this conversion section here would be totally unnecessary if i would
	//   get my shit together and write a damn constant expression grammar.
	//   or at the least put all the conversion/promotion stuff into a single
	//   function. what a mess, this code is duplicated everywhere...

	// convert constants if we're global
	bool global = m_LocalScopes.empty();
	if ( (value != NULL) && global && !ConvertConstant( *value, m_GlobalVarType ) )
	{
		m_OverrideLocation = value->m_Location;
		Message( MESSAGE_ERROR, "'initialization': cannot convert constant from '%s' to '%s'",
				 gFuBiSysExports.FindNiceTypeName( value->m_Type ),
				 gFuBiSysExports.FindNiceTypeName( m_GlobalVarType ) );
		return ( false );
	}

// Add to symbol table.

	// look up name
	const char* varName = name.ResolveString();

	// add to symbol table for this scope
	Symbol* symbol = AddSymbol( varName, name.m_Location, m_GlobalSymType, m_GlobalVarType );
	if ( symbol == NULL )
	{
		return ( false );
	}

// Allocate storage.

	if ( global )
	{
		UINT offset;

		// update symbol
		symbol->m_Store = m_GlobalShared ? Raw::STORE_SHARED : Raw::STORE_GLOBAL;
		if ( m_GlobalProperty )
		{
			symbol->m_Flags |= Raw::FLAG_PROPERTY;

			// register property
			Raw::PropertyEntry entry;
			entry.m_InitialValue = value ? value->m_Raw : 0;
			entry.m_DocsOffset   = doco  ? doco ->m_Int : 0;
			offset = AppendToBuffer( m_Pack->m_Globals, entry );
		}
		else
		{
			// register global
			Raw::GlobalEntry entry;
			offset = AppendToBuffer( m_Pack->m_Globals, entry );
		}

		// hidden?
		if ( m_GlobalHidden )
		{
			symbol->m_Flags |= Raw::FLAG_HIDDEN;
		}

		// fill in common
		Raw::GlobalEntry* entryPtr = rcast <Raw::GlobalEntry*> ( &*m_Pack->m_Globals.begin() + offset );
		entryPtr->m_Type         = scast <WORD> ( symbol->m_VarType.m_Type );
		entryPtr->m_Store        = scast <BYTE> ( symbol->m_Store );
		entryPtr->m_Flags        = scast <BYTE> ( symbol->m_Flags );
		entryPtr->m_NameOffset   = AddStringConstant( symbol->m_Name );
		entryPtr->m_InitialValue = value ? value->m_Raw : 0;
	}
	else							// local
	{
		switch ( m_GlobalSymType )
		{
			case ( SYMTYPE_VARIABLE ):
			{
				Emit( Op::CODE_ALLOCVAR, name );
			}	break;
			case ( SYMTYPE_STRING ):
			{
				Emit( Op::CODE_ALLOCSTR, name );
			}	break;
			case ( SYMTYPE_HANDLE ):
			{
				Emit( Op::CODE_ALLOCHDL, name );
				m_Pack->EmitWord( scast <WORD> ( symbol->m_VarType.m_Type ) );
			}	break;
		}
	}

	return ( true );
}

bool Compiler :: AddTempVariable( const ParseNode& node, const FuBi::TypeSpec type )
{
	// should not have temporaries at global scope
	gpassert( !m_LocalScopes.empty() );

	// strings only supported for now
	gpassert( type.IsString() );

// Add to symbol table.

	// advance temp counter
	++m_TempStringCount;

	// add to symbol table for this scope
	Symbol* symbol = AddSymbol( NULL, node.m_Location, SYMTYPE_STRING, type );
	if ( symbol == NULL )
	{
		return ( false );
	}

// Allocate storage.

	Emit( Op::CODE_ALLOCSTR, node );

	return ( true );
}

bool Compiler :: ResolveClassMethod( SearchContext& ctx )
{
	const FuBi::ClassSpec* cls = ctx.m_BaseClass;

	// try method
	FuBi::FunctionByNameIndex::const_iterator mfound = cls->m_Functions.find( ctx.m_FuncName );
	if ( mfound != cls->m_Functions.end() )
	{
		// $ don't tag as success yet - still must resolve overloads

		ctx.m_MethodIndex = &mfound->second;
		ctx.m_Found = SearchContext::FOUND_METHOD;
		return ( true );
	}

	// try variable
	FuBi::ClassSpec::VariableMap::const_iterator vfound = cls->m_Variables.find( ctx.m_FuncName );
	if ( vfound != cls->m_Variables.end() )
	{
		if ( !ctx.m_AllowVar )
		{
			ctx.m_Status = CALLSTATUS_REPORTED;
			Message( MESSAGE_ERROR, "'%s::%s': attempting to access member variable as a function call",
					 cls->m_Name.c_str(), vfound->first.c_str() );
			return ( true );
		}

		const FuBi::VariableSpec& var = vfound->second;
		if ( ctx.m_ParamCount == 0 )
		{
			// must allow reading if void params
			if ( var.m_GetMethod != NULL )
			{
				ctx.m_LocalIndex.push_back( ccast <FuBi::FunctionSpec*> ( var.m_GetMethod ) );
				ctx.m_MethodIndex = &ctx.m_LocalIndex;
				ctx.m_Found = SearchContext::FOUND_VARIABLE;
			}
			else
			{
				ctx.m_Status = CALLSTATUS_REPORTED;
				Message( MESSAGE_ERROR, "'%s::%s': this variable does not allow reading",
						 cls->m_Name.c_str(), vfound->first.c_str() );
			}
		}
		else if ( ctx.m_ParamCount == 1 )
		{
			// must allow writing if single param
			if ( var.m_SetMethod != NULL )
			{
				ctx.m_LocalIndex.push_back( ccast <FuBi::FunctionSpec*> ( var.m_SetMethod ) );
				ctx.m_MethodIndex = &ctx.m_LocalIndex;
				ctx.m_Found = SearchContext::FOUND_VARIABLE;
			}
			else
			{
				ctx.m_Status = CALLSTATUS_REPORTED;
				Message( MESSAGE_ERROR, "'%s::%s': this variable does not allow writing",
						 cls->m_Name.c_str(), vfound->first.c_str() );
			}
		}
		else
		{
			gpassert( 0 );		// how did this happen?
		}
		return ( true );
	}

	// try property
	if ( cls->m_HeaderSpec != NULL )
	{
		if ( cls->m_Flags & FuBi::ClassSpec::FLAG_PROPCANSKRIT )
		{
			int col = cls->m_HeaderSpec->FindColumn( ctx.m_FuncName );
			if ( col != FuBi::INVALID_COLUMN_INDEX )
			{
				ctx.m_PropertyIndex = col;
				ctx.m_Found = SearchContext::FOUND_PROPERTY;
				return ( true );
			}
		}
		else
		{
			ctx.m_Status = CALLSTATUS_REPORTED;
			Message( MESSAGE_ERROR, "'%s::%s': attempting to access property in a non-Skrit-capable schema",
					 cls->m_Name.c_str(), ctx.m_FuncName );
			return ( true );
		}
	}

	// try parents
	FuBi::ClassSpec::ParentColl::const_iterator i,
												begin = cls->m_ParentClasses.begin(),
												end   = cls->m_ParentClasses.end  ();
	for ( i = begin ; i != end ; ++i )
	{
		ctx.m_BaseClass = i->first;
		ctx.m_ClassOffsetAdjust = i->second;
		if ( ResolveClassMethod( ctx ) )
		{
			return ( true );
		}
	}

	// fail!!!
	return ( false );
}

bool Compiler :: EmitCall( ParseNode& node, bool optional, bool allowVar )
{
	// don't leak
	ParseNode::AutoDeleteChain autoDelete( node );

	// call on a call is a syntax error
	if ( node.m_Token == T_FUNCCALL )
	{
		m_OverrideLocation = node.m_Location;
		Message( MESSAGE_ERROR, "syntax error - attempted function call on a function call" );
		return ( false );
	}

	// if it's a userid then see if it's a user-defined function
	if ( node.m_Token == T_USERID )
	{
		//$$$ what about T_DEREFERENCE for userid? may need T_DEREFUSER...
		return ( EmitUserCall( node, optional ) );
	}
	else
	{
		return ( EmitSysCall( node, optional, allowVar ) );
	}
}

bool Compiler :: EmitSysCall( ParseNode& node, bool optional, bool allowVar )
{
	// $ YO: this function is pretty ugly - don't reuse in v2

	// note: 'optional' means that this may not be an actual function call and
	//       the id must be tested to see if it is.

	/* $$$ still unimplemented: default params */

// Overloaded function lookup.

	ParseNode* badParam = NULL;

	// find the function overload set
	SearchContext ctx( node.ResolveString(), node.GetLinkCount(), allowVar );
	if ( node.m_Type == FuBi::VAR_UNKNOWN )
	{
		// global
		ctx.m_MethodIndex = gFuBiSysExports.FindGlobalFunction( ctx.m_FuncName );
	}
	else
	{
		// class member
		ctx.InitClassSearch( gFuBiSysExports.FindClass( node.m_Type.m_Type ) );
		ResolveClassMethod( ctx );

		// property adjustment $$$
		if ( ctx.m_Found == SearchContext::FOUND_PROPERTY )
		{
			//$$$ handle it!
			node.m_Type = ctx.m_BaseClass->m_HeaderSpec->GetColumn( ctx.m_PropertyIndex ).m_Type;
			node.m_Token = T_FUNCCALL;

			GP_Unimplemented$$$();
			ctx.m_Status = CALLSTATUS_REPORTED;
		}
	}

// Overload function resolution.

	// success?
	if ( ctx.m_MethodIndex != NULL )
	{
		// iterate through all possibilities. look for ambiguity in two levels,
		// first in exact matches, and second in close matches.

		typedef stdx::fast_vector <ParseNode*> Conversions;
		Conversions conversions;

		FuBi::FunctionIndex::const_iterator i, ibegin = ctx.m_MethodIndex->begin(), iend = ctx.m_MethodIndex->end();
		const FuBi::FunctionSpec* efound = NULL, * cfound = NULL;
		int ematches = 0, cmatches = 0;
		for ( i = ibegin ; i != iend ; ++i )
		{
			// check for proper number of params (special vararg handling)
			if ( (*i)->m_Flags & FuBi::FunctionSpec::FLAG_VARARG )
			{
				if ( (*i)->m_ParamSpecs.size() > scast <UINT> ( ctx.m_ParamCount ) )
				{
					if ( ctx.m_Status < CALLSTATUS_PARAMCOUNT )
					{
						ctx.m_Status = CALLSTATUS_PARAMCOUNT;
					}
					continue;
				}
			}
			else
			{
				if ( (*i)->m_ParamSpecs.size() != scast <UINT> ( ctx.m_ParamCount ) )
				{
					if ( ctx.m_Status < CALLSTATUS_PARAMCOUNT )
					{
						ctx.m_Status = CALLSTATUS_PARAMCOUNT;
					}
					continue;
				}
			}

			// make sure this function can skrit!
			if ( !((*i)->m_Flags & FuBi::FunctionSpec::FLAG_CAN_SKRIT) )
			{
				if ( ctx.m_Status < CALLSTATUS_CANNOTSKRIT )
				{
					ctx.m_Status = CALLSTATUS_CANNOTSKRIT;
				}
				continue;
			}

			// check for type compatibility and do conversions
			bool neededConversions = false;
			FuBi::FunctionSpec::ParamSpecs::const_iterator j,
					jbegin = (*i)->m_ParamSpecs.begin(),
					jend   = (*i)->m_ParamSpecs.end();
			ParseNode* inode;
			for ( j = jbegin, inode = node.m_Next ; j != jend ; ++j, inode = inode->m_Next )
			{
				gpassert( inode != NULL );
				inode->m_Int = j - jbegin;		// m_Int is now the index of this param

				if ( (inode->m_Token == T_NULL) && FuBi::IsUser( j->m_Type.m_Type ) && !j->m_Type.IsPassByValue() )
				{
					// $ we're cool - special support for passing NULL for references
				}
				else
				{
					FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( inode->m_Type, j->m_Type );
					if ( compare == FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
					{
						// on failure to convert...
						if ( badParam == NULL )
						{
							badParam = inode;
						}
						if ( ctx.m_Status < CALLSTATUS_TYPECONVERSION )
						{
							ctx.m_Status = CALLSTATUS_TYPECONVERSION;
						}
						// do not bother continuing
						break;
					}
					else if (   (compare == FuBi::TypeSpec::COMPARE_EQUAL)
							 || (compare == FuBi::TypeSpec::COMPARE_PROMOTABLE) )
					{
						// $ we're cool
					}
					else if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
					{
						if ( cfound == NULL )
						{
							// add this to the list of entries that need
							// conversion, but only if we have not found a
							// possible match yet - do not want to overwrite
							// existing conversion entries as long as current
							// one may fail the match.
							conversions.push_back( inode );
						}

						// remember this
						neededConversions = true;
					}
				}
			}

			// got through them all ok?
			if ( j == jend )
			{
				// exact match?
				if ( !neededConversions )
				{
					++ematches;
					efound = *i;
				}
				else
				{
					// close match
					++cmatches;
					cfound = *i;
				}
			}
			else if ( cfound == NULL )
			{
				conversions.clear();		// toss out bogus conversion attempts - only keep the first successful one
			}
		}

		// check for ambiguous call
		const FuBi::FunctionSpec* found = NULL;
		if ( ematches == 1 )
		{
			found = efound;
			conversions.clear();			// not needed
		}
		else if ( (ematches == 0) && (cmatches == 1) )
		{
			found = cfound;
		}
		else
		{
			ctx.m_Status = CALLSTATUS_AMBIGUOUS;
		}

// Type conversions.

		// found and unambiguous?
		if ( found != NULL )
		{
			// do type conversions
			Conversions::const_iterator j, jbegin = conversions.begin(), jend = conversions.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				int paramIndex = (*j)->m_Int;
				const FuBi::TypeSpec& from = (*j)->m_Type;
				const FuBi::TypeSpec& to = found->m_ParamSpecs[ paramIndex ].m_Type;
				EmitConversion( "parameter", from, to, ctx.m_ParamCount - paramIndex - 1, **j );
			}

// Vararg handling.

			// special vararg handling
			int thisDepth = ctx.m_ParamCount;
			if ( found->m_Flags & FuBi::FunctionSpec::FLAG_VARARG )
			{
				// fast forward past all the formal args
				ParseNode* inode = node.m_Next;
				UINT i = 0;
				for ( UINT count = found->m_ParamSpecs.size() ; i != count ; ++i )
				{
					inode = inode->m_Next;
				}

				// check for type compatibility and do conversions
				for ( ; inode != NULL ; inode = inode->m_Next, ++i )
				{
					// convert floats to doubles as per C++ standard for vararg functions
					if ( inode->m_Type == FuBi::VAR_FLOAT )
					{
						Emit( Op::CODE_FLTTODBL, node );
						m_Pack->EmitByte( ctx.m_ParamCount - i - 1 );
						++thisDepth;
					}
					// and strings to cstrs as per my own damn standard
					else if ( inode->m_Type.IsString() )
					{
						Emit( Op::CODE_STRTOPCC, node );
						m_Pack->EmitByte( ctx.m_ParamCount - i - 1 );
					}
				}

				// output the number of params
				EmitPush( thisDepth, node );

				// depth increased even more
				++thisDepth;
			}

// Emit the function call.

			// global function?
			bool success = true;
			if ( found->m_Parent == NULL )
			{
				Emit( Op::CODE_CALL, node );
				m_Pack->EmitWord( found->m_SerialID );
			}
			// static method or namespace global?
			else if ( found->m_Flags & FuBi::FunctionSpec::FLAG_STATIC_MEMBER )
			{
				// somebody may have done a member style call using a variable,
				// if so then chuck the object
				if ( node.m_Type.m_Flags & (FuBi::TypeSpec::FLAG_POINTER | FuBi::TypeSpec::FLAG_REFERENCE | FuBi::TypeSpec::FLAG_HANDLE) )
				{
					Emit( Op::CODE_POP1, node );
				}

				Emit( Op::CODE_CALL, node );
				m_Pack->EmitWord( found->m_SerialID );
			}
			// direct call?
			else if ( node.m_Type.m_Flags & (FuBi::TypeSpec::FLAG_POINTER | FuBi::TypeSpec::FLAG_REFERENCE) )
			{
				if ( ctx.m_ClassOffsetAdjust == 0 )
				{
					Emit( Op::CODE_CALLMEMBER, node );
					m_Pack->EmitWord( found->m_SerialID );
				}
				else
				{
					Emit( Op::CODE_CALLMEMBERBASE, node );
					m_Pack->EmitWord( found->m_SerialID );
					m_Pack->EmitWord( ctx.m_ClassOffsetAdjust );
				}
			}
			// managed handle?
			else if ( node.m_Type.m_Flags & FuBi::TypeSpec::FLAG_HANDLE )
			{
				// verify we have handle capability
				if ( !(gFuBiSysExports.FindClass( node.m_Type.m_Type )->m_Flags & FuBi::ClassSpec::FLAG_MANAGED) )
				{
					Message( MESSAGE_ERROR, "'%s': illegal attempted handle call on non-managed class '%s'",
							 ctx.m_FuncName, gFuBiSysExports.FindNiceTypeName( node.m_Type.m_Type ) );
				}

				// convert to pointer first
				Emit( Op::CODE_HDLTOPTR, node );
				m_Pack->EmitWord( node.m_Type.m_Type );
				m_Pack->EmitByte( thisDepth );

				// now do function call
				if ( ctx.m_ClassOffsetAdjust == 0 )
				{
					Emit( Op::CODE_CALLMEMBER, node );
					m_Pack->EmitWord( found->m_SerialID );
				}
				else
				{
					Emit( Op::CODE_CALLMEMBERBASE, node );
					m_Pack->EmitWord( found->m_SerialID );
					m_Pack->EmitWord( ctx.m_ClassOffsetAdjust );
				}
			}
			// singleton?
			else if ( found->m_Parent->m_Flags & FuBi::ClassSpec::FLAG_SINGLETON )
			{
				if ( ctx.m_ClassOffsetAdjust == 0 )
				{
					Emit( Op::CODE_CALLSINGLETON, node );
					m_Pack->EmitWord( found->m_SerialID );
				}
				else
				{
					Emit( Op::CODE_CALLSINGLETONBASE, node );
					m_Pack->EmitWord( found->m_SerialID );
					m_Pack->EmitWord( ctx.m_ClassOffsetAdjust );
				}
			}
			else
			{
				// well jeez then there's no way to call this
				Message( MESSAGE_ERROR, "'%s': illegal attempted singleton call on non-singleton class '%s'",
						 ctx.m_FuncName, gFuBiSysExports.FindNiceTypeName( node.m_Type.m_Type ) );
				ctx.m_Status = CALLSTATUS_REPORTED;
				success = false;
			}

			if ( success )
			{
				// mask off high bits - this is important because the compiler
				// will only set what it needs. so if you call a bool function
				// it will only set 'al' rather than 'eax' to true or false.
				if ( found->m_ReturnType.IsPassByValue() )
				{
					int sizeBytes = gFuBiSysExports.GetVarTypeSizeBytes( found->m_ReturnType.m_Type );
					gpassert( sizeBytes != -1 );
					if ( sizeBytes == 2 )
					{
						Emit( Op::CODE_MASK2, node );
					}
					else if ( sizeBytes == 1 )
					{
						Emit( Op::CODE_MASK3, node );
					}
				}

				node.m_Type = found->m_ReturnType;
				node.m_Token = T_FUNCCALL;
				ctx.m_Status = CALLSTATUS_SUCCESS;
			}
		}
	}

// Error reporting.

	// print out the error message
	if ( ctx.m_Status != CALLSTATUS_SUCCESS )
	{
		switch ( ctx.m_Status )
		{
			case ( CALLSTATUS_UNDEFINED ):
			{
				if ( optional )
				{
					// probably wasn't *supposed* to be a function
					ctx.m_Status = CALLSTATUS_SUCCESS;
				}
				else
				{
					Message( MESSAGE_ERROR, "'%s': undefined function", ctx.m_FuncName );
				}
			}	break;

			case ( CALLSTATUS_PARAMCOUNT ):
			{
				if ( ctx.m_MethodIndex->size() == 1 )
				{
					const FuBi::FunctionSpec* firstSpec = *ctx.m_MethodIndex->begin();
					if ( firstSpec->m_Flags & FuBi::FunctionSpec::FLAG_VARARG )
					{
						Message( MESSAGE_ERROR, "'%s': function requires at least %d parameters", ctx.m_FuncName, firstSpec->m_ParamSpecs.size() );
					}
					else
					{
						Message( MESSAGE_ERROR, "'%s': function does not take %d parameters", ctx.m_FuncName, ctx.m_ParamCount );
					}
				}
				else
				{
					Message( MESSAGE_ERROR, "'%s': no overloaded function takes %d parameters", ctx.m_FuncName, ctx.m_ParamCount );
				}
			}	break;

			case ( CALLSTATUS_CANNOTSKRIT ):
			{
				Message( MESSAGE_ERROR, "'%s': cannot be called from a Skrit - possibly too complex, check FuBi docs", ctx.m_FuncName );
			}	break;

			case ( CALLSTATUS_TYPECONVERSION ):
			{
				gpassert( badParam != NULL );
				if ( ctx.m_MethodIndex->size() == 1 )
				{
					int paramIndex = badParam->m_Int;
					const FuBi::TypeSpec& from = badParam->m_Type;
					const FuBi::TypeSpec& to = (*ctx.m_MethodIndex)[0]->m_ParamSpecs[ paramIndex ].m_Type;

					char buffer[ 20 ];
					::sprintf( buffer, "parameter %d", paramIndex + 1 );
					ReportConversionError( ctx.m_FuncName, buffer, from, to, *badParam );
				}
				else
				{
					int paramIndex = badParam->m_Int;
					Message( MESSAGE_ERROR, "'%s': none of the %d overloads can convert parameter %d from type '%s'",
							 ctx.m_FuncName, ctx.m_MethodIndex->size(), paramIndex + 1,
							 gFuBiSysExports.FindNiceTypeName( badParam->m_Type ) );
				}
			}	break;

			case ( CALLSTATUS_AMBIGUOUS ):
			{
				Message( MESSAGE_ERROR, "'%s': ambiguous call to overloaded function", ctx.m_FuncName );
			}	break;

			case ( CALLSTATUS_REPORTED ):
			{
				// $ it's already been reported
			}	break;
		}
	}

	// now do the error but don't destroy the type if this was an optional call
	if ( ctx.m_Status != CALLSTATUS_SUCCESS )
	{
		node.m_Type = FuBi::VAR_UNKNOWN;
		return ( false );
	}

	// all ok
	return ( true );
}

bool Compiler :: EmitUserCall( ParseNode& node, bool optional )
{
	bool ok = false;

	// find the function
	Symbol* funcSym = FindSymbol( node.ResolveString() );
	if ( (funcSym != NULL) && ((funcSym->m_SymType == SYMTYPE_FUNCTION) || (funcSym->m_SymType == SYMTYPE_EVENT)) )
	{
		// it's a local skrit call
		const char* funcName = funcSym->m_Name.c_str();
		const Raw::ExportEntry* funcExport = rcast <const Raw::ExportEntry*> (
				&*m_Pack->m_Exports.begin() + funcSym->m_Offset );

		// check param count
		int paramCount = node.GetLinkCount();
		if ( paramCount == funcExport->m_ParamCount )
		{
			// check for type compatibility and do conversions
			const eVarType* i, * ibegin = funcExport->GetParamsBegin(), * iend = funcExport->GetParamsEnd();
			ParseNode* inode = NULL;
			bool fail = false;
			for ( i = ibegin, inode = node.m_Next ; i != iend ; ++i, inode = inode->m_Next )
			{
				int paramIndex = i - ibegin;
				gpassert( inode != NULL );
				FuBi::TypeSpec dstSpec = Raw::ExportEntry::BuildTypeSpec( *i );

				// test for param conversion
				if ( (inode->m_Token == T_NULL) && FuBi::IsUser( dstSpec.m_Type ) && !dstSpec.IsPassByValue() )
				{
					// $ we're cool - special support for passing NULL for references
				}
				else
				{
					FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( inode->m_Type, dstSpec );
					if ( compare == FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
					{
						// on failure to convert...
						char buffer[ 20 ];
						::sprintf( buffer, "parameter %d", paramIndex + 1 );
						ReportConversionError( funcName, buffer, inode->m_Type, dstSpec, *inode );
						fail = true;
						break;
					}
					else if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
					{
						// do a conversion
						EmitConversion( "parameter", inode->m_Type, dstSpec, paramCount - paramIndex - 1, *inode );
					}
				}
			}

			// got through them all ok?
			if ( !fail )
			{
				// re-acquire the symbol - conversion fun up above may have
				// caused a realloc in the symbol table
				funcSym = FindSymbol( node.ResolveString() );

				// emit the function call
				Emit( Op::CODE_CALLSKRIT, node );
				m_Pack->EmitWord( funcSym->m_Index );
				node.m_Type = Raw::ExportEntry::BuildTypeSpec( funcExport->m_ReturnType );
				node.m_Token = T_FUNCCALL;
				ok = true;
			}
		}
		else
		{
			Message( MESSAGE_ERROR, "'%s': Skrit function does not take %d parameters", funcName, paramCount );
		}
	}
	else
	{
		if ( optional )
		{
			// probably wasn't *supposed* to be a function
			gpassert( node.m_Next == NULL );
			ok = true;
		}
		else
		{
			m_OverrideLocation = node.m_Location;
			Message( MESSAGE_ERROR, "'%s': found userid, expecting sysid - did you accidentally add a '$'?",
					 node.ResolveString() );
		}
	}
	return ( ok );
}

bool Compiler :: EmitConditionalBranch( ParseNode& conditional, ParseNode& expr )
{
	TempVarCheckpoint( expr );

	if ( (expr.m_Type == FuBi::VAR_INT) || (expr.m_Type == FuBi::VAR_BOOL) )
	{
		switch ( expr.m_Token )
		{
			case ( T_BOOLCONSTANT ):
			case ( T_INTCONSTANT ):
			{
				m_OverrideLocation = expr.m_Location;
				Message( MESSAGE_WARNING, "'if-expr': conditional expression is constant" );
			}	break;
		}
		Emit( Op::CODE_BRANCHZERO, conditional );
		conditional.m_Int = GetCodeOffset();

		// emit temporary branch location (will be resolved at else_part level)
		m_Pack->EmitWord( 0 );

		return ( true );
	}
	else
	{
		m_OverrideLocation = expr.m_Location;
		Message( MESSAGE_ERROR, "'if-expr': bool/int type expected, found '%s'",
				 gFuBiSysExports.FindNiceTypeName( expr.m_Type ) );
		return ( false );
	}
}

bool Compiler :: EmitLoopGoto( ParseNode& node, ePatchType patchType, const char* keyword )
{
	TempVarCheckpoint( node );

	// find innermost loop
	int popCount = 1;
	for ( Scope* scope = GetBackScope() ; scope != NULL ; scope = scope->m_Parent, ++popCount )
	{
		if ( scope->m_IsLoop )
		{
			break;
		}
	}

	// better be a loop
	if ( scope == NULL )
	{
		Message( MESSAGE_ERROR, "'%s': loop control keyword found outside of a loop", keyword );
		return ( false );
	}

	// add unconditional branch (to be patched at scope dtor)
	LeaveScope( node, popCount, FREESCOPE_NODELETE );
	Emit( Op::CODE_BRANCH, node );
	PatchEntry patch;
	patch.m_PatchPoint = GetCodeOffset();
	patch.m_Type = patchType;
	scope->m_PatchPoints.push_back( patch );
	m_Pack->EmitSword( 0 );

	// ok
	return ( true );
}

bool Compiler :: ResolvePostFixExpr( ParseNode& node )
{
	// will get here through any of these methods:
	//
	//   1. an actual function call that has already been resolved. it will have
	//      a token of type T_FUNCCALL. ignore it.
	//
	//   2. a definite statement-style class method function call (void params).
	//      attempt to add it as a call, error out if not. it will have a token
	//      of type T_DEREFERENCE.
	//
	//   3. just a primary expression - type may be unknown, try to resolve it
	//      to a local user variable if an id, ignore otherwise.
	//
	//   4. a possible statement-style global function call (void params). type
	//      will be unknown, attempt to add it as a call.
	//

// Case 1.

	if ( node.m_Token == T_FUNCCALL )
	{
		return ( true );
	}

// Case 2.

	if ( node.m_Token == T_DEREFERENCE )
	{
		return ( EmitCall( node ) );
	}

// Cases 3 and 4.

	// EmitCall() will print out its own errors so only handle the success case
	if (   ((node.m_Token == T_SYSID) || (node.m_Token == T_USERID))
		&& EmitCall( node, true ) )
	{
		// look up symbol name
		const char* symName = node.ResolveString();
		Symbol* symbol = FindSymbol( symName );
		if ( (symbol != NULL) && (symbol->m_SymType != SYMTYPE_FUNCTION) )
		{
			// small note on why this global thing is necessary. the problem is
			// that we need to know the entire set of globals at the start of
			// skrit construction. but at the time of dereferencing an
			// individual variable we need to know the entire set of globals,
			// but those are not available until the entire parse is finished.
			// the alternative would be to go back and patch up the indexes on
			// local vars to match the total global counts.

			switch ( symbol->m_SymType )
			{
				case ( SYMTYPE_VARIABLE ):
				{
					Emit( Op::CODE_LOADVAR, node );
				}	break;

				case ( SYMTYPE_STRING ):
				{
					Emit( Op::CODE_LOADSTR, node );
					if ( symbol->m_Store == Raw::STORE_LOCAL )
					{
						node.m_Token = T_TEMPEVAL;	// this is a "temporary" variable if based on local var (the way C++ means temporary)
					}
				}	break;

				case ( SYMTYPE_HANDLE ):
				{
					Emit( Op::CODE_LOADHDL, node );
				}	break;

				default:
				{
					Message( MESSAGE_ERROR, "'%s': not a variable", symName );
					return ( false );
				}
			}

			m_Pack->EmitWord( symbol->m_Index );
			m_Pack->EmitByte( symbol->m_Store );
			node.m_Type = symbol->m_VarType;
		}

		if ( node.m_Type == FuBi::VAR_UNKNOWN )
		{
			Message( MESSAGE_ERROR, "'%s': undeclared identifier", symName );
			return ( false );
		}
	}
	return ( true );
}

const FuBi::FunctionSpec* Compiler :: FindFuBiEvent( const gpstring& funcName )
{
	const FuBi::FunctionSpec* eventSpec = NULL;

	FuBi::FunctionByNameIndex::const_iterator found = m_EventClass->m_Functions.find( funcName );
	if ( found != m_EventClass->m_Functions.end() )
	{
		gpassert( found->second.size() == 1 );
		eventSpec = *found->second.begin();
	}

	return ( eventSpec );
}

bool Compiler :: CheckEventBinding( void )
{
	// $ remember that the first param is a skrit object

	// get the function to match
	const Raw::ExportEntry* export = m_CurrentFunction.m_Export;
	gpassert( export != NULL );
	const FuBi::FunctionSpec* spec = gFuBiSysExports.FindFunctionBySerialID( export->m_EventBinding );
	gpassert( spec != NULL );

	// check for a match
	bool success = false;
	int badParam;
	switch ( export->CheckFunctionMatch( 1, spec, &badParam ) )
	{
		case ( Raw::ExportEntry::MATCH_OK ):
		{
			success = true;
		}	break;

		case ( Raw::ExportEntry::MATCH_DIFF_PARAM_COUNT ):
		{
			Message( MESSAGE_WARNING, "'%s': event does not receive %d parameters, handler cannot bind and will be ignored",
					 spec->m_Name.c_str(), export->m_ParamCount );
		}	break;

		case ( Raw::ExportEntry::MATCH_PARAM_MISMATCH ):
		{
			Message( MESSAGE_WARNING, "'%s': event parameter list does not match exactly starting at parameter %d, handler cannot bind and will be ignored",
					 spec->m_Name.c_str(), badParam + 1 );
		}	break;
	}

	// all ok
	return ( success );
}

FuBi::TypeSpec Compiler :: GetFunctionReturn( void ) const
{
	if ( m_CurrentFunction.m_Export != NULL )
	{
		return ( Raw::ExportEntry::BuildTypeSpec( m_CurrentFunction.m_Export->m_ReturnType ) );
	}
	else
	{
		return ( FuBi::VAR_VOID );
	}
}

void Compiler :: EndFunction( ParseNode& brace )
{
	// only do this if we haven't already returned from the function
	if (   !(Op::GetData( m_CurrentFunction.m_LastOpCode ).m_Traits & Op::TRAIT_RETURN)
		|| (m_CurrentFunction.m_LastOpCodeScope > 1)  )
	{
		// warn about not returning a value
		if ( GetFunctionReturn() != FuBi::VAR_VOID )
		{
			// warn
			m_OverrideLocation = brace.m_Location;
			Message( MESSAGE_WARNING, "'%s': non-void function must return a value",
					 m_CurrentFunction.m_Export->m_Name );

			// store dummy value on the stack
			EmitPush( 0, brace );
		}

		// back out to global scope
		LeaveToGlobalScope( brace );

		// return from function
		Emit( Op::CODE_RETURN0, brace );
	}
}

void Compiler :: TempVarCheckpoint( const ParseNode& checkNode )
{
	// bail early if no temporaries
	if ( m_TempStringCount == 0 )
	{
		return;
	}

	// clean up variables
	Scope* scope = GetBackScope();
	gpassert( scope->m_StringCount >= m_TempStringCount );
	Emit( Op::CODE_FREESTR, checkNode );
	m_Pack->EmitByte( m_TempStringCount );

	// delete the symbols
	gpassert( m_TempStringCount <= (scast <int> ( m_Symbols.size() ) - m_StartOfLocalSymbols) );
	m_Symbols.erase( m_Symbols.end() - m_TempStringCount, m_Symbols.end() );

	// no more
	scope->m_StringCount -= m_TempStringCount;
	m_TempStringCount = 0;
}

Compiler::Symbol* Compiler :: AddOrFindState( const char* name, Location location, bool declare )
{
	// add state
	bool addResults;
	Symbol* symbol = AddSymbol( name, location, SYMTYPE_STATE, FuBi::VAR_UNKNOWN, &addResults );

	// new state?
	if ( symbol != NULL )
	{
		// update index
		if ( addResults )
		{
			symbol->m_Index = scast <int> ( m_States.size() );

			// new entry in table
			State newState;
			newState.m_SymbolIndex    = symbol->m_SymbolIndex;
			newState.m_StateIndex     = symbol->m_Index;
			newState.m_Declared       = declare;
			newState.m_Config.m_State = scast <WORD> ( symbol->m_Index );
			m_States.push_back( newState );
			++GetStateHeader()->m_TotalStates;

			// debug info
#			if !GP_RETAIL
			if ( m_DebugMode )
			{
				Raw::DebugState dbgState;
				dbgState.m_State = scast <WORD> ( newState.m_StateIndex );
				dbgState.m_Size  = scast <WORD> ( dbgState.m_Size + symbol->m_Name.length() + 1 );
				AppendToBuffer( m_Pack->m_Debug, dbgState );
				AppendToBuffer( m_Pack->m_Debug, symbol->m_Name );
			}
#			endif // !GP_RETAIL
		}
		else if ( declare )
		{
			// it's been declared now, so it's safe
			m_States[ symbol->m_Index ].m_Declared = true;
		}
	}

	// if this is the first state referenced, make it the startup by default
	if ( !m_GlobalState.m_SetStartup )
	{
		GetStateHeader()->m_InitialState = scast <WORD> ( symbol->m_Index );
		m_GlobalState.m_SetStartup = true;
	}

	// return state symbol
	return ( symbol );
}

void Compiler :: PrepEmitState( void )
{
	// call this function to make sure that the state info output is set up
	if ( m_Pack->m_State.empty() )
	{
		// add it
		m_Pack->m_State.resize( sizeof( Raw::State ) );
		Raw::State* state = rcast <Raw::State*> ( &*m_Pack->m_State.begin() );

		// init it
		state->m_InitialState = scast <WORD> ( STATE_INDEX_NONE );
		state->m_TotalStates  = 0;
	}
}

Raw::State* Compiler :: GetStateHeader( void )
{
	PrepEmitState();
	return ( rcast <Raw::State*> ( &*m_Pack->m_State.begin() ) );
}

Compiler::Symbol* Compiler :: GetStateSymbol( int state )
{
	gpassert( (state >= 0) && (state < scast <int> ( m_States.size() )) );
	return ( &*(m_Symbols.begin() + m_States[ state ].m_SymbolIndex) );
}

Compiler::Symbol* Compiler :: GetCurrentStateSymbol( void )
{
	return ( GetStateSymbol( m_GlobalState.m_Current ) );
}

void Compiler :: EmitStateConfigs( void )
{
	States::iterator i, begin = m_States.begin(), end = m_States.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_NeedsSaving )
		{
			PrepEmitState();
			AppendToBuffer( m_Pack->m_State, i->m_Config );
			i->m_NeedsSaving = false;
		}
	}
}

void Compiler :: EmitTransition( void )
{
	PrepEmitState();
	gpassert( (m_Transition.m_Statics.size() + m_Transition.m_Dynamics.size()) < 256 );

	// set up base struct
	Raw::StateTransition out;
	out.m_FromState          = scast <WORD> ( m_Transition.m_FromState );
	out.m_ToState            = scast <WORD> ( m_Transition.m_ToState   );
	out.m_ResponseCodeOffset = m_Transition.m_ResponseOffset;
	out.m_ConditionCount     = scast <BYTE> ( m_Transition.m_Statics.size() + m_Transition.m_Dynamics.size() );

	// out base struct
	UINT baseOffset = AppendToBuffer( m_Pack->m_State, out );

	// add static conditions
	Transition::StaticColl::const_iterator i,
			ibegin = m_Transition.m_Statics.begin(),
			iend   = m_Transition.m_Statics.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		AppendToBuffer( m_Pack->m_State, i->m_Condition );
		m_Pack->m_State.insert( m_Pack->m_State.end(), i->m_TestData.begin(), i->m_TestData.end() );
	}

	// add dynamic conditions
	Transition::DynamicColl::const_iterator j,
			jbegin = m_Transition.m_Dynamics.begin(),
			jend   = m_Transition.m_Dynamics.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		// these are easy
		AppendToBuffer( m_Pack->m_State, *j );
	}

	BYTE* basePtr = &*m_Pack->m_State.begin() + baseOffset;
	rcast <Raw::StateEntry*> ( basePtr )->m_Size = scast <WORD> ( m_Pack->m_State.end() - basePtr );
}

bool Compiler :: EmitStaticCondition( ParseNode& eventNode, bool isTrigger )
{
	// don't leak
	ParseNode::AutoDeleteChain autoDelete( eventNode );

	// check to make sure it's sysid
	if ( !isTrigger && (eventNode.m_Token == T_USERID) )
	{
		Message( MESSAGE_ERROR, "'%s': found userid, expecting sysid - did you accidentally add a '$'?",
				 eventNode.ResolveString() );
		return ( false );
	}

	// figure out the event
	const char* eventName = eventNode.ResolveString();
	const FuBi::FunctionSpec* eventSpec = NULL;
	if ( isTrigger )
	{
		// remove the trailing '$' first
		gpstring localName( eventName );
		gpassert( *localName.rbegin() == '$' );
		localName.erase( localName.length() - 1 );

		// now find it
		eventSpec = FindFuBiEvent( localName );
	}
	else
	{
		eventSpec = FindFuBiEvent( eventName );
	}

	if ( eventSpec == NULL )
	{
		Message( MESSAGE_ERROR, "'%s': not an event\n", eventName );
		return ( false );
	}

	// verify that we don't have more params than are accepted
	if ( eventNode.GetLinkCount() > scast <int> ( eventSpec->m_ParamSpecs.size() - 1 ) )
	{
		Message( MESSAGE_ERROR, "'%s': too many parameters (%d) in event test, event only uses %d",
				 eventSpec->m_Name.c_str(), eventNode.GetLinkCount(), eventSpec->m_ParamSpecs.size() - 1 );
		return ( false );
	}

	// verify and convert what we support: int, bool, enum, exact float
	bool fail = false;
	FuBi::FunctionSpec::ParamSpecs::const_iterator testbegin = eventSpec->m_ParamSpecs.begin() + 1, test = testbegin;
	for ( ParseNode* i = eventNode.m_Next ; i != NULL ; i = i->m_Next, ++test )
	{
		// not supporting strings yet $$$ finish this
		if ( test->m_Type.IsAString() )
		{
			Message( MESSAGE_ERROR, "'%s': strings not supported for static condition testing (parameter %d) - use dynamic testing instead",
					 eventSpec->m_Name.c_str(), test - testbegin + 1 );
			fail = true;
			continue;
		}

		// not ever supporting anything wacky
		if ( !test->m_Type.IsSimple() )
		{
			Message( MESSAGE_ERROR, "'%s': parameters of type '%s' not supported for static condition testing (parameter %d) - use dynamic testing instead",
					 eventSpec->m_Name.c_str(), gFuBiSysExports.FindNiceTypeName( test->m_Type ), test - testbegin + 1 );
			fail = true;
			continue;
		}

		// check for compatibility
		if ( !ConvertConstant( *i, test->m_Type ) )
		{
			ReportConversionError( eventSpec->m_Name,
								   gpstringf( "parameter %d", test - testbegin + 1 ),
								   i->m_Type,
								   test->m_Type,
								   *i );
			fail = true;
			continue;
		}
	}
	if ( fail )
	{
		return ( false );
	}

	// now add it
	ConditionStatic* cond = &*m_Transition.m_Statics.insert( m_Transition.m_Statics.end() );
	cond->m_Condition.m_EventSerial = scast <WORD> ( eventSpec->m_SerialID );
	for ( i = eventNode.m_Next ; i != NULL ; i = i->m_Next )
	{
		AppendToBuffer( cond->m_TestData, i->m_Raw );
	}

	// done
	cond->Update();
	return ( true );
}

void Compiler :: FinishCompile( void )
{
	// report any remaining unused vars
	ReportUnusedVars( m_Symbols.size() );

	// report any unused states
	if ( m_OptExplicitStatesOnly )
	{
		States::const_iterator i, ibegin = m_States.begin(), iend = m_States.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( !i->m_Declared )
			{
				Symbol* symbol = &*(m_Symbols.begin() + i->m_SymbolIndex);
				m_OverrideLocation = symbol->m_Location;
				Message( MESSAGE_WARNING, "state '%s' was used but never explicitly declared",
						 symbol->m_Name.c_str() );
			}
		}
	}

	// emit state config info
	EmitStateConfigs();
}

void Compiler :: Emit( Op::Code data, const ParseNode& node )
{
	gpassert( !(Op::GetTraits( data ) & Op::TRAIT_DUMMY) );

#	if !GP_RETAIL
	if ( m_DebugMode )
	{
		// add debug entry to track line/instruction locations
		int line = node.m_Location.m_Line;
		if ( m_CurrentFunction.m_LastLine < line )
		{
			m_CurrentFunction.m_LastLine = line;
			Raw::DebugLocation dbgLoc;
			dbgLoc.m_CodeOffset = GetCodeOffset();
			dbgLoc.m_Line = scast <WORD> ( line );
			dbgLoc.m_SourceIndex = scast <WORD> ( m_Scanners.back().m_ScannerIndex );
			AppendToBuffer( m_Pack->m_Debug, dbgLoc );
		}
	}
#	endif // !GP_RETAIL

	m_Pack->EmitByte( scast <DWORD> ( data ) );
	m_CurrentFunction.m_LastOpCode = data;
	m_CurrentFunction.m_LastOpCodeScope = m_LocalScopes.size();
}

void Compiler :: EmitPush( int data, const ParseNode& node )
{
	if (    (data <= std::numeric_limits <char>::max())
		 && (data >= std::numeric_limits <char>::min()) )
	{
		Emit( Op::CODE_PUSH1, node );
		m_Pack->EmitChar( data );
	}
	else if (    (data <= std::numeric_limits <short>::max())
		 	  && (data >= std::numeric_limits <short>::min()) )
	{
		Emit( Op::CODE_PUSH2, node );
		m_Pack->EmitSword( data );
	}
	else
	{
		Emit( Op::CODE_PUSH4, node );
		m_Pack->EmitDword( data );
	}
}

int Compiler :: GetCodeOffset( void ) const
{
	return ( scast <int> ( m_Pack->m_ByteCode.size() ) );
}

void Compiler :: PatchCodeWord( int offset, int newValue )
{
	gpassert( (offset >= 0) && (offset <= (scast <int> ( m_Pack->m_ByteCode.size() - 2 ))) );
	*rcast <WORD*> ( &*m_Pack->m_ByteCode.begin() + offset ) = scast <WORD> ( newValue );
}

bool Compiler :: ConvertConstant( ParseNode& srcNode, const FuBi::TypeSpec& destType )
{
	FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( srcNode.m_Type, destType );
	if ( compare == FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
	{
		// $ special: test for int-to-pointerclass (this is ok!)
		if ( !destType.IsPointerClass() || !srcNode.m_Type.IsSimpleInteger() )
		{
			return ( false );
		}
	}
	else if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
	{
		if ( destType == FuBi::VAR_FLOAT )
		{
			// only expecting ints
			gpassert( srcNode.m_Type == FuBi::VAR_INT );
			srcNode.m_Float = scast <float> ( srcNode.m_Int );
		}
		else if ( destType == FuBi::VAR_INT )
		{
			// only expecting bools and floats, and bools are already ok
			gpassert( (srcNode.m_Type == FuBi::VAR_BOOL) || (srcNode.m_Type == FuBi::VAR_FLOAT) );
			if ( srcNode.m_Type == FuBi::VAR_FLOAT )
			{
				m_OverrideLocation = srcNode.m_Location;
				Message( MESSAGE_WARNING, "'constant': conversion from 'float' to '%s', possible loss of accuracy",
						 gFuBiSysExports.FindNiceTypeName( destType ) );
				srcNode.m_Int = scast <int> ( srcNode.m_Float );
			}
		}
		else if ( destType == FuBi::VAR_BOOL )
		{
			// only expecting integers at this point
			gpassert( srcNode.m_Type == FuBi::VAR_INT );
			srcNode.m_Int = !!srcNode.m_Int;
		}
	}

#	if GP_DEBUG
	if ( srcNode.m_Type.IsAString() || destType.IsAString() )
	{
		// can only do gpstring <- const char*
		gpassert( srcNode.m_Type.IsCString() );
		gpassert( destType.IsString() );
	}
#	endif // GP_DEBUG

	return ( true );
}

bool Compiler :: EmitPromotion( ParseNode& l, ParseNode& op, ParseNode& r )
{

// Equality.

	if ( l.m_Type.GetBaseType() == r.m_Type.GetBaseType() )
	{
		return ( true );
	}

// Promotion: int-to-float.

	if ( (l.m_Type == FuBi::VAR_FLOAT) && (r.m_Type == FuBi::VAR_INT) )
	{
		Emit( Op::CODE_INTTOFLT, op );
		m_Pack->EmitByte( 0 );
		r.m_Type = l.m_Type;
		return ( true );
	}
	if ( (r.m_Type == FuBi::VAR_FLOAT) && (l.m_Type == FuBi::VAR_INT) )
	{
		Emit( Op::CODE_INTTOFLT, op );
		m_Pack->EmitByte( 1 );
		l.m_Type = r.m_Type;
		return ( true );
	}

// Promotion: int-to-bool.

	if ( (l.m_Type == FuBi::VAR_BOOL) && (r.m_Type == FuBi::VAR_INT) )
	{
		Emit( Op::CODE_INTTOBOO, op );
		m_Pack->EmitByte( 0 );
		r.m_Type = l.m_Type;
		return ( true );
	}
	if ( (r.m_Type == FuBi::VAR_BOOL) && (l.m_Type == FuBi::VAR_INT) )
	{
		Emit( Op::CODE_INTTOBOO, op );
		m_Pack->EmitByte( 1 );
		l.m_Type = r.m_Type;
		return ( true );
	}

// Promotion: pointer_class/enum-to-int.

	if ( (l.m_Type == FuBi::VAR_INT) && r.m_Type.IsPointerClass() )
	{
		l.m_Type = r.m_Type;
		return ( true );
	}

	if ( (r.m_Type == FuBi::VAR_INT) && l.m_Type.IsPointerClass() )
	{
		r.m_Type = l.m_Type;
		return ( true );
	}

	if ( (l.m_Type == FuBi::VAR_INT) && FuBi::IsEnum( r.m_Type.m_Type ) )
	{
		l.m_Type = r.m_Type;
		return ( true );
	}

	if ( (r.m_Type == FuBi::VAR_INT) && FuBi::IsEnum( l.m_Type.m_Type ) )
	{
		r.m_Type = l.m_Type;
		return ( true );
	}

// Promotion: const char*-to-string.

	if ( l.m_Type.IsString() && r.m_Type.IsCString() )
	{
		// add temp
		gpverify( AddTempVariable( r, l.m_Type ) );
		Symbol* symbol = &m_Symbols.back();

		// store string in temp then get temp's pointer back
		Emit( Op::CODE_CSTRTOSTR, r );
		m_Pack->EmitWord( symbol->m_Index );
		m_Pack->EmitByte( symbol->m_Store );
		m_Pack->EmitByte( 0 );
		return ( true );
	}

	if ( r.m_Type.IsString() && l.m_Type.IsCString() )
	{
		// add temp
		gpverify( AddTempVariable( l, r.m_Type ) );
		Symbol* symbol = &m_Symbols.back();

		// store string in temp then get temp's pointer back
		Emit( Op::CODE_CSTRTOSTR, l );
		m_Pack->EmitWord( symbol->m_Index );
		m_Pack->EmitByte( symbol->m_Store );
		m_Pack->EmitByte( 1 );

		// update type
		l.m_Type = r.m_Type;
		return ( true );
	}

// Special: NULL.

	if (   ((l.m_Token == T_NULL) && FuBi::IsUser( r.m_Type.m_Type ))
	    || ((r.m_Token == T_NULL) && FuBi::IsUser( l.m_Type.m_Type )) )
	{
		return ( true );
	}

// Incompatibility.

	return ( false );
}

void Compiler :: EmitConversion( const char* what, const FuBi::TypeSpec& from, const FuBi::TypeSpec& to, int index, ParseNode& node, bool warnOnCast )
{
	if ( to == FuBi::VAR_FLOAT )						// convert to float
	{
		// only expecting integers at this point
		gpassert( from.IsSimpleInteger() );
		Emit( Op::CODE_INTTOFLT, node );
		m_Pack->EmitByte( index );
	}
	else if ( to == FuBi::VAR_BOOL )					// convert to bool
	{
		// only expecting integers at this point
		gpassert( from.IsSimpleInteger() );
		m_OverrideLocation = node.m_Location;
		if ( warnOnCast )
		{
			Message( MESSAGE_WARNING, "'%s': conversion from '%s' to 'bool', possibly unexpected results",
					 what, gFuBiSysExports.FindNiceTypeName( from ) );
		}
		Emit( Op::CODE_INTTOBOO, node );
		m_Pack->EmitByte( index );
	}
	else if ( to.IsSimpleInteger() )					// convert to int
	{
		// only expecting bools and floats at this point,
		//  and bools are already ok, no conversion necessary
		gpassert( (from == FuBi::VAR_FLOAT) || (from == FuBi::VAR_BOOL));
		if ( from == FuBi::VAR_FLOAT )
		{
			m_OverrideLocation = node.m_Location;
			if ( warnOnCast )
			{
				Message( MESSAGE_WARNING, "'%s': conversion from 'float' to '%s', possible loss of accuracy",
						 what, gFuBiSysExports.FindNiceTypeName( to ) );
			}
			Emit( Op::CODE_FLTTOINT, node );
			m_Pack->EmitByte( index );
		}
	}
	else if ( FuBi::IsUser( to.m_Type ) )				// convert handle to pointer
	{
		// only expecting handle-to-ptr conversions of the same or derived type
		// and/or derived-to-base offsets
		gpassert( to.m_Flags & (FuBi::TypeSpec::FLAG_POINTER | FuBi::TypeSpec::FLAG_REFERENCE) );
		gpassert( (to.m_Type == from.m_Type) || gFuBiSysExports.IsDerivative( from.m_Type, to.m_Type ) );

		// possible handle-to-ptr conversion
		if ( from.m_Flags & FuBi::TypeSpec::FLAG_HANDLE )
		{
			gpassert( to.m_Flags & FuBi::TypeSpec::FLAG_POINTER );

			// verify we have handle capability
			if ( !(gFuBiSysExports.FindClass( from.m_Type )->m_Flags & FuBi::ClassSpec::FLAG_MANAGED) )
			{
				Message( MESSAGE_ERROR, "'%s': illegal attempted ptr-to-handle conversion on non-managed class '%s'",
						 what, gFuBiSysExports.FindNiceTypeName( from.m_Type ) );
			}

			// do the conversion
			Emit( Op::CODE_HDLTOPTR, node );
			m_Pack->EmitWord( from.m_Type );
			m_Pack->EmitByte( index );
		}

		// fix up derivative-to-base conversions
		int offset = gFuBiSysExports.GetDerivedBaseOffset( from.m_Type, to.m_Type );
		if ( offset > 0 )
		{
			Emit( Op::CODE_BASEADJUST, node );
			m_Pack->EmitWord( offset );
			m_Pack->EmitByte( index );
		}
	}
	else if ( to.IsString() )							// convert const char* to gpstring
	{
		// only expecting const char* -> gpstring
		gpassert( from.IsCString() );

		// add a temp variable
		gpverify( AddTempVariable( node, to ) );
		Symbol* symbol = &m_Symbols.back();
		node.m_Token = T_TEMPEVAL;

		// store string in temp then get temp's pointer back
		Emit( Op::CODE_CSTRTOSTR, node );
		m_Pack->EmitWord( symbol->m_Index );
		m_Pack->EmitByte( symbol->m_Store );
		m_Pack->EmitByte( index );
	}
	else if ( to.IsCString() )							// convert gpstring to const char*
	{
		// only expecting gpstring -> const char*
		gpassert( from.IsString() );
		Emit( Op::CODE_STRTOPCC, node );
		m_Pack->EmitByte( index );
	}
	else
	{
		gpassert( 0 );		// unexpected conversion failure!
	}
}

void Compiler :: EmitCast( ParseNode& node, eVarType type )
{
	FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( node.m_Type, type );
	if ( compare != FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
	{
		// convert if necessary
		if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
		{
			EmitConversion( "expr", node.m_Type, type, 0, node, false );
			node.m_Type = type;
		}
	}
	else
	{
		ReportConversionError( "explicit cast", "expression", node.m_Type, type, node );
	}
}

void Compiler :: EmitBinaryOp( Op::Code opBase, const char* symbol, ParseNode& l, ParseNode& op, ParseNode& r )
{
	// special handling for '+' for strings
	if ( (opBase == Op::CODE_ADDI) && l.m_Type.IsAString() && r.m_Type.IsAString() )
	{
		// create a temporary to store result in
		FuBi::TypeSpec strType;
		strType.SetString();
		gpverify( AddTempVariable( op, strType ) );
		Symbol* symbol = &m_Symbols.back();

		// push it on the stack
		Emit( Op::CODE_LOADSTR, op );
		m_Pack->EmitWord( symbol->m_Index );
		m_Pack->EmitByte( symbol->m_Store );

		// now do the proper op
		if ( l.m_Type.IsString() )
		{
			if ( r.m_Type.IsString() )
			{
				Emit( Op::CODE_ADDSS, op );
			}
			else
			{
				gpassert( r.m_Type.IsCString() );
				Emit( Op::CODE_ADDSC, op );
			}
		}
		else
		{
			gpassert( l.m_Type.IsCString() );
			if ( r.m_Type.IsString() )
			{
				Emit( Op::CODE_ADDCS, op );
			}
			else
			{
				gpassert( r.m_Type.IsCString() );
				Emit( Op::CODE_ADDCC, op );
			}
		}

		// update types
		l.m_Token = T_TEMPEVAL;
		l.m_Type = strType;

		// bail
		return;
	}

	// normal handling
	if ( EmitPromotion( l, op, r ) )
	{
		bool success = true;

		if ( Op::GetTraits( opBase ) & Op::TRAIT_BITWISE )
		{
			if ( (l.m_Type == FuBi::VAR_INT) || l.m_Type.IsPointerClass() )
			{
				Emit( opBase, l );
			}
			else
			{
				success = false;
			}
		}
		else if ( Op::GetTraits( opBase ) & Op::TRAIT_LOGIC )
		{
			/* $$$ */
			// NOT IMPLEMENTED: || and && ALWAYS call the code for the right-
			// hand expr, should act like C and only exec on demand. this will
			// require changes to the grammar to support. see help on "sequence
			// points" for more info on a good way to do this.
			/* $$$ */

			if (    (l.m_Type == FuBi::VAR_INT)
				 || (l.m_Type == FuBi::VAR_BOOL)
				 || FuBi::IsUser( l.m_Type.m_Type ) )
			{
				Emit( opBase, l );
			}
			else
			{
				success = false;
			}
		}
		else
		{
			if (   (l.m_Type == FuBi::VAR_INT)
				&& (Op::GetTraits( opBase ) & Op::TRAIT_INT) )
			{
				Emit( opBase, l );
			}
			else if (   (l.m_Type == FuBi::VAR_FLOAT)
					 && (Op::GetTraits( opBase ) & Op::TRAIT_FLOAT) )
			{
				Emit( opBase + scast <Op::Code> ( 1 ), l );
			}
			else if (   (l.m_Type == FuBi::VAR_BOOL)
					 && (Op::GetTraits( opBase ) & Op::TRAIT_BOOL) )
			{
				Emit( opBase + scast <Op::Code> ( 2 ), l );
			}
			else if (   (l.m_Type.IsString())
					 && (Op::GetTraits( opBase ) & Op::TRAIT_STRING) )
			{
				Emit( opBase + scast <Op::Code> ( 3 ), l );
			}
			else if (   (l.m_Type.IsCString())
					 && (Op::GetTraits( opBase ) & Op::TRAIT_CSTRING) )
			{
				Emit( opBase + scast <Op::Code> ( 4 ), l );
			}
			else if (   FuBi::IsEnum( l.m_Type.m_Type )
					 && (Op::GetTraits( opBase ) & Op::TRAIT_ENUM) )
			{
				Emit( opBase, l );		// treat as int
			}
			else if (   FuBi::IsUser( l.m_Type.m_Type )
					 && (Op::GetTraits( opBase ) & Op::TRAIT_REFERENCE) )
			{
				Emit( opBase, l );		// treat as int
			}
			else
			{
				success = false;
			}
		}

		if ( !success )
		{
			Message( MESSAGE_ERROR, "'%s': illegal use of operator on type '%s'",
					 symbol, gFuBiSysExports.FindNiceTypeName( l.m_Type ) );
		}
	}
	else
	{
		Message( MESSAGE_ERROR, "'binary op': incompatible expression types '%s' and '%s'",
				 gFuBiSysExports.FindNiceTypeName( l.m_Type ),
				 gFuBiSysExports.FindNiceTypeName( r.m_Type ) );
	}

	if ( Op::GetTraits( opBase ) & (Op::TRAIT_COMPARE | Op::TRAIT_LOGIC) )
	{
		l.m_Type = FuBi::VAR_BOOL;
	}

	l.m_Token = T_EVAL;
}

bool Compiler :: EmitAssignment( ParseNode& l, ParseNode& op, ParseNode& r, const char* opText )
{
	if ( (l.m_Token != T_SYSID) && (l.m_Token != T_USERID) )
	{
		Message( MESSAGE_ERROR, "'%s': left expression cannot be assigned using this operator", opText );
		TempVarCheckpoint( r );
		return ( false );
	}

	// look up symbol name
	const char* symName = l.ResolveString();
	Symbol* symbol = FindSymbol( symName );
	if ( symbol == NULL )
	{
		Message( MESSAGE_ERROR, "'%s': undeclared identifier", symName );
		TempVarCheckpoint( r );
		return ( false );
	}

	l.m_Type = symbol->m_VarType;

	// special null case
	if (   (r.m_Type.m_Type  == FuBi::VAR_VOID)
		&& (r.m_Type.m_Flags == FuBi::TypeSpec::FLAG_POINTER) )
	{
		gpassert( r.m_Int == 0 );
		gpassert( r.m_Token == T_NULL );

		// store
		bool success = false;
		if ( symbol->m_SymType == SYMTYPE_VARIABLE )
		{
			if ( FuBi::IsUser( symbol->m_VarType.m_Type ) )
			{
				Emit( Op::CODE_STOREVAR, op );
				success = true;
			}
		}
		else if ( symbol->m_SymType == SYMTYPE_HANDLE )
		{
			Emit( Op::CODE_STOREHDL, op );
			success = true;
		}

		if ( success )
		{
			m_Pack->EmitWord( symbol->m_Index );
			m_Pack->EmitByte( symbol->m_Store );
		}
		else
		{
			Message( MESSAGE_ERROR, "'%s': left expression cannot be assigned to null", opText );
		}
	}
	else
	{
		// check for compatibility
		FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( r.m_Type, l.m_Type );
		if ( compare != FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
		{
			// special case: const char* going into a gpstring
			if ( l.m_Type.IsString() && r.m_Type.IsCString() )
			{
				Emit( Op::CODE_STORECSTR, op );
			}
			else
			{
				// convert if necessary
				if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
				{
					gpassert( !l.m_Type.IsAString() && !r.m_Type.IsAString() );
					EmitConversion( "assignment", r.m_Type, l.m_Type, 0, op );
				}

				// store
				switch ( symbol->m_SymType )
				{
					case ( SYMTYPE_VARIABLE ):  Emit( Op::CODE_STOREVAR, op );  break;
					case ( SYMTYPE_STRING   ):  Emit( Op::CODE_STORESTR, op );  break;
					case ( SYMTYPE_HANDLE   ):  Emit( Op::CODE_STOREHDL, op );  break;

					default:
					{
						Message( MESSAGE_ERROR, "'%s': left expression cannot be assigned using this operator", opText );
					}
				}
			}

			m_Pack->EmitWord( symbol->m_Index );
			m_Pack->EmitByte( symbol->m_Store );
		}
		else
		{
			ReportConversionError( opText, "expression", r.m_Type, l.m_Type, op );
		}
	}

	TempVarCheckpoint( r );
	return ( true );
}

bool Compiler :: EmitAssignmentOp( ParseNode& l, ParseNode& temp, ParseNode& op, ParseNode& r )
{
	// get some vars
	Op::Code opBase = Op::CODE_SITNSPIN;
	const char* symbol = NULL;
	switch ( op.m_Token )
	{
		case ( '='    ):  opBase = Op::CODE_SITNSPIN ;  symbol = "="  ;  break;
		case ( T_ABOR ):  opBase = Op::CODE_BORI     ;  symbol = "|=" ;  break;
		case ( T_ABXOR):  opBase = Op::CODE_BXORI    ;  symbol = "^=" ;  break;
		case ( T_ABAND):  opBase = Op::CODE_BANDI    ;  symbol = "&=" ;  break;
		case ( T_AADD ):  opBase = Op::CODE_ADDI     ;  symbol = "+=" ;  break;
		case ( T_ASUB ):  opBase = Op::CODE_SUBTRACTI;  symbol = "-=" ;  break;
		case ( T_AMUL ):  opBase = Op::CODE_MULTIPLYI;  symbol = "*=" ;  break;
		case ( T_ADIV ):  opBase = Op::CODE_DIVIDEI  ;  symbol = "/=" ;  break;
		case ( T_AMOD ):  opBase = Op::CODE_MODULOI  ;  symbol = "%=" ;  break;
		case ( T_APOW ):  opBase = Op::CODE_POWERI   ;  symbol = "**=";  break;

		default:
		{
			gpassert( 0 );	// $ should never get here!
		}
	}

	// if it's not a plain old assignment
	if ( opBase != Op::CODE_SITNSPIN )
	{
		// perform the binary op
		EmitBinaryOp( opBase, symbol, temp, op, r );
	}

	// try it as a class variable first
	if ( l.m_Token == T_DEREFERENCE )
	{
		l.m_Next = new ParseNode( r );
		l.m_Next->m_Next = NULL;
		return ( EmitCall( l ) );
	}
	else
	{
		return ( EmitAssignment( l, op, r, symbol ) );
	}
}

Compiler::Symbol* Compiler :: AddSymbol( const char* name, Location location, eSymType symtype, FuBi::TypeSpec vartype, bool* addResults )
{
	// starting point
	Scope* scope;
	int symbolBase, symbolIndex;
	bool isGlobal, isShared = false;
	if ( IsGlobalOnly( symtype ) || m_LocalScopes.empty() )
	{
		scope       = &m_GlobalScope;
		symbolBase  = 0;
		symbolIndex = m_StartOfLocalSymbols;
		isGlobal    = true;
		isShared    = m_GlobalShared;
	}
	else
	{
		scope       = m_LocalScopes.back();
		symbolBase  = m_StartOfLocalSymbols;
		symbolIndex = m_Symbols.size();
		isGlobal    = false;
	}

	// attempt insertion
	gpstring strname;
	if ( name != NULL )
	{
		strname = name;
		Scope::SymbolByNameIndexInsertRet rc = scope->m_SymbolsByName.insert( std::make_pair( strname, symbolIndex - symbolBase ) );

		// already there - report name collision if we're adding only
		if ( !rc.second )
		{
			Symbol& symbol = m_Symbols[ rc.first->second + symbolBase ];

			if ( addResults != NULL )
			{
				*addResults = false;

				// check to make sure it's the same type
				if ( symbol.m_SymType == symtype )
				{
					return ( &symbol );
				}
			}

			const char* text;
			switch ( symbol.m_SymType )
			{
				case ( SYMTYPE_FUNCTION ):
				{
					text = "function";
				}	break;
				case ( SYMTYPE_EVENT ):
				{
					text = "event";
				}	break;
				case ( SYMTYPE_STATE ):
				{
					text = "state";
				}	break;
				default:
				{
					text = "variable";
				}
			}
			m_OverrideLocation = location;
			Message( MESSAGE_ERROR, "'%s': symbol already exists as a '%s'", name, text );
			return ( NULL );
		}
	}

	// adjust local start for new global symbol
	if ( isGlobal )
	{
		++m_StartOfLocalSymbols;
	}

	// set up its index
	Symbol newSymbol( symtype, vartype, scope, strname, location, symbolIndex - symbolBase );
	Symbol* symbol = &*m_Symbols.insert( m_Symbols.begin() + symbolIndex, newSymbol );
	switch ( symtype )
	{
		case ( SYMTYPE_VARIABLE ):
		{
			symbol->m_Index = isShared ? m_GlobalScope.m_SharedVariableCount++
									   : (scope->m_VariableCountBase + scope->m_VariableCount++);
		}	break;
		case ( SYMTYPE_STRING ):
		{
			symbol->m_Index = isShared ? m_GlobalScope.m_SharedStringCount++
									   : (scope->m_StringCountBase + scope->m_StringCount++);
		}	break;
		case ( SYMTYPE_HANDLE ):
		{
			symbol->m_Index = isShared ? m_GlobalScope.m_SharedHandleCount++
									   : (scope->m_HandleCountBase + scope->m_HandleCount++);
		}	break;
	}

	// done
	if ( addResults != NULL )
	{
		*addResults = true;
	}
	return ( symbol );
}

Compiler::Symbol* Compiler :: FindSymbol( const char* name, Scope* base )
{
	if ( base == NULL )
	{
		base = GetBackScope();
	}

	Symbol* symbol = NULL;

	Scope::SymbolByNameIndex::const_iterator found = base->m_SymbolsByName.find( name );
	if ( found != base->m_SymbolsByName.end() )
	{
		symbol = &GetSymbol( base, found->second );
		++symbol->m_TouchCount;
	}
	else if ( base->m_Parent != NULL )
	{
		symbol = FindSymbol( name, base->m_Parent );
	}

	return ( symbol );
}

Compiler::Symbol& Compiler :: GetSymbol( Scope* scope, int index )
{
	if ( scope != &m_GlobalScope )
	{
		index += m_StartOfLocalSymbols;
	}
	gpassert( (index >= 0) && (index < scast <int> ( m_Symbols.size() )) );
	return ( m_Symbols[ index ] );
}

void Compiler :: EnterScope( const ParseNode& node )
{
	TempVarCheckpoint( node );

	int symbolIndex = scast <int> ( m_Symbols.size() ) - m_StartOfLocalSymbols;
	Scope* newScope = new Scope( GetBackScope(), symbolIndex, node.m_Location );
	if ( newScope->m_Parent != &m_GlobalScope )
	{
		newScope->m_VariableCountBase = newScope->m_Parent->m_VariableCountBase + newScope->m_Parent->m_VariableCount;
		newScope->m_StringCountBase   = newScope->m_Parent->m_StringCountBase   + newScope->m_Parent->m_StringCount;
		newScope->m_HandleCountBase   = newScope->m_Parent->m_HandleCountBase   + newScope->m_Parent->m_HandleCount;
	}
	m_LocalScopes.push_back( newScope );
}

bool Compiler :: LeaveScope( const ParseNode& node, int count, eFreeScopeMode mode )
{
	// verify depth
	gpassert( count <= scast <int> ( m_LocalScopes.size() ) );

	// verify we don't have any outstanding temporary variables
	TempVarCheckpoint( node );

	// check for global
	if ( m_LocalScopes.empty() )
	{
		if ( mode == FREESCOPE_NORMAL )
		{
			m_OverrideLocation = node.m_Location;
			Message( MESSAGE_ERROR, "'scope_end': already at global level, cannot exit scope" );
		}
		return ( false );
	}

	// clean up variables
	if ( mode != FREESCOPE_NOEMIT )
	{
		int vars = 0, strings = 0, handles = 0;

		Scope* scope = m_LocalScopes.back();
		for ( int i = 0 ; i < count ; ++i, scope = scope->m_Parent )
		{
			gpassert( scope != NULL );
			vars    += scope->m_VariableCount;
			strings += scope->m_StringCount;
			handles += scope->m_HandleCount;
		}

		if ( vars > 0 )
		{
			Emit( Op::CODE_FREEVAR, node );
			m_Pack->EmitByte( vars );
		}

		if ( strings > 0 )
		{
			Emit( Op::CODE_FREESTR, node );
			m_Pack->EmitByte( strings );
		}

		if ( handles > 0 )
		{
			Emit( Op::CODE_FREEHDL, node );
			m_Pack->EmitByte( handles );
		}
	}

	// only do this if actually exiting the scope
	if ( mode != FREESCOPE_NODELETE )
	{
		Scope* scope = m_LocalScopes.back();
		for ( int i = 0 ; i < count ; ++i )
		{
			gpassert( scope != &m_GlobalScope );

			// warn about unused
			int symbolCount = scast <int> ( m_Symbols.size() ) - (scope->m_SymbolIndex + m_StartOfLocalSymbols);
			ReportUnusedVars( symbolCount );

			// now back patch
			Scope::PatchList::const_iterator j,
											 jbegin = scope->m_PatchPoints.begin(),
											 jend   = scope->m_PatchPoints.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				switch ( j->m_Type )
				{
					case ( PATCH_HEAD ):
					{
						// reverse patch - include size of offset (2)
						PatchCodeWord( j->m_PatchPoint, scope->m_CodeHead - (j->m_PatchPoint + 2) );
					}	break;

					case ( PATCH_TAIL ):
					{
						// forward patch - include size of offset (2)
						PatchCodeWord( j->m_PatchPoint, scope->m_CodeTail - (j->m_PatchPoint + 2) );
					}	break;
				}
			}

			// delete the scope
			gpassert( symbolCount <= (scast <int> ( m_Symbols.size() ) - m_StartOfLocalSymbols) );
			m_Symbols.erase( m_Symbols.end() - symbolCount, m_Symbols.end() );
			Scope* oldScope = scope;
			scope = scope->m_Parent;
			delete ( oldScope );
			gpassert( m_LocalScopes.back() == oldScope );
			m_LocalScopes.pop_back();
		}
	}

	// done
	return ( true );
}

bool Compiler :: LeaveScope( const ParseNode& node, eFreeScopeMode mode )
{
	return ( LeaveScope( node, 1, mode ) );
}

bool Compiler :: LeaveToGlobalScope( const ParseNode& node, eFreeScopeMode mode )
{
	return ( LeaveScope( node, m_LocalScopes.size(), mode ) );
}

Compiler::Scope* Compiler :: GetBackScope( void )
{
	return ( m_LocalScopes.empty() ? &m_GlobalScope : m_LocalScopes.back() );
}

bool Compiler :: ResolveEnum( ParseNode& node, bool reportError )
{
	const char* name = node.ResolveString();

	// try to resolve id as an enum constant
	DWORD value;
	const FuBi::EnumSpec* enumSpec = gFuBiSysExports.FindEnumConstant( name, value );
	if ( enumSpec != NULL )
	{
		// cool - use it
		node.m_Type  = enumSpec->m_Type;
		node.m_Token = T_ENUMCONSTANT;
		node.m_Raw   = value;
	}
	else
	{
		if ( reportError )
		{
			Message( MESSAGE_ERROR, "'%s': not a constant", name );
		}
		node.m_Type = FuBi::VAR_UNKNOWN;
	}
	return ( node.m_Type != FuBi::VAR_UNKNOWN );
}

bool Compiler :: ResolveSysType( ParseNode& node )
{
	// try to resolve id as a fubi-exported type
	const char* name = node.ResolveString();
	node.m_Type = gFuBiSysExports.FindType( name );

	if ( FuBi::IsUser( node.m_Type.m_Type ) )
	{
		const FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( node.m_Type.m_Type );
		gpassert( spec != NULL );
		if ( spec->m_Flags & FuBi::ClassSpec::FLAG_MANAGED )
		{
			node.m_Type.m_Flags = FuBi::TypeSpec::FLAG_HANDLE;
		}
		else
		{
			node.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
		}
	}
	else if ( node.m_Type == FuBi::VAR_UNKNOWN )
	{
		Message( MESSAGE_ERROR, "undefined type '%s'", name );
		return ( false );
	}

	return ( true );
}

void Compiler :: ReportConversionError( const char* location, const char* what, const FuBi::TypeSpec& from, const FuBi::TypeSpec& to, const ParseNode& node )
{
	m_OverrideLocation = node.m_Location;

	// special error message for ptr/handle mismatch
	if ( FuBi::IsUser( from.m_Type ) && (to.m_Type == from.m_Type) )
	{
		Message( MESSAGE_ERROR, "'%s': cannot convert %s from '%s' to '%s' (this is an export bug for engineering)",
				 location, what, gFuBiSysExports.MakeFullTypeName( from ).c_str(),
				 gFuBiSysExports.MakeFullTypeName( to ).c_str() );
	}
	// special error message for handle/handle derived/base mismatch
	else if (   FuBi::IsUser( from.m_Type )
			 && gFuBiSysExports.IsDerivative( from.m_Type, to.m_Type )
			 && (from.m_Flags & FuBi::TypeSpec::FLAG_HANDLE)
			 && (to  .m_Flags & FuBi::TypeSpec::FLAG_HANDLE) )
	{
		Message( MESSAGE_ERROR, "'%s': cannot convert %s from '%s' to '%s' (base and derived classes are both handle-managed)",
				 location, what, gFuBiSysExports.FindNiceTypeName( from ),
				 gFuBiSysExports.FindNiceTypeName( to ) );
	}
	// standard error
	else
	{
		Message( MESSAGE_ERROR, "'%s': cannot convert %s from '%s' to '%s'",
				 location, what, gFuBiSysExports.FindNiceTypeName( from ),
				 gFuBiSysExports.FindNiceTypeName( to ) );
	}
}

void Compiler :: ReportUnusedVars( int symbolDepth )
{
	gpassert( (symbolDepth >= 0) && (symbolDepth <= scast <int> ( m_Symbols.size() )) );
	Symbols::const_iterator i, end = m_Symbols.end(), begin = end - symbolDepth;
	for ( i = begin ; i != end ; ++i )
	{
		if ( (i->m_TouchCount == 0) && IsVariable( i->m_SymType ) && !(i->m_Flags & Raw::FLAG_PROPERTY) )
		{
			m_OverrideLocation = i->m_Location;
			Message( MESSAGE_WARNING, "'%s': variable '%s' never referenced",
					 (i->m_Scope == &m_GlobalScope) ? "global" : "local",
					 i->m_Name.c_str() );

			//$$$ warn about unused goto labels etc.
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::Scope implementation

void Compiler::Scope :: Reset( void )
{
	m_Parent            = NULL;
	m_SymbolsByName     . clear();
	m_SymbolIndex       = 0;
	m_VariableCountBase = 0;
	m_VariableCount     = 0;
	m_StringCount       = 0;
	m_StringCountBase   = 0;
	m_HandleCount       = 0;
	m_HandleCountBase   = 0;
	m_Location          = Location();
	m_IsLoop            = false;
	m_PatchPoints       . clear();
	m_CodeHead          = 0;
	m_CodeTail          = 0;
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::GlobalScope implementation

void Compiler::GlobalScope :: Reset( void )
{
	Scope::Reset();

	m_SharedVariableCount = 0;
	m_SharedStringCount   = 0;
	m_SharedHandleCount   = 0;
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::Function implementation

void Compiler::Function :: Reset( void )
{
	m_Export          = NULL;
	m_LastOpCode      = Op::CODE_SITNSPIN;
	m_LastOpCodeScope = 0;
	m_NestedLevel     = 0;
	m_LastLine        = 0;
	m_LocalState      = false;
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::State implementation

void Compiler::State :: Reset( void )
{
	m_SymbolIndex = 0;
	m_StateIndex  = STATE_INDEX_NONE;
	m_Config      . Reset();
	m_NeedsSaving = false;
	m_Declared    = false;
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::Transition implementation

void Compiler::Transition :: Reset( void )
{
	m_FromState = STATE_INDEX_NONE;
	m_ToState   = STATE_INDEX_NONE;
	m_Statics.clear();
	m_Dynamics.clear();
	m_ResponseOffset = scast <UINT> ( INVALID_FUNCTION_INDEX );
}

bool Compiler::Transition :: IsValid( void ) const
{
	return (    (m_FromState != STATE_INDEX_NONE)
			 && (m_ToState   != STATE_INDEX_NONE) );
}

//////////////////////////////////////////////////////////////////////////////
// struct Compiler::GlobalState implementation

void Compiler::GlobalState :: Reset( void )
{
	m_SetStartup   = false;
	m_ChoseStartup = false;

	ResetLocal();
}

void Compiler::GlobalState :: ResetLocal( void )
{
	m_Current = STATE_INDEX_NONE;
	m_NestedLevel = 0;
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
