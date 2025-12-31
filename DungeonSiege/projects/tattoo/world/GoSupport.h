//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoSupport.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains various support utilities for the game object and
//             persistence system, including traits for custom types.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOSUPPORT_H
#define __GOSUPPORT_H

//////////////////////////////////////////////////////////////////////////////

#include "Choreographer.h"
#include "Enchantment.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "Go.h"
#include "siege_engine.h"
#include "siege_persist.h"
#include "WorldMessage.h"
#include "WorldState.h"

//////////////////////////////////////////////////////////////////////////////
// random fun

	// note that all params are in web style format (key=val&key=val...)

// CameraFx

	FUBI_EXPORT bool StartCameraFx( const char* name, const char* params );
	FUBI_EXPORT bool StartCameraFx( const char* name );

// LightFx

	FUBI_EXPORT bool StartLightFx( LightId siegeLight, const char* name, const char* params );
	FUBI_EXPORT bool StartLightFx( LightId siegeLight, const char* name );

// DecalFx

	FUBI_EXPORT bool StartDecalFx( DecalId siegeDecal, const char* name, const char* params );
	FUBI_EXPORT bool StartDecalFx( DecalId siegeDecal, const char* name );

//////////////////////////////////////////////////////////////////////////////
// type LightId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( LightId ) == sizeof( DWORD ) );
	COMPILER_ASSERT( sizeof( LightId ) == sizeof( siege::database_guid ) );

	// conversions
    FUBI_EXPORT inline DWORD   MakeInt    ( LightId l )					{  return ( (DWORD)l );  }
	FUBI_EXPORT inline LightId MakeLightId( DWORD d )					{  return ( (LightId)d );  }

	// internal conversions
	inline siege::database_guid MakeSiegeGuid( LightId l )				{  return ( siege::database_guid( MakeInt( l ) ) );  }
	inline LightId              MakeLightId  ( siege::database_guid g )	{  return ( MakeLightId( g.GetValue() ) );  }

// Constants.

	extern const LightId LIGHTID_INVALID;

// Support.

	bool IsValid( LightId l, bool testExists = true );

// Structure.

	struct LightId_
	{
	private:

	// Methods.

		// traits
		FUBI_EXPORT bool  IsValid( void ) const					{  return ( ::IsValid( this ) );  }

		// query
		FUBI_EXPORT DWORD GetColor    ( void ) const;
		FUBI_EXPORT bool  GetActive   ( void ) const;
		FUBI_EXPORT float GetIntensity( void ) const;
		FUBI_EXPORT void  GetPosition ( SiegePos& pos ) const;

		// api
		FUBI_EXPORT void  SetColor    ( DWORD color );
		FUBI_EXPORT void  SetActive   ( bool active );
		FUBI_EXPORT void  SetIntensity( float intensity );

	// Constants.

		FUBI_EXPORT static LightId GetInvalidLightId( void )	{  return ( LIGHTID_INVALID );  }

	// cannot actually instantiate one of these... it's for use as a handle only

		LightId_( void );
		LightId_( const LightId_& );
	   ~LightId_( void );
	};


//////////////////////////////////////////////////////////////////////////////
// type DecalId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( DecalId ) == sizeof( DWORD ) );
	COMPILER_ASSERT( sizeof( DecalId ) == sizeof( siege::database_guid ) );

	// conversions
    FUBI_EXPORT inline DWORD   MakeInt    ( DecalId d )					{  return ( (DWORD)d );  }
	FUBI_EXPORT inline DecalId MakeDecalId( DWORD d )					{  return ( (DecalId)d );  }

	// internal conversions
	inline siege::database_guid MakeSiegeGuid( DecalId d )				{  return ( siege::database_guid( MakeInt( d ) ) );  }
	inline DecalId              MakeDecalId  ( siege::database_guid g )	{  return ( MakeDecalId( g.GetValue() ) );  }

// Constants.

	extern const DecalId DECALID_INVALID;

// Support.

	bool IsValid( DecalId d, bool testExists = true );

// Structure.

	struct DecalId_
	{
	private:

	// Methods.

		// traits
		FUBI_EXPORT bool  IsValid( void ) const					{  return ( ::IsValid( this ) );  }

		// query
		FUBI_EXPORT bool  GetActive   ( void ) const;
		FUBI_EXPORT BYTE  GetAlpha    ( void ) const;

		// api
		FUBI_EXPORT void  SetActive   ( bool active );
		FUBI_EXPORT void  SetActive   ( void )					{  SetActive( true );  }
		FUBI_EXPORT void  SetAlpha    ( BYTE alpha );

	// Constants.

		FUBI_EXPORT static DecalId GetInvalidDecalId( void )	{  return ( DECALID_INVALID );  }

	// cannot actually instantiate one of these... it's for use as a handle only

		DecalId_( void );
		DecalId_( const DecalId_& );
	   ~DecalId_( void );
	};


//////////////////////////////////////////////////////////////////////////////
// type SiegeId declaration

// Handle definition.

	COMPILER_ASSERT( sizeof( SiegeId ) == sizeof( DWORD ) );
	COMPILER_ASSERT( sizeof( SiegeId ) == sizeof( siege::database_guid ) );

	// conversions
    FUBI_EXPORT inline DWORD   MakeInt    ( SiegeId s )					{  return ( (DWORD)s );  }
	FUBI_EXPORT inline SiegeId MakeSiegeId( DWORD d )					{  return ( (SiegeId)d );  }

	// internal conversions
	inline siege::database_guid MakeSiegeGuid( SiegeId d )				{  return ( siege::database_guid( MakeInt( d ) ) );  }
	inline SiegeId              MakeSiegeId  ( siege::database_guid g )	{  return ( MakeSiegeId( g.GetValue() ) );  }

// Constants.

	extern const SiegeId SIEGEID_INVALID;

// Support.

	bool IsValid( SiegeId d, bool testExists = true );

// Structure.

	struct SiegeId_
	{
	private:

	// Methods.

		// traits
		FUBI_EXPORT bool  IsValid( void ) const					{  return ( ::IsValid( this ) );  }

	// Constants.

		FUBI_EXPORT static SiegeId GetInvalidSiegeId( void )	{  return ( SIEGEID_INVALID );  }

	// cannot actually instantiate one of these... it's for use as a handle only

		SiegeId_( void );
		SiegeId_( const SiegeId_& );
	   ~SiegeId_( void );
	};

//////////////////////////////////////////////////////////////////////////////
// traits declarations

// Handles

	FUBI_DECLARE_POINTERCLASS_TRAITS( Scid,      SCID_INVALID      );
	FUBI_DECLARE_POINTERCLASS_TRAITS( PlayerId,  PLAYERID_INVALID  );
	FUBI_DECLARE_POINTERCLASS_TRAITS( FrustumId, FRUSTUMID_INVALID );
	FUBI_DECLARE_POINTERCLASS_TRAITS( RegionId,  REGIONID_INVALID  );

	FUBI_DECLARE_TRAITS_IMPL( Goid, "Goid", (GOID_INVALID), PointerClass, FuBi::GetTypeByName( "Goid" ) )
	{
#		if GP_DEBUG
		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
		{
			if ( persist.IsSaving() && (GetGoidClass( obj ) == GO_CLASS_LOCAL) )
			{
				// this makes sure that we don't persist any lodfi local go's.
				// those should be effectively INVISIBLE to the system, and
				// nobody should be keeping pointers to them.
				GoHandle go( obj );
				gpassert( go && !go->IsLodfi() );
			}

			return ( PointerClassTraitBase <Goid> ::Xfer( persist, xfer, key, obj, defValue ) );
		}
#		endif // GP_DEBUG
	};

// Enums

	FUBI_DECLARE_ENUM_TRAITS( ePlayerController,  PC_INVALID      );
	FUBI_DECLARE_ENUM_TRAITS( eLifeState,         LS_IGNORE       );
	FUBI_DECLARE_ENUM_TRAITS( eActorAlignment,    AA_NEUTRAL      );
	FUBI_DECLARE_ENUM_TRAITS( ePContentType,      PT_INVALID      );
	FUBI_DECLARE_ENUM_TRAITS( eEquipSlot,         ES_NONE         );
	FUBI_DECLARE_ENUM_TRAITS( eInventoryLocation, IL_INVALID      );
	FUBI_DECLARE_ENUM_TRAITS( eAttackClass,       AC_BEASTFU      );
	FUBI_DECLARE_ENUM_TRAITS( eDefendClass,       DC_SKIN         );
	FUBI_DECLARE_ENUM_TRAITS( eMagicClass,        MC_NONE         );
	FUBI_DECLARE_ENUM_TRAITS( eQueryTrait,        QT_ANY          );
	FUBI_DECLARE_ENUM_TRAITS( eActionOrigin,      AO_INVALID      );
	FUBI_DECLARE_ENUM_TRAITS( eAnimChore,         CHORE_NONE      );
	FUBI_DECLARE_ENUM_TRAITS( eAnimStance,        AS_PLAIN        );
	FUBI_DECLARE_ENUM_TRAITS( eWorldEvent,        WE_INVALID      );
	FUBI_DECLARE_ENUM_TRAITS( eAlteration,        ALTER_INVALID   );
	FUBI_DECLARE_ENUM_TRAITS( eWorldState,        WS_INVALID      );

	FUBI_DECLARE_BITFIELD_ENUM_TRAITS( eTargetTypeFlags, TT_NONE );

// eLogicalNodeFlags

	using siege::eLogicalNodeFlags;
	FUBI_DECLARE_BITFIELD_ENUM_TRAITS( eLogicalNodeFlags, siege::LF_NONE );

	FEX inline bool TestBits( eLogicalNodeFlags l, eLogicalNodeFlags r )
		{  return ( !!(l & r) );  }

// GoString

	FUBI_DECLARE_USER_TRAITS_EX( GoString, GoString, ; )
	{
		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const U& defValue )
			{  return ( XferAsString( persist, xfer, key, obj, defValue ) );  }

		template <>
			static bool Xfer <GoString> ( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const GoString& defValue )
			{  return ( XferAsString( persist, xfer, key, obj, defValue.c_str() ) );  }

		static bool XferAsString( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const char* defValue );
	};

// GowString

	FUBI_DECLARE_USER_TRAITS_EX( GowString, GowString, ; )
	{
		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const U& defValue )
			{  return ( XferAsString( persist, xfer, key, obj, defValue ) );  }

		template <>
			static bool Xfer <GowString> ( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const GowString& defValue )
			{  return ( XferAsString( persist, xfer, key, obj, defValue.c_str() ) );  }

		static bool XferAsString( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const wchar_t* defValue );
	};

// Go*

	FUBI_DECLARE_POD_TRAITS_EX( Go*, Gop )
	{
		static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
	};

// Gob's

	namespace FuBi
	{
		template <typename T>
		struct GobTraitBase : PodTraitBase <T>
		{
			static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const Type& defValue )
			{
				bool rc = FuBi::Traits <Type::Type>::Xfer( persist, xfer, key, obj.GetNatural(), defValue.GetNatural() );
				if ( persist.IsRestoring() )
				{
					obj.Reset();
				}
				return ( rc );
			}

			static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, Type& obj, const Type::Type& defValue )
			{
				bool rc = FuBi::Traits <Type::Type>::Xfer( persist, xfer, key, obj.GetNatural(), defValue );
				if ( persist.IsRestoring() )
				{
					obj.Reset();
				}
				return ( rc );
			}

			static void ToString( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL )
				{  FuBi::Traits <Type::Type>::ToString( out, obj, xfer );  }
			static bool FromString( const char* str, Type& obj )
				{  return ( FuBi::Traits <Type::Type>::FromString( str, obj.GetNatural() ) );  }
		};
	}

#	define FUBI_DECLARE_GOB_TRAITS( T, NAME ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, ;, Gob, FuBi::GetTypeByName( #NAME ) )

	FUBI_DECLARE_GOB_TRAITS( GobInt,   GobInt   )  {  };
	FUBI_DECLARE_GOB_TRAITS( GobFloat, GobFloat )  {  };
	FUBI_DECLARE_GOB_TRAITS( GobBool,  GobBool  )  {  };

//////////////////////////////////////////////////////////////////////////////
// namespace GoMath declaration

namespace GoMath
{
	// $ special random numbers exclusively meant for go creation - use in
	//   skrit pcontent queries etc.

	FUBI_EXPORT float RandomFloat( float maxFloat );
	FUBI_DOC        ( RandomFloat$0,		"maxFloat",
					 "Returns a randomly generated float between 0.0 and 'maxFloat'." );

	FUBI_EXPORT float RandomFloat( float minFloat, float maxFloat );
	FUBI_DOC        ( RandomFloat$1,      "minFloat,"     "maxFloat",
					 "Returns a randomly generated float between 'minFloat' and 'maxFloat'." );

	FUBI_EXPORT int   RandomInt( int maxInt );
	FUBI_DOC        ( RandomInt$0,		"maxInt",
					 "Returns a randomly generated integer between 0 and 'maxInt'." );

	FUBI_EXPORT int   RandomInt( int minInt, int maxInt );
	FUBI_DOC        ( RandomInt$1,    "minInt,"   "maxInt",
					 "Returns a randomly generated integer between 'minInt' and 'maxInt'." );

	FUBI_EXPORT float OrbitAngleToTarget( Goid go, Goid target );
	FUBI_DOC		( OrbitAngleToTarget, "go," "target",
					 "Returns the radian orbit angle between a go's current orientation and the position of target." );

	FUBI_EXPORT float AzimuthAngleToTarget( Goid go, Goid target );
	FUBI_DOC		( AzimuthAngleToTarget, "go," "target",
					 "Returns the radian azimuth angle between a go's current orientation and the position of target." );
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOSUPPORT_H

//////////////////////////////////////////////////////////////////////////////
