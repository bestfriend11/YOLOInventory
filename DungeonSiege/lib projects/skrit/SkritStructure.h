//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritStructure.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the raw data structures for a Skrit compiled binary.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITSTRUCTURE_H
#define __SKRITSTRUCTURE_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"
#include "FuBiDefs.h"
#include "GpColl.h"
#include "SkritDefs.h"

#include <limits>

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace FileSys
{
	class Reader;
}

namespace Skrit  {  // begin of namespace Skrit
namespace Raw    {  // begin of namespace Raw

//////////////////////////////////////////////////////////////////////////////
// storage layout

/*
	Header:

		the header is used primarily to figure out whether or not we need to
		compile the script. check these things:

		  1. product and skrit id - if no match then it ain't a skrit binary
		     and needs compiling.

		  2. header version - if older then just always recompile.

		  3. the sysexport checksum - all the function indexes will be out of
		     date and mismatched on each new exe. force a recompile then.

		bottom line: precompiling a skrit to binary is a convenience only meant
		for speeding up execution. the rule is "when in doubt, recompile".

		pack in the original source code ONLY if we're precompiling to a file,
		otherwise throw it out.

	General rules:

		Keep it simple.

		All offsets are based at the start of the section they deal with. This
		way, the compiler can build tables independently without needing to do
		a lot of memory moving. When the final output is required (only for
		precompiling) then it can finally link everything together. Keep the
		vectors around for future compiles to save on startup/shutdown time.

		All structures are packed end to end.

		If an index is required, it precedes the table.

		If there are variable-length members, the entry stores a size for each
		member, and packs the members end to end at the end.

		Nothing is aligned to a particular boundary except sections, which are
		DWORD-aligned. source at end is not aligned.
*/

//////////////////////////////////////////////////////////////////////////////
// constant declarations

// types
const FOURCC SKRIT_ID = REVERSE_FOURCC( 'Skrt' );		// identifies that it's a compiled skrit

// general constants
const DWORD HEADER_VERSION = MAKEVERSION( 1, 5, 0 );	// current header version - update as structure changes

// section indexes
enum
{
	SET_BEGIN_ENUM( SECTION_, 0 ),

	SECTION_EXPORTS,		// table of all functions in this skrit
	SECTION_GLOBALS,		// global variables that persist beyond skrit execution lifetime
	SECTION_STRINGS,		// string constants referenced from within the skrit
	SECTION_STATE,			// state machine configuration
	SECTION_BYTECODE,		// bytecode to be executed - function offsets in the export table
#	if !GP_RETAIL
	SECTION_DEBUG,			// debug info
#	endif // !GP_RETAIL

	SET_END_ENUM( SECTION_ ),
};

// store
enum eStore
{
	STORE_LOCAL,			// stored in local (per-machine) stack
	STORE_GLOBAL,			// stored in global (per-object instance) stack
	STORE_SHARED,			// stored in global (shared among all objects) stack
};

// global flags
enum eFlags
{
	// $ this must be 8 bits or less

	FLAG_NONE     =      0,
	FLAG_PROPERTY = 1 << 0,		// this variable is a property
	FLAG_HIDDEN   = 1 << 1,		// this variable is hidden from view
};

MAKE_ENUM_BIT_OPERATORS( eFlags );

// forward declarations
class DebugDb;

//////////////////////////////////////////////////////////////////////////////
// raw struct declarations

#pragma pack ( push, 1 )

// Header.

	struct Header
	{
		// $ note: NEVER change the structure of the top four DWORD's in
		//         this structure. there will be no way to get at the source
		//         code otherwise.

		// basic data
		FOURCC m_ProductID;								// ID of product (human readable) - always PRODUCT_ID
		FOURCC m_SkritID;   							// ID of skrit (human readable) - always SKRIT_ID
		DWORD  m_TotalSize;								// total size of entire structure - header, all sections, source
		DWORD  m_SourceOffset;							// offset from top of file original source code, always packed at end and null terminated (or 0 if no source)
		DWORD  m_HeaderVersion;							// version of this header
		DWORD  m_SysExportsDigest;						// checksum of exe's exports
		DWORD  m_SectionOffsets[ SECTION_COUNT ];		// offsets to each section
		DWORD  m_SectionSizes  [ SECTION_COUNT ];		// sizes of each section

		// custom data
		eVarType m_OwnerType;							// owner type of this skrit

		void Init( void );

		static bool ShouldCompile( const_mem_ptr mem, eVarType ownerType );		// returns true if the memory points to data that needs to be recompiled
		bool ShouldCompile( eVarType ownerType ) const;
	};

	COMPILER_ASSERT( (sizeof( Header ) % 4) == 0 );		// should be dword-aligned

// Exports.

	struct ExportEntry									// a particular instance of an export
	{
		DWORD    m_FunctionOffset;						// offset in the byte code where this function begins
		BYTE     m_NameLength;							// length of function name
		WORD     m_DocsLength;							// length of doc text
		BYTE     m_ParamCount;							// number of parameters this function takes
		eVarType m_ReturnType;							// return type as given by FuBi
		WORD     m_EventBinding;						// serial ID of event function binding - set to INVALID_FUNCTION_INDEX if not bound
		WORD     m_StateMember;							// which state is this a member of - set to STATE_INDEX_NONE if not a member of a state
		DWORD    m_AtTime;								// export trigger time in msec, or in frames if high bit is set
		char     m_Name  [ /*m_NameLength +*/ 1 ];		// name of the function
	//	char     m_Docs  [ m_DocsLength + 1 ];			// docs for the function (optional)
	//	eVarType m_Params[ m_ParamCount ];				// the parameters this function takes

		// read-only API
		const char*        GetDocs       ( void ) const  {  return ( m_Name + m_NameLength + 1 );  }
		      char*        GetDocs       ( void )        {  return ( m_Name + m_NameLength + 1 );  }
		const eVarType*    GetParamsBegin( void ) const  {  return ( rcast <const eVarType*> ( GetDocs() + (m_DocsLength ? (m_DocsLength + 1) : 0) ) );  }
		const eVarType*    GetParamsEnd  ( void ) const  {  return ( GetParamsBegin() + m_ParamCount );  }
		const ExportEntry* GetNext       ( void ) const  {  return ( rcast <const ExportEntry*> ( GetParamsEnd() ) );  }
		UINT               GetSize       ( void ) const  {  return ( rcast <UINT> ( GetNext() ) - rcast <UINT> ( this ) );  }

		// time api
		bool  HasAtTime       ( void ) const  {  return ( m_AtTime != (DWORD)TIME_NONE );  }
		bool  IsTimeInFrames  ( void ) const  {  return ( !!(m_AtTime & (1 << 31)) );  }
		int   GetTimeInFrames ( void ) const  {  gpassert( IsTimeInFrames() );  return ( m_AtTime & ~(1 << 31) );  }
		int   GetTimeInMsec   ( void ) const  {  gpassert( !IsTimeInFrames() );  return ( m_AtTime );  }
		float GetTimeInSeconds( void ) const  {  return ( GetTimeInMsec() / 1000.0f );  }
		void  SetTimeInFrames ( int frames )  {  m_AtTime = frames | (1 << 31);  }
		void  SetTimeInMsec   ( int msec   )  {  m_AtTime = msec;  }
		void  SetTimeInSeconds( float sec  )  {  m_AtTime = (DWORD)(sec * 1000.0f);  }

		enum eMatch
		{
			MATCH_OK,
			MATCH_RETURN_MISMATCH,
			MATCH_DIFF_PARAM_COUNT,
			MATCH_PARAM_MISMATCH,
		};

		// utility
		static FuBi::TypeSpec BuildTypeSpec( eVarType type );
		eMatch CheckFunctionMatch( int paramOffset, const FuBi::FunctionSpec* spec, int* badParam = NULL ) const;

#		if !GP_RETAIL
		gpstring BuildNameAndParams( const void* param, bool allowBinaryOut = false ) const;
#		endif // !GP_RETAIL
	};

	struct Exports										// export section is just a list of exports packed end-to-end
	{
		ExportEntry m_Entries[ 1 /*count*/ ];

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const;
#		endif // !GP_RETAIL
	};

// Globals.

	struct GlobalEntry
	{
		WORD  m_Type;									// (eVarType) type of var, if user type then it's always a pointer
		BYTE  m_Store;									// (eStore) where the global variable is stored
		BYTE  m_Flags;									// (eGlobalFlags) flags specific to this global variable
		DWORD m_NameOffset;								// offset into string table for name
		DWORD m_InitialValue;							// what to initialize this to - if a string, this will be a string constant offset

		// read-only API
		inline UINT        GetSize( void ) const;
		const GlobalEntry* GetNext( void ) const  {  return ( rcast <const GlobalEntry*> ( rcast <const BYTE*> ( this ) + GetSize() ) );  }
	};

	struct PropertyEntry : GlobalEntry
	{
		DWORD m_DocsOffset;								// offset into string table for docs
	};

	inline UINT GlobalEntry :: GetSize( void ) const
	{
		return ( (m_Flags & FLAG_PROPERTY) ? sizeof( PropertyEntry ) : sizeof( GlobalEntry ) );
	}

	struct Globals										// all the global variables
	{
		GlobalEntry m_Entries[ 1 /*count*/ ];

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const;
#		endif // !GP_RETAIL
	};

// String constants.

	struct Strings										// just a list of unindexed null-terminated strings packed end-to-end (referenced by offset from within the bytecode)
	{
		char m_Strings[ 1 /*count*/ ];

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const;
#		endif // !GP_RETAIL
	};

// State machine structures.

	enum eStateType
	{
		STATE_CONFIG,									// state configuration info
		STATE_TRANSITION,								// state transition entry
	};

	struct StateEntry
	{
		BYTE m_Type;									// type of state entry (eStateType)
		WORD m_Size;									// size of this entry

		StateEntry( void )  {  }
		StateEntry( eStateType type, UINT size )  {  m_Type = scast <BYTE> ( type );  m_Size = scast <WORD> ( size );  }
		const StateEntry* GetNext( void ) const  {  return ( rcast <const StateEntry*> ( rcast <const BYTE*> ( this ) + m_Size ) );  }
	};

	struct StateConfig : StateEntry
	{
		WORD  m_State;									// index of this state
		float m_PollPeriod;								// < 0.0 if not polled

		StateConfig( void ) : StateEntry( STATE_CONFIG, sizeof( StateConfig ) )  {  Reset();  }

		void Reset( void )
		{
			m_State      = 0;
			m_PollPeriod = -1.0f;
		}
	};

	struct ConditionEntry
	{
		BYTE m_IsStatic;								// true for ConditionStatic type, false for ConditionDynamic type

		ConditionEntry( bool isStatic )  {  m_IsStatic = isStatic;  }
	};

	struct ConditionStatic : ConditionEntry
	{
		WORD m_EventSerial;								// event that will be a "hit"
		BYTE m_TestDataSize;							// size of params that follows
	//  BYTE m_TestData[ m_TestDataSize ];				// params that must match to be a "hit" (ONLY int, bool, enum, exact float)

		ConditionStatic( void ) : ConditionEntry( true )
		{
			m_EventSerial  = (WORD)STATE_INDEX_NONE;
			m_TestDataSize = 0;
		}

		size_t                GetSize     ( void ) const  {  return ( sizeof( ConditionStatic ) + m_TestDataSize );  }
		const ConditionEntry* GetNext     ( void ) const  {  return ( rcast <const ConditionStatic*> ( rcast <const BYTE*> ( this ) + GetSize() ) );  }
		const BYTE*           GetDataBegin( void ) const  {  return ( rcast <const BYTE*> ( this + 1 ) );  }
		const BYTE*           GetDataEnd  ( void ) const  {  return ( GetDataBegin() + m_TestDataSize );  }

#		if !GP_RETAIL
		void OutputParams( ReportSys::ContextRef ctx ) const;
#		endif // !GP_RETAIL
	};

	struct ConditionDynamic : ConditionEntry
	{
		DWORD m_CodeOffset;								// offset in bytecode to condition test code (MUST return an int or bool, whatever is ok for 'if')

		ConditionDynamic( void ) : ConditionEntry( false )
		{
			m_CodeOffset = scast <DWORD> ( INVALID_FUNCTION_INDEX );
		}

		size_t                GetSize( void ) const  {  return ( sizeof( ConditionDynamic ) );  }
		const ConditionEntry* GetNext( void ) const  {  return ( rcast <const ConditionStatic*> ( this + 1 ) );  }
	};

	struct StateTransition : StateEntry
	{
		WORD  m_FromState;								// transition rule is from this state...
		WORD  m_ToState;								// ...to this state
		DWORD m_ResponseCodeOffset;						// where is the code that we execute right before
		BYTE  m_ConditionCount;							// how many conditions that must all be true for a transition?
	//  ConditionEntry m_Conditions[ /*count*/ ];

		StateTransition( void ) : StateEntry( STATE_TRANSITION, sizeof( StateTransition ) )
		{
			m_FromState          = scast <WORD>  ( STATE_INDEX_NONE );
			m_ToState            = scast <WORD>  ( STATE_INDEX_NONE );
			m_ResponseCodeOffset = scast <DWORD> ( INVALID_FUNCTION_INDEX );
			m_ConditionCount     = 0;
		}

		const ConditionEntry* GetFirst( void ) const  {  return ( rcast <const ConditionEntry*> ( this + 1 ) );  }
	};

	struct State
	{
		WORD       m_InitialState;						// startup state
		WORD       m_TotalStates;						// total state count
	//  StateEntry m_Entries[ /*count*/ ];				// don't uncomment this - it will mess up sizeof

		const StateEntry* GetFirst( void ) const  {  return ( rcast <const StateEntry*> ( this + 1 ) );  }

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize, DebugDb* debugDb = NULL ) const;
#		endif // !GP_RETAIL
	};


// Byte code.

	struct ByteCode										// stream of bytecode - contains all functions for the skrit
	{
		BYTE m_Code[ 1 /*count*/ ];

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize, DebugDb* debugDb = NULL ) const;
		const BYTE* GenerateNextDocs( ReportSys::ContextRef ctx, UINT sectionSize, const BYTE* last = NULL, const DebugDb* debugDb = NULL ) const;
#		endif // !GP_RETAIL
	};

// Debug info.

#	if !GP_RETAIL

	enum eDebugType
	{
		DEBUG_LOCATION,									// location information
		DEBUG_STATE,									// state information
		DEBUG_SOURCEFILE,								// source code file information
	};

	struct DebugEntry
	{
		BYTE m_Type;									// type of debug entry (eDebugType)
		WORD m_Size;									// size of this entry

		DebugEntry( void )  {  }
		DebugEntry( eDebugType type, UINT size )  {  m_Type = scast <BYTE> ( type );  m_Size = scast <WORD> ( size );  }
		const DebugEntry* GetNext( void ) const  {  return ( rcast <const DebugEntry*> ( rcast <const BYTE*> ( this ) + m_Size ) );  }
	};

	struct DebugLocation : DebugEntry					// location info
	{
		DWORD m_CodeOffset;								// offset that code for this line starts
		WORD  m_SourceIndex;							// index into source file index for the file containing this line
		WORD  m_Line;									// what line in the source code matches this object code

		DebugLocation( void ) : DebugEntry( DEBUG_LOCATION, sizeof( DebugLocation ) )  {  }
	};

	struct DebugState : DebugEntry						// state info
	{
		WORD m_State;									// state number
//		char m_Name[ count ];							// name of this state

		DebugState( void ) : DebugEntry( DEBUG_STATE, sizeof( DebugState ) )  {  }

		const char* GetName( void ) const  {  return ( rcast <const char*> ( this + 1 ) );  }
		char* GetName( void )  {  return ( rcast <char*> ( this + 1 ) );  }
	};

	struct DebugSourceFile : DebugEntry					// source code file
	{
		WORD m_IncluderIndex;							// index of including source file
		WORD m_IncludeLine;								// line it's included on
//		char m_Name[ count ];							// name of this file

		DebugSourceFile( void ) : DebugEntry( DEBUG_SOURCEFILE, sizeof( DebugSourceFile ) )  
		{
			m_IncluderIndex = 0;
			m_IncludeLine = 0;
		}

		const char* GetName( void ) const  {  return ( rcast <const char*> ( this + 1 ) );  }
		char* GetName( void )  {  return ( rcast <char*> ( this + 1 ) );  }
	};

	struct Debug										// just a list of DebugEntry derivatives packed end-to-end
	{
		DebugEntry m_Entries[ 1 /*count*/ ];

#		if !GP_RETAIL
		void GenerateDocs( ReportSys::ContextRef ctx, UINT sectionSize ) const;
#		endif // !GP_RETAIL
	};

#	endif // !GP_RETAIL

#pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////////
// containers

struct Pack
{
	Buffer m_Exports;
	Buffer m_Globals;
	Buffer m_Strings;
	Buffer m_State;
	Buffer m_ByteCode;
#	if !GP_RETAIL
	Buffer m_Debug;
#	endif // !GP_RETAIL

// Setup.

	Pack( void );

	void Reset( void );
	void Swap ( Pack& pack );

// Storage.

	void   Copy    ( const Header* header );
	size_t GetSize ( void ) const;
	void   Save    ( mem_ptr out ) const;
	UINT32 AddCrc32( UINT32 seed = 0 ) const;

#	if !GP_RETAIL
	void   GenerateDocs    ( ReportSys::ContextRef ctx ) const;
	void   DisassembleMixed( ReportSys::ContextRef ctx, FileSys::Reader* base = NULL, DWORD startIp = 0, DWORD maxBytes = 0, DWORD tagIp = 0xFFFFFFFF ) const;
	void   DisassembleRaw  ( ReportSys::ContextRef ctx, DWORD startIp = 0, DWORD maxBytes = 0, DWORD tagIp = 0xFFFFFFFF ) const;
#	endif // !GP_RETAIL

// Generation.

	template <typename T, typename U>
	void AssertValidRange( T data, U )
	{
		gpassert( data <= std::numeric_limits <U>::max() );
		gpassert( data >= std::numeric_limits <U>::min() );
	}

	// bytecode output
	void EmitByte ( DWORD data )  {  GPDEBUG_ONLY( AssertValidRange( data, BYTE () ) );  m_ByteCode.push_back( scast <BYTE> ( data ) );  }
	void EmitChar ( int   data )  {  GPDEBUG_ONLY( AssertValidRange( data, char () ) );  m_ByteCode.push_back( scast <char> ( data ) );  }
	void EmitWord ( DWORD data )  {  GPDEBUG_ONLY( AssertValidRange( data, WORD () ) );  m_ByteCode.resize( m_ByteCode.size() + 2 ); *rcast <WORD*> ( &*m_ByteCode.end() - 2 ) = scast <WORD> ( data );  }
	void EmitSword( int   data )  {  GPDEBUG_ONLY( AssertValidRange( data, short() ) );  m_ByteCode.resize( m_ByteCode.size() + 2 ); *rcast <short*> ( &*m_ByteCode.end() - 2 ) = scast <short> ( data );  }
	void EmitDword( DWORD data )  {  m_ByteCode.resize( m_ByteCode.size() + 4 ); *rcast <DWORD*> ( &*m_ByteCode.end() - 4 ) = data;  }
	void EmitDword( int   data )  {  m_ByteCode.resize( m_ByteCode.size() + 4 ); *rcast <int  *> ( &*m_ByteCode.end() - 4 ) = data;  }

// Read-only access.

	const Exports*  GetExports ( void ) const  {  return ( m_Exports .empty() ? NULL : rcast <const Exports *> ( &*m_Exports .begin() ) );  }
	const Globals*  GetGlobals ( void ) const  {  return ( m_Globals .empty() ? NULL : rcast <const Globals *> ( &*m_Globals .begin() ) );  }
	const Strings*  GetStrings ( void ) const  {  return ( m_Strings .empty() ? NULL : rcast <const Strings *> ( &*m_Strings .begin() ) );  }
	const State*    GetState   ( void ) const  {  return ( m_State   .empty() ? NULL : rcast <const State   *> ( &*m_State   .begin() ) );  }
	const ByteCode* GetByteCode( void ) const  {  return ( m_ByteCode.empty() ? NULL : rcast <const ByteCode*> ( &*m_ByteCode.begin() ) );  }
#	if !GP_RETAIL
	const Debug*    GetDebug   ( void ) const  {  return ( m_Debug   .empty() ? NULL : rcast <const Debug   *> ( &*m_Debug   .begin() ) );  }
#	endif // !GP_RETAIL

private:
	Buffer* m_Buffers[ SECTION_COUNT ];

	SET_NO_COPYING( Pack );
};

//////////////////////////////////////////////////////////////////////////////
// class DebugDb declaration

#if !GP_RETAIL

class DebugDb
{
public:
	SET_NO_INHERITED( DebugDb );

	DebugDb( const Pack& pack );
	DebugDb( void )  {  }
   ~DebugDb( void )  {  }

	typedef stdx::fast_vector <DebugLocation> LocationColl;
	typedef stdx::fast_vector <DebugSourceFile> SourceFileColl;

	LocationColl::const_iterator GetLocationBegin( void ) const
		{  return ( m_Locations.begin() );  }
	LocationColl::const_iterator GetLocationEnd( void ) const
		{  return ( m_Locations.end() );  }

	SourceFileColl::const_iterator GetSourceFileBegin( void ) const
		{  return ( m_SourceFiles.begin() );  }
	SourceFileColl::const_iterator GetSourceFileEnd( void ) const
		{  return ( m_SourceFiles.end() );  }

	const DebugLocation* GetLocationFromIp( DWORD ip ) const;

	gpstring GetStateName( int state ) const;
	gpstring GetValidStateName( int state ) const;

	gpstring GetStateName( WORD state ) const;
	gpstring GetValidStateName( WORD state ) const;

	gpstring GetSourceFileName( int index ) const;
	gpstring GetValidSourceFileName( int index ) const;

	int GetSourceFileCount( void ) const
		{  return ( scast <int> ( m_SourceFiles.size() ) ); }

private:
	typedef stdx::fast_vector <gpstring> StringColl;

	StringColl     m_States;
	LocationColl   m_Locations;
	StringColl     m_SourceFileNames;
	SourceFileColl m_SourceFiles;

	SET_NO_COPYING( DebugDb );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Raw
}  // end of namespace Skrit

#endif  // __SKRITSTRUCTURE_H

//////////////////////////////////////////////////////////////////////////////
