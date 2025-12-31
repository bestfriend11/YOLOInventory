//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoBase.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains base classes and helper components for Go's.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOBASE_H
#define __GOBASE_H

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"

//////////////////////////////////////////////////////////////////////////////
// class GobVariable <T> declaration

// $ this is a "game object buffered variable" class. maintains the separation
//   between the natural and modified versions of statistics.

enum eGobType
{
	GOB_INT,
	GOB_FLOAT,
	GOB_BOOL,
};

template <typename T, const eGobType GOB>
class GobVariable
{
public:
	typedef GobVariable <T, GOB> ThisType;

// Setup.

	GobVariable( void )  {  }
	GobVariable( const T& data )							: m_Natural( data ), m_Modified( data )  {  }
	GobVariable( const T& natural, const T& modified )		: m_Natural( natural ), m_Modified( modified )  {  }

	void Reset( void )										{  m_Modified = m_Natural;  }

// Query.

	const T& GetNatural ( void ) const						{  return ( m_Natural );  }
	T&       GetNatural ( void )							{  return ( m_Natural );  }
	const T& GetModified( void ) const						{  return ( m_Modified );  }
	T&       GetModified( void )							{  return ( m_Modified );  }
	bool     IsModified ( void ) const						{  return ( m_Natural != m_Modified );  }

// Mutation.

	void SetNatural ( const T& data )						{  m_Natural = data;  }
	void SetModified( const T& data )						{  m_Modified = data;  }

// Operators.

	operator const T& ( void ) const						{  return ( m_Modified );  }

	static eGobType GetGobType( void )						{  return ( GOB );  }

private:
	T m_Natural;
	T m_Modified;
};

template <typename T, const eGobType GOB, typename U>
bool operator == ( const GobVariable <T, GOB> gob, const U& data )  {  return ( gob.GetModified() == data );  }

template <typename T, const eGobType GOB, typename U>
bool operator != ( const GobVariable <T, GOB> gob, const U& data )  {  return ( gob.GetModified() != data );  }

// $ specialization for ints

class GobInt : public GobVariable <int, GOB_INT>
{
public:
	typedef GobVariable <int, GOB_INT> Inherited;
	typedef int Type;

	GobInt( void )											: Inherited( 0 )  {  }
	GobInt( int data )										: Inherited( data )  {  }
	GobInt( int natural, int modified )						: Inherited( natural, modified )  {  }

	GobInt& operator = ( int data )							{  SetModified( data );  return ( *this );  }
};

// $ specialization for floats

class GobFloat : public GobVariable <float, GOB_FLOAT>
{
public:
	typedef GobVariable <float, GOB_FLOAT> Inherited;
	typedef float Type;

	GobFloat( void )										: Inherited( 0 )  {  }
	GobFloat( float data )									: Inherited( data )  {  }
	GobFloat( float natural, float modified )				: Inherited( natural, modified )  {  }

	GobFloat& operator = ( float data )						{  SetModified( data );  return ( *this );  }
};

// $ specialization for bools

class GobBool : public GobVariable <bool, GOB_BOOL>
{
public:
	typedef GobVariable <bool, GOB_BOOL> Inherited;
	typedef bool Type;

	GobBool( void )											: Inherited( 0 )  {  }
	GobBool( bool data )									: Inherited( data )  {  }
	GobBool( bool natural, bool modified )					: Inherited( natural, modified )  {  }

	GobBool& operator = ( bool data )						{  SetModified( data );  return ( *this );  }
};

//////////////////////////////////////////////////////////////////////////////
// class GoString declaration

// $ for performance and memory efficiency, the GoString always points to
//   a string in the content db. this is a special class really only meant for
//   usage as a member within a go component.

class GoString
{
public:
	GoString( void )								{  m_String = &gpstring::EMPTY;  }
	GoString( const GoString& str )					{  m_String = str.m_String;  }
	GoString( const gpstring& str )					{  m_String = &gpstring::EMPTY;  Assign( str );  }
	GoString( const char* str )						{  m_String = &gpstring::EMPTY;  Assign( str );  }

	GoString& Assign( const GoString& str )			{  m_String = str.m_String;  return ( *this );  }
	GoString& Assign( const gpstring& str );
	GoString& Assign( const char* str );

	const gpstring& GetString( void ) const			{  return ( *m_String );  }

	bool        empty ( void ) const				{  return ( m_String->empty() );  }
	size_t      length( void ) const				{  return ( m_String->length() );  }
	const char* c_str ( void ) const				{  return ( *m_String );  }

	GoString& operator = ( const GoString& str )	{  return ( Assign( str ) );  }
	GoString& operator = ( const gpstring& str )	{  return ( Assign( str ) );  }
	GoString& operator = ( const char* str )		{  return ( Assign( str ) );  }

	operator const gpstring& ( void ) const			{  return ( GetString() );  }

private:
	const gpstring* m_String;
};

// define traits for GoString type
template <> struct StringTraits <GoString> : StringManagedTag <char>  {  };

//////////////////////////////////////////////////////////////////////////////
// class GowString declaration

// $ for performance and memory efficiency, the GowString always points to
//   a string in the content db. this is a special class really only meant for
//   usage as a member within a go component.

class GowString
{
public:
	GowString( void )								{  m_String = &gpwstring::EMPTY;  }
	GowString( const GowString& str )				{  m_String = str.m_String;  }
	GowString( const gpwstring& str )				{  m_String = &gpwstring::EMPTY;  Assign( str );  }
	GowString( const wchar_t* str )					{  m_String = &gpwstring::EMPTY;  Assign( str );  }

	GowString& Assign( const GowString& str )		{  m_String = str.m_String;  return ( *this );  }
	GowString& Assign( const gpwstring& str );
	GowString& Assign( const wchar_t* str );

	const gpwstring& GetString( void ) const		{  return ( *m_String );  }

	bool           empty ( void ) const				{  return ( m_String->empty() );  }
	size_t         length( void ) const				{  return ( m_String->length() );  }
	const wchar_t* c_str ( void ) const				{  return ( *m_String );  }

	GowString& operator = ( const GowString& str )	{  return ( Assign( str ) );  }
	GowString& operator = ( const gpwstring& str )	{  return ( Assign( str ) );  }
	GowString& operator = ( const wchar_t* str )	{  return ( Assign( str ) );  }

	operator const gpwstring& ( void ) const		{  return ( GetString() );  }

private:
	const gpwstring* m_String;
};

// define traits for GowString type
template <> struct StringTraits <GowString> : StringManagedTag <wchar_t>  {  };

//////////////////////////////////////////////////////////////////////////////
// helpers

// GoComponent factory.

	// $ important: server-only components are always created on both the
	//   client and the server, but the server is the one that gets the update
	//   events.

	// $ quite a bit of hacking went into getting this just right. don't
	//   rearrange this code or you'll resurrect comdat collision errors from
	//   the linker (apparently due to long public symbol name lengths).

#	define DECLARE_GO_COMPONENT_IMPL( T, NAME, SERVER_ONLY, CAN_SERVER_ONLY, DEV_ONLY )	\
		struct GocExporter_##T : GocExporter, Singleton <GocExporter_##T>		\
		{																		\
	    	GocExporter_##T( void )												\
				: GocExporter( NAME, #T,										\
							   makeFunctor( Factory <T>::Proc ),				\
							   SERVER_ONLY,										\
							   CAN_SERVER_ONLY,									\
							   DEV_ONLY )  {  }									\
	    };																		\
		template struct ::GlobalStatic <GocExporter_##T>;						\
		inline int T :: GetCacheIndex( void )									\
		{																		\
			return ( Singleton <GocExporter_##T>::GetSingleton().m_CacheIndex );\
		}

#	define DECLARE_GO_COMPONENT( T, NAME )							DECLARE_GO_COMPONENT_IMPL( T, NAME, false, false, false )
#	define DECLARE_DEV_GO_COMPONENT( T, NAME )						DECLARE_GO_COMPONENT_IMPL( T, NAME, false, false, true  )
#	define DECLARE_GO_SERVER_ONLY_COMPONENT( T, NAME )				DECLARE_GO_COMPONENT_IMPL( T, NAME, false, true , false )
#	define DECLARE_DEV_GO_SERVER_ONLY_COMPONENT( T, NAME )			DECLARE_GO_COMPONENT_IMPL( T, NAME, false, true , true  )
#	define DECLARE_GO_SERVER_COMPONENT( T, NAME )					DECLARE_GO_COMPONENT_IMPL( T, NAME, true , false, false )
#	define DECLARE_DEV_GO_SERVER_COMPONENT( T, NAME )				DECLARE_GO_COMPONENT_IMPL( T, NAME, true , false, true  )
#	define DECLARE_GO_SERVER_ONLY_SERVER_COMPONENT( T, NAME )		DECLARE_GO_COMPONENT_IMPL( T, NAME, true , true , false )
#	define DECLARE_DEV_GO_SERVER_ONLY_SERVER_COMPONENT( T, NAME )	DECLARE_GO_COMPONENT_IMPL( T, NAME, true , true , true  )

	// functor type
	typedef CBFunctor2wRet
			<
				// in
				const char*,	// component type name
				Go*,			// owning Go of component

				// out
				GoComponent*	// new'd component
			>
			GocFactoryCb;

	// the exporter
	struct GocExporter : GlobalRegistrar <GocExporter>
	{
		const char*  m_Name;
		const char*  m_InternalName;
		GocFactoryCb m_FactoryCb;
		bool         m_ServerOnly;
		bool         m_CanServerExistOnly;
		bool         m_DevOnly;
		int          m_CacheIndex;

		GocExporter( const char* name, const char* internalName, GocFactoryCb cb, bool serverOnly = false, bool canServerExistOnly = false, bool devOnly = false );

		template <typename T>
		struct Factory
		{
			static GoComponent* Proc( const char* /*name*/, Go* parent )
				{  return ( new T( parent ) );  }
		};
	};

// New Go construction requests.

	// $ note that these are packed for benefit of rpc's

#	pragma pack ( push, 1 )

	struct GoCreateReq
	{
		FUBI_POD_CLASS( GoCreateReq );

		// $ note: a player id of PLAYERID_INVALID will default to the parent
		//         go's player. it will assert if invalid and no parent.

		Scid     m_Scid;
		RegionId m_RegionId;
		PlayerId m_PlayerId;
		int      m_MpPlayerCount;
		SiegePos m_StartingPos;
		Quat     m_StartingOrient;
		bool     m_UseStartingPos    : 1;
		bool     m_UseStartingOrient : 1;
		bool     m_SnapToTerrain     : 1;
		bool     m_LocalGo           : 1;
		bool     m_LodfiGo           : 1;
		bool     m_FadeIn            : 1;
		bool     m_ForcePosition     : 1;
		bool     m_PrepareToDrawNow  : 1;
		bool     m_NoStartupFx       : 1;

		GoCreateReq( void )  {  Init();  }
		GoCreateReq( Scid scid, RegionId regionId, PlayerId playerId = PLAYERID_INVALID );

		void Init            ( void );
		void CalcPosition    ( void );
		int  GetMpPlayerCount( void ) const;
		void Finish          ( Go* go ) const;

	FEX void SetStartingPos( const SiegePos& siegePos );
	FEX void SetStartingOrient( const Quat& quat )
			{  m_StartingOrient = quat;  m_UseStartingOrient = true;  }
	FEX void SetSnapToTerrain( bool snap = true )
			{  m_SnapToTerrain = snap;  }
	FEX void SetFadeIn( bool set = true )
			{  m_FadeIn = set;  }
	FEX void SetForcePosition( bool set = true )
			{  m_ForcePosition = set;  }
	FEX void SetPrepareToDrawNow( bool set = true )
			{  m_PrepareToDrawNow = set;  }
	FEX void SetNoStartupFx( bool set = true )
			{  m_NoStartupFx = set;  }

		void Xfer( FuBi::BitPacker& packer, GoCreateReq& def );

#		if !GP_RETAIL
		void     RpcToString( gpstring& appendHere ) const;
		gpstring RpcToString( void ) const;
#		endif // !GP_RETAIL

		GPDEBUG_ONLY( bool AssertValid( void ) const; )
	};

#	pragma pack ( pop )

	struct CreateGoXfer
	{
		GoCreateReq   m_CreateReq;
		Goid          m_StartGoid;
		DWORD         m_RandomSeed;
		bool          m_Standalone;
		const_mem_ptr m_Data;
		Buffer        m_DataBuffer;

		CreateGoXfer( void )
		{
			m_StartGoid  = GOID_INVALID;
			m_RandomSeed = 0;
			m_Standalone = false;
		}

		void CopyForDefault( const CreateGoXfer& src )
		{
			// only copy stuff we use
			m_CreateReq  = src.m_CreateReq;
		}

		void Xfer( FuBi::BitPacker& packer, CreateGoXfer& def );

#		if !GP_RETAIL
		void RpcToString( gpstring& appendHere ) const;
#		endif // !GP_RETAIL
	};

#	pragma pack ( push, 1 )

	struct GoCloneReq
	{
		FUBI_POD_CLASS( GoCloneReq );

		// $ note: a player id of PLAYERID_INVALID will default to the source
		//         go's player if no parent, otherwise it will be the parent
		//         go's player.

		Goid     m_Parent;
		Goid     m_CloneSource;
		PlayerId m_PlayerId;
		int      m_MpPlayerCount;
		SiegePos m_StartingPos;
		Quat     m_StartingOrient;
		bool     m_UseStartingPos     : 1;
		bool     m_UseStartingOrient  : 1;
		bool     m_SnapToTerrain      : 1;
		bool     m_OmniGo             : 1;
		bool     m_AllClients         : 1;
		bool     m_FadeIn             : 1;
		bool     m_ForcePosition	  : 1;
		bool     m_ForceServerOnly    : 1;
		bool     m_ForceClientAllowed : 1;
		bool     m_PrepareToDrawNow   : 1;
		bool     m_NoStartupFx        : 1;
		bool     m_XferCharacterPost  : 1;
		bool     m_PrepareAmmo        : 1;

		GoCloneReq( void )  {  Init();  }
		GoCloneReq( Goid parent, Goid cloneSource, PlayerId playerId = PLAYERID_INVALID );
		GoCloneReq( Goid parent, const char* templateName, PlayerId playerId = PLAYERID_INVALID );
		GoCloneReq( Goid cloneSource, PlayerId playerId = PLAYERID_INVALID );
		GoCloneReq( const char* templateName, PlayerId playerId = PLAYERID_INVALID );

		void Init            ( void );
		void CalcPosition    ( void );
		int  GetMpPlayerCount( void ) const;
		void Finish          ( Go* go ) const;
		void FinishPost      ( Go* go ) const;

		bool InheritsStartingPosOrient( void ) const;

	FEX void SetStartingPos( const SiegePos& siegePos );
	FEX void SetStartingOrient( const Quat& quat )
			{  m_StartingOrient = quat;  m_UseStartingOrient = true;  }
	FEX void SetSnapToTerrain( bool snap = true )
			{  m_SnapToTerrain = snap;  }
	FEX void SetOmni( bool omni = true )
			{  m_OmniGo = omni;  }
	FEX void SetFadeIn( bool set = true )
			{  m_FadeIn = set;  }
	FEX void SetForcePosition( bool set = true )
			{  m_ForcePosition = set;  }
	FEX void SetForceServerOnly( bool set = true )
			{  m_ForceServerOnly = set;  }
	FEX void SetForceClientAllowed( bool set = true )
			{  m_ForceClientAllowed = set;  }
	FEX void SetPrepareToDrawNow( bool set = true )
			{  m_PrepareToDrawNow = set;  }
	FEX void SetNoStartupFx( bool set = true )
			{  m_NoStartupFx = set;  }

		Player* GetPlayer( Go* tryOtherGo = NULL ) const;

		void Xfer( FuBi::BitPacker& packer, GoCloneReq& def );

#		if !GP_RETAIL
		void RpcToString( gpstring& appendHere ) const;
#		endif // !GP_RETAIL

		GPDEBUG_ONLY( bool AssertValid( void ) const; )
	};

	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid parent, Goid cloneSource, PlayerId playerId );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid parent, Goid cloneSource );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid cloneSource, PlayerId playerId );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid cloneSource );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid parent, const char* templateName, PlayerId playerId );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( Goid parent, const char* templateName );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( const char* templateName, PlayerId playerId );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( const char* templateName );
	FUBI_EXPORT GoCloneReq& MakeGoCloneReq( void );

#	pragma pack ( pop )

	struct CloneGoXfer
	{
		GoCloneReq    m_CloneReq;
		Goid          m_StartGoid;
		DWORD         m_RandomSeed;
		const char*   m_TemplateName;
		gpstring      m_TemplateNameBuffer;
		bool          m_Standalone;
		const_mem_ptr m_Data;
		Buffer        m_DataBuffer;

		CloneGoXfer( void )
		{
			m_StartGoid    = GOID_INVALID;
			m_RandomSeed   = 0;
			m_TemplateName = NULL;
			m_Standalone   = false;
		}

		void CopyForDefault( const CloneGoXfer& src )
		{
			// only copy stuff we use
			m_CloneReq = src.m_CloneReq;

			// special handling for string
			if ( src.m_TemplateName != NULL )
			{
				m_TemplateNameBuffer = src.m_TemplateName;
				m_TemplateName = m_TemplateNameBuffer;
			}
			else
			{
				m_TemplateName = NULL;
				m_TemplateNameBuffer.clear();
			}
		}

		void Xfer( FuBi::BitPacker& packer, CloneGoXfer& def );

#		if !GP_RETAIL
		void RpcToString( gpstring& appendHere ) const;
#		endif // !GP_RETAIL
	};

// Query performance helper.

	// $ this class is meant to help out querying of const data from a go
	//   component through its go data component. it caches the string-to-
	//   column lookup. note that if the schema changes the ResetColumnCaches
	//   function should be called.

	class AutoConstQueryBase : public GlobalRegistrar <AutoConstQueryBase>
	{
	public:
		AutoConstQueryBase( const char* fieldName )
			: m_FieldName( fieldName ), m_Column( -1 )  {  }

		static void ResetColumnCaches( void )
		{
			for ( AutoConstQueryBase* i = ms_Root ; i != NULL ; i = i->m_Next )
			{
				i->m_Column = -1;
			}
		}

	protected:
		const char* m_FieldName;
		int         m_Column;
	};

	template <typename T>
	class AutoConstQuery : public AutoConstQueryBase
	{
	public:
		AutoConstQuery( const char* fieldName )
			: AutoConstQueryBase( fieldName )  {  }

		const T& Get( const FuBi::Record* record )
		{
			if ( m_Column == -1 )
			{
				m_Column = record->FindColumn( m_FieldName );
			}
#			if !GP_RETAIL
			if ( m_Column == -1 )
			{
				gperrorf(( "Column '%s' not found in component! Did you fetch components.gas?\n", m_FieldName ));
			}
#			endif // !GP_RETAIL
			return ( record->GetExact( m_Column, (T*)0 ) );
		}
	};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOBASE_H

//////////////////////////////////////////////////////////////////////////////
