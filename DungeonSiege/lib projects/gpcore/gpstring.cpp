//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpstring.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "gpstring.h"

#include "LocHelp.h"
#include "KernelTool.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// gpstring tracking implementations

#if GPSTRING_TRACKING

namespace std
{

#if GPSTRING_LOG_DELETES

static StringAColl& TrackingStringHeaderBase :: GetRetiredStringsA( void )
{
	static StringAColl s_Coll;
	return ( s_Coll );
}

static StringWColl& TrackingStringHeaderBase :: GetRetiredStringsW( void )
{
	static StringWColl s_Coll;
	return ( s_Coll );
}

static void TrackingStringHeaderBase :: RetireString( const char* str )
{
	Lock();
	int len = ::strlen( str );
	char* newStr = new char[ len + 1 ];
	::memcpy( newStr, str, len + 1 );
	GetRetiredStringsA().push_back( newStr );
	Unlock();
}

static void TrackingStringHeaderBase :: RetireString( const wchar_t* str )
{
	Lock();
	int len = ::strlen( str );
	wchar_t* newStr = new wchar_t[ len + 1 ];
	::memcpy( newStr, str, (len + 1) * 2 );
	GetRetiredStringsW().push_back( newStr );
	Unlock();
}

#endif // GPSTRING_LOG_DELETES

static kerneltool::Critical& GetStringTrackingCritical( void )
{
	static kerneltool::Critical s_Critical;
	return ( s_Critical );
}

void TrackingStringHeaderBase :: Lock( void )
{
	GetStringTrackingCritical().Enter();
}

void TrackingStringHeaderBase :: Unlock( void )
{
	GetStringTrackingCritical().Leave();
}

}

#endif // GPSTRING_TRACKING

//////////////////////////////////////////////////////////////////////////////
// localization helper implementations

bool WideCharToMultiByte( UINT codePage, gpstring& out, const wchar_t* in, int len, bool append )
{
	gpassert( in != NULL );

	// if -1 autodetect
	if ( len == -1 )
	{
		len = ::wcslen( in );
	}

	// get length of output string
	if ( len > 0 )
	{
		int outBytes = ::WideCharToMultiByte( codePage, 0, in, len, NULL, 0, NULL, NULL );

		// if valid, do the conversion
		if ( outBytes > 0 )
		{
			// get out setup
			size_t oldSize = 0;
			if ( append )
			{
				oldSize = out.length();
			}
			else
			{
				out.clear();
			}

			// alloc buffer
			out.resize( outBytes + oldSize );

			// write it
			outBytes = ::WideCharToMultiByte( codePage, 0, in, len, out.begin_split() + oldSize, outBytes, NULL, NULL );
		}

		// done
		return ( outBytes > 0 );
	}
	else
	{
		// nothing to do
		out.clear();
		return ( true );
	}
}

bool WideCharToMultiByte( UINT codePage, gpstring& out, const gpwstring& in, bool append )
{
	return ( WideCharToMultiByte( codePage, out, in, in.length(), append ) );
}

bool MultiByteToWideChar( UINT codePage, gpwstring& out, const char* in, int len, bool append )
{
	gpassert( in != NULL );

	// if -1 autodetect
	if ( len == -1 )
	{
		len = ::strlen( in );
	}

	// get length of output string
	if ( len > 0 )
	{
		int outChars = ::MultiByteToWideChar( codePage, 0, in, len, NULL, 0 );

		// if valid, do the conversion
		if ( outChars > 0 )
		{
			// get out setup
			size_t oldSize = 0;
			if ( append )
			{
				oldSize = out.length();
			}
			else
			{
				out.clear();
			}

			// alloc buffer
			out.resize( outChars + oldSize );

			// write it
			outChars = ::MultiByteToWideChar( codePage, 0, in, len, out.begin_split() + oldSize, outChars );
		}

		// done
		return ( outChars > 0 );
	}
	else
	{
		// nothing to do
		out.clear();
		return ( true );
	}
}

bool MultiByteToWideChar( UINT codePage, gpwstring& out, const gpstring& in, bool append )
{
	return ( MultiByteToWideChar( codePage, out, in, in.length(), append ) );
}

bool ToAnsi( gpstring& out, const wchar_t* in, int len, bool append )
{
	return ( WideCharToMultiByte( gpstring::GetCodePage(), out, in, len, append ) );
}

bool ToAnsi( gpstring& out, const gpwstring& in, bool append )
{
	return ( ToAnsi( out, in, in.length(), append ) );
}

bool ToUnicode( gpwstring& out, const char* in, int len, bool append )
{
	return ( MultiByteToWideChar( gpstring::GetCodePage(), out, in, len, append ) );
}

bool ToUnicode( gpwstring& out, const gpstring& in, bool append )
{
	return ( ToUnicode( out, in, in.length(), append ) );
}

gpstring ToAnsi( const wchar_t* in, int len, bool append )
{
	gpstring out;
	WideCharToMultiByte( gpstring::GetCodePage(), out, in, len, append );
	return ( out );
}

gpstring ToAnsi( const gpwstring& in, bool append )
{
	gpstring out;
	ToAnsi( out, in, in.length(), append );
	return ( out );
}

gpwstring ToUnicode( const char* in, int len, bool append )
{
	gpwstring out;
	MultiByteToWideChar( gpstring::GetCodePage(), out, in, len, append );
	return ( out );
}

gpwstring ToUnicode( const gpstring& in, bool append )
{
	gpwstring out;
	ToUnicode( out, in, in.length(), append );
	return ( out );
}

// $ UTF-8 spec is RFC 2279

gpstring UnicodeToUtf8( const wchar_t* in, int len )
{
	// $ note: this function does not handle "surrogates" (Unicode D800-DFFF)
	//         for UCS-4/UTF-16 as specified in RFC 2279

	gpstring out;

	// get length
	gpassert( in != NULL );
	if ( len == -1 )
	{
		len = ::wcslen( in );
	}

	// convert
	for ( ; len > 0 ; ++in, --len )
	{
		// calc num bytes required
		if ( *in > 0x7FF )
		{
			// 3 bytes
			out += (char)( 0xE0 | (BYTE)(*in >> 12) );
			out += (char)( 0x80 | (BYTE)((*in & 0xFFF) >> 6 ) );
			out += (char)( 0x80 | (BYTE)(*in & 0x3F) );
		}
		else if ( *in > 0x7F )
		{
			// 2 bytes
			out += (char)( 0xC0 | (BYTE)(*in >> 6) );
			out += (char)( 0x80 | (BYTE)(*in & 0x3F) );
		}
		else
		{
			// 1 byte (ANSI)
			out += (char)LOBYTE( *in );
		}
	}

	return ( out );
}

gpstring UnicodeToUtf8( const gpwstring& in )
{
	return ( UnicodeToUtf8( in, in.length() ) );
}

gpwstring Utf8ToUnicode( const char* in, int len )
{
	// $ note: this function ain't too fast, but it should be safe. it also
	//         doesn't have the ability to report errors - it just aborts on
	//         detecting an error.

	gpwstring out;

	// get length
	gpassert( in != NULL );
	if ( len == -1 )
	{
		len = ::strlen( in );
	}

	// convert
	while ( len > 0 )
	{
		int w = 0;

		// calc num bytes to read - only look for valid bit sequence
		if ( (in[ 0 ] & 0xF0) == 0xE0 )
		{
			// 3 bytes
			len -= 3;
			if ( (len >= 0) && ((in[ 1 ] & 0xC0) == 0x80) && ((in[ 2 ] & 0xC0) == 0x80) )
			{
				w =   (((unsigned char)in[ 0 ] & 0x1F) << 12)
					| (((unsigned char)in[ 1 ] & 0x3F) << 6)
					|  ((unsigned char)in[ 2 ] & 0x3F);
				in += 3;
			}
		}
		else if ( (in[ 0 ] & 0xE0) == 0xC0 )
		{
			// 2 bytes
			len -= 2;
			if ( (len >= 0) && ((in[ 1 ] & 0xC0) == 0x80) )
			{
				w =   (((unsigned char)in[ 0 ] & 0x1F) << 6)
					|  ((unsigned char)in[ 1 ] & 0x3F);
				in += 2;
			}
		}
		else if ( (in[ 0 ] & 0x80) == 0x00 )
		{
			// 1 byte (ANSI)
			len -= 1;
			if ( len >= 0 )
			{
				w = (unsigned char)in[ 0 ];
				in += 1;
			}
		}

		// add it
		if ( w != 0 )
		{
			out += (wchar_t)w;
		}
		else
		{
			break;
		}
	}

	return ( out );
}

gpwstring Utf8ToUnicode( const gpstring& in )
{
	return ( Utf8ToUnicode( in, in.length() ) );
}

// LoadString() has a bunch of bugs on Win9x, plus there is no Unicode version
// on Win9x. And it sucks to not know the right buffer size in advance. So I
// wrote these instead. Woo.

bool LoadStringW( gpwstring& out, HINSTANCE instance, UINT id, LANGID language )
{
	// $ string resources are stored in blocks of 16 - unicode format, prefixed
	//   with the size. must iterate through blocks after finding correct one.
	//   see KB Q196774 and Q20011.

	bool success = false;

	// use exe by default
	if ( instance == NULL )
	{
		instance = ::GetModuleHandle( NULL );
	}

	// find the correct block
	HRSRC rsrc = ::FindResourceEx( instance, RT_STRING, MAKEINTRESOURCE( (id / 16) + 1 ), language );
	if ( rsrc != NULL )
	{
		// load it $ NOT a "real" HGLOBAL (see docs)
		HGLOBAL res = ::LoadResource( instance, rsrc );
		if ( res != NULL )
		{
			// lock for copy $ no need to free this up later
			const wchar_t* data = rcast <const wchar_t*> ( ::LockResource( res ) );
			if ( data != NULL )
			{
				// now iterate until we find it
				for ( int i = id % 16 ; i > 0 ; --i )
				{
					// not there yet, skip the string and the preceding size
					data += 1 + *rcast <const WORD*> ( data );
				}

				// got it - now do the assignment (note: not NULL-terminated)
				out.assign( data + 1, *rcast <const WORD*> ( data ) );

				// done!
				success = true;
			}
		}
	}

	// done
	return ( success );
}

bool LoadStringA( gpstring& out, UINT codePage, HINSTANCE instance, UINT id, LANGID language )
{
	// use current ANSI code page by default
	if ( codePage == 0 )
	{
		codePage = ::GetACP();
	}

	// get it
	gpwstring temp;
	bool success = ::LoadStringW( temp, instance, id, language );
	if ( success )
	{
		// convert
		success = ::WideCharToMultiByte( codePage, out, temp );
	}

	// done
	return ( success );
}

BOOL CALLBACK EnumResLangHelperProc
	(
		HMODULE  module,	// module handle
		LPCTSTR  type,		// resource type
		LPCTSTR  name,		// resource name
		WORD     lang,		// language identifier
		LONG_PTR param		// application-defined parameter
	)
{
	EnumResLangHelperStruct* sparam = (EnumResLangHelperStruct*)param;
	gpassert( sparam->m_Proc != NULL );

	BOOL rc = FALSE;

	// easy out
	switch ( (DWORD)type )
	{
		case ( RT_STRING ):
		{
			// find the correct block
			HRSRC rsrc = ::FindResourceEx( module, type, name, lang );
			if ( rsrc != NULL )
			{
				// load it $ NOT a "real" HGLOBAL (see docs)
				HGLOBAL res = ::LoadResource( module, rsrc );
				if ( res != NULL )
				{
					// lock for copy $ no need to free this up later
					const wchar_t* data = (const wchar_t*)::LockResource( res );
					if ( data != NULL )
					{
						// done!
						rc = TRUE;

						// now iterate
						for ( int i = 0 ; i < 16 ; ++i )
						{
							// skip the string and the preceding size
							WORD size = *(const WORD*)data;
							if ( size != 0 )
							{
								// callback
								if ( !(*sparam->m_Proc)( module, type, MAKEINTRESOURCE( ((DWORD)(name - 1) * 16) + i ), lang, sparam->m_Param ) )
								{
									rc = FALSE;
									break;
								}
							}
							data += 1 + size;
						}
					}
				}
			}
		}
		break;

		default:
		{
			rc = (*sparam->m_Proc)( module, type, name, lang, sparam->m_Param );
		}
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// class gpbstring_base implementation

UINT      gpbstring_base::ms_CodePage          = CP_ACP;
HINSTANCE gpbstring_base::ms_SystemInstance    = NULL;
HINSTANCE gpbstring_base::ms_LocalizedInstance = NULL;
LANGID    gpbstring_base::ms_Language          = 0;

//////////////////////////////////////////////////////////////////////////////
// class gpstring implementation

bool gpstring :: load( UINT codePage, HINSTANCE instance, UINT id, LANGID lang )
{
	// try specific language
	bool success = LoadStringA( *this, codePage, instance, id, lang );
	if ( !success )
	{
		// try user's default language
		LANGID userLang = LocMgr::GetDefaultLanguage();
		if ( userLang != lang )
		{
			success = LoadStringA( *this, codePage, instance, id, userLang );
		}

		// try neutral language
		if ( !success && (lang != 0) && (userLang != 0) )
		{
			success = LoadStringA( *this, codePage, instance, id, 0 );
		}

		// always clear on failure
		if ( !success )
		{
			clear();
		}
	}

	// done
	return ( success );
}

bool gpstring :: load( UINT id )
{
	return ( load_system( id ) );
}

bool gpstring :: load_system( UINT id )
{
	return ( load( CP_ACP, GetSystemInstance(), id, 0 ) );
}

bool gpstring :: load_screen( UINT id )
{
	return ( load( GetCodePage(), GetLocalizedInstance(), id, GetLanguage() ) );
}

void gpstring :: to_lower( void )
{
	if ( !empty() )
	{
		_strlwr( begin_split() );
	}
}

void gpstring :: to_upper( void )
{
	if ( !empty() )
	{
		_strupr( begin_split() );
	}
}

bool gpstring :: is_lower( void ) const
{
	return ( std::find_if( begin(), end(), isupper ) == end() );
}

bool gpstring :: is_upper( void ) const
{
	return ( std::find_if( begin(), end(), islower ) == end() );
}

gpstring& gpstring :: appendf( const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( append( printer, printer.length() ) );
}

gpstring& gpstring :: assignf( const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( assign( printer, printer.length() ) );
}

gpstring& gpstring :: insertf( size_type p0, const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( insert( p0, printer, printer.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class gpwstring implementation

bool gpwstring :: load( UINT /*codePage*/, HINSTANCE instance, UINT id, LANGID lang )
{
	// try specific language
	bool success = LoadStringW( *this, instance, id, lang );
	if ( !success )
	{
		// try user's default language
		LANGID userLang = LocMgr::GetDefaultLanguage();
		if ( userLang != lang )
		{
			success = LoadStringW( *this, instance, id, userLang );
		}

		// try neutral language
		if ( !success && (lang != 0) && (userLang != 0) )
		{
			success = LoadStringW( *this, instance, id, 0 );
		}

		// always clear on failure
		if ( !success )
		{
			clear();
		}
	}

	// done
	return ( success );
}

bool gpwstring :: load( UINT id )
{
	return ( load_screen( id ) );
}

bool gpwstring :: load_system( UINT id )
{
	return ( load( 0, GetSystemInstance(), id, GetLanguage() ) );
}

bool gpwstring :: load_screen( UINT id )
{
	return ( load( 0, GetLocalizedInstance(), id, GetLanguage() ) );
}

void gpwstring :: to_lower( void )
{
	if ( !empty() )
	{
		_wcslwr( begin_split() );
	}
}

void gpwstring :: to_upper( void )
{
	if ( !empty() )
	{
		_wcsupr( begin_split() );
	}
}

bool gpwstring :: is_lower( void ) const
{
	return ( std::find_if( begin(), end(), iswupper ) == end() );
}

bool gpwstring :: is_upper( void ) const
{
	return ( std::find_if( begin(), end(), iswlower ) == end() );
}

gpwstring& gpwstring :: appendf( const wchar_t* format, ... )
{
	auto_dynamic_vsnwprintf printer( format, va_args( format ) );
	return ( append( printer, printer.length() ) );
}

gpwstring& gpwstring :: assignf( const wchar_t* format, ... )
{
	auto_dynamic_vsnwprintf printer( format, va_args( format ) );
	return ( assign( printer, printer.length() ) );
}

gpwstring& gpwstring :: insertf( size_type p0, const wchar_t* format, ... )
{
	auto_dynamic_vsnwprintf printer( format, va_args( format ) );
	return ( insert( p0, printer, printer.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
// helper functions

gpstring gpstringf( const char* format, ... )
{
	auto_dynamic_vsnprintf printer( format, va_args( format ) );
	return ( gpstring().assign( printer, printer.length() ) );
}

gpwstring gpwstringf( const wchar_t* format, ... )
{
	auto_dynamic_vsnwprintf printer( format, va_args( format ) );
	return ( gpwstring().assign( printer, printer.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
