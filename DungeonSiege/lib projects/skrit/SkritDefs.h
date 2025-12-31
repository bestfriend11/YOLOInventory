//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritDefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains definitions and high level API functions to support
//             Skrits. See Skrit.* for implementation.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITDEFS_H
#define __SKRITDEFS_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"

#include <vector>

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// language notes

/*	*** State definition ***

	Skrits by default do not have any state machinery and behave as ordinary
	modules that can have functions called and events handled. Once a single
	state is introduced into the system, however, the Skrit becomes a full state
	machine. This happens the first time the compiler adds a state symbol to the
	system, which occurs any time a state name is used, which is generally one
	of these cases:

		* The 'state' tag is used to open a state block.
		* A 'transition' table uses a state symbol in a transition entry.
		* The setstate keyword is used.

	State machines must have a startup state, and the first state symbol
	introduced into the system becomes the default startup state. To override
	this default behavior (highly recommended) just use the 'startup' tag before
	opening a state block:

		startup state s0$
		{
			//...
		}

	State transitions can occur one of four ways:

	1.	Explicitly. A function of any kind within the Skrit can just use the
		'setstate' command.

	2.	Automatically through static event handling. The transition table tracks
		incoming events and sets a bit in a bitfield each time it finds a match
		for a waiting transition. If all the bits are set for a transition, it
		will switch states.

	3.	Automatically through dynamic event handling. Each time a static event
		comes in, if a transition doesn't occur, all the dynamic events are
		checked as well. Each transition that has all its static bits set will
		have its dynamic triggers checked. The first that succeeds will fire the
		transition and switch states.

	4.	Manually and indirectly through an explicit call to Poll(). This just
		goes through all the transitions that have all their static bits set and
		checks the dynamic triggers for each. Polling is necessary in order to
		pick up transitions that have dynamic triggers but static events aren't
		common enough to get the dynamic checking in often enough.

	*** Event handling ***

	This is what happens when an event is passed to a Skrit via external C++
	function call:

	1.	The event is sent to a handler if one exists. If no handler exists, then
		the engine ignores the event and reports success.

		The handler lookup works like this. First Skrit looks for an event
		handler within the current state. If there is no current state (which
		can only happen if this is not a state machine) or there is no handler,
		then it checks all the event handlers at the global level within the
		Skrit for a match.

	2.	If (a) the event handler does not change the state, and (b) there
		actually is a state machine, then the Skrit checks to see if a
		transition should occur.

	*** State transitions ***

	* Static Events *

	1. Static event comes in.

	2. Execute the event handler if it exists.

	3. If event was handled, and the handler changed the state, then bail
	   early.

	4. If we're not currently in a state, then no automatic transition is
	   possible, so we abort. This shouldn't be possible if any states exist
	   in the system, except during Skrit startup.

	5. Go through all the transitions out of the current state and set the bit


	Skrit startup:
	w/ state entering etc.

	*** Standard events ***

	$$$ go through all events...

	$$$ what about ordering of the transition checks?

	$$$ how does transition action get called? what's the EXACT order of
	function calls, including the OnEnterState and OnExitState stuff?

*/

//////////////////////////////////////////////////////////////////////////////
// macro definitions

// Function macros.

#	define SKRIT_IMPORT FUBI_EXPORT

	// return helpers
#	define SKRIT_RETURN( TYPE ) \
		return ( *rcast <TYPE*> ( &rc.m_Raw ) )
#	define SKRIT_NO_RETURN \
		return
#	define SKRIT_RAW_RETURN \
		return ( rc )

	// rc helpers
#	define SKRIT_NO_RC
#	define SKRIT_RC_PARAM( RC_PARAM ) \
		RC_PARAM = rc.m_Code

// Event macros.

#	define SKRIT_EVENT_IMPL( FUNC_NAME, SKRIT_PARAM, SKIP, ASSIGN_RC, RETURN )	\
	{																			\
		using namespace FuBi;													\
		using namespace Skrit;													\
																				\
		/* get function spec and 2nd param start */								\
		FUBI_FIND_FUNCTION( FUNC_NAME, ResolveEvent,							\
							FUBI_PARAM_SKIP( SKIP ) );							\
																				\
		/* call the function */													\
		Result rc = ( s_FunctionSpec != NULL )									\
				  ? SendEvent( SKRIT_PARAM, s_FunctionSpec, paramStart )		\
				  : Result( RESULT_FATAL, 0 );									\
																				\
		/* interested in handling? */											\
		ASSIGN_RC;																\
																				\
		/* return the results (if requested) */									\
		RETURN;																	\
	}

#	define SKRIT_EVENTV( FUNC_NAME, SKRIT_PARAM ) \
		SKRIT_EVENT_IMPL( #FUNC_NAME, SKRIT_PARAM, 1, SKRIT_NO_RC, SKRIT_NO_RETURN )
#	define SKRIT_EVENT( TYPE, FUNC_NAME, SKRIT_PARAM ) \
		SKRIT_EVENT_IMPL( #FUNC_NAME, SKRIT_PARAM, 1, SKRIT_NO_RC, SKRIT_RETURN( TYPE ) )
#	define SKRIT_RESULT_EVENTV( FUNC_NAME, SKRIT_PARAM, RC_PARAM ) \
		SKRIT_EVENT_IMPL( #FUNC_NAME, SKRIT_PARAM, 2, SKRIT_RC_PARAM( RC_PARAM ), SKRIT_NO_RETURN )
#	define SKRIT_RESULT_EVENT( TYPE, FUNC_NAME, SKRIT_PARAM, RC_PARAM ) \
		SKRIT_EVENT_IMPL( #FUNC_NAME, SKRIT_PARAM, 2, SKRIT_RC_PARAM( RC_PARAM ), SKRIT_RETURN( TYPE ) )

// Call macros.

#	define SKRIT_CALL_IMPL( FUNC_NAME, SKRIT_PARAM, INDEX_PARAM, RETURN )		\
	{																			\
		using namespace FuBi;													\
		using namespace Skrit;													\
																				\
		/* get function spec and 2nd param start */								\
		FUBI_FIND_FUNCTION( FUNC_NAME, ResolveCall,								\
							FUBI_PARAM_SKIP( 2 ) );								\
																				\
		/* call the function */													\
		Result rc = ( s_FunctionSpec != NULL )									\
				  ? CallSkrit( SKRIT_PARAM, INDEX_PARAM, s_FunctionSpec,		\
				  			   paramStart )										\
				  : Result( RESULT_FATAL, 0 );									\
		RETURN;																	\
	}

#	define SKRIT_CALLV( FUNC_NAME, SKRIT_PARAM, INDEX_PARAM ) \
		SKRIT_CALL_IMPL( #FUNC_NAME, SKRIT_PARAM, INDEX_PARAM, SKRIT_NO_RETURN )
#	define SKRIT_CALL( TYPE, FUNC_NAME, SKRIT_PARAM, INDEX_PARAM ) \
		SKRIT_CALL_IMPL( #FUNC_NAME, SKRIT_PARAM, INDEX_PARAM, SKRIT_RETURN( TYPE ) )

// Report streams.

#	if !GP_RETAIL
	ReportSys::Context& GetSkritContext( void );
#	define gSkritContext GetSkritContext()
#	endif // !GP_RETAIL

#	define gpskrit( msg ) DEV_MACRO_REPORT( &gSkritContext, msg )
#	define gpskritf( msg ) DEV_MACRO_REPORTF( &gSkritContext, msg )

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class Compiler;
class Engine;
class Machine;
class Object;
class ObjectImpl;

struct CreateReq;
struct CloneReq;

namespace Raw
{
	struct Header;
	struct ExportEntry;
	struct Exports;
	struct Strings;
	struct State;
	struct StateConfig;
	struct StateTransition;
	struct ConditionStatic;
	struct ConditionDynamic;
	struct ByteCode;
	struct Debug;
	struct Pack;

	class DebugDb;

	enum eStore;
}

namespace Op
{
	enum Code;
}

// we use these a lot
using FuBi::eVarType;
using FuBi::Store;

// lots of stuff needs buffers
typedef stdx::fast_vector <BYTE> Buffer;

//////////////////////////////////////////////////////////////////////////////
// class HObject declaration

class HObject
{
public:
	SET_NO_INHERITED( HObject );

	// ctor/dtor
	HObject( void )								{  m_Handle = 0;  }
	HObject( const CreateReq& createReq )		{  CreateSkrit( createReq );  }		// create
	HObject( const CloneReq& cloneReq )			{  CloneSkrit( cloneReq );  }		// clone

	// acquire/release
	bool    CreateSkrit ( const CreateReq& createReq );								// create a skrit given the spec
	bool    CloneSkrit  ( const CloneReq& cloneReq );								// clone an existing skrit into this one
	void    ReleaseSkrit( void );													// release ref from the skrit

	// simple query
	bool            IsNull      ( void ) const		{  return ( !m_Handle );  }		// check to see if it points at nothing
	bool            IsValid     ( void ) const;										// check to see if it points to a non-null valid entry against mgr's table
	bool            IsPersistent( void ) const		{  return ( !!m_Persist );  }	// will this skrit persist?
	const gpstring& GetName     ( void ) const;										// get name of this skrit (from database)

	// access
	Object*       Get     ( void );													// dereference object - assert and returns NULL if invalid or null
	const Object* GetConst( void ) const;											// dereference object - assert and returns NULL if invalid or null ( $ SCOTTB - different name from other Get() to prevent ambiguity when calling from watch window)

	// operators
	Object*       operator ->   ( void )		{  return ( Get() );  }				// member dereference operator
	const Object* operator ->   ( void ) const	{  return ( GetConst() );  }		// member dereference operator
	Object&       operator *    ( void )		{  return ( *Get() );  }			// member dereference operator
	const Object& operator *    ( void ) const	{  return ( *GetConst() );  }		// member dereference operator
				  operator bool ( void ) const	{  return ( !IsNull() );  }			// check for null

	// comparisons
	bool operator == ( const ThisType& other ) const	{  return ( m_Handle == other.m_Handle );  }
	bool operator != ( const ThisType& other ) const	{  return ( m_Handle != other.m_Handle );  }

	// special constants
	enum
	{
		// sizes to use for bit fields
		MAX_BITS_INDEX   = 16,				// LSB (  0 - 15 )
		MAX_BITS_MAGIC   = 15,				//     ( 16 - 30 )
		MAX_BITS_PERSIST =  1,				// MSB ( 31 )

		// sizes to compare against for asserting dereferences
		MAX_INDEX = (1 << MAX_BITS_INDEX) - 1,
		MAX_MAGIC = (1 << MAX_BITS_MAGIC) - 1,
	};

private:
	friend Engine;

	union
	{
		COMPILER_ASSERT(   (MAX_BITS_INDEX == sizeof( WORD ))
						&& ((  MAX_BITS_INDEX
							 + MAX_BITS_MAGIC
							 + MAX_BITS_PERSIST) == 32) );

		union
		{
			struct
			{
				WORD m_Index;								// index into resource vector
				WORD m_Dummy1;
			};

			struct
			{
				unsigned m_Dummy2  : MAX_BITS_INDEX;
				unsigned m_Magic   : MAX_BITS_MAGIC;		// magic number to check against structure
				unsigned m_Persist : MAX_BITS_PERSIST;		// flag for whether or not this object is persistent
			};
		};
		DWORD m_Handle;
	};

	HObject( UINT index, UINT magic, bool persist )
	{
		gpassert( index < MAX_INDEX );
		gpassert( (magic != 0) && (magic <= MAX_MAGIC) );

		m_Index   = (WORD)index;
		m_Magic   = magic;
		m_Persist = persist;
	}
};

// keep it a dword for efficiency and consistency
COMPILER_ASSERT( sizeof( HObject ) == sizeof( DWORD ) );

//////////////////////////////////////////////////////////////////////////////
// class HAutoObject declaration

class HAutoObject : public HObject
{
public:
	SET_INHERITED( HAutoObject, HObject );

	// ctor/dtor
	HAutoObject( void )							{  }							// default ctor nulls "this"
	HAutoObject( int )							{  }							// for implicit init to null
	HAutoObject( HObject h )					: Inherited( h )  {  }			// copy the given handle and own it
	HAutoObject( const CreateReq& createReq )	{  CreateSkrit( createReq );  }	// copy the given handle and own it
	HAutoObject( const CloneReq& cloneReq )		{  CloneSkrit( cloneReq );  }	// copy the given handle and own it
   ~HAutoObject( void )							{  ReleaseSkrit();  }			// auto-frees the handle if it's open

	// acquire
	bool    CreateSkrit ( const CreateReq& createReq );							// create a skrit given the spec
	bool    CloneSkrit  ( const CloneReq& cloneReq );							// clone an existing skrit into this one
	void    ReleaseSkrit( void );												// release ref from the skrit

	HObject ReleaseHandle( void );												// lets go of the owned handle without closing it and sets "*this" to null (returns old value)

	// reassign
	HAutoObject& operator = ( HObject h );										// copy the given handle and own it

	// query
	inline HObject GetHandle( void ) const  {  return ( scast <const Inherited&> ( *this ) );  }

	// no smart pointer semantics for efficiency - prevent all copying
	SET_NO_COPYING( HAutoObject );
};

//////////////////////////////////////////////////////////////////////////////
// helpers

// Configuration.

	// product id
	extern const FOURCC& PRODUCT_ID;
	void SetProductId( DWORD id );

// New Skrit construction requests.

	// m_CreateAlways        - if true, it will always create a new instance of
	//                         the skrit (cloning it if it exists). if false, it
	//                         will create the first if necessary, otherwise it
	//                         will addref on the existing. creation/cloning will
	//                         always addref on the resulting object.

	// m_IgnoreFileErrors    - if true, no file-related skrit errors will be
	//                         reported. it's useful to ignore file errors if you
	//                         don't know in advance if a skrit will exist or not
	//                         when attempting to "get" it.

	// m_DeferConstruction   - if true, the OnConstruct() etc. and initial state
	//                         entry functions will be deferred until later. you
	//                         must call CommitCreation() on the skrit to finish
	//                         construction.

	// m_ForceFloatConstants - if true, force all int-looking constants to be
	//                         float type instead. this is meant for the
	//                         expression evaluator, and is the same thing as
	//                         #option +force_float_constants.

	// m_OwnerType           - this is the class type to use for the skrit's owner
	//                         (for the 'owner' keyword). it must match the
	//                         'owner=' construct in the skrit code or else the
	//                         skrit will fail to construct. it defaults to no
	//                         owner, in which case 'owner' becomes a synonym for
	//                         'this' in the skrit (where the type of 'this' is
	//                         Skrit::Object).

	// m_Owner               - this is the "this" of the owner. the 'owner'
	//                         keyword will resolve to this pointer at runtime.

	enum ePersistType
	{
		PERSIST_DETECT = -1,	// autodetect the persist type from _persist$ property or take the default value if a clone
		PERSIST_NO,				// force no persist
		PERSIST_YES,			// force persist (this is the default)
	};

	inline bool IsMatch( ePersistType type, bool persist )
	{
		if ( type == PERSIST_DETECT )
		{
			return ( true );
		}
		return ( (type == PERSIST_YES) == persist );
	}

	struct CreateReq
	{
		const char*  m_FileName;				// required
		bool         m_CreateAlways;			// default: true
		bool         m_IgnoreFileErrors;		// default: false
		bool         m_IgnoreCompileErrors;		// default: false
		bool         m_ForceSummary;			// default: false
		bool         m_DeferConstruction;		// default: false
		bool         m_ForceFloatConstants;		// default: false
		ePersistType m_PersistType;				// default: PERSIST_YES
		eVarType     m_OwnerType;				// default: unknown
		void*        m_Owner;					// default: null

		CreateReq( const char* fileName = NULL, bool createAlways = true )
			{  Init( fileName, createAlways );  }
		CreateReq( const gpstring& fileName, bool createAlways = true )
			{  Init( fileName, createAlways );  }

		CreateReq& Init( const char* fileName, bool createAlways = true );

		CreateReq& SetOwner( eVarType type, void* owner = NULL );
		CreateReq& SetOwner( const char* typeName, void* owner = NULL );

		CreateReq& SetPersist      ( bool persist )  {  m_PersistType = persist ? PERSIST_YES : PERSIST_NO;  return ( *this );  }
		CreateReq& SetPersistDetect( void )          {  m_PersistType = PERSIST_DETECT;  return ( *this );  }

		GPDEBUG_ONLY( bool AssertValid( void ) const; )
	};

	struct GetReq : CreateReq
	{
		GetReq( const char* fileName, bool createAlways = false )
			: CreateReq( fileName, createAlways )  {  }
		GetReq( const gpstring& fileName, bool createAlways = false )
			: CreateReq( fileName, createAlways )  {  }
	};

	struct CloneReq
	{
		HObject      m_CloneSource;				// required
		bool         m_DeferConstruction;		// default: false
		ePersistType m_PersistType;				// default: PERSIST_DEFAULT
		bool         m_CloneGlobals;			// default: false
		eVarType     m_OwnerType;				// default: unknown
		void*        m_Owner;					// default: null

		CloneReq( HObject cloneSource = HObject() );
		CloneReq( HObject cloneSource, const CreateReq& createReq );

		CloneReq& SetOwner( eVarType type, void* owner = NULL );
		CloneReq& SetOwner( const char* typeName, void* owner = NULL );

		CloneReq& SetPersist      ( bool persist )  {  m_PersistType = persist ? PERSIST_YES : PERSIST_NO;  return ( *this );  }
		CloneReq& SetPersistDetect( void )          {  m_PersistType = PERSIST_DETECT;  return ( *this );  }

		GPDEBUG_ONLY( bool AssertValid( void ) const; )
	};

//////////////////////////////////////////////////////////////////////////////
// result from an executed Skrit

enum eResult
{
	SET_BEGIN_ENUM( RESULT_, 0 ),

	// benign return conditions
	RESULT_SUCCESS,			// successfully executed function
	RESULT_NOEVENT,			// object does not have a response for this event

	// warnings
	RESULT_WARNING,			// had one or more warnings during execution

	// ---------------------   ERRORS FOLLOW

	// errors
	RESULT_ERROR,			// had one or more errors during execution (trumps warnings)

	// fatals
	RESULT_FATAL,			// had one or more fatals during execution (trumps errors and warnings)
	RESULT_NOFUNCTION,		// skrit does not have this function
	RESULT_BADPARAMS,		// skrit call had a parameter mismatch

	SET_END_ENUM( RESULT_ ),
};

const char* ToString  ( eResult result );
bool        FromString( const char* str, eResult& result );

inline bool IsError  ( eResult result )  {  return ( result >= RESULT_ERROR );  }
inline bool IsSuccess( eResult result )  {  return ( !IsError( result ) );  }

struct Result
{
	eResult m_Code;

	union
	{
		DWORD       m_Raw;
		int         m_Int;
		bool        m_Bool;
		float       m_Float;
		const char* m_String;
		void*       m_Generic;
	};

	Result( void )
		{  m_Code = RESULT_FATAL;  m_Raw  = 0;  }
	Result( eResult result )
		{  m_Code = result;  m_Raw = 0;  }
	Result( eResult result, DWORD raw )
		{  m_Code = result;  m_Raw = raw;  }
	Result( eResult result, int i )
		{  m_Code = result;  m_Int = i;  }
	Result( eResult result, float f )
		{  m_Code = result;  m_Float = f;  }

	bool IsError( void ) const
		{  return ( Skrit::IsError( m_Code ) );  }
	bool IsSuccess( void ) const
		{  return ( Skrit::IsSuccess( m_Code ) );  }
};

inline bool IsError  ( const Result& result )  {  return ( result.IsError  () );  }
inline bool IsSuccess( const Result& result )  {  return ( result.IsSuccess() );  }

//////////////////////////////////////////////////////////////////////////////
// Skrit API declarations

// Command execution.

	Result ExecuteCommand     ( const char* name, const char* command, int len = -1 );
	bool   EvalFloatExpression( const char* expr, float& result );
	bool   EvalIntExpression  ( const char* expr, int  & result );
	bool   EvalBoolExpression ( const char* expr, bool & result );

// Callback methods.

	Result SendEvent( Object* object, UINT eventSerial, const void* params );
	Result SendEvent( HObject hobject, UINT eventSerial, const void* params );
	Result SendEvent( Object* object, const FuBi::FunctionSpec* spec, const void* params );
	Result SendEvent( HObject hobject, const FuBi::FunctionSpec* spec, const void* params );
	Result CallSkrit( Object* object, int funcIndex, const FuBi::FunctionSpec* spec, const void* params );
	Result CallSkrit( HObject hobject, int funcIndex, const FuBi::FunctionSpec* spec, const void* params );
	Result CallSkrit( Object* object, const char* funcName, const FuBi::FunctionSpec* spec, const void* params );
	Result CallSkrit( HObject hobject, const char* funcName, const FuBi::FunctionSpec* spec, const void* params );

//////////////////////////////////////////////////////////////////////////////
// constants

// Errors.

	const int INVALID_FUNCTION_INDEX = -1;

// States.

	const int STATE_INDEX_NONE = -1;

	inline int GetStateIndex( WORD state )
	{
		// it doesn't sign extend...
		return ( (state == (WORD)STATE_INDEX_NONE ) ? STATE_INDEX_NONE : state );
	}

// Events.

	const int TIME_NONE = -1;

//////////////////////////////////////////////////////////////////////////////
// helper methods

// the generic case

template <typename T>
inline UINT AppendToBuffer( Buffer& buffer, const T& obj )
{
	UINT offset = buffer.size();
	buffer.insert( buffer.end(),
				   rcast <const BYTE*> ( &obj ),
				   rcast <const BYTE*> ( &obj + 1 ) );
	return ( offset );
}

// for memory buffers

template <>
inline UINT AppendToBuffer( Buffer& buffer, const const_mem_ptr& mem )
{
	UINT offset = buffer.size();
	const BYTE* begin = rcast <const BYTE*> ( mem.mem );
	buffer.insert( buffer.end(), begin, begin + mem.size );
	return ( offset );
}

// for iterating through variable sized objects

template <typename T, typename U>
inline T Advance( U old, UINT size )
{
	return ( rcast <T> ( rcast <const BYTE*> ( old ) + size ) );
}

// for strings

template <>
inline UINT AppendToBuffer( Buffer& buffer, const gpstring& str )
{
	return ( AppendToBuffer( buffer, const_mem_ptr( str.c_str(), str.length() + 1 ) ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

using Skrit::ToString;
using Skrit::FromString;

//////////////////////////////////////////////////////////////////////////////

#endif  // __SKRITDEFS_H

//////////////////////////////////////////////////////////////////////////////
