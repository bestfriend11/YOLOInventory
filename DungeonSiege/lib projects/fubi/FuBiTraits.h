//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTraits.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the traits system for FuBi types. Traits define
//             operations on types - conversions, attributes, etc.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBITRAITS_H
#define __FUBITRAITS_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiTypes.h"

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// conversion helpers

bool IsNumber          ( const char* num );
void AppendEscapedChar ( gpstring& out, char c, bool forceHex = false );
bool GetUnescapedChar  ( const char*& str, char& obj );
void AppendFourCcString( gpstring& out, int c );
bool IntFromFourCc     ( const char* str, int& obj );

//////////////////////////////////////////////////////////////////////////////
// trait help

// Definition helper macros.

	template <typename T>
	struct TraitMiddle
	{
	};

#	define FUBI_DECLARE_TRAITS_IMPL( T, NAME, DEF, BASE, VARTYPE )				\
	template <> struct FuBi::TraitMiddle < T > : FuBi::BASE##TraitBase < T >	\
	{																			\
		static const char* GetInternalVarName( void )							\
			{  return ( NAME );  }												\
		static const char* GetExternalVarName( void )							\
			{  return ( NAME );  }												\
		static T& GetDefaultValue( void )										\
			{  static T s_DefValue DEF;  return ( s_DefValue );  }				\
		static eVarType GetVarType( void )										\
			{  return ( VARTYPE );  }											\
	};																			\
	template <> struct FuBi::Traits < T > : FuBi::TraitMiddle < T >

#	define FUBI_DECLARE_ENUM_TRAITS_EX( T, NAME, DEF ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, (DEF), Enum, FuBi::GetTypeByName( #NAME ) )  {  }

#	define FUBI_DECLARE_ENUM_TRAITS( T, DEF ) \
		   FUBI_DECLARE_ENUM_TRAITS_EX( T, T, DEF )

#	define FUBI_DECLARE_BITFIELD_ENUM_TRAITS_EX( T, NAME, DEF ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, (DEF), BitfieldEnum, FuBi::GetTypeByName( #NAME ) )  {  }

#	define FUBI_DECLARE_BITFIELD_ENUM_TRAITS( T, DEF ) \
		   FUBI_DECLARE_BITFIELD_ENUM_TRAITS_EX( T, T, DEF )

#	define FUBI_DECLARE_VAR_TRAITS( T, TYPE ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #T, ;, Var, TYPE )

#	define FUBI_DECLARE_POINTERCLASS_TRAITS_EX( T, NAME, DEF ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, (DEF), PointerClass, FuBi::GetTypeByName( #NAME ) )  {  }

#	define FUBI_DECLARE_POINTERCLASS_TRAITS( T, DEF ) \
		   FUBI_DECLARE_POINTERCLASS_TRAITS_EX( T, T, DEF )

#	define FUBI_DECLARE_POD_TRAITS_EX( T, NAME ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, ;, Pod, FuBi::GetTypeByName( #NAME ) )

#	define FUBI_DECLARE_POD_TRAITS( T ) \
			FUBI_DECLARE_POD_TRAITS_EX( T, T )

#	define FUBI_DECLARE_SPECIAL_TRAITS( T, TYPE ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #T, ;, Special, TYPE )

#	define FUBI_DECLARE_USER_TRAITS_EX( T, NAME, DEF ) \
		   FUBI_DECLARE_TRAITS_IMPL( T, #NAME, DEF, User, FuBi::GetTypeByName( #NAME ) )

#	define FUBI_DECLARE_USER_TRAITS( T, DEF ) \
		   FUBI_DECLARE_USER_TRAITS_EX( T, T, DEF )

#	define FUBI_DECLARE_XFER_TRAITS( T ) \
		   template <> struct FuBi::Traits < T > : FuBi::TraitTypeBase < T >

#	define FUBI_DECLARE_CAST_TRAITS( T, CAST )									\
	FUBI_DECLARE_XFER_TRAITS( T )												\
	{																			\
		static void ToString( gpstring& out, const Type& obj,					\
							  eXfer xfer = XFER_NORMAL )						\
			{  out = ::ToString( *(CAST*)&obj, xfer );  }						\
		static bool FromString( const char* str, Type& obj )					\
			{  return ( ::FromString( str, *(CAST*)&obj ) );  }					\
																				\
		static bool XferDef( PersistContext& persist, eXfer xfer,				\
							 const char* key, Type& obj )						\
		{																		\
			return ( persist.Xfer( xfer, key, *(CAST*)&obj ) );					\
		}																		\
	}

#   define FUBI_DECLARE_PAIR_TRAITS( T )										\
	FUBI_DECLARE_XFER_TRAITS( T )												\
	{																			\
		static bool XferDef( PersistContext& persist, eXfer xfer,				\
							 const char* key, Type& obj )						\
		{																		\
			return ( persist.XferPair( xfer, key, obj ) );						\
		}																		\
	}

#	define FUBI_DECLARE_SELF_TRAITS( T )										\
	template <> struct FuBi::Traits < T > : FuBi::TraitBase < T >				\
	{																			\
		static bool XferDef( PersistContext& persist, eXfer /*xfer*/,			\
							 const char* key, Type& obj )						\
		{																		\
			bool enter = key != PersistContext::VALUE_STR;						\
			bool success = (!enter || persist.EnterBlock( key ))				\
						   && obj.Xfer( persist );								\
			if ( enter )														\
			{																	\
				persist.LeaveBlock();											\
			}																	\
			return ( success );													\
		}																		\
	};

// Helper base classes.

    template <typename T>
    struct TraitTypeBase : Trait
    {
		typedef T Type;

		static int    GetSize ( void )  {  return ( sizeof( T ) );  }
		static eFlags GetFlags( void )  {  return ( FLAG_NONE );  }
    };

	template <typename T>
	struct TraitBase : TraitTypeBase <T>
	{
		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
		{
			// $$ this if/else could be optimized away using a little bit of
			//    metaprogramming.

			if (    persist.AsText()
				 || (Traits <T>::GetFlags() & FLAG_FORCETEXT)
				 || !(Traits <T>::GetFlags() & FLAG_POD) )
			{
				return ( persist.XferAsText( xfer, key, obj, defValue, Traits <T>::GetVarType() ) );
			}
			else
			{
				return ( persist.XferAsBinary( key, obj, defValue ) );
			}
		}

		static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, T& obj )
		{
			return ( Traits <T>::Xfer( persist, xfer, key, obj, Traits <T>::GetDefaultValue() ) );
		}
	};

	template <typename T>
	struct EnumTraitBase : TraitBase <T>
	{
		static eFlags GetFlags( void )
			{  return ( FLAG_FORCETEXT | FLAG_POD );  }
		static void ToString( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
			{  gpassert( !IsMemoryBadFood( (DWORD)obj ) );  out = ::ToString( obj );  }
		static bool FromString( const char* str, Type& obj )
			{  return ( ::FromString( str, obj ) );  }

		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
			{  return ( persist.XferAsText( xfer, key, obj, defValue, VAR_UNKNOWN ) );  }
	};

	template <typename T>
	struct BitfieldEnumTraitBase : TraitBase <T>
	{
		static eFlags GetFlags( void )
			{  return ( FLAG_FORCETEXT | FLAG_POD );  }
		static void ToString( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
			{  gpassert( !IsMemoryBadFood( (DWORD)obj ) );  out = ::ToFullString( obj );  }
		static bool FromString( const char* str, Type& obj )
			{  return ( ::FromFullString( str, obj ) );  }

		template <typename U>
		static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
		{
			gpstring temp;

			if ( persist.IsSaving() )
			{
				temp = ::ToFullString( obj );
			}

			bool success = persist.XferString( xfer, key, temp, VAR_UNKNOWN );

			if ( persist.IsRestoring() )
			{
				if ( success )
				{
					success = ::FromFullString( temp, obj );
					if ( !success )
					{
#						if !GP_RETAIL
						persist.ReportConversionError( temp, key );
#						endif // !GP_RETAIL
					}
				}

				if ( !success )
				{
					obj = defValue;
				}
			}

			return ( success );
		}
	};

	template <typename T>
	struct VarTraitBase : TraitBase <T>
	{
		static eFlags GetFlags( void )  {  return ( FLAG_POD );  }
	};

	template <typename T>
	struct PointerClassTraitBase : VarTraitBase <T>
	{
		static void ToString( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
			{  FuBi::Traits <DWORD>::ToString( out, rcast <const DWORD&> ( obj ), XFER_HEX );  }
		static bool FromString( const char* str, Type& obj )
			{  return ( FuBi::Traits <DWORD>::FromString( str, rcast <DWORD&> ( obj ) ) );  }
	};

	template <typename T>
	struct PodTraitBase : VarTraitBase <T>
	{
	};

	template <typename T>
	struct SpecialTraitBase : TraitBase <T>
	{
	};

	template <typename T>
	struct UserTraitBase : TraitBase <T>
	{
	};

//////////////////////////////////////////////////////////////////////////////
// class XferHelper <T> declaration

// $ class requirements
//
//   for reading:
//
//      bool CLS::ReadKeyValue( [string] key, gpstring& obj );
//      bool ::FromString( [string] key, T& obj );
//
//   for writing:
//
//      bool CLS::SetKeyValue( [string] key, const gpstring& obj );
//      gpstring ::ToString( const T& obj, FuBi::eXfer xfer );

template <typename CLS>
class XferHelper
{
public:
	SET_NO_INHERITED( XferHelper <CLS> );
	typedef CLS ClassType;

	XferHelper( void )  {  }
   ~XferHelper( void )  {  }

// Reading.

	template <typename T>
	bool Read( const char* key, T& obj ) const
	{
		gpstring out;
		if ( GetClass()->ReadKeyValue( key, out ) )
		{
			if ( ::FromString( out, obj ) )
			{
				return ( true );
			}
		}

		return ( false );
	}

	template <typename T, typename U>
	bool Read( const char* key, T& obj, const U& def ) const
	{
		if ( !Read( key, obj ) )
		{
			obj = def;
			return ( false );
		}
		return ( true );
	}

	// type specific
	bool      GetBool   ( const char* key, bool def = false ) const							{  bool out;  Read( key, out, def );  return ( out );  }
	int       GetInt    ( const char* key, int def = 0 ) const								{  int out;  Read( key, out, def );  return ( out );  }
	float     GetFloat  ( const char* key, float def = 0 ) const							{  float out;  Read( key, out, def );  return ( out );  }
	gpstring  GetString ( const char* key, const gpstring& def = gpstring::EMPTY ) const	{  gpstring out;  return ( GetClass()->ReadKeyValue( key, out ) ? out : def );  }
	gpwstring GetWString( const char* key, const gpwstring& def = gpwstring::EMPTY ) const	{  gpwstring out;  Read( key, out, def );  return ( out );  }

// Writing.

	// helpers
	template <typename T>
	bool Set( const char* key, const T& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
	{
		gpstring out = ::ToString( obj, xfer );
		return ( GetClass()->SetKeyValue( key, out ) );
	}

	// type specific
	bool SetBool( const char* key, bool value, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  return ( Set( key, value, xfer ) );  }
	bool SetInt( const char* key, int value, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  return ( Set( key, value, xfer ) );  }
	bool SetFloat( const char* key, float value, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  return ( Set( key, value, xfer ) );  }
	bool SetString( const char* key, const char* value, FuBi::eXfer /*xfer*/ = FuBi::XFER_NORMAL )
		{  return ( GetClass()->SetKeyValue( key, value ) );  }
	bool SetWString( const char* key, const wchar_t* value, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  return ( Set( key, value, xfer ) );  }

private:
	const ClassType* GetClass( void ) const  {  return ( scast <const ClassType*> ( this ) );  }
	ClassType* GetClass( void )  {  return ( scast <ClassType*> ( this ) );  }

	SET_NO_COPYING( XferHelper <CLS> );
};

//////////////////////////////////////////////////////////////////////////////
// traits for standard types

// $ future: optimize these a little with _ultoa() and friends rather than
//           the much slower sprintf.

FUBI_DECLARE_VAR_TRAITS( char, VAR_CHAR )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( signed char, VAR_SCHAR )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( unsigned char, VAR_UCHAR )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( short, VAR_SHORT )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( unsigned short, VAR_USHORT )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( int, VAR_INT )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( unsigned int, VAR_UINT )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( __int64, VAR_INT64 )
{
	static const char* GetExternalVarName( void )	{  return ( "int64" );  }
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( unsigned __int64, VAR_UINT64 )
{
	static const char* GetExternalVarName( void )	{  return ( "unsigned int64" );  }
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( float, VAR_FLOAT )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_FLOAT | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( double, VAR_DOUBLE )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_FLOAT | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

// $ special handling for void type
template <> struct Traits <void> : Trait
{
	typedef void Type;

	static const char* GetInternalVarName( void )	{  return ( "void" );  }
	static const char* GetExternalVarName( void )	{  return ( "void" );  }
	static int      GetSize( void )					{  return ( 0 );  }
	static eFlags   GetFlags( void )  				{  return ( FLAG_NONE );  }
	static eVarType GetVarType( void )				{  return ( VAR_VOID );  }
};

FUBI_DECLARE_VAR_TRAITS( bool, VAR_BOOL )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( long, VAR_LONG )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( unsigned long, VAR_ULONG )
{
	static eFlags   GetFlags  ( void )				{  return ( FLAG_INTEGER | FLAG_POD );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer xfer = XFER_NORMAL );
	static bool     FromString( const char* str, Type& obj );
};

FUBI_DECLARE_VAR_TRAITS( long double, VAR_LONGDOUBLE )
{
};

//////////////////////////////////////////////////////////////////////////////
// traits for special types

FUBI_DECLARE_SPECIAL_TRAITS( mem_ptr, VAR_MEM_PTR )  {  };
FUBI_DECLARE_SPECIAL_TRAITS( const_mem_ptr, VAR_CONST_MEM_PTR )  {  };

FUBI_DECLARE_SPECIAL_TRAITS( std::gp_string, VAR_STRING )
{
	static const char* GetExternalVarName( void )	{  return ( "string" );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
													{  out = obj;  }
	static bool     FromString( const char* str, Type& obj )
													{  obj = str;  return ( true );  }
};

FUBI_DECLARE_SPECIAL_TRAITS( gpstring, VAR_STRING )
{
	static const char* GetExternalVarName( void )	{  return ( "string" );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
													{  out = obj;  }
	static bool     FromString( const char* str, Type& obj )
													{  obj = str;  return ( true );  }

	template <typename U>
	static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
	{
		bool success = persist.XferString( xfer, key, obj, VAR_STRING );
		if ( persist.IsRestoring() && !success )
		{
			obj = defValue;
		}
		return ( success );
	}
};

FUBI_DECLARE_SPECIAL_TRAITS( std::gp_wstring, VAR_WSTRING )
{
	static const char* GetExternalVarName( void )	{  return ( "wstring" );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
													{  out = ::UnicodeToUtf8( obj );  }
	static bool     FromString( const char* str, Type& obj )
													{  obj = ::Utf8ToUnicode( str ).get_std_string();  return ( true );  }
};

FUBI_DECLARE_SPECIAL_TRAITS( gpwstring, VAR_WSTRING )
{
	static const char* GetExternalVarName( void )	{  return ( "wstring" );  }
	static void     ToString  ( gpstring& out, const Type& obj, eXfer /*xfer*/ = XFER_NORMAL )
													{  out = ::UnicodeToUtf8( obj );  }
	static bool     FromString( const char* str, Type& obj )
													{  obj = ::Utf8ToUnicode( str );  return ( true );  }

	template <typename U>
	static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
	{
		bool success = persist.XferString( xfer, key, obj );
		if ( persist.IsRestoring() && !success )
		{
			obj = defValue;
		}
		return ( success );
	}
};

// FourCc helpers

	bool IntFromFourCc( const char* str, int& obj );

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// global helper declarations

// ToString().

	template <typename T>
	inline void ToString( gpstring& out, const T& obj, FuBi::eXfer xfer )
		{  FuBi::Traits <T>::ToString( out, obj, xfer );  }

	inline void ToString( gpstring& out, const wchar_t* obj, FuBi::eXfer /*xfer*/ )
		{  out = ::UnicodeToUtf8( obj );  }

	template <typename T>
	inline gpstring ToString( const T& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL )
		{  gpstring out;  FuBi::Traits <T>::ToString( out, obj, xfer );  return ( out );  }

	inline gpstring ToString( const wchar_t* obj, FuBi::eXfer /*xfer*/ = FuBi::XFER_NORMAL )
		{  return ( ::UnicodeToUtf8( obj ) );  }

	bool ToString( FuBi::eVarType type, gpstring& out, const void* obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL, FuBi::eXMode xmode = FuBi::XMODE_DEFAULT );
	bool ToString( const FuBi::TypeSpec& type, gpstring& out, const void* obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL, FuBi::eXMode xmode = FuBi::XMODE_DEFAULT );

// FromString().

	template <typename T>
	inline bool FromString( const char* str, T& obj )
		{  return ( FuBi::Traits <T>::FromString( str, obj ) );  }

	template <typename T>
	inline bool FromString( const char* str, int len, T& obj )
		{  gpstring temp( str, len );  return ( FuBi::Traits <T>::FromString( temp, obj ) );  }

	template <typename T>
	inline bool FromString( const char* str, const char* end, T& obj )
		{  gpstring temp( str, end );  return ( FuBi::Traits <T>::FromString( temp, obj ) );  }

	bool FromString( FuBi::eVarType type, const char* str, void* obj, FuBi::eXMode xmode = FuBi::XMODE_DEFAULT );
	bool FromString( const FuBi::TypeSpec& type, const char* str, void* obj, FuBi::eXMode xmode = FuBi::XMODE_DEFAULT );

//////////////////////////////////////////////////////////////////////////////

#endif  // __FUBITRAITS_H

//////////////////////////////////////////////////////////////////////////////
