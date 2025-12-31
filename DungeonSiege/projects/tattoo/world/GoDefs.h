//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoDefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains definitions related to the Game Object system.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GODEFS_H
#define __GODEFS_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"
#include "PtrHelp.h"
#include "Quat.h"
#include "Siege_Pos.h"
#include "GpColl.h"

#include <vector>
#include <set>
#include <list>

//////////////////////////////////////////////////////////////////////////////
// documentation

/*	Issues:

		Randomization - static and dynamic

		LODFI support

		Modifier + Template rules & further specialization (GUI icons, SFX playback)

		Decay -> Dust, Dust -> Retiring

		Resurrection base as GoChild (to get Scid) or component

		Overridden *data* in triggers - re-tag field as ADVANCED?

		Creation of Go reaching client AFTER deletion does. how to handle??

		LODFI types as local go's only (but not in siege editor!!)
*/

//////////////////////////////////////////////////////////////////////////////
// forward declarations

// External types.

	class BoneTranslator;
	class ChoreDictionary;
	class Enchantment;
	class EnchantmentStorage;
	class FastFuelHandle;
	class FuelHandle;
	class Job;
	class Player;
	class WorldMessage;
	class NavGoal;
	class World;

namespace trigger
{
	class Storage;
}

namespace siege
{
	class SiegeNode;
	enum eCollectFlags;
	enum eRequestFlags;
	struct ASPECTINFO;
	class database_guid;
	enum eLogicalNodeFlags;
}

	typedef siege::database_guid SiegeGuid;
	using siege::SiegeNode;

// Enums.

	enum eAlteration;
	enum eAnimChore;
	enum eAnimStance;
	enum eCombatOrders;
	enum eDefendClass;
	enum eFocusOrders;
	enum eJobAbstractType;
	enum eActionOrigin;
	enum eJobResult;
	enum eJobSpecificType;
	enum eJobTraits;
	enum eMagicClass;
	enum eTargetTypeFlags;
	enum eUsageContextFlags;
	enum eMovementOrders;
	enum eWorldEvent;
	enum eWorldState;

	enum MESSAGE_DISPATCH_FLAG;

	enum eShutdown
	{
		SHUTDOWN_FULL,					// destroy everything
		SHUTDOWN_FOR_RELOAD_GAME,		// kill everything required to restore a savegame
		SHUTDOWN_FOR_RELOAD_MAP,		// kill everything required to reload the existing map
		SHUTDOWN_FOR_NEW_MAP,			// kill required to clear the map
		SHUTDOWN_FOR_NEW_GAME,			// kill everything required to exit from game to front end
	};

// Core.

	class Go;
	class GoHandle;
	class GoDb;
	class GoComponent;
	class GoDataComponent;
	class GoDataTemplate;
	class ContentDb;
	class PContentDb;

	typedef std::vector <const GoDataTemplate*> TemplateColl;
	typedef std::set    <const GoDataTemplate*> TemplateSet;

// Components.

	class GoActor;
	class GoAspect;
	class GoAttack;
	class GoBody;
	class GoCommon;
	class GoConversation;
	class GoDefend;
	class GoFader;
	class GoFollower;
#	if !GP_RETAIL
	class GoEdit;
	class GoGizmo;
#	endif
	class GoGold;
	class GoGui;
	class GoInventory;
	class GoMagic;
	class GoMind;
	class GoParty;
	class GoPhysics;
	class GoPlacement;
	class GoPotion;
	class GoStore;
	class GoStash;
		// ...

// Other.

	struct AnimReq;
	struct GoCloneReq;
	struct GoCreateReq;
	struct GoGoldRangeColl;
	struct GoPotionRangeColl;
	struct GoInvRangeColl;
	class Formation;

// Ref-counted pointers.

	typedef RefPtr <BoneTranslator>     RpBoneTranslator;
	typedef RefPtr <ChoreDictionary>    RpChoreDictionary;
	typedef RefPtr <EnchantmentStorage> RpEnchantmentStorage;
	typedef RefPtr <trigger::Storage>   RpTriggerStorage;
	typedef RefPtr <GoGoldRangeColl>    RpGoGoldRangeColl;
	typedef RefPtr <GoPotionRangeColl>  RpGoPotionRangeColl;
	typedef RefPtr <GoInvRangeColl>     RpGoInvRangeColl;

// Handles.

	struct Scid_;
	typedef const Scid_* Scid;

	struct Goid_;
	typedef const Goid_* Goid;

	struct PlayerId_;
	typedef const PlayerId_* PlayerId;

	struct FrustumId_;
	typedef const FrustumId_* FrustumId;

	struct TeamId_;
	typedef const TeamId_* TeamId;

	struct RegionId_;
	typedef const RegionId_* RegionId;

	const RegionId REGIONID_INVALID = (RegionId)0;

	struct LightId_;
	typedef const LightId_* LightId;

	struct DecalId_;
	typedef const DecalId_* DecalId;

	struct SiegeId_;
	typedef const SiegeId_* SiegeId;

    inline DWORD    MakeInt     ( RegionId r )		{  return ( (DWORD)r );  }
	inline RegionId MakeRegionId( DWORD r    )		{  return ( (RegionId)r );  }

	struct GenericId_;
	typedef const GenericId_* GenericId;

// Basic constants.

	const FOURCC LOADER_GO_TYPE         = 'Go..';
	const FOURCC LOADER_GO_PRELOAD_TYPE = 'GoPr';
	const FOURCC LOADER_LODFI_TYPE      = 'GoLo';

	// extra info to pass to LOADER_LODFI_TYPE
	struct LodfiLoaderInfo
	{
		RegionId  m_RegionId;
		SiegeGuid m_NodeGuid;
		bool      m_FadeIn;

		LodfiLoaderInfo( RegionId regionId, SiegeGuid nodeGuid, bool fadeIn )
		{
			gpassert( regionId != REGIONID_INVALID );
			m_RegionId = regionId;
			m_NodeGuid = nodeGuid;
			m_FadeIn   = fadeIn;
		}

		operator void* ( void )
		{
			return ( this );
		}
	};

//////////////////////////////////////////////////////////////////////////////
// colors

const DWORD COLOR_WHITE			= 0xFFFFFFFF;
const DWORD COLOR_BLACK			= 0xFF000000;
const DWORD COLOR_LIGHT_GREY	= 0xFFAAAAAA;
const DWORD COLOR_GREY			= 0xFF888888;
const DWORD COLOR_DARK_GREY		= 0xFF666666;
const DWORD COLOR_LIGHT_RED		= 0xFFFF3333;
const DWORD COLOR_RED			= 0xFFFF0000;
const DWORD COLOR_DARK_RED		= 0xFFCC0000;
const DWORD COLOR_LIGHT_YELLOW	= 0xFFFFFF99;
const DWORD COLOR_YELLOW		= 0xFFFFFF00;
const DWORD COLOR_LIGHT_ORANGE	= 0xFFFFCC66;
const DWORD COLOR_ORANGE		= 0xFFFF9900;
const DWORD COLOR_PURPLE		= 0xFF9933CC;
const DWORD COLOR_LIGHT_BLUE	= 0xFF99CCFF;
const DWORD COLOR_BLUE			= 0xFF0000EE;
const DWORD COLOR_DARK_BLUE		= 0xFF0000AA;
const DWORD COLOR_STEEL_BLUE	= 0xFF6666FF;
const DWORD COLOR_CHERRY		= 0xFFFF0033;
const DWORD COLOR_BROWN			= 0xFF993333;
const DWORD COLOR_CYAN			= 0xFF339966;
const DWORD COLOR_LIGHT_GREEN	= 0xFF99FFCC;
const DWORD COLOR_GREEN			= 0xFF00CC00;
const DWORD COLOR_DARK_GREEN	= 0xFF008800;

//////////////////////////////////////////////////////////////////////////////
// helpers

// Simple query.

FEX	bool IsServerLocal( void );
FEX	inline bool IsServerRemote( void )  {  return ( !IsServerLocal() );  }

FEX	bool IsMultiPlayer( void );
FEX	inline bool IsSinglePlayer( void )  {  return ( !IsMultiPlayer() );  }

FEX	bool IsPrimaryThread( void );

FEX	bool IsWorldEditMode( void );

FEX bool IsSendLocalOnly( DWORD machineId );

FEX	unsigned int GetWorldSimCount( void );

// Simple helper types.

	class LoaderScope
	{
	public:
		LoaderScope( void )					{  gpassert( ms_InstanceCount >= 0 );  ++ms_InstanceCount;  }
	   ~LoaderScope( void )					{  --ms_InstanceCount;  gpassert( ms_InstanceCount >= 0 );  }

		static bool IsLoading( void )		{  return ( ms_InstanceCount != 0 );  }

	private:
		DECL_THREAD static int ms_InstanceCount;
	};

// Runtime check macros.

#	if GP_DEBUG
#	define CHECK_PRIMARY_THREAD_ONLY \
	if ( !IsPrimaryThread() ) \
	{ \
		gperror( "This function can only be called from the primary thread!\n" ); \
	}
#	else // GP_DEBUG
#	define CHECK_PRIMARY_THREAD_ONLY
#	endif // GP_DEBUG

#	if GP_DEBUG
	#define SINGLETON_ONCE_PER_SIM \
	{ \
		static unsigned int s_LastSimExecuted; \
		gpassertm( s_LastSimExecuted != GetWorldSimCount(), "This function can't be called more than once per sim!" ); \
		s_LastSimExecuted = GetWorldSimCount(); \
		CHECK_PRIMARY_THREAD_ONLY; \
	}
#	else // !GP_RETAIL
#	define SINGLETON_ONCE_PER_SIM
#	endif // !GP_RETAIL

#	define LOADER_OR_RPC_ONLY \
		/*$$$ MAKE THIS WORK!!! gpassertm( IsLoadingGo() || FUBI_IN_RPC_DISPATCH, "This function can only be called from a loader or RPC!\n" )*/

// Report streams

#	if !GP_RETAIL
	ReportSys::Context& GetGoLifeContext( void );
#	define gGoLifeContext GetGoLifeContext()
	ReportSys::Context& GetGoMsgContext( void );
#	define gGoMsgContext GetGoMsgContext()
	ReportSys::Context& GetGoUberContext( void );
#	define gGoUberContext GetGoUberContext()
#	endif // !GP_RETAIL

#	define gpgolife( msg ) DEV_MACRO_REPORT( &gGoLifeContext, msg )
#	define gpgolifef( msg ) DEV_MACRO_REPORTF( &gGoLifeContext, msg )
#	define gpgomsg( msg ) DEV_MACRO_REPORT( &gGoMsgContext, msg )
#	define gpgomsgf( msg ) DEV_MACRO_REPORTF( &gGoMsgContext, msg )
#	define gpgouber( msg ) DEV_MACRO_REPORT( &gGoUberContext, msg )
#	define gpgouberf( msg ) DEV_MACRO_REPORTF( &gGoUberContext, msg )

// Numbers.

	// special wrapper for RNG's to check on and log MP go-generation sync
	class GoRngNR : public RandomGeneratorBase <GoRngNR>
	{
	public:
		typedef stdx::fast_vector <DWORD> DwordColl;

		GoRngNR( const char* name )				: m_Name( name )  {  GPDEV_ONLY( m_LogRefs = 0 );  }

		void  SetSeed    ( DWORD seed );
		DWORD GetSeed    ( void ) const			{  return ( m_Rng.GetSeed() );  }
		void  Randomize  ( void )				{  m_Rng.Randomize();  }
		DWORD RandomDword( void );

		struct AutoLog
		{
#			if !GP_RETAIL
			AutoLog( void );
		   ~AutoLog( void );
#			endif // !GP_RETAIL
		};

	private:
		friend AutoLog;

		const char*       m_Name;
		RandomGeneratorNR m_Rng;

#		if !GP_RETAIL
		int       m_LogRefs;
		DwordColl m_RngLog;
#		endif // !GP_RETAIL
	};

	// global random number generator for the go system - use for creation type
	// random numbers only!!
	GoRngNR& GetGoCreateRng( void );

//////////////////////////////////////////////////////////////////////////////
// type Goid declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( Goid ) == sizeof( DWORD ) );

    // to int
    FUBI_EXPORT inline DWORD MakeInt( Goid g )  {  return ( (DWORD)g );  }

	// to goid
	FUBI_EXPORT Goid MakeGoid( DWORD intGoid );
	FUBI_DOC       ( MakeGoid,      "intGoid",
					"Returns a Goid based on its integer version." );

// Helpers.

	// $ these are the Goid classes used to select the database. note that the
	//   top two bits of every class of Goid must always be reserved for the
	//   selector.

	const int GO_CLASS_GLOBAL    = 0;			// global go (normal) - must be replicated across all machines (marshalled by the server)
	const int GO_CLASS_LOCAL     = 1;			// local-only go for waypoints and other client-only gizmos
	const int GO_CLASS_CLONE_SRC = 2;			// clone source go (ammo)
	const int GO_CLASS_CONSTANT  = 3;			// special constant

	// $ note that the funky union code in the following GoidBits structs is
	//   meant for fast indexing - getting the m_Index as a WORD (via ax) is
	//   much faster than as a bitfield (which involves shifts and ors in eax).
	//   so long as MAX_BITS_INDEX == 16 this will work.

#	pragma pack ( push, 1 )
	struct GlobalGoidBits
	{
		enum
		{
			// sizes to use for bit fields
			MAX_BITS_MAJOR_INDEX = 16,			// LSB (  0 - 15 )
			MAX_BITS_MINOR_INDEX =  8,			//     ( 16 - 23 )
			MAX_BITS_MAGIC       =  6,			//     ( 24 - 29 )
			MAX_BITS_CLASS       =  2,			// MSB ( 30 - 31 )

			// sizes to compare against for asserting dereferences
			MAX_MAJOR_INDEX  = (1 << MAX_BITS_MAJOR_INDEX) - 1,
			MAX_MINOR_INDEX  = (1 << MAX_BITS_MINOR_INDEX) - 1,
			MAX_MAGIC        = (1 << MAX_BITS_MAGIC      ) - 1,
			MAX_CLASS        = (1 << MAX_BITS_CLASS      ) - 1,
		};

		union
		{
			COMPILER_ASSERT(   (MAX_BITS_MAJOR_INDEX == sizeof( WORD ))
							&& ((  MAX_BITS_MAJOR_INDEX
								 + MAX_BITS_MINOR_INDEX
								 + MAX_BITS_MAGIC
								 + MAX_BITS_CLASS) == 32) );

			union
			{
				struct
				{
					WORD m_MajorIndex;								// major index into resource bucket set
					WORD m_Dummy1;
				};

				struct
				{
					unsigned m_Dummy2     : MAX_BITS_MAJOR_INDEX;
					unsigned m_MinorIndex : MAX_BITS_MINOR_INDEX;	// minor index into resource bucket
					unsigned m_Magic      : MAX_BITS_MAGIC;			// magic number to check against structure
					unsigned m_Class      : MAX_BITS_CLASS;			// the class of this Go
				};
			};
			DWORD m_Handle;
		};

		GlobalGoidBits( void )						{  m_Handle = 0;  }
		GlobalGoidBits( Goid goid )					{  m_Handle = MakeInt( goid );  }
		explicit GlobalGoidBits( DWORD handle )		{  m_Handle = handle;  }

		GlobalGoidBits( UINT majorIndex, UINT minorIndex, UINT magic )
		{
			gpassert( majorIndex <= MAX_MAJOR_INDEX );
			gpassert( minorIndex <= MAX_MINOR_INDEX );
			gpassert( (magic != 0) && (magic <= MAX_MAGIC) );

			m_MajorIndex = (WORD)majorIndex;
			m_MinorIndex = minorIndex;
			m_Magic      = magic;
			m_Class      = GO_CLASS_GLOBAL;
		}

		operator Goid ( void ) const				{  return ( (Goid)m_Handle );  }
	};
#	pragma pack ( pop )

#	pragma pack ( push, 1 )
	struct LocalGoidBits
	{
		enum
		{
			// sizes to use for bit fields
			MAX_BITS_INDEX = 16,				// LSB (  0 - 15 )
			MAX_BITS_MAGIC = 14,				//     ( 16 - 29 )
			MAX_BITS_CLASS =  2,				// MSB ( 30 - 31 )

			// sizes to compare against for asserting dereferences
			MAX_INDEX  = (1 << MAX_BITS_INDEX) - 1,
			MAX_MAGIC  = (1 << MAX_BITS_MAGIC) - 1,
			MAX_CLASS  = (1 << MAX_BITS_CLASS) - 1,
		};

		union
		{
			COMPILER_ASSERT(   (MAX_BITS_INDEX == sizeof( WORD ))
							&& ((  MAX_BITS_INDEX
								 + MAX_BITS_MAGIC
								 + MAX_BITS_CLASS) == 32) );

			union
			{
				struct
				{
					WORD m_Index;								// index into resource vector
					WORD m_Dummy1;
				};

				struct
				{
					unsigned m_Dummy2 : MAX_BITS_INDEX;
					unsigned m_Magic  : MAX_BITS_MAGIC;			// magic number to check against structure
					unsigned m_Class  : MAX_BITS_CLASS;			// the class of this Go
				};
			};
			DWORD m_Handle;
		};

		LocalGoidBits( void )						{  m_Handle = 0;  }
		LocalGoidBits( Goid goid )					{  m_Handle = MakeInt( goid );  }
		explicit LocalGoidBits( DWORD handle )		{  m_Handle = handle;  }

		LocalGoidBits( UINT index, UINT magic, bool local )
		{
			gpassert( index <= MAX_INDEX );
			gpassert( (magic != 0) && (magic <= MAX_MAGIC) );

			m_Index = (WORD)index;
			m_Magic = magic;
			m_Class = local ? GO_CLASS_LOCAL : GO_CLASS_CLONE_SRC;
		}

		operator Goid ( void ) const				{  return ( (Goid)m_Handle );  }
	};
#	pragma pack ( pop )

#	pragma pack ( push, 1 )
	struct ConstantGoidBits
	{
		enum
		{
			// sizes to use for bit fields
			MAX_BITS_CONSTANT = 30,				// LSB (  0 - 29 )
			MAX_BITS_CLASS    =  2,				// MSB ( 30 - 31 )

			// sizes to compare against for asserting dereferences
			MAX_CONSTANT = (1 << MAX_BITS_CONSTANT) - 1,
			MAX_CLASS    = (1 << MAX_BITS_CLASS   ) - 1,
		};

		union
		{
			COMPILER_ASSERT( (MAX_BITS_CONSTANT + MAX_BITS_CLASS) == 32 );

			struct
			{
				unsigned m_Constant : MAX_BITS_CONSTANT;		// arbitrary constant value, whatever you like
				unsigned m_Class    : MAX_BITS_CLASS;			// the class of this Go
			};
			DWORD m_Handle;
		};

		ConstantGoidBits( void )					{  m_Handle = 0;  }
		ConstantGoidBits( Goid goid )				{  m_Handle = MakeInt( goid );  }
		explicit ConstantGoidBits( DWORD handle )	{  m_Handle = handle;  }

		ConstantGoidBits( UINT constant )
		{
			gpassert( constant <= MAX_CONSTANT );

			m_Constant = constant;
			m_Class    = GO_CLASS_CONSTANT;
		}

		operator Goid ( void ) const				{  return ( (Goid)m_Handle );  }
	};
#	pragma pack ( pop )

// Constructors.

    // to global
	inline Goid MakeGlobalGoid( DWORD d )											{  return ( GlobalGoidBits( d ) );  }
	inline Goid MakeGlobalGoid( UINT majorIndex, UINT minorIndex, UINT magic )		{  return ( GlobalGoidBits( majorIndex, minorIndex, magic ) );  }

	// to local
	inline Goid MakeLocalGoid( DWORD d )											{  return ( LocalGoidBits( d ) );  }
	inline Goid MakeLocalGoid( UINT index, UINT magic )								{  return ( LocalGoidBits( index, magic, true ) );  }

	// to clone source
	inline Goid MakeCloneSrcGoid( DWORD d )											{  return ( LocalGoidBits( d ) );  }
	inline Goid MakeCloneSrcGoid( UINT index, UINT magic )							{  return ( LocalGoidBits( index, magic, false ) );  }

	// to constant
	inline Goid MakeConstantGoid( UINT constant )									{  return ( ConstantGoidBits( constant ) );  }

	// to general
	inline Goid MakeGoid( DWORD d )													{  return ( (Goid)d );  }

// Constants.

	extern const Goid GOID_INVALID;
	extern const Goid GOID_ANY;
	extern const Goid GOID_NONE;

// Support.

	bool IsValid  ( Goid g, bool testExists = true );
	bool IsValidMp( Goid g );
	Go*  GetGo    ( Goid g );
    Scid GetScid  ( Goid g );

	inline DWORD GetGoidClass     ( Goid g )	{  return ( GlobalGoidBits( g ).m_Class );  }

	inline bool  IsGlobalGoid     ( Goid g )	{  return ( (g != GOID_INVALID) && (GetGoidClass( g ) == GO_CLASS_GLOBAL) );  }
	inline bool  IsLocalGoid      ( Goid g )	{  return ( GetGoidClass( g ) == GO_CLASS_LOCAL     );  }
	inline bool  IsCloneSourceGoid( Goid g )	{  return ( GetGoidClass( g ) == GO_CLASS_CLONE_SRC );  }

	const char* GoidClassToString( DWORD gc );
	inline const char* GoidClassToString( Goid g )		{  return ( GoidClassToString( GetGoidClass( g ) ) );  }

#	if !GP_RETAIL
	const char* GoidToDebugString( Goid g );
#	endif // !GP_RETAIL

// Structure.

	struct Goid_
	{
	private:

	// Methods.

		FUBI_EXPORT bool IsValid( void ) const  {  return ( ::IsValid( this ) );  }
		FUBI_MEMBER_DOC( IsValid, "", "Returns true if this refers to a valid game object." );

		FUBI_EXPORT bool IsValidMp( void ) const  {  return ( ::IsValidMp( this ) );  }
		FUBI_MEMBER_DOC( IsValidMp, "", "Returns true if this refers to a valid game object that is safe for MP transfer." );

		FUBI_EXPORT Go*  GetGo( void ) const  {  return ( ::GetGo( this ) );  }
		FUBI_MEMBER_DOC( GetGo, "", "Returns a pointer to the Go that this Goid refers to, or NULL if invalid." );

		FUBI_EXPORT Scid GetScid( void ) const  {  return ( ::GetScid( this ) );  }
		FUBI_MEMBER_DOC( GetScid, "", "Returns the corresponding Scid, or Scid.InvalidScid if invalid." );

	// Constants.

		FUBI_EXPORT static Goid GetInvalidGoid( void )  {  return ( GOID_INVALID );  }
		FUBI_MEMBER_DOC(        GetInvalidGoid, "", "Returns the invalid Goid constant." );

		FUBI_EXPORT static Goid GetAnyGoid( void )  {  return ( GOID_ANY );  }
		FUBI_MEMBER_DOC(        GetAnyGoid, "", "Returns the 'any' Goid constant." );

		FUBI_EXPORT static Goid GetNoneGoid( void )  {  return ( GOID_NONE );  }
		FUBI_MEMBER_DOC(        GetNoneGoid, "", "Returns the 'none' Goid constant." );

	// cannot actually instantiate one of these... it's for use as a handle only

		Goid_( void );
		Goid_( const Goid_& );
	   ~Goid_( void );
	};

// Collections.

	struct GopColl;
	struct GoidColl;

	// need this everywhere
	typedef stdx::fast_vector <BYTE> Buffer;

	// collection of Go pointers
	struct GopColl : public stdx::fast_vector <Go*>
	{
		void Translate( GoidColl& out ) const			{ Translate( out, GOID_INVALID ); }
		void Translate( GoidColl& out, Goid ignore ) const;	// translation will not include ignored id

#if !GP_RETAIL
		bool AssertValid()
		{
			bool valid = true;
			for( GopColl::iterator i = this->begin(); i != this->end(); ++i )
			{
				if( (*i) == NULL )
				{
					gpassertm( 0, "Invalid GopColl" );
					valid = false;
				}
			}
			return valid;
		}
#endif

	private:
		FUBI_EXPORT void Add  ( Go* go )				{  push_back( go ); }
		FUBI_EXPORT int  Size ( void ) const			{  return ( scast <int> ( size() ) );  }
		FUBI_EXPORT bool Empty( void ) const			{  return ( empty() );  }
		FUBI_EXPORT Go*  Get  ( int index ) const		{  gpassert( (index >= 0) && (index < Size()) );  return ( (*this)[index] );  }
		FUBI_EXPORT void Set  ( int index, Go* g )		{  gpassert( (index >= 0) && (index < Size()) );  (*this)[index] = g;  }
		FUBI_EXPORT void Clear( void )					{  clear(); }
	};

	// collection of Goids
	struct GoidColl : public stdx::fast_vector <Goid>
	{
		// returns # translated
		int Translate( GopColl& out ) const;

	private:
		FUBI_EXPORT void Add  ( Goid goid )				{  push_back( goid ); }
		FUBI_EXPORT int  Size ( void ) const			{  return ( scast <int> ( size() ) );  }
		FUBI_EXPORT bool Empty( void ) const			{  return ( empty() );  }
		FUBI_EXPORT Goid Get  ( int index ) const		{  gpassert( (index >= 0) && (index < Size()) );  return ( (*this)[index] );  }
		FUBI_EXPORT void Set  ( int index, Goid g )		{  gpassert( (index >= 0) && (index < Size()) );  (*this)[index] = g;  }
		FUBI_EXPORT void Clear( void )					{  clear(); }
	};

	typedef std::set  <Go*>  GopSet;
	typedef std::list <Go*>  GopList;
	typedef std::set  <Goid> GoidSet;
	typedef std::_Bvector    BitVector;

//////////////////////////////////////////////////////////////////////////////
// type Scid declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( Scid ) == sizeof( DWORD ) );

    // to int
	FUBI_EXPORT inline DWORD MakeInt( Scid s )		{  return ( (DWORD)s );  }

	// to scid
	FUBI_EXPORT Scid MakeScid( DWORD intScid );
	FUBI_DOC       ( MakeScid,      "intScid",
					"Returns a Scid based on its integer version." );

// Constructors.

    // to scid
	inline Scid MakeScid( DWORD d )	{  return ( (Scid)d );  }

// Constants.

	extern const Scid SCID_INVALID;		// bad scid
	extern const Scid SCID_SPAWNED;		// no scid - this go was spawned directly from template

// Inline support.

	       bool IsValid   ( Scid s, bool testExists = true );
	inline bool IsInstance( Scid s, bool testExists = true )	{  return ( (s != SCID_SPAWNED) && IsValid( s, testExists ) );  }

	Goid GetGoid( Scid s );
	Go*  GetGo  ( Scid s );

// Structure.

	struct Scid_
	{
	private:

	// Methods.

		FUBI_EXPORT bool IsValid( void ) const  {  return ( ::IsValid( this ) );  }
		FUBI_MEMBER_DOC( IsValid, "", "Returns true if this refers to a valid piece of static content." );

		FUBI_EXPORT bool IsInstance( void ) const  {  return ( ::IsInstance( this ) );  }
		FUBI_MEMBER_DOC( IsInstance, "", "Returns true if this refers to a valid, non-spawned piece of static content." );

		FUBI_EXPORT Goid GetGoid( void ) const  {  return ( ::GetGoid( this ) );  }
		FUBI_MEMBER_DOC( GetGoid, "", "Returns the corresponding Goid, or Goid.InvalidGoid if invalid." );

		FUBI_EXPORT Go*  GetGo( void ) const  {  return ( ::GetGo( this ) );  }
		FUBI_MEMBER_DOC( GetGo, "", "Returns a pointer to the Go that this Scid refers to, or NULL if invalid." );

	// Constants.

		FUBI_EXPORT static Scid GetInvalidScid( void )  {  return ( SCID_INVALID );  }
		FUBI_MEMBER_DOC(        GetInvalidScid, "", "Returns the invalid Scid constant." );

		FUBI_EXPORT static Scid GetSpawnedScid( void )  {  return ( SCID_SPAWNED );  }
		FUBI_MEMBER_DOC(        GetSpawnedScid, "", "Returns the 'spawned' Scid constant." );

	// cannot actually instantiate one of these... it's for use as a handle only

		Scid_( void );
		Scid_( const Scid_& );
	   ~Scid_( void );
	};

//////////////////////////////////////////////////////////////////////////////
// type PlayerId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( PlayerId ) == sizeof( DWORD ) );

    // to int
FEX	inline DWORD MakeInt( PlayerId p )				{  return ( (DWORD)p );  }

	// to bit position
	inline int MakeIndex( PlayerId p )				{  return ( ::GetShift( MakeInt( p ) ) );  }

// Constructors.

    // to player id
FEX	inline PlayerId MakePlayerId( DWORD d ) 		{  return ( (PlayerId)d );  }

	// from bit position
	inline PlayerId MakePlayerIdFromIndex( int d )	{  return ( MakePlayerId( 1 << d ) );  }

// Constants.

	extern const PlayerId PLAYERID_INVALID;		// bad player id
//	extern const PlayerId PLAYERID_NEXTFREE;	// autocreate player id based on next free slot
	extern const PlayerId PLAYERID_COMPUTER;	// hard coded computer player id

// Inline support.

	bool IsValid ( PlayerId p, bool testExists = true );

// Structure.

	struct PlayerId_
	{
	private:

		FUBI_EXPORT bool IsValid( void ) const  {  return ( ::IsValid( this ) );  }
		FUBI_MEMBER_DOC( IsValid, "", "Returns true if this refers to a valid player id." );

		FUBI_EXPORT static PlayerId GetInvalidPlayerId( void )  {  return ( PLAYERID_INVALID );  }
		FUBI_MEMBER_DOC(            GetInvalidPlayerId, "", "Returns the invalid PlayerId constant." );

	// cannot actually instantiate one of these... it's for use as a handle only

		PlayerId_( void );
		PlayerId_( const PlayerId_& );
	   ~PlayerId_( void );
	};

//////////////////////////////////////////////////////////////////////////////
// type FrustumId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( FrustumId ) == sizeof( DWORD ) );

    // to int
	FUBI_EXPORT inline DWORD MakeInt( FrustumId f )		{  return ( (DWORD)f );  }

	// to bit position
	FUBI_EXPORT inline int MakeIndex( FrustumId f )		{  return ( ::GetShift( MakeInt( f ) ) );  }

// Constructors.

    // to frustum id
	FUBI_EXPORT inline FrustumId MakeFrustumId( DWORD d )
		{  return ( (FrustumId)d );  }

// Constants.

	extern const FrustumId FRUSTUMID_INVALID;		// bad frustum id
	extern const FrustumId FRUSTUMID_NEXTFREE;		// autocreate frustum id based on next free slot

// Inline support.

	FUBI_EXPORT bool IsValid ( FrustumId f, bool testExists = true );
	FUBI_EXPORT bool IsActive( FrustumId f );

// Structure.

	struct FrustumId_
	{
	private:

	// cannot actually instantiate one of these... it's for use as a handle only

		FrustumId_( void );
		FrustumId_( const FrustumId_& );
	   ~FrustumId_( void );
	};

//////////////////////////////////////////////////////////////////////////////
// type TeamId declaration

    // to int
	inline DWORD MakeInt( TeamId x )		{  return ( (DWORD)x );  }

//////////////////////////////////////////////////////////////////////////////
// type GenericId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( GenericId ) == sizeof( DWORD ) );

    // to int
	inline DWORD MakeInt( GenericId g )		{  return ( (DWORD)g );  }

// Constructors.

    // to generic id
	inline GenericId MakeGenericId( DWORD d )
		{  return ( (GenericId)d );  }

// Constants.

	const GenericId GENERICID_INVALID = 0;		// bad generic id

// Inline support.

	inline bool IsValid( GenericId g )  {  return ( g != GENERICID_INVALID );  }

// Structure.

	struct GenericId_
	{
	private:

		FUBI_EXPORT bool IsValid( void ) const  {  return ( ::IsValid( this ) );  }
		FUBI_MEMBER_DOC( IsValid, "", "Returns true if this refers to a valid generic id." );

		FUBI_EXPORT static GenericId GetInvalidGenericId( void )  {  return ( GENERICID_INVALID );  }
		FUBI_MEMBER_DOC(             GetInvalidGenericId, "", "Returns the invalid GenericId constant." );

	// cannot actually instantiate one of these... it's for use as a handle only

		GenericId_( void );
		GenericId_( const GenericId_& );
	   ~GenericId_( void );
	};

//////////////////////////////////////////////////////////////////////////////
// enum ePlayerSkin declaration

enum ePlayerSkin
{
	SET_BEGIN_ENUM( PS_, 0 ),

	PS_FLESH,
	PS_CLOTH,

	SET_END_ENUM( PS_ ),
};

//////////////////////////////////////////////////////////////////////////////
// enum ePlayerController declaration

enum ePlayerController
{
	SET_BEGIN_ENUM( PC_, 0 ),

	PC_INVALID,
	PC_HUMAN,
	PC_COMPUTER,
	PC_REMOTE_HUMAN,
	PC_REMOTE_COMPUTER,

	SET_END_ENUM( PC_ ),
};

const char* ToString  ( ePlayerController pc );
bool        FromString( const char* str, ePlayerController& pc );

//////////////////////////////////////////////////////////////////////////////
// enum eLifeState declaration

enum eLifeState
{
	SET_BEGIN_ENUM( LS_, 0 ),

	LS_IGNORE,					// ignore this thing wrt life query issues
	LS_ALIVE_CONSCIOUS,			// actor is alive and conscious
	LS_ALIVE_UNCONSCIOUS,		// actor is alive but unconscious
	LS_DEAD_NORMAL,				// actor is dead, nothing special (next state is LS_DECAY_FRESH)
	LS_DEAD_CHARRED,			// actor is a charred corpse (next state is LS_DECAY_BONES)
	LS_DECAY_FRESH,				// actor is dead, freshly decaying
	LS_DECAY_BONES,				// decaying, only bones visible
	LS_DECAY_DUST,				// decaying, just dust now
	LS_GONE,					// gone forever
	LS_GHOST,					// multiplayer only feature, player is a dead and has turned into a ghost

	SET_END_ENUM( LS_ ),
};

const char* ToString  ( eLifeState ls );
bool        FromString( const char* str, eLifeState& ls );

FUBI_EXPORT inline bool IsAlive( eLifeState ls )		{ return ( (ls == LS_ALIVE_CONSCIOUS) || (ls == LS_ALIVE_UNCONSCIOUS) );  }
FUBI_EXPORT inline bool IsDead( eLifeState ls )			{ return ( (ls == LS_DEAD_NORMAL) || (ls == LS_DEAD_CHARRED) ); }
FUBI_EXPORT inline bool IsGhost( eLifeState ls )		{ return ( ls == LS_GHOST ); }
FUBI_EXPORT inline bool IsConscious( eLifeState ls )	{ return ( ls == LS_ALIVE_CONSCIOUS ); }

//////////////////////////////////////////////////////////////////////////////
// enum eActorAlignment declaration

enum eActorAlignment
{
	SET_BEGIN_ENUM( AA_, 0 ),

	AA_GOOD,
	AA_NEUTRAL,
	AA_EVIL,

	SET_END_ENUM( AA_ ),
};

const char* ToString  ( eActorAlignment e );
bool        FromString( const char* str, eActorAlignment& e );

//////////////////////////////////////////////////////////////////////////////
// enum ePContentType declaration

enum ePContentType
{
	SET_BEGIN_ENUM( PT_, 0 ),

// Basic classes.

	// basic
	PT_ARMOR,
	PT_WEAPON,
	PT_AMULET,
	PT_RING,
	PT_SPELL,
	PT_SCROLL,
	PT_POTION,
	PT_SPELLBOOK,

// Specialized types.

	// armor
	PT_BODY,
	PT_HELM,
	PT_GLOVES,
	PT_BOOTS,
	PT_SHIELD,

	// weapon
	PT_MELEE,
	PT_RANGED,

	// spells
	PT_CMAGIC,
	PT_NMAGIC,

	// final
	PT_INVALID,

	SET_END_ENUM( PT_ ),

// Counts and iterator bounds (no string equivalent).

	// $ be sure to update these if the above enum constants change
	PT_BASIC_BEGIN  = PT_ARMOR,
	PT_BASIC_END    = PT_BODY,
	PT_BASIC_COUNT  = PT_BASIC_END - PT_BASIC_BEGIN,
	PT_ARMOR_BEGIN  = PT_BASIC_END,
	PT_ARMOR_END    = PT_MELEE,
	PT_ARMOR_COUNT  = PT_ARMOR_END - PT_ARMOR_BEGIN,
	PT_WEAPON_BEGIN = PT_ARMOR_END,
	PT_WEAPON_END   = PT_CMAGIC,
	PT_WEAPON_COUNT = PT_WEAPON_END - PT_WEAPON_BEGIN,
	PT_SPELL_BEGIN  = PT_WEAPON_END,
	PT_SPELL_END    = PT_INVALID,
	PT_SPELL_COUNT  = PT_SPELL_END - PT_SPELL_BEGIN,
};

const char*   ToString           ( ePContentType e );
const char*   ToQueryString      ( ePContentType e );
ePContentType ToBasicPContentType( ePContentType e );
bool          FromString         ( const char* str, ePContentType& e );
bool          FromQueryString    ( const char* str, ePContentType& e );

inline bool IsBasicPContentType ( ePContentType e )  {  return ( (e >= PT_BASIC_BEGIN ) && (e < PT_BASIC_END ) );  }
inline bool IsArmorPContentType ( ePContentType e )  {  return ( (e >= PT_ARMOR_BEGIN ) && (e < PT_ARMOR_END ) );  }
inline bool IsWeaponPContentType( ePContentType e )  {  return ( (e >= PT_WEAPON_BEGIN) && (e < PT_WEAPON_END) );  }
inline bool IsSpellPContentType ( ePContentType e )  {  return ( (e >= PT_SPELL_BEGIN ) && (e < PT_SPELL_END ) );  }

//////////////////////////////////////////////////////////////////////////////
// enum eEquipSlot declaration

enum eEquipSlot
{
	SET_BEGIN_ENUM( ES_, 0 ),

// Equippable items.

	// visible when equipped
	ES_SHIELD_HAND,
	ES_WEAPON_HAND,
	ES_FEET,
	ES_CHEST,
	ES_HEAD,
	ES_FOREARMS,

	// not visible when equipped
	ES_AMULET,
	ES_SPELLBOOK,
	ES_RING_0,
	ES_RING_1,
	ES_RING_2,
	ES_RING_3,

// Special constants (not equippable).

	// special
	ES_RING,				// generic ring that will go anywhere

	// final
	ES_NONE,				// no equip slot
	ES_ANY,

	SET_END_ENUM( ES_ ),

// Counts and iterator bounds (no string equivalent).

	// $ be sure to update these if the above enum constants change
	ES_VISIBLE_BEGIN   = ES_SHIELD_HAND,
	ES_INVISIBLE_BEGIN = ES_AMULET,
	ES_ARMOR_BEGIN     = ES_FEET,
	ES_ARMOR_END       = ES_AMULET,
	ES_SPECIAL_BEGIN   = ES_RING,
	ES_VISIBLE_END     = ES_INVISIBLE_BEGIN,
	ES_INVISIBLE_END   = ES_SPECIAL_BEGIN,
	ES_SPECIAL_END     = ES_NONE,

	// autodetect counts
	ES_VISIBLE_COUNT   = ES_VISIBLE_END   - ES_VISIBLE_BEGIN,
	ES_INVISIBLE_COUNT = ES_INVISIBLE_END - ES_INVISIBLE_BEGIN,
	ES_ARMOR_COUNT     = ES_ARMOR_END     - ES_ARMOR_BEGIN,
	ES_EQUIP_COUNT     = ES_VISIBLE_COUNT + ES_INVISIBLE_COUNT,
	ES_SPECIAL_COUNT   = ES_SPECIAL_END   - ES_SPECIAL_BEGIN,
};

const char*   ToString      ( eEquipSlot e );
ePContentType ToPContentType( eEquipSlot e );
bool          FromString    ( const char* str, eEquipSlot& e );

const char*   GetArmorString( eEquipSlot e );

inline bool   IsVisibleEquipSlot( eEquipSlot e )	{  return ( (e >= ES_VISIBLE_BEGIN) && (e < ES_VISIBLE_END) );  }
inline bool   IsArmorEquipSlot  ( eEquipSlot e )	{  return ( (e >= ES_ARMOR_BEGIN) && (e < ES_ARMOR_END) );  }
inline bool   IsEquippableSlot  ( eEquipSlot e )	{  return ( (e >= ES_BEGIN) && (e < ES_SPECIAL_END) );  }
inline bool	  IsRingSlot		( eEquipSlot e )	{  return ( (e >= ES_RING_0 ) && ( e <= ES_RING ) ); }

MAKE_ENUM_MATH_OPERATORS( eEquipSlot );

/////////////////////////////////////////////////////////////
// enum eInventoryLocation declaration

enum eInventoryLocation
{
	SET_BEGIN_ENUM( IL_, 0 ),

// Singular slots.

	// active set
	IL_ACTIVE_MELEE_WEAPON,
	IL_ACTIVE_RANGED_WEAPON,
	IL_ACTIVE_PRIMARY_SPELL,
	IL_ACTIVE_SECONDARY_SPELL,
	
	// spell scroll slots
	IL_SPELL_1,
	IL_SPELL_2,
	IL_SPELL_3,
	IL_SPELL_4,
	IL_SPELL_5,
	IL_SPELL_6,
	IL_SPELL_7,
	IL_SPELL_8,
	IL_SPELL_9,
	IL_SPELL_10,
	IL_SPELL_11,
	IL_SPELL_12,
	
	// shield set
	IL_SHIELD,

// Special constants.

	// default/error state
	IL_INVALID,

	// general collection
	IL_ALL,
	IL_ALL_ACTIVE,
	IL_ALL_SPELLS,
	IL_MAIN,

	SET_END_ENUM( IL_ ),

// Counts and iterator bounds (no string equivalent)

	// $ be sure to update this if the above enum constants change
	IL_ACTIVE_COUNT = IL_ACTIVE_SECONDARY_SPELL - IL_ACTIVE_MELEE_WEAPON + 1,
};

COMPILER_ASSERT( IL_ACTIVE_MELEE_WEAPON  < IL_ACTIVE_RANGED_WEAPON   );
COMPILER_ASSERT( IL_ACTIVE_RANGED_WEAPON < IL_ACTIVE_PRIMARY_SPELL   );
COMPILER_ASSERT( IL_ACTIVE_PRIMARY_SPELL < IL_ACTIVE_SECONDARY_SPELL );

inline bool IsSingularSlot( eInventoryLocation il )	{  return ( il < IL_INVALID );  }
inline bool IsActiveSlot  ( eInventoryLocation il )	{  return ( (il >= IL_ACTIVE_MELEE_WEAPON) && (il <= IL_ACTIVE_SECONDARY_SPELL) );  }
inline bool IsSpellSlot   ( eInventoryLocation il )	{  return ( (il >= IL_ACTIVE_PRIMARY_SPELL) && (il <= IL_SPELL_12) );  }
inline bool IsAddableSlot ( eInventoryLocation il )	{  return ( (il != IL_INVALID) && (il != IL_ALL) );  }

const char* ToString  ( eInventoryLocation a );
bool        FromString( const char* str, eInventoryLocation& a );

//////////////////////////////////////////////////////////////////////////////
// enum eAttackClass

enum eAttackClass
{
	SET_BEGIN_ENUM( AC_, 0 ),

	AC_BEASTFU,
	AC_AXE,
	AC_CLUB,
	AC_DAGGER,
	AC_HAMMER,
	AC_MACE,
	AC_STAFF,
	AC_SWORD,
	AC_BOW,
	AC_MINIGUN,
	AC_ARROW,
	AC_BOLT,
	AC_COMBAT_MAGIC,
	AC_NATURE_MAGIC,

	SET_END_ENUM( AC_ ),
};

const char*   ToString       ( eAttackClass e );
const char*   ToQueryString  ( eAttackClass e );
ePContentType ToPContentType ( eAttackClass e );
bool          FromString     ( const char* str, eAttackClass& e );
bool          FromQueryString( const char* str, eAttackClass& e );

inline bool IsAmmo( eAttackClass e )  {  return ( (e == AC_ARROW) || (e == AC_BOLT) );  }

//////////////////////////////////////////////////////////////////////////////
// enum eDefendClass

enum eDefendClass
{
	SET_BEGIN_ENUM( DC_, 0 ),

	DC_SKIN,
	DC_SHIELD,

	SET_END_ENUM( DC_ ),
};

const char* ToString  ( eDefendClass e );
bool        FromString( const char* str, eDefendClass& e );

//////////////////////////////////////////////////////////////////////////////
// enum eMagicClass

enum eMagicClass
{
	SET_BEGIN_ENUM( MC_, 0 ),

	MC_NONE,
	MC_POTION,
	MC_COMBAT_MAGIC,
	MC_NATURE_MAGIC,

	SET_END_ENUM( MC_ ),
};

const char* ToString  ( eMagicClass e );
bool        FromString( const char* str, eMagicClass& e );

inline bool IsSpell ( eMagicClass mc )  {  return ( (mc == MC_COMBAT_MAGIC) || (mc == MC_NATURE_MAGIC) );  }
inline bool IsPotion( eMagicClass mc )  {  return ( mc == MC_POTION );  }

//////////////////////////////////////////////////////////////////////////////
// enum eTargetTypeFlags

enum eTargetTypeFlags
{
	TT_NONE						= 0,

	// Op
	TT_AND						= 1 <<	0,

	////////////////////////////////////////
	//	actors

	// go type

	TT_ACTOR				   	= 1 <<  1,
	TT_ACTOR_PACK_ONLY		   	= 1 <<  2,
	TT_NOT_ACTOR				= 1 <<	3,
	TT_SELF					   	= 1 <<  4,
	TT_BREAKABLE				= 1 <<	5,

	TT_CAN_STORE_MANA			= 1 <<  6,
	TT_INJURED_FRIEND			= 1 <<	7,

	// tactical

	TT_DEAD_FRIEND				= 1 <<  8,
	TT_UNCONSCIOUS_FRIEND		= 1 <<  9,
	TT_CONSCIOUS_FRIEND			= 1 << 10,

	TT_DEAD_ENEMY				= 1 << 11,
	TT_UNCONSCIOUS_ENEMY	   	= 1 << 12,
	TT_CONSCIOUS_ENEMY		   	= 1 << 13,

	TT_SUMMONED					= 1 << 14,
	TT_POSSESSED				= 1 << 15,
	TT_ANIMATED					= 1 << 16,

	TT_HUMAN_PARTY_MEMBER		= 1 << 17,
	TT_SCREEN_PARTY_MEMBER		= 1 << 18,

	// alignment

	TT_GOOD					   	= 1 << 19,
	TT_NEUTRAL				   	= 1 << 20,
	TT_EVIL					   	= 1 << 21,

	////////////////////////////////////////
	//	items

	TT_WEAPON_MELEE			   	= 1 << 22,
	TT_WEAPON_RANGED		   	= 1 << 23,
	TT_ARMOR				   	= 1 << 24,
	TT_SHIELD				   	= 1 << 25,
	TT_EQUIPPABLE				= 1 << 26,

	TT_TRANSMUTABLE				= 1 << 27,

	TT_TERRAIN				   	= 1 << 28,


};

const char* ToString      ( eTargetTypeFlags e );
gpstring    ToFullString  ( eTargetTypeFlags e );
bool        FromString    ( const char* str, eTargetTypeFlags& e );
bool        FromFullString( const char* str, eTargetTypeFlags& e );

MAKE_ENUM_BIT_OPERATORS( eTargetTypeFlags );

//////////////////////////////////////////////////////////////////////////////
// enum eUsageContextFlags

enum eUsageContextFlags
{
	UC_NONE						= 0,

	UC_LIFE_GIVING				= 1 << 1, // This spell should give life to the target
	UC_LIFE_GETTING				= 1 << 2, // This spell should take life from the target and give it to the source

	UC_MANA_GIVING				= 1 << 3, // This spell should give mana to the target
	UC_MANA_GETTING				= 1 << 4, // This spell should take mana from the target and give it to the source

	UC_REANIMATING				= 1 << 5,

	UC_PASSIVE					= 1 << 6, // This spell shouldn't directly effect a friend or enemy
	UC_AGGRESSIVE				= 1 << 7, // This spell's main purpose is to hinder the target

	UC_OFFENSIVE				= 1 << 8, // This spell is meant to do damage to the target
	UC_DEFENSIVE				= 1 << 9, // This spell should help the target
};

const char* ToString		( eUsageContextFlags e );
gpstring    ToFullString	( eUsageContextFlags e );
bool        FromString		( const char* str, eUsageContextFlags& e );
bool        FromFullString	( const char* str, eUsageContextFlags& e );

MAKE_ENUM_BIT_OPERATORS( eUsageContextFlags );

//////////////////////////////////////////////////////////////////////////////
// enum eQueryTrait declaration

enum eQueryTrait
{
	SET_BEGIN_ENUM( QT_, 0 ),

	QT_NONE,
	QT_ANY,

	QT_LIFE,
	QT_LIFE_LOW,
	QT_LIFE_HIGH,
	QT_LIFE_HEALING,
	QT_LIFE_DAMAGING,

	QT_MANA,
	QT_MANA_LOW,
	QT_MANA_HIGH,
	QT_MANA_HEALING,
	QT_MANA_DAMAGING,

	QT_REANIMATING,

	QT_ALIVE_CONSCIOUS,
	QT_ALIVE_UNCONSCIOUS,
	QT_ALIVE,
	QT_DEAD,
	QT_GHOST,

	QT_WEAPON,
	QT_MELEE_WEAPON,
	QT_RANGED_WEAPON,
	QT_MELEE_WEAPON_SELECTED,
	QT_RANGED_WEAPON_SELECTED,

	QT_ONE_SHOT_SPELL,
	QT_MULTIPLE_SHOT_SPELL,
	QT_COMMAND_CAST_SPELL,
	QT_AUTO_CAST_SPELL,

	QT_FIGHTING,

	QT_ACTOR,
	QT_ITEM,
	QT_POTION,
	QT_SPELL,

	QT_ARMOR,
	QT_ARMOR_WEARABLE,
	QT_SHIELD,

	QT_CASTABLE,
	QT_ATTACKABLE,

	QT_SELECTED,
	QT_VISIBLE,
	QT_INVISIBLE,
	QT_HAS_LOS,
	QT_PACK,
	QT_NONPACK,

	QT_BUSY,
	QT_IDLE,

	QT_GOOD,
	QT_NEUTRAL,
	QT_EVIL,

	QT_FRIEND,
	QT_ENEMY,

	QT_SURVIVAL_FACTOR,
	QT_OFFENSE_FACTOR,

	SET_END_ENUM( QT_ ),
};

const char* ToString  ( eQueryTrait a );
bool        FromString( const char* str, eQueryTrait& a );

struct QtColl : public std::vector <eQueryTrait>
{
private:
	FUBI_EXPORT void Add  ( eQueryTrait qt )			{  push_back( qt );  }
	FUBI_EXPORT void Clear( void )						{  clear();  }
};


/////////////////////////////////////////////////////////////
// eActionOrigin

enum eActionOrigin
{
	SET_BEGIN_ENUM( AO_, 0 ),

	AO_INVALID,
	AO_USER,
	AO_REFLEX,
	AO_PARTY,
	AO_COMMAND,

	SET_END_ENUM( AO_ )
};

const char* ToString( eActionOrigin value );
bool FromString( const char* str, eActionOrigin & value );

/////////////////////////////////////////////////////////////
// eVoiceSound

enum eVoiceSound
{
	SET_BEGIN_ENUM( VS_, 0 ),

	VS_INVALID,
	VS_ENEMY_SPOTTED,

	SET_END_ENUM( VS_ )
};

const char* ToString( eVoiceSound value );
const char* ToEventString( eVoiceSound e );
bool FromString( const char* str, eVoiceSound & value );

//////////////////////////////////////////////////////////////////////////////
// equip requirements declaration

struct EquipRequirement
{
	gpstring m_Skill;
	float    m_MinRequired;
};

struct EquipRequirementColl : std::vector <EquipRequirement>
{
	SET_INHERITED( EquipRequirementColl, std::vector <EquipRequirement> );

	EquipRequirementColl( void )						{  }
	EquipRequirementColl( const gpstring& str )			{  Load( str );  }

	int      Load    ( const gpstring& str );
	void     Merge   ( const gpstring& str );
	gpstring ToString( void ) const;

	gpwstring MakeToolTipString( Go* forActor ) const;
	bool      CanEquip         ( Go* forActor ) const;
};

//////////////////////////////////////////////////////////////////////////////
// struct MpWorld declaration

struct MpWorld
{
	gpstring  m_Name;				// name of this world, also used as path prefix to use for objects and index
	gpwstring m_ScreenName;			// name of this world (pretranslated, for showing on UI)
	gpwstring m_Description;		// extra description to print for this (pretranslated, "you must be a man among men to play in this world")
	float     m_RequiredLevel;		// minimum required uber-level that your hero must be to unlock this world

	MpWorld( void )
	{
		m_RequiredLevel = 0;
	}

	bool Load( FastFuelHandle fuel );
	bool Load( FuelHandle fuel );
	bool Xfer( FuBi::PersistContext& persist );

	struct LessByRequiredLevel
	{
		bool operator () ( const MpWorld& l, const MpWorld& r ) const
		{
			if ( l.m_RequiredLevel < r.m_RequiredLevel )
			{
				return ( true );
			}
			if ( r.m_RequiredLevel < l.m_RequiredLevel )
			{
				return ( false );
			}

			return ( compare_no_case( l.m_ScreenName, r.m_ScreenName ) < 0 );
		}
	};
};

struct MpWorldColl : std::list <MpWorld>
{
	gpstring GetDefaultMpWorldName( void ) const;

	bool Load( FastFuelHandle fuel );
	bool Load( FuelHandle fuel );
};

//////////////////////////////////////////////////////////////////////////////
// class MapInfo declaration

struct MapInfo
{
	gpwstring   m_ScreenName;			// for printing on the screen (pre-translated)
	gpwstring   m_Description;			// description printing on the screen (pre-translated)
	gpstring    m_InternalName;			// for choosing the map internally (paths and such)
	bool        m_IsSingleplayerOnly;	// is this a singleplayer-only map?
	bool        m_IsMultiplayerOnly;	// is this a multiplayer-only map?
	bool        m_IsDevOnly;			// is this for development purposes only?
	gpstring    m_TankFileName;			// this is the tank to use for this map, empty if not in a tank
	GUID        m_TankGuid;				// guid of the tank containing this mapinfo
	MpWorldColl m_MpWorlds;				// multiplayer worlds in this map

	MapInfo( void )
	{
		m_IsSingleplayerOnly = false;
		m_IsMultiplayerOnly = false;
		m_IsDevOnly = false;
		::ZeroObject( m_TankGuid );
	}

	struct LessByScreenName
	{
		bool operator () ( const MapInfo& l, const MapInfo& r )
		{
			// put non-dev-only first
			if ( !l.m_IsDevOnly && r.m_IsDevOnly )
			{
				return ( true );
			}
			else if ( !r.m_IsDevOnly && l.m_IsDevOnly )
			{
				return ( false );
			}
			else
			{
				return ( compare_no_case( l.m_ScreenName, r.m_ScreenName ) < 0 );
			}
		}
	};
};

typedef stdx::fast_vector <MapInfo> MapInfoColl;

//////////////////////////////////////////////////////////////////////////////

#endif  // __GODEFS_H

//////////////////////////////////////////////////////////////////////////////
