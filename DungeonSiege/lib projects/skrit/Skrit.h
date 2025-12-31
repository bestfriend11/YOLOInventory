//////////////////////////////////////////////////////////////////////////////
//
// File     :  Skrit.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains Skrit (Siege script) system API's and types. Also
//             has the lexer, parser, and compiler.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRIT_H
#define __SKRIT_H

//////////////////////////////////////////////////////////////////////////////

// if you get this error then you are #including Skrit.h and you shouldn't
// be! did you really want to #include SkritDefs.h instead? build problems
// will occur if you try to use Skrit.h directly!
#ifndef __PRECOMP_SKRIT_H
#error Do not #include Skrit.h directly! Use SkritDefs.h instead!
#endif

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"
#include "ReportSys.h"
#include "SkritDefs.h"
#include "SkritStructure.h"

#include <map>
#include <list>

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// forward declarations

class Scanner;

//////////////////////////////////////////////////////////////////////////////
// struct Location declaration

#pragma pack ( push, 1 )

struct Location
{
	const Scanner* m_Scanner;

	// these are both 1-based
	WORD m_Line;
	WORD m_Col;

	Location( void )										{  Reset();  }
	Location( const Scanner* scanner, int line, int col )	{  m_Scanner = scanner;  m_Line = (WORD)line;  m_Col = (WORD)col;  }

	void Reset  ( void )									{  m_Scanner = NULL;  m_Line = 0;  m_Col = 0;  }
	bool IsValid( void ) const								{  return ( m_Line != 0 );  }
};

#pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////////
// define YYSTYPE for base classes

struct ParseNode
{
	int            m_Token;			// what's my token?
	Location       m_Location;		// where is this thing?
	FuBi::TypeSpec m_Type;			// type of this node
	ParseNode*     m_Next;			// next in list

	union							// custom data
	{
		DWORD       m_Raw;			// raw access
		int         m_Int;			// for integer constants or random context-sensitive int data
		float       m_Float;		// for float constants
		void*       m_Generic;		// for generic pointer data

		// m_Int is:
		//
		//   SKRIT_T_SYSID          - offset into scanner's string table
		//   SKRIT_T_USERID         - offset into scanner's string table
		//   SKRIT_T_STRINGCONSTANT - offset into string table where constant lives
	};

	void        DeleteChain         ( void );				// recursively delete linked nodes
	void        AddLinkEnd          ( ParseNode* node );	// add a new link to the end of the chain
	int         GetLinkCount        ( void ) const;			// see how many links attached
	void        SetNonConstantToEval( void );				// set to T_EVAL token if not a constant
	const char* ResolveString       ( void ) const;

	// helper
	struct AutoDeleteChain
	{
		ParseNode* m_Node;

		AutoDeleteChain( ParseNode& node ) : m_Node( &node )  {  }
	   ~AutoDeleteChain( void )  {  m_Node->DeleteChain();  }
	};
};

// lex/yacc needs this stuff
typedef ParseNode YYSTYPE;
#define YYSTYPE YYSTYPE

// pick up base classes now that YYSTYPE is available
#if GP_DEBUG
#include "Skrit-L-Debug.h.out"
#include "Skrit-Y-Debug.h.out"
#elif GP_RELEASE
#include "Skrit-L-Release.h.out"
#include "Skrit-Y-Release.h.out"
#elif GP_RETAIL
#include "Skrit-L-Retail.h.out"
#include "Skrit-Y-Retail.h.out"
#elif GP_PROFILING
#include "Skrit-L-Profiling.h.out"
#include "Skrit-Y-Profiling.h.out"
#endif

//////////////////////////////////////////////////////////////////////////////
// class Reporter declaration

#if !GP_ERROR_FREE

class Reporter
{
public:
	SET_NO_INHERITED( Reporter );

	Reporter( void );

	enum eMessageType
	{
		MESSAGE_INFO,		// informational
		MESSAGE_WARNING,	// simple warning
		MESSAGE_ERROR,		// nonfatal error
		MESSAGE_FATAL,		// fatal error
	};

	void Reset     ( void );
	void SetName   ( const char* name )  {  m_Name = name;  }
	void Message   ( Location location, eMessageType type, const char* msg, va_list args );
	void Summary   ( bool force = false );
	bool HasMessage( void ) const  {  return ( (m_Warnings > 0) || (m_Errors > 0) || (m_Fatals > 0) );  }
	bool HasError  ( void ) const  {  return ( (m_Errors > 0) || (m_Fatals > 0) );  }
	void Report    ( bool forceSummary = false );
	void SetDelayed( bool delayed )  {  m_DelayedReporting = delayed;  }
	bool Success   ( void );

	const char* GetName( void ) const;

private:
	bool                    m_DelayedReporting;
	ReportSys::StringSink   m_StringSink;
	ReportSys::LocalContext m_LocalContext;
	int                     m_Warnings;
	int                     m_Errors;
	int                     m_Fatals;
	const char*             m_Name;

	SET_NO_COPYING( Reporter );
};

#endif // GP_ERROR_FREE

//////////////////////////////////////////////////////////////////////////////
// class Scanner declaration

class Scanner : public BaseScanner
{
public:
	SET_INHERITED( Scanner, BaseScanner );

	enum eSkipMode
	{
		SKIP_NONE,			// not skipping anything
		SKIP_KEEP,			// we hit an #only( type ) but are not skipping code
		SKIP_CODE,			// we're skipping code 
	};

// Setup.

	// ctor/dtor
	Scanner( Compiler* compiler );
	virtual ~Scanner( void );

	// setup
	virtual void Reset( void );

	// name
	void            SetName( const gpstring& name )			{  m_Name = name;  }
	const gpstring& GetName( void ) const					{  return ( m_Name );  }

// Commands.

	// action
	virtual int Scan( void );

// Query.

	Location    GetLocation     ( void ) const				{  return ( Location( this, GetLine(), GetColumn() ) );  }
	Location    GetStartLocation( void ) const				{  return ( Location( this, m_StartLine, m_StartCol ) );  }
	const char* ResolveString   ( UINT offset ) const		{  return ( rcast <const char*> ( &*m_Strings.begin() ) + offset );  }
	bool        IsPreprocessing ( void ) const				{  return ( m_PreprocessLine != 0 );  }
	bool        SetSkipMode     ( bool skipCode );
	void        ClearSkipMode   ( void );

private:
	virtual void Message( eMessageType type, const char* msg, ... );
	void Message( Location location, eMessageType type, const char* msg, ... );

	bool Preprocess( void );

	// implemented in skrit.l
	int  ScanIdentifier    ( void );
	int  ScanIntConstant   ( void );
	int  ScanFloatConstant ( void );
	int  ScanStringConstant( void );
	int  ScanCharConstant  ( void );

	// local utility
	UINT AddCurrentString( void );

	// redirection
	Compiler* m_Compiler;

	// misc
	gpstring  m_Name;						// used for reporting
	Buffer    m_Strings;					// this just collects identifiers and stuff - otherwise it's lost in the stream
	int       m_PreprocessLine;				// line we're on for preprocessing
	bool      m_DoEscapes;					// process escape chars for strings
	eSkipMode m_SkipMode;					// type of skipping we're doing

	friend Inherited;
	SET_NO_COPYING( Scanner );
};

int GetKeywordCount( void );
const char* GetKeyword( int index );

//////////////////////////////////////////////////////////////////////////////
// class Compiler declaration

class Compiler : public BaseParser
{
public:
	SET_INHERITED( Compiler, BaseParser );

// Setup.

	// ctor/dtor.
	Compiler( void );
	virtual ~Compiler( void );

	// options
#	if !GP_RETAIL
	void SetDebugMode( bool set = true )  {  m_DebugMode = set;  }
#	endif // !GP_RETAIL
	bool GetOptExplicitStatesOnly ( void ) const  {  return ( m_OptExplicitStatesOnly  );  }
	bool GetOptForceFloatConstants( void ) const  {  return ( m_OptForceFloatConstants );  }

	// setup
	virtual void Reset( void );

// Compile API.

	// compile
	bool    Compile( const char* name, const_mem_ptr mem,						// compile to internal buffers, return false on failure
					 const CreateReq* defOptions = NULL );	
	mem_ptr Save   ( void ) const;												// save compiled output to memory buffer
	void    MoveTo ( ObjectImpl& object );										// move compiled output to the object data

	// special
	void IncludeDirective( ParseNode& node );									// this is a requested #include midstream
	void OptionDirective ( ParseNode& node, bool enable );						// this is a requested #option midstream
	void OnlyDirective   ( ParseNode& node );									// this is a requested #only midstream

	// post-compile query
	eVarType GetOwnerType( void ) const  {  return ( m_OwnerType );  }
	void ReportLastCompile( bool forceSummary = false );

// Scanner API.

	// strings - returns offset into string table
	int AddStringConstant( const char* name, int len = -1 );
	int AddStringConstant( const gpstring& name )  {  return ( AddStringConstant( name, name.length() ) );    }

	// get strings
	const char* ResolveStringConstant( int offset );

	// access to scanner (lexer)
	Scanner*       GetScanner( void )        {  return ( m_Scanners.back().m_Scanner );  }
	const Scanner* GetScanner( void ) const  {  return ( m_Scanners.back().m_Scanner );  }

	// registration of 'only' directive conditions
	void RegisterDefaultOnlyConditions( void );
	void RegisterOnlyCondition( const gpstring& name, bool value )  {  m_ConditionMap[ name ] = value;  }

#	if !GP_ERROR_FREE
	Reporter* GetReporter( void )  {  return ( &m_Reporter );  }
#	endif // !GP_ERROR_FREE

private:

// Overrides.

	virtual void           Message      ( eMessageType type, const char* msg, ... );
	virtual void           InternalError( eMessageType type, const char* msg );
	virtual int            Scan         ( void );
	virtual const YYSTYPE& GetLValue    ( void ) const  {  return ( GetScanner()->GetLValue() );  }
	virtual bool           IsEof        ( void ) const  {  return ( GetScanner()->IsEof() );  }

// Private utility.

	// forward declarations
	struct Scope;
	struct Symbol;
	struct SearchContext;
	enum   eSymType;
	enum   ePatchType;

	// exports
	Raw::ExportEntry*         AddExport            ( const char* funcName, Location location, const char* docs, int paramCount, int eventSerial );
	Raw::ExportEntry*         AddExportParam       ( Raw::ExportEntry* oldEntry, const ParseNode& param );
	bool                      SetGlobalType        ( ParseNode& type );
	bool                      AddVariable          ( ParseNode& name, ParseNode* value, ParseNode* doco );
	bool                      AddTempVariable      ( const ParseNode& node, const FuBi::TypeSpec type );
	bool                      AddTempVariable      ( ParseNode& node );
	bool                      ResolveClassMethod   ( SearchContext& ctx );
	bool                      EmitCall             ( ParseNode& node, bool optional = false, bool allowVar = true );
	bool                      EmitSysCall          ( ParseNode& node, bool optional = false, bool allowVar = true );
	bool                      EmitUserCall         ( ParseNode& node, bool optional = false );
	bool                      EmitConditionalBranch( ParseNode& conditional, ParseNode& expr );
	bool                      EmitLoopGoto         ( ParseNode& node, ePatchType patchType, const char* keyword );
	bool                      ResolvePostFixExpr   ( ParseNode& node );
	const FuBi::FunctionSpec* FindFuBiEvent        ( const gpstring& funcName );
	bool                      CheckEventBinding    ( void );
	FuBi::TypeSpec            GetFunctionReturn    ( void ) const;
	void                      EndFunction          ( ParseNode& brace );
	void                      TempVarCheckpoint    ( const ParseNode& checkNode );

	// state table
	Symbol*     AddOrFindState       ( const char* name, Location location, bool declare = false );
	void        PrepEmitState        ( void );
	Raw::State* GetStateHeader       ( void );
	Symbol*     GetStateSymbol       ( int state );
	Symbol*     GetCurrentStateSymbol( void );
	void        EmitStateConfigs     ( void );
	void        EmitTransition       ( void );
	bool        EmitStaticCondition  ( ParseNode& eventNode, bool isTrigger = false );
	void        FinishCompile        ( void );

	// bytecode output
	void Emit         ( Op::Code data, const ParseNode& node );
	void EmitPush     ( int data, const ParseNode& node );
	int  GetCodeOffset( void ) const;
	void PatchCodeWord( int offset, int newValue );

	// expression conversions
	bool ConvertConstant ( ParseNode& srcNode, const FuBi::TypeSpec& destType );
	bool EmitPromotion   ( ParseNode& l, ParseNode& op, ParseNode& r );
	void EmitConversion  ( const char* what, const FuBi::TypeSpec& from, const FuBi::TypeSpec& to, int index, ParseNode& node, bool warnOnCast = true );
	void EmitCast        ( ParseNode& node, eVarType type );
	void EmitBinaryOp    ( Op::Code opBase, const char* symbol, ParseNode& l, ParseNode& op, ParseNode& r );
	bool EmitAssignment  ( ParseNode& l, ParseNode& op, ParseNode& r, const char* opText = "=" );
	bool EmitAssignmentOp( ParseNode& l, ParseNode& temp, ParseNode& op, ParseNode& r );

	enum eFreeScopeMode
	{
		FREESCOPE_NORMAL,
		FREESCOPE_STRINGSONLY,
		FREESCOPE_NOEMIT,
		FREESCOPE_NODELETE,
	};

	// symbols
	Symbol* AddSymbol         ( const char* name, Location location, eSymType symtype, FuBi::TypeSpec vartype = FuBi::VAR_UNKNOWN, bool* addResults = NULL );
	Symbol* FindSymbol        ( const char* name, Scope* base = NULL );
	Symbol& GetSymbol         ( Scope* scope, int index );
	void    EnterScope        ( const ParseNode& node );
	bool    LeaveScope        ( const ParseNode& node, int count, eFreeScopeMode mode = FREESCOPE_NORMAL );
	bool    LeaveScope        ( const ParseNode& node, eFreeScopeMode mode = FREESCOPE_NORMAL );
	bool    LeaveToGlobalScope( const ParseNode& node, eFreeScopeMode mode = FREESCOPE_NORMAL );
	Scope*  GetBackScope      ( void );
	bool    ResolveEnum       ( ParseNode& node, bool reportError );
	bool    ResolveSysType    ( ParseNode& node );

	// reporting
	void ReportConversionError( const char* location, const char* what, const FuBi::TypeSpec& from, const FuBi::TypeSpec& to, const ParseNode& node );
	void ReportUnusedVars( int symbolDepth );

// Private types.

	enum eSymType				// type of a symbol
	{
		SYMTYPE_VARIABLE,		// simple variable     - name, type, index in m_Variables
		SYMTYPE_STRING,			// string variable     - name, index in m_Strings
		SYMTYPE_HANDLE,			// handle              - name, type, offset in m_Handle
		SYMTYPE_FUNCTION,		// function definition - name only
		SYMTYPE_EVENT,			// event handler       - name only
		SYMTYPE_STATE,			// state definition    - name only, state number (index)
	};

	static bool IsVariable( eSymType type )
	{
		switch ( type )
		{
			case SYMTYPE_VARIABLE: case SYMTYPE_STRING: case SYMTYPE_HANDLE: return ( true );
		}
		return ( false );
	}

	static bool IsGlobalOnly( eSymType type )
	{
		switch ( type )
		{
			case SYMTYPE_FUNCTION: case SYMTYPE_EVENT: case SYMTYPE_STATE: return ( true );
		}
		return ( false );
	}

	// $ this local/global scope and symbol management is sucky. do not repeat
	//   in a future engine.

	// local/global symbol
	struct Symbol
	{
		eSymType       m_SymType;		// type of this symbol
		FuBi::TypeSpec m_VarType;		// simple type - if it's user-defined then it's always a pointer
		int            m_Index;			// index in variable collection for the skrit object, or index of function export in skrit
		Raw::eStore    m_Store;			// which store it's in (if a variable)
		Raw::eFlags    m_Flags;			// flags for this variable
		int            m_Offset;		// offset into function exports if a function
		Scope*         m_Scope;			// scope it belongs to
		gpstring       m_Name;			// what's my name?
		Location       m_Location;		// where it was created
		int            m_TouchCount;	// number of times this has been searched for
		int            m_SymbolIndex;	// index in global vector

		Symbol( void )  {  }

		Symbol( eSymType symtype, FuBi::TypeSpec vartype, Scope* scope, const gpstring& name, Location location, int symbolIndex )
		{
			m_SymType     = symtype;
			m_VarType     = vartype;
			m_Index       = 0;
			m_Offset      = 0;
			m_Store       = Raw::STORE_LOCAL;
			m_Flags       = Raw::FLAG_NONE;
			m_Scope       = scope;
			m_Name        = name;
			m_Location    = location;
			m_TouchCount  = 0;
			m_SymbolIndex = symbolIndex;
		}
	};

	// patching
	enum ePatchType
	{
		PATCH_HEAD,						// patch to point to start of scope
		PATCH_TAIL,						// patch to point to end of scope (right after the '}')
	};

	// patching
	struct PatchEntry
	{
		int        m_PatchPoint;		// where in code to patch
		ePatchType m_Type;				// what kind of patch is this?
	};

	// scope
	struct Scope
	{
		typedef std::map <gpstring, int, istring_less> SymbolByNameIndex;
		typedef std::pair <SymbolByNameIndex::iterator, bool> SymbolByNameIndexInsertRet;
		typedef std::list <PatchEntry> PatchList;

		Scope*            m_Parent;				// parent scope - iterate through these to resolve names
		SymbolByNameIndex m_SymbolsByName;		// index of symbols by their names
		int               m_SymbolIndex;		// starting index of symbol table where this scope begins
		int               m_VariableCountBase;	// base for variable index (from parent)
		int               m_VariableCount;		// current number of simple variables in this scope
		int               m_StringCountBase;	// base for string index (from parent)
		int               m_StringCount;		// current number of strings in this scope
		int               m_HandleCountBase;	// base for handle index (from parent)
		int               m_HandleCount;		// current number of handles in this scope
		Location          m_Location;			// where this scope was opened
		bool              m_IsLoop;				// is this a loop? use for break and continue verification
		PatchList         m_PatchPoints;		// these will be patched with the
		int               m_CodeHead;			// offset to code for head
		int               m_CodeTail;			// offset to code for tail

		Scope( Scope* parent = NULL, int startSymbol = 0, Location location = Location( NULL, 1, 1 ) )
		{
			m_Parent            = parent;
			m_SymbolIndex       = startSymbol;
			m_VariableCountBase = 0;
			m_VariableCount     = 0;
			m_StringCountBase   = 0;
			m_StringCount       = 0;
			m_HandleCountBase   = 0;
			m_HandleCount       = 0;
			m_Location          = location;
			m_IsLoop            = false;
			m_CodeHead          = 0;
			m_CodeTail          = 0;
		}

		void Reset( void );

		SET_NO_COPYING( Scope );
	};

	// global scope
	struct GlobalScope : Scope
	{
		int m_SharedVariableCount;
		int m_SharedStringCount;
		int m_SharedHandleCount;

		GlobalScope( void )
		{
			m_SharedVariableCount = 0;
			m_SharedStringCount   = 0;
			m_SharedHandleCount   = 0;
		}

		void Reset( void );

		SET_NO_COPYING( GlobalScope );
	};

	// function spec
	struct Function
	{
		// $ update Reset() function any time the members change

		Raw::ExportEntry* m_Export;				// pointer to export table
		Op::Code          m_LastOpCode;			// last thing we did
		int               m_LastOpCodeScope;	// what level of scope it was at
		int               m_NestedLevel;		// >0 if recovering from nested function
		int               m_LastLine;			// the last line that we have opcode location info for
		bool              m_LocalState;			// state is local to this function only

		Function( void )  {  Reset();  }
		void Reset( void );

		SET_NO_COPYING( Function );
	};

	// state spec
	struct State
	{
		// $ update Reset() function any time the members change

		int              m_SymbolIndex;			// index of this state as a symbol
		int              m_StateIndex;			// index of this state period
		Raw::StateConfig m_Config;				// configuration to be stored in state out stream
		bool             m_NeedsSaving;			// should this be saved?
		bool             m_Declared;			// true if this was explicitly declared

		State( void )  {  Reset();  }
		void Reset( void );
	};

	// static condition spec
	struct ConditionStatic
	{
		Raw::ConditionStatic m_Condition;		// the condition
		Buffer m_TestData;						// data to test against

		void Update( void )
		{
			gpassert( m_TestData.size() < 256 );
			m_Condition.m_TestDataSize = scast <BYTE> ( m_TestData.size() );
		}
	};

	// dynamic condition spec
	typedef Raw::ConditionDynamic ConditionDynamic;

	// state transition spec
	struct Transition
	{
		typedef stdx::fast_vector <ConditionStatic>  StaticColl;
		typedef stdx::fast_vector <ConditionDynamic> DynamicColl;

		int         m_FromState;
		int         m_ToState;
		StaticColl  m_Statics;
		DynamicColl m_Dynamics;
		DWORD       m_ResponseOffset;

		Transition( void )  {  Reset();  }
		void Reset( void );
		bool IsValid( void ) const;
	};

	// global state spec
	struct GlobalState
	{
		// $ update Reset() function any time the members change

		int  m_Current;							// current state index
		bool m_SetStartup;						// we chose a startup state automatically
		bool m_ChoseStartup;					// user chose a startup state
		int  m_NestedLevel;						// >0 if recovering from nested state

		GlobalState( void )  {  Reset();  }
		void Reset( void );
		void ResetLocal( void );

		SET_NO_COPYING( GlobalState );
	};

	// these are in overriding order - the "most critical" error is the
	//  one that gets reported, even though lots of problems may pop up
	//  during function matching.
	enum eCallStatus
	{
		CALLSTATUS_UNDEFINED,			// intermediate state
		CALLSTATUS_PARAMCOUNT,			// parameter count mismatch
		CALLSTATUS_CANNOTSKRIT,			// function cannot be called by skrit
		CALLSTATUS_TYPECONVERSION,		// could not perform type conversion
		CALLSTATUS_AMBIGUOUS,			// ambiguous call to overloaded function
		CALLSTATUS_REPORTED,			// failure that we already reported
		CALLSTATUS_SUCCESS,				// success
	};

	friend SearchContext;

	// context info for name resolution on class member
	struct SearchContext
	{
		enum eFound
		{
			FOUND_NONE,
			FOUND_METHOD,
			FOUND_VARIABLE,
			FOUND_PROPERTY,
		};

		// in
		const char* m_FuncName;
		int         m_ParamCount;
		bool        m_AllowVar;

		// out
		const FuBi::FunctionIndex* m_MethodIndex;
		int                        m_PropertyIndex;
		eCallStatus                m_Status;
		const FuBi::ClassSpec*     m_BaseClass;
		const FuBi::ClassSpec*     m_DerivedClass;
		int                        m_ClassOffsetAdjust;
		eFound                     m_Found;

		// local
		FuBi::FunctionIndex m_LocalIndex;

		SearchContext( const char* funcName, int paramCount, bool allowVar )
		{
			// take in
			m_FuncName   = funcName;
			m_ParamCount = paramCount;
			m_AllowVar   = allowVar;

			// setup out
			m_MethodIndex       = NULL;
			m_PropertyIndex     = -1;
			m_Status            = CALLSTATUS_UNDEFINED;
			m_BaseClass         = NULL;
			m_DerivedClass      = NULL;
			m_ClassOffsetAdjust = 0;
			m_Found             = FOUND_NONE;
		}

		void InitClassSearch( const FuBi::ClassSpec* cls )
		{
			m_BaseClass    = cls;
			m_DerivedClass = cls;
		}
	};

// Private data.

	struct ScannerEntry
	{
		my Scanner*   m_Scanner;
		const_mem_ptr m_SourceCode;
		bool          m_OwnsMemory;
		int           m_ScannerIndex;

		ScannerEntry( Scanner* takeScanner = NULL, int scannerIndex = 0 )
		{
			m_Scanner = takeScanner;
			m_OwnsMemory = false;
			m_ScannerIndex = scannerIndex;
		}

		void Reset( void )
		{
			m_Scanner->Reset();
			m_SourceCode = const_mem_ptr();
			m_OwnsMemory = false;
			m_ScannerIndex = 0;
		}

		void Init( const char* name, const_mem_ptr mem, bool owns = false )
		{
			m_Scanner->Init( mem );
			m_Scanner->SetName( name );
			m_SourceCode = mem;
			m_OwnsMemory = owns;
		}

		void Delete( void )
		{
			::Delete( m_Scanner );

			if ( m_OwnsMemory )
			{
				delete ( (BYTE*)m_SourceCode.mem );
				m_OwnsMemory = false;
			}

			m_SourceCode = const_mem_ptr();
		}
	};

	typedef std::list         <ScannerEntry>                 Scanners;		// stack of owned scanners
	typedef stdx::fast_vector <Symbol>                       Symbols;		// cookie == index
	typedef std::list         <my Scope*>                    Scopes;		// tail is current
	typedef stdx::fast_vector <State>                        States;		// new states opened as they are ref'd
	typedef stdx::linear_map  <gpstring, bool, istring_less> ConditionMap;	// condition names mapped onto bools - used for 'only' directive

	// configuration
#	if !GP_RETAIL
	bool m_DebugMode;
#	endif // !GP_RETAIL

	// cache
	Scanners               m_Scanners;			// our current lexer stack (always at least one entry)
	Scanners               m_OldScanners;		// old scanners held open until done compiling
	const FuBi::ClassSpec* m_EventClass;		// where events get registered (namespace called "FuBiEvent")
	eVarType               m_SkritObjectType;	// cached Skrit::Object type
	eVarType               m_SkritVmType;		// cached Skrit::Machine type

	// options
	bool         m_OptExplicitStatesOnly;		// must explicitly declare all states
	bool         m_OptForceFloatConstants;		// int-looking constants are forced to be floats instead
	ConditionMap m_ConditionMap;				// 'only' conditions

	// error reporting
#	if !GP_ERROR_FREE
	Reporter m_Reporter;
#	endif // !GP_ERROR_FREE
	Location m_OverrideLocation;

	// compile state
	Raw::Pack*     m_Pack;						// raw buffer pack
	Function       m_CurrentFunction;			// function we're working on compiling now
	eSymType       m_GlobalSymType;				// current global type for new symbols
	FuBi::TypeSpec m_GlobalVarType;				// current global type for declaring variables
	bool           m_GlobalShared;				// whether or not new global vars are shared
	bool           m_GlobalProperty;			// whether or not new global vars are properties
	bool           m_GlobalHidden;				// whether or not new global vars are to be tagged as hidden
	int            m_TempStringCount;			// how many string temporaries are currently allocated?
	eVarType       m_OwnerType;					// type of the owner (defaults to skrit object, represented by VAR_UNKNOWN)
	int            m_OwnerUsedLine;				// only allow it to be set once
	int            m_AtTimeFrames;				// running total of at-times in frames
	int            m_AtTimeMsec;				// running total of at-times in msec
	int            m_NextScannerIndex;			// index to use for next scanner

	// symbols
	Symbols     m_Symbols;						// all symbols, referenced by index
	int         m_StartOfLocalSymbols;			// start of local symbols in the "all" index
	Scopes      m_LocalScopes;					// scopes where local symbols live
	GlobalScope m_GlobalScope;					// all functions, global vars, and states live here
	int         m_FunctionCount;				// count of functions defined in this skrit

	// states
	GlobalState m_GlobalState;					// current state of our states :)
	States      m_States;						// collection of all states declared so far
	Transition  m_Transition;					// current transition we're working on

	friend Inherited;
	SET_NO_COPYING( Compiler );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

#endif  // __SKRIT_H

//////////////////////////////////////////////////////////////////////////////
