//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTypes.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains FuBi (function binding system) types.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBITYPES_H
#define __FUBITYPES_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FuBiDefs.h"

#include <vector>
#include <map>

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// documentation

/*	*** Case sensitivity ***

		Keep all strings compares case sensitive during symbol table
		construction. The system may export two separate functions with same
		names of different case. Have to check for and warn on this. After
		construction is done, use case insensitive compares but retain the case
		for GenerateDocs() and possibly save game.

	*** Name sharing ***

		All the types here use gpstring so that the names are reference counted
		and shared. Make sure that copies are constructed from gpstrings rather
		than const char* to take advantage of this.
*/

//////////////////////////////////////////////////////////////////////////////
// types

// Common collections.

	typedef stdx::fast_vector <ClassSpec*> ClassIndex;
	typedef stdx::fast_vector <FunctionSpec*> FunctionIndex;
	typedef stdx::fast_vector <EnumSpec*> EnumIndex;
	typedef std::map <gpstring, FunctionIndex, istring_less> FunctionByNameIndex;
	typedef std::pair <FunctionByNameIndex::iterator, bool> FunctionByNameIndexInsertRet;
	typedef stdx::fast_vector <const FunctionIndex*> FunctionIndexColl;

// Simple enums.

#	if !GP_RETAIL
	enum eDocsLevel
	{
		DOCS_NONE,				// nothing extra, just simplest form
		DOCS_MINIMAL,			// simplest form plus doc text
		DOCS_NORMAL,			// normal mode
		DOCS_VERBOSE,			// go crazy!!
	};
#	endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// enum eVarType declaration

	// $ actual declaration in fubidefs.h, but here are the helpers

// Type attribute query.

	inline bool IsBasic  ( eVarType type )  {  return ( (type >= VAR_BEGIN) && (type < VAR_END) );  }
	inline bool IsSpecial( eVarType type )  {  return ( (type >= VAR_SPECIAL_BEGIN) && (type < VAR_SPECIAL_END) );  }
	inline bool IsEnum   ( eVarType type )  {  return ( (type >= VAR_ENUM) && (type < VAR_USER) );  }
	inline bool IsUser   ( eVarType type )  {  return ( type >= VAR_USER );  }

	bool IsRegularEnum  ( eVarType type );
	bool IsIrregularEnum( eVarType type );
	bool IsUnsignedInt  ( eVarType type );

//////////////////////////////////////////////////////////////////////////////
// struct VarTypeSpec declaration

struct VarTypeSpec
{
	const char*    m_InternalName;		// internal name of type
	const char*    m_ExternalName;		// external name (presented to users)
	int            m_SizeBytes;			// size in bytes
	Trait::eFlags  m_Flags;				// flags for this type
	ToStringProc   m_ToStringProc;		// convert this type to a string
	FromStringProc m_FromStringProc;	// convert this type from a string
	CopyVarProc    m_CopyVarProc;		// make a copy of the type

	VarTypeSpec( void )  {  ::ZeroObject( *this );  }
	VarTypeSpec( const ClassSpec* spec );
	VarTypeSpec( const EnumSpec* spec );
	VarTypeSpec( const char* iname, const char* ename, int size, Trait::eFlags flags, ToStringProc to, FromStringProc from, CopyVarProc copy );
};

//////////////////////////////////////////////////////////////////////////////
// struct TypeSpec declaration

struct TypeSpec
{
	enum eFlags
	{
		FLAG_NONE            =      0,
		FLAG_CONST           = 1 << 0,		// const variable
		FLAG_POINTER         = 1 << 1,		// pointer-to-type
		FLAG_POINTER_POINTER = 1 << 2,		// pointer-to-pointer-to-type (special for fubicookie)
		FLAG_REFERENCE       = 1 << 3,		// reference-to-type
		FLAG_HANDLE          = 1 << 4,		// ResHandle <type>
	};

	enum eCompare
	{
		COMPARE_EQUAL,					// types are exactly equal
		COMPARE_PROMOTABLE,				// types can be promoted to match (this is free but not exact equal case)
		COMPARE_COMPATIBLE,				// types can be converted to match
		COMPARE_INCOMPATIBLE,			// types are not compatible without ane explicit cast
	};

	eVarType m_Type;
	eFlags   m_Flags;

	TypeSpec( void )                        : m_Type( VAR_UNKNOWN ), m_Flags( FLAG_NONE )  {  }
	TypeSpec( eVarType type )               : m_Type( type ),        m_Flags( FLAG_NONE )  {  }
	TypeSpec( eVarType type, eFlags flags ) : m_Type( type ),        m_Flags( flags )  {  }

	bool operator == ( eVarType type ) const	{  return ( (m_Type == type) && (m_Flags == FLAG_NONE) );  }
	bool operator != ( eVarType type ) const	{  return ( (m_Type != type) || (m_Flags != FLAG_NONE) );  }
	bool operator == ( TypeSpec type ) const	{  return ( (m_Type == type.m_Type) && (m_Flags == type.m_Flags) );  }
	bool operator != ( TypeSpec type ) const	{  return ( (m_Type != type.m_Type) || (m_Flags != type.m_Flags) );  }

	bool     IsSimple         ( void ) const;
	bool     IsSimpleInteger  ( void ) const;
	bool     IsSimpleFloat    ( void ) const;
	bool     IsPointerClass   ( void ) const;
	bool     IsPassByValue    ( void ) const		{  return ( !(m_Flags & (FLAG_POINTER | FLAG_REFERENCE | FLAG_HANDLE)) );  }
	bool     IsCString        ( void ) const;
	bool     IsCStringW       ( void ) const;
	bool     IsString         ( void ) const;
	bool     IsStringW        ( void ) const;
	bool     IsAString        ( void ) const		{  return ( IsString() || IsCString() );  }
	bool     IsAStringW       ( void ) const		{  return ( IsStringW() || IsCStringW() );  }
	void     SetCString       ( void );
	void     SetString        ( void );
	bool     IsFuBiCookie     ( void ) const;
	bool     IsSingleton      ( void ) const;
	bool     ContentsAreSimple( void ) const;
	bool     CanRPC           ( GPDEBUG_ONLY( gpstring* cantRpcReason ) ) const;
	bool     CanPack          ( void ) const;
	bool     CanSkrit         ( void ) const;
	int      GetSizeBytes     ( void ) const;
	TypeSpec GetBaseType      ( void ) const;

#	if !GP_RETAIL
	void GenerateDocs( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL
};

MAKE_ENUM_BIT_OPERATORS( TypeSpec::eFlags );

TypeSpec::eCompare TestCompatibility( const TypeSpec& tfrom, const TypeSpec& tto );

//////////////////////////////////////////////////////////////////////////////
// struct ConstraintSpec declaration

#if !GP_RETAIL

struct ConstraintSpec
{
	enum eType
	{
		TYPE_INT_RANGE,
		TYPE_FLOAT_RANGE,
		TYPE_STRINGS,
	};

	eType m_Type;

	ConstraintSpec( eType type )  {  m_Type = type;  }
	virtual ~ConstraintSpec( void )  {  }

	virtual ConstraintSpec* Clone( void ) = 0;
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ConstrainRange declaration

#if !GP_RETAIL

template <typename T, const ConstraintSpec::eType TYPE>
struct ConstrainRange : ConstraintSpec
{
	SET_INHERITED( ConstrainRange, ConstraintSpec );

	enum eFlags
	{
		FLAG_NONE            =      0,
		FLAG_LEFT_INCLUSIVE  = 1 << 0,		// a <= x
		FLAG_RIGHT_INCLUSIVE = 1 << 1,		// x <= b
		FLAG_LEFT_OPEN       = 1 << 2,		// a <  x
		FLAG_RIGHT_OPEN      = 1 << 3,		// x <  b
	};

	eFlags m_Flags;
	T      m_Left;
	T      m_Right;

	ConstrainRange( void ) : Inherited( TYPE )  {  m_Flags = FLAG_NONE;  m_Left = m_Right = 0;  }

	virtual ConstraintSpec* Clone( void )  {  return ( new ThisType( *this ) );  }
};

typedef ConstrainRange <int  , ConstraintSpec::TYPE_INT_RANGE  > IntConstrainRange;
typedef ConstrainRange <float, ConstraintSpec::TYPE_FLOAT_RANGE> FloatConstrainRange;

MAKE_ENUM_BIT_OPERATORS( IntConstrainRange  ::eFlags );
MAKE_ENUM_BIT_OPERATORS( FloatConstrainRange::eFlags );

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ConstrainStrings declaration

#if !GP_RETAIL

struct ConstrainStrings : public ConstraintSpec
{
	SET_INHERITED( ConstrainStrings, ConstraintSpec );

	struct Entry
	{
		gpstring m_String;
		gpstring m_Docs;

		Entry( void )
			{  }
		Entry( const gpstring& str )
			: m_String( str )  {  }
		Entry( const gpstring& str, const gpstring& docs )
			: m_String( str ), m_Docs( docs )  {  }
	};

	typedef stdx::fast_vector <Entry> StringColl;

	ConstrainStrings( void ) : Inherited( TYPE_STRINGS )  {  }

	virtual int OnGetStrings( StringColl& strings ) = 0;
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class ConstrainStaticStrings declaration

#if !GP_RETAIL

class ConstrainStaticStrings : public ConstrainStrings
{
public:
	SET_INHERITED( ConstrainStaticStrings, ConstrainStrings );

	void AddStaticEntry( const Entry& entry )			{  m_StringColl.push_back( entry );  }

	virtual ConstraintSpec* Clone( void )				{  return ( new ThisType( *this ) );  }
	virtual int OnGetStrings( StringColl& strings )		{  strings = m_StringColl;  return ( m_StringColl.size() );  }

private:
	StringColl m_StringColl;
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class ConstrainEnumStrings declaration

#if !GP_RETAIL

class ConstrainEnumStrings : public ConstrainStrings
{
public:
	SET_INHERITED( ConstrainEnumStrings, ConstrainStrings );

	ConstrainEnumStrings( eVarType enumType );

	virtual ConstraintSpec* Clone( void )				{  return ( new ThisType( *this ) );  }
	virtual int OnGetStrings( StringColl& strings );

private:
	eVarType m_EnumType;
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ParamSpec declaration

struct ParamSpec
{
	struct Extra
	{
		gpstring           m_Name;				// name of parameter
		gpstring           m_DefaultValue;		// default value to be used (NULL if no default)
		bool               m_DefaultIsCode;		// set to true of default value is actually code to be run to calc def value
		bool               m_NoXfer;			// don't bother persisting this
#		if !GP_RETAIL
		my ConstraintSpec* m_ConstraintSpec;	// constraint function applied to param
#		endif // !GP_RETAIL

		Extra( void );
	   ~Extra( void );

		Extra& operator = ( const Extra& other );
	};

	TypeSpec m_Type;
	Extra*   m_Extra;

	ParamSpec( const ParamSpec& other );
	ParamSpec( const TypeSpec& type )								: m_Type( type ), m_Extra( NULL )  {  }
   ~ParamSpec( void )                   							{  delete ( m_Extra );  }

	ParamSpec& operator = ( const ParamSpec& other );

	bool operator == ( TypeSpec type ) const						{  return ( m_Type == type );  }

	// extra mod
	Extra* GetExtra  ( void )										{  BuildExtra();  return ( m_Extra );  }
	void   BuildExtra( void )										{  if ( m_Extra == NULL )  {  m_Extra = new Extra;  }  }

	// attributes
	void                  SetName          ( const gpstring& name )	{  GetExtra()->m_Name = name;  }
	const gpstring&       GetName          ( void ) const			{  gpassert( m_Extra != NULL );  return ( m_Extra->m_Name );  }
	gpstring              GetDefaultValue  ( void ) const;
	void                  SetDefaultValue  ( const gpstring& defValue, bool isCode = false );
	void                  SetNoDefaultValue( void );

	// utility
	DWORD CalcDigest  ( void ) const;

#	if !GP_RETAIL
	void                  SetConstraint( ConstraintSpec* spec );
	const ConstraintSpec* GetConstraint( void ) const;
	void                  GenerateDocs ( ReportSys::ContextRef context = NULL, eDocsLevel level = DOCS_NORMAL ) const;
#	endif // !GP_RETAIL
};

//////////////////////////////////////////////////////////////////////////////
// struct FunctionSpec declaration

struct FunctionSpec
{
	enum eFlags
	{
		FLAG_NONE              =       0,

		// $ keep these in sync with FunctionSpecFlags in FuBiDefs.h

		// call types
		FLAG_CALL_CDECL        = 1 <<  0,	// __cdecl
		FLAG_CALL_FASTCALL     = 1 <<  1,	// __fastcall
		FLAG_CALL_STDCALL      = 1 <<  2,	// __stdcall
		FLAG_CALL_THISCALL     = 1 <<  3,	// __thiscall
		FLAG_CALL_MASK         =     0xF,	// use to mask call convention

		// function traits
		FLAG_MEMBER            = 1 <<  4,	// member function of a class
		FLAG_STATIC_MEMBER     = 1 <<  5,	// static member function of a class
		FLAG_CONST             = 1 <<  6,	// member function is const
		FLAG_VARARG            = 1 <<  7,	// variable arg function

		// permissions
		FLAG_HIDDEN            = 1 <<  8,	// hidden from ordinary queries - reserved function or set/get
		FLAG_DOCO              = 1 <<  9,	// this is a doco function - not critical
		FLAG_CHECK_SERVER_ONLY = 1 << 10,	// this only has permission to execute on the server, check for it
		FLAG_DEV_ONLY          = 1 << 11,	// only allow this command in development builds (!GP_RETAIL)
		FLAG_RETRY             = 1 << 12,	// this is a "retryable" function - retry until it succeeds

		// misc
		FLAG_POSTPROCESSED     = 1 << 13,	// function already postprocessed for doco etc.
		FLAG_SIMPLE_ARGS       = 1 << 14,	// args do not contain things like pointers or handles
		FLAG_CAN_RPC           = 1 << 15,	// this function can be called via RPC
		FLAG_CAN_SKRIT         = 1 << 16,	// this function can be called from SiegeSkrit
		FLAG_CAN_PACK_FRONT    = 1 << 17,	// front has packable param
		FLAG_CAN_PACK_BACK     = 1 << 18,	// back has packable param
		FLAG_SKRIT_IMPORT      = 1 << 19,	// this function is meant for skrit importing
		FLAG_EVENT             = 1 << 20,	// this function is a skrit event
		FLAG_TRAIT             = 1 << 21,	// trait-related function

		// membership
		FLAG_MEMBER_OF_ALL     = 1 << 22,	// enabled for everybody (same as setting all the other bits)
		FLAG_MEMBER_OF_GAME    = 1 << 23,	// just enabled for game
		FLAG_MEMBER_OF_CONSOLE = 1 << 24,	// just enabled for the development console
		FLAG_MEMBER_OF_EDITOR  = 1 << 25,	// just enabled for SiegeEditor
		FLAG_MEMBER_OF_TRIAL   = 1 << 26,	// this is a trial function - do not rely on it yet!
	};

	static eFlags ToFlags( FunctionSpecFlags::ePermissions bit );
	static eFlags ToFlags( FunctionSpecFlags::eMembership  bit );
	static bool   TestMembership( eFlags flags, FunctionSpecFlags::eMembership bit );

	static const eFlags DEFAULT_MEMBERSHIP;

	typedef stdx::fast_vector <ParamSpec> ParamSpecs;
	typedef CBFunctor3 <gpstring& /*appendHere*/, eVarType /*type*/, const_mem_ptr /*object*/> PodTranslateCb;
	typedef CBFunctor3 <gpstring& /*appendHere*/, int /*paramIndex*/, const_mem_ptr /*mem*/> MemTranslateCb;

	gpstring    m_Name;					// name of this function
	ClassSpec*  m_Parent;				// member of a class, or NULL if global
	eFlags      m_Flags;				// calling convention, modifiers, traits
	DWORD       m_FunctionPtr;			// memory address of start of function
	UINT        m_SerialID;				// serial number of function based on EXE import order
	ParamSpecs  m_ParamSpecs;			// parameter types and names this function takes
	UINT        m_ParamSizeBytes;		// total size of parameter set, if vararg then it's the size of the known params
	TypeSpec    m_ReturnType;			// return type of function
	DWORD       m_Digest;				// this is a digest of this function - use as a checksum

#	if !GP_RETAIL
	const char* m_Docs;					// docs for this function
	const char* m_MangledName;			// original mangled name of the function
	gpstring    m_UnmangledName;		// after we unmangle it - ready to parse
#	endif // !GP_RETAIL

	FunctionSpec( void );

	bool     IsValidGet               ( void ) const;
	bool     IsValidSet               ( void ) const;
	bool     GetRpcRequiresObjectParam( void ) const;
	void     PostProcess              ( void );
	gpstring BuildQualifiedName       ( void ) const;

#	if !GP_RETAIL

	gpstring BuildQualifiedNameAndParams( const_mem_ptr params, const void* object, bool isRpcPacked, bool allowBinaryOut = false, int skipParams = 0 ) const;
	void     AssertValid                ( void ) const;
	void     GenerateDocs               ( ReportSys::ContextRef context = NULL, eDocsLevel level = DOCS_NORMAL ) const;

	static void RegisterPodTranslateCb  ( eVarType type, PodTranslateCb cb );
	static void UnregisterPodTranslateCb( eVarType type );

	static void RegisterMemTranslateCb  ( UINT serialId, MemTranslateCb cb );
	static void UnregisterMemTranslateCb( UINT serialId );

#	endif // !GP_RETAIL
};

MAKE_ENUM_BIT_OPERATORS( FunctionSpec::eFlags );

//////////////////////////////////////////////////////////////////////////////
// struct VariableSpec declaration

struct VariableSpec
{
	gpstring            m_Name;
	const ClassSpec*    m_Parent;
	TypeSpec            m_Type;
	const FunctionSpec* m_GetMethod;				// NULL if write-only
	const FunctionSpec* m_SetMethod;				// NULL if read-only

#	if !GP_RETAIL
	const char*         m_Docs;
#	endif // !GP_RETAIL

#	if !GP_RETAIL
	void AssertValid ( void ) const;
	void GenerateDocs( ReportSys::ContextRef context = NULL, eDocsLevel level = DOCS_NORMAL ) const;
#	endif // !GP_RETAIL

	static bool IsMatched( const FunctionSpec* get, const FunctionSpec* set );
};

//////////////////////////////////////////////////////////////////////////////
// struct ClassSpec declaration

struct ClassSpec
{
	// flags
	enum eFlags
	{
		FLAG_NONE          =      0,
		FLAG_MANAGED       = 1 << 0,			// this is a "managed" class via ResHandleMgr <>
		FLAG_SINGLETON     = 1 << 1,			// this is a "singleton" class via derive from Singleton <>
		FLAG_CANRPC        = 1 << 2,			// pointers to this class can be sent across the network
		FLAG_PROPCANSKRIT  = 1 << 3,			// the properties in this class are available to skrit
		FLAG_POSTPROCESSED = 1 << 4,			// class already postprocessed for doco, variables, etc.
		FLAG_HIDDEN        = 1 << 5,			// don't bother with this class, it's just placeholder
		FLAG_HAS_STATICS   = 1 << 6,			// this class has public (non-hidden) static methods or it's a namespace with global functions
		FLAG_POD           = 1 << 7,			// plain old data
		FLAG_POINTERCLASS  = 1 << 8,			// pointer class - probably a handle type, can never be instantiated
	};

	// collections
	typedef std::map <gpstring, VariableSpec, istring_less> VariableMap;
	typedef std::pair <VariableMap::iterator, bool> VariableMapInsertRet;
	typedef std::pair <const ClassSpec*, int> ParentSpec;
	typedef stdx::fast_vector <ParentSpec> ParentColl;

	// procedures
	typedef bool  (__cdecl *DoesHandleMgrExistProc     )( void  );
	typedef UINT  (__cdecl *HandleAddRefProc           )( DWORD );
	typedef UINT  (__cdecl *HandleReleaseProc          )( DWORD );
	typedef bool  (__cdecl *HandleIsValidProc          )( DWORD );
	typedef void* (__cdecl *HandleGetProc              )( DWORD );
	typedef void* (__cdecl *GetClassSingletonProc      )( void  );
	typedef DWORD (__cdecl *InstanceToNetProc          )( void* );
	typedef void* (__cdecl *NetToInstanceProc          )( DWORD, FuBiCookie* );
	typedef void  (__cdecl *GetHeaderSpecProc          )( ClassHeaderSpec& );

	// spec
	gpstring               m_Name;						// class name
	eVarType               m_Type;						// VAR_USER-based index
	eFlags                 m_Flags;						// flags for this class type
	FunctionSpec::eFlags   m_Membership;				// membership flags
	ClassHeaderSpec*       m_HeaderSpec;				// table specification for schema
	VarTypeSpec*           m_VarTypeSpec;				// traits about this class for string conversion etc.
	ParentColl             m_ParentClasses;				// who are our parent classes?

#	if !GP_RETAIL
	const char*            m_Docs;						// doco for entire class
#	endif // !GP_RETAIL

	// members
	FunctionByNameIndex    m_Functions;					// static/nonstatic methods
	VariableMap            m_Variables;					// set/get pairs

	// callbacks
	DoesHandleMgrExistProc m_DoesHandleMgrExistProc;	// check for existence of handlemgr
	HandleAddRefProc       m_HandleAddRefProc;			// add ref to a handle
	HandleReleaseProc      m_HandleReleaseProc;			// remove ref from a handle
	HandleIsValidProc      m_HandleIsValidProc;			// check handle for validity
	HandleGetProc          m_HandleGetProc;				// dereference a handle into an object pointer
	GetClassSingletonProc  m_GetClassSingletonProc;		// get a pointer to the class singleton instance
	InstanceToNetProc      m_InstanceToNetProc;			// convert a pointer to a network cookie
	NetToInstanceProc      m_NetToInstanceProc;			// convert a network cookie to a pointer (NULL on failure)

	ClassSpec( void );
	ClassSpec( const ClassSpec& other );
   ~ClassSpec( void );

	ClassSpec& operator = ( const ClassSpec& other );

	bool         IsDerivedFrom       ( eVarType type ) const;
	int          GetDerivedBaseOffset( eVarType base ) const;
	void         SetPod              ( size_t size );

	bool  AddMemberFunction( FunctionSpec* function );
	void  PostProcess      ( void );
	DWORD AddExtraDigest   ( DWORD digest ) const;

#	if !GP_RETAIL
	void AssertValid ( void ) const;
	void GenerateDocs( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL

private:
	// special: will auto-create spec but only on internal class request
	VarTypeSpec* GetVarTypeSpec( void ) const;
};

MAKE_ENUM_BIT_OPERATORS( ClassSpec::eFlags );

//////////////////////////////////////////////////////////////////////////////
// struct EnumSpec declaration

struct EnumSpec
{
	// spec
	EnumExporter m_Exporter;					// exported spec
	gpstring     m_Name;						// enum name
	eVarType     m_Type;						// VAR_ENUM-based index
	VarTypeSpec  m_VarTypeSpec;					// traits about this enum for string conversion etc.

	EnumSpec( void );

	void PostProcess( void );

#	if !GP_RETAIL
	void GenerateDocs( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBITYPES_H

//////////////////////////////////////////////////////////////////////////////
