//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBi.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains FuBi (function binding system) system API's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBI_H
#define __FUBI_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "FuBiTypes.h"
#include "KernelTool.h"

#include <map>
#include <set>
#include <list>

namespace FileSys
{
	class StreamWriter;
	class Reader;
}

class FuelHandle;

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// utility function declarations

// Blind call redirectors.

	// these will blind-call the function passing in the argument block

	// $ From MS docs:
	//
	// If you are writing assembly routines for the floating point coprocessor,
	// you must preserve the floating point control word and clean the
	// coprocessor stack unless you are returning a float or double value
	// (which your function should return in ST(0)).

	DWORD BlindCallFunction_CdeclCall     ( const void* args, size_t sz, DWORD func );
	DWORD BlindCallFunction_StdCall       ( const void* args, size_t sz, DWORD func );
	DWORD BlindCallFunction_ThisCall      ( const void* args, size_t sz, void* object, DWORD func );
	DWORD BlindCallFunction_ThisCallVarArg( const void* args, size_t sz, void* object, DWORD func );

// Misc utility.

	// loop through all these functions and doco them + check params
	void PostProcess( FunctionByNameIndex& functions );

	// add a new function to the index - returns false on a critical failure
	bool AddFunctionToIndex( FunctionByNameIndex& index, FunctionSpec* spec );

// Export table.

#	pragma pack ( push, 1 )

	const DWORD FUBI_SIGNATURE = '!(*)';

	struct ExportTableHeader
	{
		DWORD m_Signature;			// should be 'FuBi'
		DWORD m_CompressedSize;		// size of compressed data
		DWORD m_UncompressedSize;	// size of uncompressed data
	//  ... data follows
	};

	void Encrypt( void* data, size_t length );
	void Decrypt( void* data, size_t length );

#	pragma pack ( pop )

// Export manifest.

	struct ModuleEntry
	{
		gpstring m_Name;
		DWORD    m_HeaderCrc;
		DWORD    m_ActualCrc;

		ModuleEntry( const gpstring& name, DWORD headerCrc, DWORD actualCrc )
			: m_Name( name )
		{
			m_HeaderCrc = headerCrc;
			m_ActualCrc = actualCrc;
		}

		ModuleEntry( void )
		{
			m_HeaderCrc = m_ActualCrc = 0;
		}

		bool Xfer( PersistContext& persist );
	};

	typedef stdx::fast_vector <ModuleEntry> ModuleColl;

	struct Manifest
	{
		DWORD      m_Digest;
		ModuleColl m_Modules;

		void SaveToFuel    ( const char* key, FuelHandle fuel );
		void LoadFromSystem( void );
		bool LoadFromFuel  ( const char* key, FuelHandle fuel );
		bool Xfer          ( const char* key, PersistContext& persist );
	};

//////////////////////////////////////////////////////////////////////////////
// class SysExports declaration

class SignatureParser;

// purpose: this class builds and collects specifications for all user-defined
//          functions and types in the game. by "user-defined" i mean "some
//          random engineer's code".

class SysExports : public Singleton <SysExports>
{
public:
	SET_NO_INHERITED( SysExports );

// Types.

	enum  {  VERSION = MAKEVERSION( 1, 0, 0 )  };	// update this as sysexports changes seriously

	enum eOptions
	{
		// bind-time
		OPTION_NO_EXTRA_DOC_WARNING     = 1 << 0,	// disable the "found doco for nonexistent export" warning
		OPTION_NO_REQUIRE_SYNC          = 1 << 1,	// allow rpc's to be used without requiring a sync call
		OPTION_NO_CHECK_ALL_RPC_NESTING = 1 << 2,	// don't check for all-all nesting in sent rpc's
		OPTION_NO_STRICT                = 1 << 3,	// relax certain super-strict warnings
		OPTION_BUFFER_RPCS              = 1 << 4,	// redirect and buffer incoming rpc's as they come in

		// ...

		OPTION_NONE = 0,
	};

	struct SyncSpec
	{
		DWORD m_Size;								// size of this structure
		DWORD m_Version;
		DWORD m_Digest;
		DWORD m_ModuleCount;						// # modules imported into fubi

		SyncSpec( void )
		{
			::ZeroObject( *this );
			m_Size = sizeof( *this );
		}

		SyncSpec( DWORD digest, DWORD moduleCount )
		{
			m_Size        = sizeof( *this );
			m_Version     = VERSION;
			m_Digest      = digest;
			m_ModuleCount = moduleCount;
		}

		bool Write( FileSys::StreamWriter& writer ) const;
		bool Read ( FileSys::Reader      & reader );

		bool operator == ( const SyncSpec& other ) const
		{
			return ( ::memcmp( this, &other, sizeof( *this ) ) == 0 );
		}

		bool operator != ( const SyncSpec& other ) const
		{
			return ( !(*this == other) );
		}
	};

	typedef std::list <gpstring> StringList;
	typedef std::list <gpwstring> WStringList;

// Setup.

	// ctor/dtor
	SysExports( void );
   ~SysExports( void );

	// local options
	void SetOptions   ( eOptions options, bool set = true )	{  m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )					{  SetOptions( options, false );  }
	void ToggleOptions( eOptions options )					{  m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const			{  return ( (m_Options & options) != 0 );  }

	// synchronizing for rpc's
	bool Synchronize( const SyncSpec& spec );
	void SetSynchronize( bool set = true )					{  m_RpcSyncComplete = set;  }
	void ClearSynchronize( void )							{  SetSynchronize( false );  }

	// spec building
	SyncSpec MakeSynchronizeSpec( void ) const;

	// buffering for rpc's
	void EnableRpcBuffering    ( void );
	void DisableRpcBuffering   ( bool flushPending = true );
	void FlushRpcBuffers       ( bool deleteOnly = false );
	void AddRpcBufferExceptions( const char* funcName );

	// callback registration
	void RegisterRebroadcaster( IRebroadcaster* cb )		{  m_Rebroadcaster = cb;  }

	// exception list for nested rpc warning
#	if GP_DEBUG
	void RegisterNestedRpcException( const char* caller, const char* called );
#	endif // GP_DEBUG

// Importing.

	struct ExportEntry
	{
		gpstring m_MangledName;
		gpstring m_UnmangledName;
		DWORD    m_Address;

		ExportEntry( void )
			{  }
		ExportEntry( const gpstring& mangledName, const gpstring& unmangledName, DWORD address )
			: m_MangledName  ( mangledName )
			, m_UnmangledName( unmangledName )
			, m_Address      ( address )  {  }
	};

	typedef std::list <ExportEntry> ExportColl;

	bool ImportFunction( const char* mangledName, const char* unmangledName, DWORD address, SignatureParser* parser );
	bool ImportBindings( HMODULE module = NULL, ExportColl* dest = NULL, bool finish = true );
	bool ImportModules ( const char* modulePathSpec = "*.dll", ExportColl* dest = NULL, bool finish = true );
	bool ImportFinish  ( void );

// Type info.

	const FunctionSpec*  FindFunctionByAddrExact     ( DWORD ptr, const char* funcName, int funcNameLen ) const;
	const FunctionSpec*  FindFunctionByAddrNear      ( DWORD ptr, const char* funcName, int funcNameLen ) const;
	const FunctionSpec*  FindFunctionBySerialID      ( UINT serialID ) const;
	const FunctionIndex* FindFunctionsByQualifiedName( const char* funcName ) const;
	int                  FindFunctionsByQualifiedName( FunctionIndexColl& coll, const char* funcName ) const;
	UINT                 FindEventSerialByName       ( const char* eventName ) const;

	int GetFunctionSize( const FunctionSpec* spec );
	int GetFunctionCount( void ) const  {  return ( scast <int> ( m_FunctionsBySerialID.size() ) );  }

	const char* FindTypeName      ( eVarType type ) const;
	const char* FindNiceTypeName  ( const FuBi::TypeSpec& spec ) const;
	gpstring    MakeFullTypeName  ( const FuBi::TypeSpec& spec ) const;
	eVarType    FindType          ( const char* name ) const;
	eVarType    FindOrCreateType  ( const char* name );
	eVarType    FindTypeInternalOk( const char* name ) const;

	ClassSpec*       GetClass ( const gpstring& name );
	const ClassSpec* FindClass( eVarType type ) const;
	ClassSpec*       FindClass( eVarType type );
	const ClassSpec* FindClass( const char* name ) const;

	const ClassSpec* FindEventNamespace( void ) const;
	const ClassSpec* FindSkritObject   ( void ) const;
	const ClassSpec* FindSkritVm       ( void ) const;
	const ClassSpec* FindSiegePosData  ( void ) const;
	const ClassSpec* FindSiegePos      ( void ) const;
	const ClassSpec* FindQuat          ( void ) const;
	const ClassSpec* FindVector3       ( void ) const;
	const ClassSpec* FindFuBiCookie    ( void ) const;

	EnumSpec*       GetEnum         ( const gpstring& name );
	const EnumSpec* FindEnum        ( eVarType type ) const;
	const EnumSpec* FindEnumConstant( const char* name, DWORD& value, bool fastOnly = false ) const;

	const VarTypeSpec* GetVarTypeSpec     ( eVarType type ) const;
	int                GetVarTypeSizeBytes( eVarType type ) const;
	bool               IsSimpleInteger    ( eVarType type ) const;
	bool               IsSimpleFloat      ( eVarType type ) const;
	bool               IsPod              ( eVarType type ) const;

	bool IsKeyword           ( const gpstring& name ) const;
	bool IsDerivative        ( eVarType derived, eVarType base ) const;
	int  GetDerivedBaseOffset( eVarType derived, eVarType base ) const;

	bool ToString  ( eVarType type, gpstring& out, const void* obj, eXfer xfer = XFER_NORMAL, eXMode xmode = XMODE_DEFAULT ) const;
	bool ToString  ( const TypeSpec& type, gpstring& out, const void* obj, eXfer xfer = XFER_NORMAL, eXMode xmode = XMODE_DEFAULT ) const;
	bool FromString( eVarType type, const char* str, void* obj, eXMode xmode = XMODE_DEFAULT ) const;
	bool FromString( const TypeSpec& type, const char* str, void* obj, eXMode xmode = XMODE_DEFAULT ) const;
	bool CopyVar   ( eVarType type, void* dst, const void* src, eXMode xmode = XMODE_DEFAULT ) const;
	bool CopyVar   ( const TypeSpec& type, void* dst, const void* src, eXMode xmode = XMODE_DEFAULT ) const;
	int  CompareVar( eVarType type, const void* l, const void* r ) const;
	int  CompareVar( const TypeSpec& type, const void* l, const void* r ) const;

// Iterators.

	ClassIndex::const_iterator GetClassBegin( void ) const  {  return ( m_ClassVarTypeIndex.begin() );  }
	ClassIndex::const_iterator GetClassEnd  ( void ) const  {  return ( m_ClassVarTypeIndex.end() );  }

	EnumIndex::const_iterator GetEnumBegin( void ) const  {  return ( m_EnumVarTypeIndex.begin() );  }
	EnumIndex::const_iterator GetEnumEnd  ( void ) const  {  return ( m_EnumVarTypeIndex.end() );  }

	FunctionByNameIndex::const_iterator GetGlobalsBegin( void ) const  {  return ( m_GlobalFunctions.begin() );  }
	FunctionByNameIndex::const_iterator GetGlobalsEnd  ( void ) const  {  return ( m_GlobalFunctions.end() );  }

// Global functions.

	inline const FunctionIndex* FindGlobalFunction( const char* name ) const;

// Remote procedure call methods.

	const FunctionSpec* ResolveRpc             ( DWORD EIP, const char* funcName, int funcNameLen ) const;								// used by FUBI_RPC_CALL macro
	Cookie              SendRpc                ( const FunctionSpec* spec, void* thisParam, const void* firstParam, DWORD toAddr );	// outgoing RPC request (thread-safe)
	static bool         IsSending              ( void )  {  return ( ms_IsSending );  }
	static int          GetCurrentRpcPacketSize( bool take = false )  {  int size = ms_CurrentRpcPacketSize;  if ( take )  {  ms_CurrentRpcPacketSize = 0;  }  return ( size );  }
	Cookie              DispatchNextRpc        ( DWORD callerAddr, DWORD packetSize );													// incoming RPC request (not thread-safe - already serialized by transport)
	bool                RpcTestMacro           ( DWORD addr, const FunctionSpec* spec );												// "should i rpc?" test function intended to be called by various FUBI_RPC_ macros
	static bool         IsDispatching          ( void )  {  return ( ms_IsDispatching );  }
	bool                ClearDispatcher        ( void );
	void                EnterRpc               ( const bool* oldDispatching = NULL );
	void                LeaveRpc               ( void );
	void                PushNestDetectCancel   ( void );
	void                PopNestDetectCancel    ( void );

// Callback methods.

	const FunctionSpec* ResolveEvent( DWORD EIP, const char* funcName, int funcNameLen ) const;		// used by SKRIT_EVENT macro
	const FunctionSpec* ResolveCall ( DWORD EIP, const char* funcName, int funcNameLen ) const;		// used by SKRIT_CALL macro

// Misc.

	DWORD             GetDigest    ( void ) const  {  gpassert( m_ImportComplete );  return ( m_Digest );  }
	const ModuleColl& GetModuleColl( void ) const  {  gpassert( m_ImportComplete );  return ( m_Modules );  }

#	if !GP_RETAIL
	void GenerateDocs( ReportSys::ContextRef context = NULL ) const;
#	endif // !GP_RETAIL

// Debugging.

#	if !GP_RETAIL
	void DumpStatistics( ReportSys::ContextRef ctx ) const;
	void DumpStatistics( void ) const  {  DumpStatistics( NULL );  }
#	endif // !GP_RETAIL

#	if GP_DEBUG
	void CheckSanity( void ) const;
#	endif // GP_DEBUG

private:

// Maps.

	// these are containers that own their contents

	// $ note: lesson learned after hours of debugging - the release linker will
	//   fold identical functions together under the same entry point, while
	//   giving each a separate export entry. this means that many functions
	//   will share the same address! so the FunctionByAddrMap must be multimap.

	typedef std::map          <gpstring, ClassSpec*, istring_less>      ClassByNameMap;
	typedef std::map          <const char*, EnumSpec*, istring_less>    EnumByNameMap;
	typedef std::pair         <eVarType, DWORD>                         EnumConstant;
	typedef std::map          <const char*, EnumConstant, istring_less> StringToEnumMap;
	typedef std::multimap     <DWORD, FunctionSpec>                     FunctionByAddrMap;
	typedef std::pair         <ClassByNameMap::iterator, bool>          ClassByNameMapInsertRet;
	typedef std::pair         <EnumByNameMap::iterator, bool>           EnumByNameMapInsertRet;
	typedef std::set          <gpstring, istring_less>                  StringSet;
	typedef stdx::fast_vector <VarTypeSpec>                             VarTypeSpecColl;

#	if GP_DEBUG
	typedef std::pair         <UINT /*caller*/, UINT /*called*/>        SerialIdPair;
	typedef stdx::fast_vector <SerialIdPair>                            SerialIdPairColl;
	typedef stdx::fast_vector <UINT>                                    SerialIdColl;
#	endif // GP_DEBUG

	const FunctionSpec* FindFunctionByAddrHelper( const char* funcName, int funcNameLen, FunctionByAddrMap::const_iterator found ) const;
	RpcVerifyColl&      GetRpcVerifyColl        ( void );

	ClassByNameMap    m_Classes;					// all classes
	EnumByNameMap     m_Enums;						// all enums
	FunctionByAddrMap m_Functions;					// all functions

// Indexes.

	// these are containers that point to elements of maps

	ClassIndex          m_ClassVarTypeIndex;		// map eVarType - VAR_USER --> const ClassSpec*
	EnumIndex           m_EnumVarTypeIndex;			// map eVarType - VAR_ENUM --> const EnumSpec*
	EnumIndex           m_IrregularEnums;			// just a set of our irregular enums
	StringToEnumMap     m_EnumConstants;			// map const char* (enum string) --> DWORD (enum int)
	FunctionByNameIndex m_GlobalFunctions;			// all global functions - note it's a map of vectors to support overloads
	FunctionIndex       m_FunctionsBySerialID;		// index == serial ID (assigned at import time)
	VarTypeSpecColl     m_VarTypeSpecs;				// built-in type specs

#	if GP_DEBUG
	SerialIdPairColl    m_NestedRpcExceptionColl;	// exceptions to the nested rpc coll
	SerialIdColl        m_NestedRpcXCallers;		// these rpc callers can match exception on any called
	SerialIdColl        m_NestedRpcXCalleds;		// these rpc calleds can match exception on any caller
#	endif // GP_DEBUG

// RPC support.

	typedef kerneltool::Critical Critical;
	typedef stdx::fast_vector <std::pair <DWORD, RpcVerifyColl> > RpcColl;
	typedef stdx::fast_vector <BYTE> Buffer;
	typedef stdx::linear_set <UINT> IdColl;

	struct Packet
	{
		DWORD  m_CallerAddr;						// note that the params here are a copied owned buffer that must be deleted!
		Buffer m_Data;								// rpc data with packet
	};

	typedef stdx::fast_vector <Packet> PacketColl;

	mutable Critical    m_SendCritical;				// used for serializing RPC sends
	StringList          m_RpcStrings;				// used for storing gpstrings pulled over net
	WStringList         m_WRpcStrings;				// used for storing gpwstrings pulled over net
	Buffer              m_ParamBuffer;				// sometimes we use this for param storage with packed front/back params
	RpcColl             m_RpcColl;					// per-thread map of rpc call stack
	IRebroadcaster*     m_Rebroadcaster;			// rebroadcasting callback for rpc work
	PacketColl          m_BufferedRpcs;				// all buffered rpc's so far
	IdColl              m_BufferedExceptIds;		// when buffering, skip these id's from the mix

// Other.

	eOptions   m_Options;							// general options for this object
	StringSet  m_Keywords;							// keywords exported from system
	DWORD      m_Digest;							// this is a digest of all of FuBi's exports
	ModuleColl m_Modules;							// this is all the modules we've imported
	bool       m_RenameComplete;					// renaming is complete
	bool       m_ImportComplete;					// import is complete
	bool       m_RpcSyncComplete;					// rpc synching has been done

	DECL_THREAD static bool  ms_IsSending;
	DECL_THREAD static bool  ms_IsDispatching;
	DECL_THREAD static int   ms_CurrentRpcPacketSize;

	SET_NO_COPYING( SysExports );
};

MAKE_ENUM_BIT_OPERATORS( SysExports::eOptions );

#define gFuBiSysExports FuBi::SysExports::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class SysExports inline implementation

inline const ClassSpec* SysExports :: FindClass( eVarType type ) const
{
	type -= VAR_USER;
	gpassert( (type >= 0) && (type < scast <int> ( m_ClassVarTypeIndex.size() )) );
	return ( m_ClassVarTypeIndex[ type ] );
}

inline ClassSpec* SysExports :: FindClass( eVarType type )
{
	type -= VAR_USER;
	gpassert( (type >= 0) && (type < scast <int> ( m_ClassVarTypeIndex.size() )) );
	return ( m_ClassVarTypeIndex[ type ] );
}

inline const ClassSpec* SysExports :: FindClass( const char* name ) const
{
	eVarType type = FindType( name );
	return ( (type == VAR_UNKNOWN) ? NULL : FindClass( type ) );
}

inline const ClassSpec* SysExports :: FindEventNamespace( void ) const
{
	return ( FindClass( EVENT_NAMESPACE ) );
}

inline const ClassSpec* SysExports :: FindSkritObject( void ) const
{
	return ( FindClass( TYPE_SKRITOBJECT ) );
}

inline const ClassSpec* SysExports :: FindSkritVm( void ) const
{
	return ( FindClass( TYPE_SKRITVM ) );
}

inline const ClassSpec* SysExports :: FindSiegePosData( void ) const
{
	return ( FindClass( TYPE_SIEGEPOSDATA ) );
}

inline const ClassSpec* SysExports :: FindSiegePos( void ) const
{
	return ( FindClass( TYPE_SIEGEPOS ) );
}

inline const ClassSpec* SysExports :: FindQuat( void ) const
{
	return ( FindClass( TYPE_QUAT ) );
}

inline const ClassSpec* SysExports :: FindVector3( void ) const
{
	return ( FindClass( TYPE_VECTOR3 ) );
}

inline const ClassSpec* SysExports :: FindFuBiCookie( void ) const
{
	return ( FindClass( TYPE_FUBICOOKIE ) );
}

inline const EnumSpec* SysExports :: FindEnum( eVarType type ) const
{
	type -= VAR_ENUM;
	gpassert( (type >= 0) && (type < scast <int> ( m_EnumVarTypeIndex.size() )) );
	return ( m_EnumVarTypeIndex[ type ] );
}

inline const FunctionSpec* SysExports :: FindFunctionByAddrExact( DWORD ptr, const char* funcName, int funcNameLen ) const
{
	gpassert( (funcName != NULL) && (::strlen( funcName ) == (size_t)funcNameLen) );

	// $ find a function by its pointer (exact match) - pointer is a real address

	// find first function
	FunctionByAddrMap::const_iterator found = m_Functions.find( ptr );
	if ( found != m_Functions.end() )
	{
		return ( FindFunctionByAddrHelper( funcName, funcNameLen, found ) );
	}
	else
	{
		return ( NULL );
	}
}

inline const FunctionSpec* SysExports :: FindFunctionBySerialID( UINT serialID ) const
{
	// find a function by its local serial ID assigned at startup time

	if ( serialID < m_FunctionsBySerialID.size() )
	{
		gpassert( m_FunctionsBySerialID[ serialID ]->m_SerialID == serialID );
		return ( m_FunctionsBySerialID[ serialID ] );
	}
	else
	{
		return ( NULL );
	}
}

inline const FunctionIndex* SysExports :: FindGlobalFunction( const char* name ) const
{
	const FunctionIndex* spec = NULL;

	FunctionByNameIndex::const_iterator found = m_GlobalFunctions.find( name );
	if ( found != m_GlobalFunctions.end() )
	{
		spec = &found->second;
	}

	return ( spec );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBI_H

//////////////////////////////////////////////////////////////////////////////
