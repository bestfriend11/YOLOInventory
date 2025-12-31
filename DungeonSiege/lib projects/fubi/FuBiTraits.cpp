//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiTraits.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiTraits.h"

#include "FuBi.h"
#include "GpMath.h"
#include "StringTool.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helper method implementations

bool IsNumber( const char* num )
{
	gpassert( num != NULL );

	for ( ; *num != '\0' ; ++num )
	{
		switch ( *num )
		{
			case ( '0' ):
			case ( '1' ):
			case ( '2' ):
			case ( '3' ):
			case ( '4' ):
			case ( '5' ):
			case ( '6' ):
			case ( '7' ):
			case ( '8' ):
			case ( '9' ):
			{
				return ( true );
			}

			case ( '.'  ):
			case ( '+'  ):
			case ( '-'  ):
			case ( ' '  ):
			case ( '\n' ):
			case ( '\t' ):
			case ( '\v' ):
			case ( '\r' ):
			case ( '\f' ):
			{
				// $ w/s or prefix - advance and continue
			}	break;

			default:
			{
				// failed!
				return ( false );
			}
		}
	}

	// empty string ok
	return ( true );
}

template <typename T> bool IntFromString( const char* str, T& obj )
{
	int err = 0;
	long val = stringtool::strtol( str, NULL, 0, &err );

	if ( err || ((val < (long)std::numeric_limits <T>::min()) || (val > (long)std::numeric_limits <T>::max())) )
	{
		return ( false );
	}
	else
	{
		if ( (val == 0) && !IsNumber( str ) )
		{
			return ( false );
		}
		obj = scast <T> ( val );
		return ( true );
	}
}

void AppendEscapedChar( gpstring& out, char c, bool forceHex )
{
	if ( !forceHex )
	{
		if ( c == '\0' )
		{
			out += '\\';
			out += '0';
			return;
		}
		else
		{
			char esc = stringtool::GetEscapeCharacterLetter( c );
			if ( esc != '\0' )
			{
				out += '\\';
				out += esc;
				return;
			}
			else if ( c == '\'' )
			{
				out += '\\';
				out += '\'';
				return;
			}
			else if ( !::iscntrl( c ) )
			{
				out += c;
				return;
			}
		}
	}

	out.appendf( "\\x%02x", c );
}

bool GetUnescapedChar( const char*& str, char& obj )
{
	bool success = true;

	// test for escape
	if ( *str == '\\' )
	{
		++str;

		// test for hex
		if ( *str == 'x' )
		{
			// extract it - this code is based on yyScanner::MapEscapes()
			for ( int n = 1, v = 0 ; n <= 2 ; ++n )
			{
				int c = *++str;
				if ( (*str >= '0') && (*str <= '9') )
				{
					c -= '0';
				}
				else if ( (c >= 'a') && (c <= 'f') )
				{
					c = (c - 'a') + 10;
				}
				else if ( (c >= 'A') && (c <= 'F') )
				{
					c = (c - 'A') + 10;
				}
				else
				{
					break;
				}
				v = (v * 16) + c;
			}

			if ( n == 1 )
			{
				// no higits, it's ok
				obj = 'x';
			}
			else
			{
				if ( v > 255 )
				{
					gpwarningf(( "Hex constant 0x%X too large for char", v ));
					success = false;
					v = 255;
				}
				obj = scast <char> ( v );
			}
		}
		else if ( *str == '\'' )
		{
			obj = '\'';
		}
		else if ( *str == '0' )
		{
			obj = '\0';
		}
		else
		{
			// normal escape
			success = (obj = stringtool::GetEscapeCharacter( *str )) != '\0';
		}
	}
	else
	{
		// easy
		obj = *str;
	}

	++str;
	return ( success );
}

void AppendFourCcString( gpstring& out, int c )
{
	// prefix
	out += '\'';

	// local buffer to store it in (auto reverse for human readable)
	char buffer[ 5 ];
	*(int*)buffer = REVERSE_FOURCC( c );
	buffer[ 4 ] = '\0';

	// write them out
	bool skipping = true;
	for ( const char* i = buffer ; i < &ELEMENT_LAST( buffer ) ; ++i )
	{
		if ( (*i != '\0') || !skipping )
		{
			skipping = false;
			AppendEscapedChar( out, *i );
		}
	}

	// postfix
	out += '\'';
}

bool IntFromFourCc( const char* str, int& obj )
{
	// skip leading quote
	if ( *str == '\'' )
	{
		++str;
	}

	// setup
	obj = 0;
	char* ptr = rcast <char*> ( &obj );

	// retrieve until (a) no more, (b) unescaped single quote, or (c) \0
	for ( int i = 0 ; (i < 4) && (*str != '\0') && (*str != '\'') ; ++i )
	{
		obj <<= 8;
		if ( !GetUnescapedChar( str, *ptr ) )
		{
			return ( false );
		}
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// traits for standard types

template <> void Traits <char> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out += '\'';
	AppendEscapedChar( out, obj, xfer == XFER_HEX );
	out += '\'';
}

template <> bool Traits <char> :: FromString( const char* str, Type& obj )
{
	// skip leading quote
	if ( *str == '\'' )
	{
		++str;
	}

	// take it
	return ( GetUnescapedChar( str, obj ) );
}

template <> void Traits <signed char> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	Traits <char>::ToString( out, obj, xfer );
}

template <> bool Traits <signed char> :: FromString( const char* str, Type& obj )
{
	return ( Traits <char>::FromString( str, rcast <char&> ( obj ) ) );
}

template <> void Traits <unsigned char> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%02x" : "%u", obj );
}

template <> bool Traits <unsigned char> :: FromString( const char* str, Type& obj )
{
	return ( IntFromString( str, obj ) );
}

template <> void Traits <short> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%04x" : "%d", obj );
}

template <> bool Traits <short> :: FromString( const char* str, Type& obj )
{
	return ( IntFromString( str, obj ) );
}

template <> void Traits <unsigned short> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%04x" : "%u", obj );
}

template <> bool Traits <unsigned short> :: FromString( const char* str, Type& obj )
{
	return ( IntFromString( str, obj ) );
}

template <> void Traits <int> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	if ( xfer == XFER_FOURCC )
	{
		AppendFourCcString( out, obj );
	}
	else
	{
		out.appendf( (xfer == XFER_HEX) ? "0x%08X" : "%d", obj );
	}
}

template <> bool Traits <int> :: FromString( const char* str, Type& obj )
{
	if ( *str == '\'' )
	{
		return ( IntFromFourCc( str, obj ) );
	}
	else
	{
		int err = 0;
		obj = stringtool::strtol( str, NULL, 0, &err );
		return ( !err && ((obj != 0) || IsNumber( str )) );
	}
}

template <> void Traits <unsigned int> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	if ( xfer == XFER_FOURCC )
	{
		AppendFourCcString( out, obj );
	}
	else
	{
		out.appendf( (xfer == XFER_HEX) ? "0x%08X" : "%u", obj );
	}
}

template <> bool Traits <unsigned int> :: FromString( const char* str, Type& obj )
{
	if ( *str == '\'' )
	{
		return ( IntFromFourCc( str, rcast <int&> ( obj ) ) );
	}
	else
	{
		int err = 0;
		obj = stringtool::strtoul( str, NULL, 0, &err );
		return ( !err && ((obj != 0) || IsNumber( str )) );
	}
}

template <> void Traits <__int64> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%016I64x" : "%I64d", obj );
}

template <> bool Traits <__int64> :: FromString( const char* str, Type& obj )
{
	bool hex = ( (str[ 0 ] == '0') && ((str[ 1 ] == 'x') || (str[ 1 ] == 'X')) );
	return ( ::sscanf( str, hex ? "%I64x" : "%I64d", &obj ) == 1 );
}

template <> void Traits <unsigned __int64> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	out.appendf( (xfer == XFER_HEX) ? "0x%016I64x" : "%I64u", obj );
}

template <> bool Traits <unsigned __int64> :: FromString( const char* str, Type& obj )
{
	bool hex = ( (str[ 0 ] == '0') && ((str[ 1 ] == 'x') || (str[ 1 ] == 'X')) );
	return ( ::sscanf( str, hex ? "%I64x" : "%I64u", &obj ) == 1 );
}

template <> void Traits <float> :: ToString( gpstring& out, const Type& obj, eXfer /*xfer*/ )
{
	gpassert( !IsMemoryBadFood( *rcast <const DWORD*> ( &obj ) ) );
	DEBUG_FLOAT_VALIDATE( obj );
	if ( obj == FLOAT_MAX )
	{
		out.append( "float_max" );
	}
	else if ( obj == -FLOAT_MAX )
	{
		out.append( "-float_max" );
	}
	else
	{
		out.appendf( "%g", obj );
	}
}

template <> bool Traits <float> :: FromString( const char* str, Type& obj )
{
	int err = 0;
	const char* end = NULL;
	obj = scast <float> ( stringtool::strtod( str, &end, &err ) );

	if ( (err == 0) && ((obj != 0) || (end != str)) )
	{
		return ( true );
	}
	else if ( ::same_no_case( str, "float_max" ) )
	{
		obj = FLOAT_MAX;
		return ( true );
	}
	else if ( ::same_no_case( str, "-float_max" ) )
	{
		obj = -FLOAT_MAX;
		return ( true );
	}
	else
	{
		return ( false );
	}
}

template <> void Traits <double> :: ToString( gpstring& out, const Type& obj, eXfer /*xfer*/ )
{
	gpassert( !IsMemoryBadFood( *rcast <const DWORD*> ( &obj ) ) );
	DEBUG_FLOAT_VALIDATE( obj );
	if ( obj == DBL_MAX )
	{
		out.append( "dbl_max" );
	}
	else if ( obj == -DBL_MAX )
	{
		out.append( "-dbl_max" );
	}
	else
	{
		out.appendf( "%g", obj );
	}
}

template <> bool Traits <double> :: FromString( const char* str, Type& obj )
{
	int err = 0;
	const char* end = NULL;
	obj = stringtool::strtod( str, &end, &err );

	if ( (err == 0) && ((obj != 0) || (end != str)) )
	{
		return ( true );
	}
	else if ( ::same_no_case( str, "dbl_max" ) )
	{
		obj = DBL_MAX;
		return ( true );
	}
	else if ( ::same_no_case( str, "-dbl_max" ) )
	{
		obj = -DBL_MAX;
		return ( true );
	}
	else
	{
		return ( false );
	}
}

template <> void Traits <bool> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	gpassert( (obj == true) || (obj == false) );		// catch uninitialized vars

	if ( xfer == XFER_HEX )
	{
		out.appendf( "0x%x", obj );
	}
	else if ( out.empty() )
	{
		// $ special optimization: we do a lot of xfer's of bools so this will
		//   avoid the alloc/dealloc on the temporary by using ref counting.

		static gpstring s_True( "true" ), s_False( "false" );
		out = obj ? s_True : s_False;
	}
	else
	{
		out.append( obj ? "true" : "false" );
	}
}

template <> bool Traits <bool> :: FromString( const char* str, Type& data )
{
	bool success = false;
	switch ( *str )
	{
		case ( 'y' ):  case ( 'Y' ):  case ( 't' ):  case ( 'T' ):
		{
			data = true;
			success = true;
		}	break;

		case ( 'n' ):  case ( 'N' ):  case ( 'f' ):  case ( 'F' ):
		{
			data = false;
			success = true;
		}	break;

		case ( 'o' ):  case ( 'O' ):
		{
			if ( str[ 1 ] == 'n' )
			{
				data = true;
				success = true;
			}
			else if ( str[ 1 ] == 'f' )
			{
				data = false;
				success = true;
			}
			else
			{
				success = false;
			}
		}	break;

		case ( '0' ):  case ( '1' ):  case ( '2' ):  case ( '3' ):  case ( '4' ):
		case ( '5' ):  case ( '6' ):  case ( '7' ):  case ( '8' ):  case ( '9' ):
		{
			int i;
			if ( ::FromString( str, i ) )
			{
				data = ( i == 0 ) ? false : true;
				success = true;
			}
		}	break;
	}
	return ( success );
}

template <> void Traits <long> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	Traits <int>::ToString( out, obj, xfer );
}

template <> bool Traits <long> :: FromString( const char* str, Type& obj )
{
	return ( Traits <int>::FromString( str, rcast <int&> ( obj ) ) );
}

template <> void Traits <unsigned long> :: ToString( gpstring& out, const Type& obj, eXfer xfer )
{
	Traits <unsigned int>::ToString( out, obj, xfer );
}

template <> bool Traits <unsigned long> :: FromString( const char* str, Type& obj )
{
	return ( Traits <unsigned int>::FromString( str, rcast <unsigned int&> ( obj ) ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// global helper implementations

bool ToString( FuBi::eVarType type, gpstring& out, const void* obj, FuBi::eXfer xfer, FuBi::eXMode xmode )
	{  return ( gFuBiSysExports.ToString( type, out, obj, xfer, xmode ) );  }
bool ToString( const FuBi::TypeSpec& type, gpstring& out, const void* obj, FuBi::eXfer xfer, FuBi::eXMode xmode )
	{  return ( gFuBiSysExports.ToString( type, out, obj, xfer, xmode ) );  }
bool FromString( FuBi::eVarType type, const char* str, void* obj, FuBi::eXMode xmode )
	{  return ( gFuBiSysExports.FromString( type, str, obj, xmode ) );  }
bool FromString( const FuBi::TypeSpec& type, const char* str, void* obj, FuBi::eXMode xmode )
	{  return ( gFuBiSysExports.FromString( type, str, obj, xmode ) );  }

//////////////////////////////////////////////////////////////////////////////
