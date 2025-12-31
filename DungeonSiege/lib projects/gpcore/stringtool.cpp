//////////////////////////////////////////////////////////////////////////////
//
// File     :  StringTool.cpp
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "StringTool.h"

#include "FileSysUtils.h"
#include "StdHelp.h"

#include <objbase.h>
#include <algorithm>
#include <cmath>

#define DOUBLE DOUBLE

#define _CRTBLD
#include "crtsrc\fltintrn.h"
#include "crtsrc\cruntime.h"

#define STRINGTOOLA
#include "StringToolAW.cpp"
#undef STRINGTOOLA
#define STRINGTOOLW
#include "StringToolAW.cpp"
#undef STRINGTOOLW

//////////////////////////////////////////////////////////////////////////////
// string.h extensions

// adapted from clever strspn.c in the VC++ CRT

inline void fill_control_map( unsigned char map[ 32 ], const char* control )
{
	// clear it
	::ZeroMemory( map, 32 );

	// init it
	for (  const unsigned char* ctrl = rcast <const unsigned char*> (control)
		 ; *ctrl != '\0' ; ++ctrl )
	{
		map[ *ctrl >> 3 ] |= (1 << (*ctrl & 7));
	}
}

size_t strrcspn( const char* string, const char* control )
{
	gpassert( (string != NULL) && (control != NULL) );

	int count = ::strlen( string ) - 1;
	const unsigned char* str = rcast <const unsigned char*> (string) + count;

	unsigned char map[ 32 ];
	fill_control_map( map, control );

	// 1st char in control map stops search
	while ( (count >= 0) && !(map[ *str >> 3 ] & (1 << (*str & 7))) )
	{
		--str;
		--count;
	}

	return ( count );
}

size_t wcsrcspn( const wchar_t* string, const wchar_t* control )
{
	gpassert( (string != NULL) && (control != NULL) );

	int count = ::wcslen( string ) - 1;
	const wchar_t* str = string + count;

	// 1st char in control map stops search
	while ( count >= 0 )
	{
		for ( const wchar_t* i = control ; *i ; ++i )
		{
			if ( *i == *str )
			{
				return ( count );
			}
		}

		--str;
		--count;
	}

	return ( count );
}

size_t strncspn( const char* string, const char* control, size_t count )
{
	// $$$ update this function to only do the control map method if the length
	//     of control and/or count is of a size where it makes sense.

	gpassert( (string != NULL) && (control != NULL) );

	if ( count == -1 )
	{
		count = strlen( string );
	}

	int offset = 0;
	const unsigned char* str = rcast <const unsigned char*> ( string );

	unsigned char map[ 32 ];
	fill_control_map( map, control );

	// 1st char in control map stops search
	while ( count && (!(map[ *str >> 3 ] & (1 << (*str & 7)))) )
	{
		--count;
		++offset;
		++str;
	}
	return ( offset );
}

size_t strncspn_not( const char* string, const char* control, size_t count )
{
	gpassert( (string != NULL) && (control != NULL) );

	int offset = 0;
	const unsigned char* str = rcast <const unsigned char*> ( string );

	unsigned char map[ 32 ];
	fill_control_map( map, control );

	// 1st char not in control map stops search
	while ( count && (map[ *str >> 3 ] & (1 << (*str & 7))) )
	{
		--count;
		++offset;
		++str;
	}
	return ( offset );
}

char* strnchr( const char* string, char c, size_t count )
{
	for ( ; *string && count ; ++string, --count )
	{
		if ( *string == c )
		{
			return ( ccast <char*> ( string ) );
		}
	}
	return ( NULL );
}

bool advance_snprintf( char*& buffer, int& count, const char* format, ... )
{
	// early bailouts
	if ( count <= 1 )
	{
		if ( count == 1 )
		{
			// terminate
			*buffer = '\0';
		}
		return ( false );
	}

	// output to the buffer and handle error
	int advance = ::_vsnprintf( buffer, count, format, va_args( format ) );
	if ( advance < 0 )
	{
		buffer[ count - 1 ] = '\0';
		buffer += count - 1;
		count = 1;
		return ( false );
	}

	// success
	buffer += advance;
	count -= advance;
	return ( true );
}

void* memrchr( const void* buf, int c, size_t count )
{
	void* end = (BYTE*)buf + count;
	while ( end != buf )
	{
		end = (BYTE*)end - 1;
		if ( *(BYTE*)end == (BYTE)c )
		{
			return ( end );
		}
	}

	return ( NULL );
}

void* memrchr_not( const void* buf, int c, size_t count )
{
	void* end = (BYTE*)buf + count;
	while ( end != buf )
	{
		end = (BYTE*)end - 1;
		if ( *(BYTE*)end != (BYTE)c )
		{
			return ( end );
		}
	}

	return ( NULL );
}

char* strlwr_copy( char* dst, const char* src )
{
	gpassert( dst != NULL );
	gpassert( src != NULL );

	for ( ; *src != '\0' ; ++src, ++dst )
	{
		*dst = (char)tolower( *src );
	}
	*dst = '\0';

	return ( dst );
}

char* strupr_copy( char* dst, const char* src )
{
	gpassert( dst != NULL );
	gpassert( src != NULL );

	for ( ; *src != '\0' ; ++src, ++dst )
	{
		*dst = (char)toupper( *src );
	}
	*dst = '\0';

	return ( dst );
}

//////////////////////////////////////////////////////////////////////////////

namespace stringtool  {  // begin of namespace stringtool

//////////////////////////////////////////////////////////////////////////////

const char* ALL_WHITESPACE = " \n\t\v\r\f";

//////////////////////////////////////////////////////////////////////////////


int CopyString( char* dest, const char* src, int maxDestChars, int srcLength )
{
	// $ this does about the same thing as strncpy except simpler. unlike
	//   strncpy, it will _always_ leave room for the terminating null, and it
	//   will _not_ unnecessarily pad the end with nulls.

	gpassert( (dest != NULL) && (src != NULL) );
	gpassert( maxDestChars >= 0 );
	gpassert( srcLength >= -1 );

	// $ early out if nothing to do!
	if ( maxDestChars == 0 )
	{
		return ( 0 );
	}

	// autodetect string length
	if ( srcLength == -1 )
	{
		srcLength = ::strlen( src );
	}

	// clamp max size
	::minimize( srcLength, maxDestChars - 1 );

	// copy what fits then add null term
	::memcpy( dest, src, srcLength );
	dest[ srcLength ] = '\0';

	// return length
	return ( srcLength );
}


int CopyString( wchar_t* dest, const wchar_t* src, int maxDestChars, int srcLength )
{
	// $ this does about the same thing as strncpy except simpler. unlike
	//   strncpy, it will _always_ leave room for the terminating null, and it
	//   will _not_ unnecessarily pad the end with nulls.

	gpassert( (dest != NULL) && (src != NULL) );
	gpassert( maxDestChars > 0 );
	gpassert( srcLength >= -1 );

	// autodetect string length
	if ( srcLength == -1 )
	{
		srcLength = ::wcslen( src );
	}
	// add null term
	++srcLength;

	if ( srcLength > maxDestChars )
	{
		// copy what fits then add null term
		::memcpy( dest, src, (maxDestChars - 1) * 2 );
		dest[ maxDestChars - 1 ] = L'\0';

		// return length minus null term
		return ( maxDestChars - 1 );
	}
	else
	{
		// copy it exactly
		::memcpy( dest, src, srcLength * 2 );

		// return length minus null term
		return ( srcLength - 1 );
	}
}


void PadFrontToLength( gpstring & in, char pad_value, unsigned int to_length )
{
	int prepend_len = to_length - in.size();
	if( prepend_len > 0 )
	{
		in.insert( in.begin_split(), prepend_len, pad_value );
	}
}


void PadBackToLength( gpstring & in, char pad_value, unsigned int to_length )
{
	int append_len = to_length - in.size();
	if( append_len > 0 )
	{
		in.append( append_len, pad_value );
	}
}


void PadBack( gpstring & in, const char* pad_with, unsigned int pad_iterations )
{
	gpassert( pad_with && *pad_with );
	for ( unsigned int i=0 ; i < pad_iterations ; ++i )
	{
		in += pad_with;
	}
}


void RemoveLeadingWhiteSpace( const char*& ib, const char* ie )
{
	gpassert( ib <= ie );

	while ( (ib < ie) && ::isspace( *ib ) )
	{
		++ib;
	}
}


void RemoveTrailingWhiteSpace( const char* ib, const char*& ie )
{
	gpassert( ib <= ie );

	while ( (ie > ib) && ::isspace( ie[ -1 ] ) )
	{
		--ie;
	}
}


void RemoveBorderingWhiteSpace( const char*& ib, const char*& ie )
{
	RemoveLeadingWhiteSpace( ib, ie );
	RemoveTrailingWhiteSpace( ib, ie );
}


void ReplaceAllChars( gpstring & in_out, char replace, char replace_with )
{
	std::replace( in_out.begin_split(), in_out.end_split(), replace, replace_with );
}


bool ReplaceRootPath( gpstring const & path, gpstring const & oldRoot, gpstring const & newRoot, gpstring & newPath )
{
	gpassert( &path != &newPath );

	if( !oldRoot.empty() )
	{
		if( IsAbsolutePath( oldRoot ) )
		{
			gpstring::size_type pos = path.find_first_of( oldRoot );
			if( pos != gpstring::npos )
			{
				newPath.append( newRoot );
				newPath.append( path, oldRoot.size(), path.size() );
				return true;
			}
		}
		gpassertm( 0, "path parse error" );
		return false;
	}
	else
	{
		newPath.append( newRoot );
		newPath.append( path );
		return true;
	}
}


bool IsAbsolutePath( const char * path )
{
	return ( FileSys::IsAbsolute( FileSys::GetPathType( path ) ) );
}


bool Get( const char * in, unsigned int & out )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	out = strtoul( in, NULL, 0 );
	return ( true );
}


bool Get( const char * in, int & out )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	out = strtol( in, NULL, 0 );
	return ( true );
}


bool Get( const char * in, unsigned long & out )
{
	return ( Get( in, (unsigned int&)out ) );
}


bool Get( const char * in, long & out )
{
	return ( Get( in, (int&)out ) );
}


bool Get( const char * in, double & out )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	out = strtod( in );
	return ( true );
}


bool Get( const char * in, float & out )
{
	double temp;
	bool rc = Get( in, temp );
	if ( rc )
	{
		out = (float)temp;
	}
	return ( rc );
}


bool Get( const char * in, bool & out )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	if( stricmp( in, "true" ) == 0 )
	{
		out = true;
		return ( true );
	}
	else if( stricmp( in, "false" ) == 0 )
	{
		out = false;
		return ( true );
	}
	else
	{
		gpassertm( 0, "Invalid input." );
		return( false );
	}
}


bool Get( const char * in, __int64 & int64 )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	int64 = _atoi64( in );
	return( true );
}

bool Get( const char * in, gpstring & out )
{
	gpassert( in != NULL );
	gpassert( in[ strncspn_not( in, ALL_WHITESPACE ) ] != '\0' );

	out = in;
	RemoveBorderingWhiteSpace( out );
	return ( true );
}


bool Get( const wchar_t * in, unsigned int & out )
{
	gpassert( in != NULL );

	out = wcstoul( in, NULL, 0 );
	return ( true );
}


bool Get( const wchar_t * in, int & out )
{
	gpassert( in != NULL );

	out = wcstol( in, NULL, 0 );
	return ( true );
}


bool Get( const wchar_t * in, unsigned long & out )
{
	return Get( in, (unsigned int&)out );
}


bool Get( const wchar_t * in, long & out )
{
	return Get( in, (int&)out );
}


bool Get( const wchar_t * in, double & out )
{
	gpassert( in != NULL );

	out = wcstod( in, NULL );
	return (true);
}


bool Get( const wchar_t * in, float & out )
{
	double temp;
	bool rc = Get( in, temp );
	if ( rc )
	{
		out = (float)temp;
	}
	return ( rc );
}


bool Get( const wchar_t * in, bool & out )
{
	gpassert( in != NULL );

	if( _wcsicmp( in, L"true" ) == 0 )
	{
		out = true;
		return( true );
	}
	else if( _wcsicmp( in, L"false" ) == 0 )
	{
		out = false;
		return( true );
	}
	else
	{
		gpassertm( 0, "Invalid input." );
		return( false );
	}
}


bool Get( const wchar_t * in, __int64 & int64 )
{
	return( Get( ::ToAnsi( in ), int64 ) );
}


bool Get( const wchar_t * in, gpwstring & out )
{
	out = in;
	return true;
}


bool GetDelimitedValue( const char* in, char delimiter, unsigned int desired_delimited, const char*& begin, const char*& end )
{
	// skip if empty
	if ( !in || !*in )
	{
		return ( false );
	}

	// get the value
	unsigned int current_delimited = 0;
	const char* iter = in;
	begin = end = iter;
	for ( ; ; )
	{
		if ( *iter == '\0' )
		{
			gperror( "Couldn't get delimited value." );
			return ( false );
		}

		begin = iter;
		end = NextField( iter, delimiter );

		if ( current_delimited == desired_delimited )
		{
			break;
		}

		++current_delimited;
	}

	// skip w/s
	while ( ::isspace( *begin ) && (begin != end) )
	{
		++begin;
	}
	while ( (end > begin) && ::isspace( end[ -1 ] ) )
	{
		--end;
	}

	// done
	return ( true );
}


bool GetDelimitedValue( const char* in, char delimiter, unsigned int desired_delimited, gpstring& out, bool removeInnerSpace )
{
	// get delim
	const char* begin, * end;
	if ( !GetDelimitedValue( in, delimiter, desired_delimited, begin, end ) )
	{
		return ( false );
	}

	// copy in chunks, removing internal w/s as we go
	if ( removeInnerSpace )
	{
		out.clear();
		bool adding = true;
		for ( const char* iter = begin ; iter < end ; ++iter )
		{
			if ( ::isspace( *iter ) )
			{
				if ( adding )
				{
					if ( iter != begin )
					{
						out.append( begin, iter - begin );
					}
					adding = false;
				}
			}
			else if ( !adding )
			{
				begin = iter;
				adding = true;
			}
		}

		// add remaining
		if ( adding )
		{
			out.append( begin, iter - begin );
		}
	}
	else
	{
		out.assign( begin, end );
	}

	// done
	return ( true );
}


unsigned int GetNumDelimitedStrings( const char* in, char delimiter )
{
	unsigned int count = 0;

	if ( in && *in )
	{
		++count;
		while ( in && *in )
		{
			in = strchr( in, delimiter );
			if ( in != NULL )
			{
				++in;
				++count;
			}
		}
	}

	return ( count );
}


bool GetBackDelimitedString( const char* in, char delimiter, gpstring & out )
{
	unsigned int numStrings = GetNumDelimitedStrings( in, delimiter );
	if ( numStrings == 0 )
	{
		return ( false );
	}

	return ( GetDelimitedValue( in, delimiter, numStrings - 1, out ) );
}


bool RemoveFrontDelimitedString( gpstring const & in, char delimiter, gpstring & out )
{
	if ( in.empty() )
	{
		return false;
	}

	gpstring::size_type pos = in.find_first_of( delimiter );
	if ( pos != gpstring::npos )
	{
		++pos;
		if ( &in == &out )
		{
			out.erase( 0, pos );
		}
		else
		{
			out.assign( in.begin() + pos, in.end() );
		}
	}
	else
	{
		out.clear();
	}
	return true;
}


void Set( unsigned int x, gpstring & out, bool hexFormat, bool append )
{
	if ( append )
	{
		out.appendf( hexFormat ? "0x%08X" : "%u", x );
	}
	else
	{
		out.assignf( hexFormat ? "0x%08X" : "%u", x );
	}
}


void Set( unsigned long x, gpstring & out, bool hexFormat, bool append )
{
	Set( (unsigned int)x, out, hexFormat, append );
}


void Set( __int64 x, gpstring & out, bool hexFormat, bool append )
{
	if ( append )
	{
		out.appendf( hexFormat ? "0x%016I64X" : "%I64d", x );
	}
	else
	{
		out.assignf( hexFormat ? "0x%016I64X" : "%I64d", x );
	}
}


void Set( int x, gpstring & out, bool hexFormat, bool append )
{
	if ( append )
	{
		out.appendf( hexFormat ? "0x%08X" : "%d", x );
	}
	else
	{
		out.assignf( hexFormat ? "0x%08X" : "%d", x );
	}
}


void Set( float x, gpstring & out, bool hexFormat, bool append )
{
	Set( (double)x, out, hexFormat, append );
}


void Set( double x, gpstring & out, bool /*hexFormat*/, bool append )
{
	if ( append )
	{
		out.appendf( "%f", x );
	}
	else
	{
		out.assignf( "%f", x );
	}
}


void Set( bool x, gpstring & out, bool /*hexFormat*/, bool append )
{
	if ( append )
	{
		out += x ? "true" : "false";
	}
	else
	{
		out = x ? "true" : "false";
	}
}


void Set( const char * x, gpstring & out, bool /*hexFormat*/, bool append )
{
	if ( append )
	{
		out += x;
	}
	else
	{
		out = x;
	}
}


void AppendExtension( gpstring& s, const char* ext )
{
	gpassert( (ext != NULL) && (*ext == '.') );

	if ( !s.empty() && (s.rfind( '.' ) == gpstring::npos) )
	{
		s += ext;
	}
}


const char* AppendExtension( gpstring& local, const char* s, const char* ext )
{
	gpassert( s != NULL );
	gpassert( (ext != NULL) && (*ext == '.') );

	if ( (*s != '\0') && ::strrchr( s, '.' ) == NULL )
	{
		local = s;
		local += ext;
		s = local;
	}
	return ( s );
}


void ReplaceExtension( gpstring& s, const char* ext )
{
	gpassert( (ext != NULL) && (*ext == '.') );

	size_t pos = s.rfind( '.' );
	if ( pos == gpstring::npos )
	{
		if ( !s.empty() )
		{
			s += ext;
		}
	}
	else
	{
		s.replace( pos, gpstring::npos, ext );
	}
}


void RemoveExtension( gpstring& s )
{
	size_t pos = s.rfind( '.' );
	if ( pos != gpstring::npos )
	{
		s.erase( pos );
	}
}


void RemoveBorderingQuotes( const char*& ib, const char*& ie )
{
	if ( ib != ie )
	{
		if ( *ib == '\"' )
		{
			++ib;
		}
		if ( ib != ie )
		{
			if ( ie[ -1 ] == '\"' )
			{
				--ie;
			}
		}
	}
}



bool SetLastErrorText( DWORD error, gpstring & s )
{
	// get the message
	LPTSTR buffer;
	DWORD rc = ::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
			(LPTSTR)&buffer,
			0,
			NULL );

	// process
	if ( rc != 0 )
	{
		// take and release
		s = buffer;
		::LocalFree(buffer);

		// replace \n's with spaces
		std::replace( s.begin_split(), s.end_split(), '\n', ' ' );

		// remove \r's
		for ( char* i = s.begin_split() ; *i != '\0' ; )
		{
			if ( *i == '\r' )
			{
				i = s.erase( i );
			}
			else
			{
				++i;
			}
		}

		// remove trailing spaces
		RemoveTrailingWhiteSpace( s );

		// ok
		return ( true );
	}
	else
	{
		// convert to int form
		char buffer[20];
		::sprintf( buffer, "0x%08X", error );
		s = buffer;

		// fail
		return ( false );
	}
}


bool SetLastErrorText( gpstring & s )
{
	return ( SetLastErrorText( ::GetLastError(), s ) );
}


gpstring GetLastErrorText( DWORD error )
{
	gpstring s;
	SetLastErrorText( error, s );
	return ( s );
}


gpstring GetLastErrorText( void )
{
	gpstring s;
	SetLastErrorText( s );
	return ( s );
}


bool ExpandEnvironmentStrings( const char* var, gpstring& s )
{
	gpassert( var != NULL );

	int size = ::ExpandEnvironmentStrings( var, NULL, 0 );
	if ( size == 0 )
	{
		s.clear();
		return ( false );
	}

	s.resize( size - 1 );
	::ExpandEnvironmentStrings( var, s.begin_split(), size );

	// $ hack - looks like ExpandEnvironmentStrings() returns size including null char sometimes
	s.resize( ::strlen( s.begin() ) );

	return ( true );
}


gpstring ExpandEnvironmentStrings( const char* var )
{
	gpstring s;
	ExpandEnvironmentStrings( var, s );
	return ( s );
}


bool GetEnvironmentVariable( const char* name, gpstring& s, bool autoExpand )
{
	gpassert( name != NULL );

	int size = ::GetEnvironmentVariable( name, NULL, 0 );
	if ( size == 0 )
	{
		s.clear();
		return ( false );
	}

	s.resize( size - 1 );
	::GetEnvironmentVariable( name, s.begin_split(), size );

	if ( autoExpand )
	{
		gpstring temp;
		if ( ExpandEnvironmentStrings( s, temp ) )
		{
			s = temp;
		}
		else
		{
			return ( false );
		}
	}

	return ( true );
}


gpstring GetEnvironmentVariable( const char* name, bool autoExpand )
{
	gpstring s;
	GetEnvironmentVariable( name, s, autoExpand );
	return ( s );
}

int ReadLine( const void* mem, int available, gpstring& s, bool* foundEol )
{
	int copied = 0, skip = 0;
	bool localFoundEol = false;

	// find the newline
	const void* found = ::memchr( mem, '\n', available );
	if ( found == NULL )
	{
		copied = available;

		// the cutoff could have been at \r - note that we don't consider this
		// the found eol because we must stop at \n.
		if ( ((char*)mem)[ copied - 1 ] == '\r' )
		{
			--copied;
			++skip;
		}
	}
	else
	{
		copied = rcast <int> ( found ) - rcast <int> ( mem );
		skip = 1;
		localFoundEol = true;

		// don't copy '\r' if there was one
		if ( (found > mem) && (rcast <const char*> ( found ) [ -1 ] == '\r') )
		{
			--copied;
			++skip;
		}
	}

	// alloc mem and copy into string
	s.append( rcast <const char*> ( mem ), copied );

	// update
	if ( foundEol != NULL )
	{
		*foundEol = localFoundEol;
	}

	// where's the next line start?
	return ( copied + skip );
}


const char* NextField( const char*& iter, char delim )
{
	gpassert( iter != NULL );

	// $ i cannot use strchr() here because it returns NULL if not found.
	//   god damn stupid inconsistent unix string functions...

	const char* end = iter;
	for (  ; *iter ; ++iter, ++end )
	{
		if ( *iter == delim )
		{
			++iter;			// skip delim
			break;
		}
	}

	return ( end );
}


const char* NextField( const char*& iter, const char* delim )
{
	gpassert( (iter != NULL) && (delim != NULL) && (*delim != '\0') );

	iter += ::strcspn( iter, delim );

	const char* end = iter;
	if ( *iter != '\0' )
	{
		++iter;		// skip delim
	}

	return ( end );
}


const char* NextCommandLineToken( const char*& iter )
{
	gpassert( iter != NULL );

	const char* begin = iter, * end = begin;
	bool quote = false;

	for ( ; *iter != '\0' ; ++iter, ++end )
	{
		if ( *iter == '\"' )
		{
			// toggle quote state
			quote = !quote;
		}
		else if ( *iter == '/' )
		{
			// check for '//' style comment
			if ( !quote && iter[ 1 ] == '/' )
			{
				// ffwd and break
				iter += ::strlen( iter );
				break;
			}
		}
		else if ( ::isspace( *iter ) )
		{
			// if not quoted, it's a delimiter
			if ( !quote )
			{
				// ffwd and break
				while ( ::isspace( *iter ) )
				{
					++iter;
				}
				break;
			}
		}

		//$$$ add support for /* and */ style comments

		//$$$ add support for @ command file options (?)
	}

	return ( end );
}


int GetNextTabColumn( int current, int tabSize )
{
	return ( tabSize - ((current - 1) % tabSize) + current );
}


int GetNextTabPadding( int current, int tabSize )
{
	return ( GetNextTabColumn( current, tabSize ) - current );
}


void ConvertTabsToSpaces( gpstring& str, int tabSize, int startColumn )
{
	// $$$ THIS HAS A BUG IN IT that biddle found
	//
	// pass in a string that has embedded tabs inside and a newline at the end.
	// it will remove the newline if there are tabs detected, and leave it if
	// they are not. inconsistent. fix it. biddle also says problem with if you
	// pass in a single newline as the whole string, or something... FIX -sb

	gpstring out;
	size_t found = 0, lastFound = 0;
	bool converted = false;
	while ( (found = str.find( '\t', lastFound )) != gpstring::npos )
	{
		// add to string
		out.append( str, lastFound, found - lastFound );
		out.append( GetNextTabPadding( out.size() + startColumn, tabSize ), ' ' );

		// advance
		lastFound = found + 1;
		converted = true;
	}

	// only bother if string had any tabs
	if ( converted )
	{
		// append remainder
		out.append( str, lastFound, str.size() - lastFound );
		str = out;
	}
}


int GetWidthInDigits( int value )
{
	return ( scast <int> ( ceil( log10( value + 1 ) ) ) );
}


bool IsDosWildcardMatch( const char* pattern, const char* str, bool caseSensitive )
{
	gpassert( (pattern != NULL) && (str != NULL) );

	// $ this code is a vastly simplified version of fnmatch() from GNU 'make'.

	// $$ future: support ';' delimiter to match multiple wildcards

	const char* p = pattern;
	const char* s = str;
	char c;

	// early-out, + special: '*.*' will always match without requiring a '.' (for compatibility)
	if ( (p[0] == '*') && ((p[1] == '\0') || ((p[1] == '.') && (p[2] == '*') && (p[3] == '\0'))) )
	{
		return ( true );
	}

	// keep going until the end of the pattern
	while ( (c = *p++) != '\0' )
	{
		if ( c == '?' )									// single-char match
		{
			// a '?' needs to match one character
			if ( *s == '\0' )
			{
				return ( false );
			}
		}
		else if ( c == '*' )							// zero or more char match
		{
			// skip to end of combined '*' and '?' wildcard
			for ( ; ; )
			{
				// advance along the pattern
				c = *p++;

				// a '?' needs to match one character
				if ( c == '?' )
				{
					if ( *s == '\0' )
					{
						return ( false );
					}
					else
					{
						// one character of the string is consumed in matching
						//  this '?' wildcard, so '*???' won't match if there are
						//  less than three characters.
						++s;
					}
				}
				else if ( c != '*' )		// continue until no more '?' or '*'
				{
					break;
				}
			}

			// the wildcard(s) is/are the last element of the pattern. easy out.
			if ( c == '\0' )
			{
				return ( true );
			}

			// restart pattern match recursively
			if ( caseSensitive )
			{
				for ( --p ; *s != '\0' ; ++s )
				{
					if ( (*s == c ) && IsDosWildcardMatch( p, s, true ) )
					{
						return ( true );
					}
				}
			}
			else
			{
				c = (char)tolower( c );
				for ( --p ; *s != '\0' ; ++s )
				{
					if (   (tolower( *s ) == c )
						&& IsDosWildcardMatch( p, s ) )
					{
						return ( true );
					}
				}
			}

			// if we make it here, no match is possible
			return ( false );
		}
		else
		{
			// single character, first match failure kills entire match

			if ( caseSensitive )
			{
				if ( c != *s )
				{
					return ( false );
				}
			}
			else
			{
				if ( tolower( c ) != tolower( *s ) )
				{
					return ( false );
				}
			}
		}

		// advance string
		++s;
	}

	// made it to the end? must be a match.
	return ( *s == '\0' );
}


int TokenizeCommandLine( stdx::fast_vector <gpstring> & tokens, const char* cmdLine, bool removeQuotes )
{
	int tokenCount = 0;

	if ( cmdLine == NULL )
	{
		cmdLine = ::GetCommandLine();
	}

	const char* tokenBegin = cmdLine, * tokenIter = tokenBegin;
	const char* tokenEnd = NextCommandLineToken( tokenIter );
	while ( *tokenBegin != '\0' )
	{
		// add token
		if ( removeQuotes )
		{
			if ( *tokenBegin == '\"' )
			{
				++tokenBegin;
			}
			if ( tokenEnd[ -1 ] == '\"' )
			{
				--tokenEnd;
			}
		}
		tokens.push_back( gpstring( tokenBegin, tokenEnd - tokenBegin ) );
		++tokenCount;

		// advance
		tokenBegin = tokenIter;
		tokenEnd   = NextCommandLineToken( tokenIter );
	}

	return ( tokenCount );
}

gpstring MakeFourCcString( int fourCc, bool reverse )
{
	// auto reverse for human readable, if 'reverse' is true, leave it backwards
	if ( !reverse )
	{
		fourCc = REVERSE_FOURCC( fourCc );
	}

	// local buffer to store it in
	char buffer[ 5 ];
	*(int*)buffer = fourCc;
	buffer[ 4 ] = '\0';

	// skip until we have a non-null char
	const char* out = buffer;
	while ( (*out == '\0') && (out < &ELEMENT_LAST( buffer )) )
	{
		++out;
	}

	// return local static storage
	return ( out );
}

void ToString( gpstring& out, const wchar_t* data, int len )
{
	::ToAnsi( out, data, len, true );
}

bool FromString( const char* str, wchar_t* data, int maxLen )
{
	// $$ move this into gpstring.cpp/h as a new function that outputs to a buffer rather than a gp(w)string

	gpassert( str != NULL );
	gpassert( data != NULL );

	// get length of string
	int len = ::MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
	bool success = !clamp_max( len, maxLen );

	// bail out early if no room
	if ( len == 0 )
	{
		return ( true );
	}

	// copy the max we can
	int result = ::MultiByteToWideChar( CP_ACP, 0, str, len, data, len );
	if ( result > 0 )
	{
		data[ result - 1 ] = 0;
	}
	else
	{
		data[ 0 ] = 0;
		success = false;
	}

	return ( success );
}

void ToString( gpstring& out, const GUID& data )
{
	wchar_t buffer[ 100 ];
	int result = ::StringFromGUID2( data, buffer, ELEMENT_COUNT( buffer ) );
	if ( result != 0 )
	{
		::ToAnsi( out, buffer, result - 1 );
	}
	else
	{
		out.clear();
	}
}

bool FromString( const char* str, GUID& data )
{
	gpassert( str != NULL );

	wchar_t buffer[ 100 ];
	return (   FromString( str, buffer, ELEMENT_COUNT( buffer ) )
			&& SUCCEEDED( IIDFromString( buffer, &data ) ) );
}

bool IsEscapeCharacter( char c )
{
	switch ( c )
	{
		case '\a':  case '\b':  case '\f':  case '\n':  case '\r':
		case '\t':  case '\v':  case '\"':  case '\\':
			return ( true );
	}
	return ( false );
}

bool IsEscapeCharacterLetter( char c )
{
	switch ( c )
	{
		case 'a':  case 'b':  case 'f' :  case 'n' :  case 'r':
		case 't':  case 'v':  case '\"':  case '\\':
			return ( true );
	}
	return ( false );
}

char GetEscapeCharacter( char c )
{
	switch ( c )
	{
		case 'a' :  return ( '\a' );
		case 'b' :  return ( '\b' );
		case 'f' :  return ( '\f' );
		case 'n' :  return ( '\n' );
		case 'r' :  return ( '\r' );
		case 't' :  return ( '\t' );
		case 'v' :  return ( '\v' );
		case '\"':  return ( '\"' );
		case '\\':  return ( '\\' );
	}
	return ( '\0' );
}

char GetEscapeCharacterLetter( char c )
{
	switch ( c )
	{
		case '\a':  return ( 'a'  );
		case '\b':  return ( 'b'  );
		case '\f':  return ( 'f'  );
		case '\n':  return ( 'n'  );
		case '\r':  return ( 'r'  );
		case '\t':  return ( 't'  );
		case '\v':  return ( 'v'  );
		case '\"':  return ( '\"' );
		case '\\':  return ( '\\' );
	}
	return ( '\0' );
}

/***
*       This code is adapted from CRT's strtol.c but octal detection with
*       base == 0 removed. Octal is outdated. It's still supported though if
*       you explicitly use 8 as the base. I also don't use errno in these
*       functions because it's slow as hell. It involves TLS lookups, and calls
*       to ::GetLastError() and ::SetLastError(), both of which are also very
*       slow. These three things are so slow, in fact, that they take up 97% of
*       the function call time if you use errno to detect overflow.
*                                                                 -Scott
*
*Purpose:
*       strtol - convert ascii string to long signed integer NO OCTAL
*       strtoul - convert ascii string to long unsigned integer NO OCTAL
*
*******************************************************************************/

/***
*strtol, strtoul(nptr,endptr,ibase) - Convert ascii string to long un/signed
*       int.
*
*Purpose:
*       Convert an ascii string to a long 32-bit value.  The base
*       used for the caculations is supplied by the caller.  The base
*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
*       ascii string must be examined to determine the base of the
*       number:
*               (a) First char = '0', second char = 'x' or 'X',
*                   use base 16.
*               (b) First char in range '0' - '9', use base 10.
*
*       If the 'endptr' value is non-NULL, then strtol/strtoul places
*       a pointer to the terminating character in this value.
*       See ANSI standard for details
*
*Entry:
*       nptr == NEAR/FAR pointer to the start of string.
*       endptr == NEAR/FAR pointer to the end of the string.
*       ibase == integer base to use for the calculations.
*
*       string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*       Good return:
*               result
*
*       Overflow return:
*               strtol -- LONG_MAX or LONG_MIN
*               strtoul -- ULONG_MAX
*               strtol/strtoul -- errno == ERANGE
*
*       No digits or bad base return:
*               0
*               endptr = nptr*
*
*Exceptions:
*       None.
*******************************************************************************/

/* flag values */
const int FL_UNSIGNED  = 1;       /* strtoul called */
const int FL_NEG       = 2;       /* negative sign found */
const int FL_OVERFLOW  = 4;       /* overflow occured */
const int FL_READDIGIT = 8;       /* we've read at least one correct digit */

static unsigned long __cdecl strtoxl (
		const char *nptr,
		const char **endptr,
		int ibase,
		int* err,
		int flags = 0
		)
{
	if ( err )
	{
		*err = 0;
	}

	const char *p;
	char c;
	unsigned long number;
	unsigned digval;
	unsigned long maxval;

	p = nptr;                       /* p is our scanning pointer */
	number = 0;                     /* start with zero */

	c = *p++;                       /* read char */
	while ( isspace((int)(unsigned char)c) )
			c = *p++;               /* skip whitespace */

	if (c == '-') {
			flags |= FL_NEG;        /* remember minus sign */
			c = *p++;
	}
	else if (c == '+')
			c = *p++;               /* skip sign */

	if (ibase < 0 || ibase == 1 || ibase > 36) {
			/* bad base! */
			if (endptr)
					/* store beginning of string in endptr */
					*endptr = nptr;
			return 0L;              /* return 0 */
	}
	else if (ibase == 0) {
			/* determine base free-lance, based on first two chars of
			   string */
			if (c != '0')
					ibase = 10;
			else if (*p == 'x' || *p == 'X')
					ibase = 16;
			else
					ibase = 10;
	}

	if (ibase == 16) {
			/* we might have 0x in front of number; remove if there */
			if (c == '0' && (*p == 'x' || *p == 'X')) {
					++p;
					c = *p++;       /* advance past prefix */
			}

			/* treat result as unsigned */
			flags |= FL_UNSIGNED;
	}

	/* if our number exceeds this, we will overflow on multiply */
	maxval = ULONG_MAX / ibase;


	for (;;) {      /* exit in middle of loop */
			/* convert c to value */
			if ( isdigit((int)(unsigned char)c) )
					digval = c - '0';
			else if ( isalpha((int)(unsigned char)c) )
					digval = toupper(c) - 'A' + 10;
			else
					break;
			if (digval >= (unsigned)ibase)
					break;          /* exit loop if bad digit found */

			/* record the fact we have read one digit */
			flags |= FL_READDIGIT;

			/* we now need to compute number = number * base + digval,
			   but we need to know if overflow occured.  This requires
			   a tricky pre-check. */

			if (number < maxval || (number == maxval &&
			(unsigned long)digval <= ULONG_MAX % ibase)) {
					/* we won't overflow, go ahead and multiply */
					number = number * ibase + digval;
			}
			else {
					/* we would have overflowed -- set the overflow flag */
					flags |= FL_OVERFLOW;
			}

			c = *p++;               /* read next digit */
	}

	--p;                            /* point to place that stopped scan */

	if (!(flags & FL_READDIGIT)) {
			/* no number there; return 0 and point to beginning of
			   string */
			if (endptr)
					/* store beginning of string in endptr later on */
					p = nptr;
			number = 0L;            /* return 0 */
	}
	else if ( (flags & FL_OVERFLOW) ||
			  ( !(flags & FL_UNSIGNED) &&
				( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
				  ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
	{
			/* overflow or signed overflow occurred */
			if ( err )
			{
				*err = ERANGE;
			}
			if ( flags & FL_UNSIGNED )
					number = ULONG_MAX;
			else if ( flags & FL_NEG )
					number = (unsigned long)(-LONG_MIN);
			else
					number = LONG_MAX;
	}

	if (endptr != NULL)
			/* store pointer to char that stopped the scan */
			*endptr = p;

	if (flags & FL_NEG)
			/* negate result if there was a neg sign */
			number = (unsigned long)(-(long)number);

	return number;                  /* done. */
}

long strtol( const char* str, const char** end, int radix, int* err )
{
	return ( (long)strtoxl( str, end, radix, err ) );
}

unsigned long strtoul( const char* str, const char** end, int radix, int* err )
{
	return ( strtoxl( str, end, radix, err, FL_UNSIGNED ) );
}

/***
*       This code is adapted from CRT's strtod.c but I removed TLS and errno
*       as per above strtoxl function notes.
*                                                                 -Scott
*******************************************************************************/

/***
*strtod.c - convert string to floating point number
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert character string to floating point number
*
*******************************************************************************/

/***
*double strtod(nptr, endptr) - convert string to double
*
*Purpose:
*       strtod recognizes an optional string of tabs and spaces,
*       then an optional sign, then a string of digits optionally
*       containing a decimal point, then an optional e or E followed
*       by an optionally signed integer, and converts all this to
*       to a floating point number.  The first unrecognized
*       character ends the string, and is pointed to by endptr.
*
*Entry:
*       nptr - pointer to string to convert
*
*Exit:
*       returns value of character string
*       char **endptr - if not NULL, points to character which stopped
*                       the scan
*
*Exceptions:
*
*******************************************************************************/

double strtod (
	const char *nptr,
	REG2 const char **endptr,
	int* err
	)
{

	if ( err )
	{
		*err = 0;
	}

#ifdef _MT
	struct _flt answerstruct;
#endif  /* _MT */

	FLT      answer;
	double       tmp;
	unsigned int flags;
	REG1 char *ptr = (char *) nptr;

	/* scan past leading space/tab characters */

	while ( isspace((int)(unsigned char)*ptr) )
		ptr++;

	/* let _fltin routine do the rest of the work */

#ifdef _MT
	/* ok to take address of stack variable here; fltin2 knows to use ss */
	answer = _fltin2( &answerstruct, ptr, strlen(ptr), 0, 0);
#else  /* _MT */
	answer = _fltin(ptr, strlen(ptr), 0, 0);
#endif  /* _MT */

	if ( endptr != NULL )
		*endptr = (char *) ptr + answer->nbytes;

	flags = answer->flags;
	if ( flags & (512 | 64)) {
		/* no digits found or invalid format:
		   ANSI says return 0.0, and *endptr = nptr */
		tmp = 0.0;
		if ( endptr != NULL )
			*endptr = (char *) nptr;
	}
	else if ( flags & (128 | 1) ) {
		if ( *ptr == '-' )
			tmp = -HUGE_VAL;        /* negative overflow */
		else
			tmp = HUGE_VAL;         /* positive overflow */
		if ( err )
		{
			*err = ERANGE;
		}
	}
	else if ( flags & 256 ) {
		tmp = 0.0;                  /* underflow */
		if ( err )
		{
			*err = ERANGE;
		}
	}
	else
		tmp = answer->dval;

	return(tmp);
}

inline bool IsEscapeToken( const char* pChar )
{
	return ( (pChar[ 0 ] == '\\') && IsEscapeCharacterLetter( pChar[ 1 ] ) );
}

inline void AddEscapeToken( char EscapeChar, gpstring & Out )
{
	gpassertm( IsEscapeCharacter( EscapeChar ), "This is not a valid escape character." );

	Out += '\\';
	Out += GetEscapeCharacterLetter( EscapeChar );
}

inline void AddEscapeCharacter( char TokenChar, gpstring & Out )
{
	gpassertm( IsEscapeCharacterLetter( TokenChar ), "This is not a valid escape-code token character." );

	Out += GetEscapeCharacter( TokenChar );
}


//$$$ fix this up - use strcspn for speed and change the incoming gpstring
//    into const char*. also use more efficient methods to avoid the double
//    query that most of these use.

bool ContainsEscapeCharacters( gpstring const & in )
{
	gpstring::size_type i = 0;
	while( i != in.size() )
	{
		if( IsEscapeCharacter( in[i] ) )
		{
			return true;
		}
		++i;
	}
	return false;
}


void EscapeCharactersToTokens( gpstring const & in, gpstring & out )
{
	gpstring::const_iterator i, begin = in.begin(), end = in.end();
	for ( i = begin ; i != end ; ++i )
	{
		if( IsEscapeCharacter( *i ) )
		{
			AddEscapeToken( *i, out );
		}
		else
		{
			out += *i;
		}
	}
}


gpstring EscapeCharactersToTokens( gpstring const & in )
{
	gpstring temp;
	EscapeCharactersToTokens( in, temp );
	return ( temp );
}

void EscapeTokensToCharacters( gpstring const & in, gpstring & out )
{
	gpstring::size_type i = 0;
	while( i < in.size() )
	{
		if( IsEscapeToken( in.c_str() + i ) )
		{
			++i;
			AddEscapeCharacter( in[i], out );
			++i;
		}
		else
		{
			out += in[ i ];
			++i;
		}
	}
}

gpstring EscapeTokensToCharacters( gpstring const & in )
{
	gpstring temp;
	EscapeTokensToCharacters( in, temp );
	return ( temp );
}

#if !GP_RETAIL

void RemoveEmbedded( gpstring& str, const char* begin, const char* end )
{
	for ( ; ; )
	{
		size_t posb = str.find( begin );
		if ( posb != str.npos )
		{
			size_t pose = str.find( end );
			if ( (pose != str.npos) && (pose > posb) )
			{
				str.erase( posb, pose - posb + strlen( end ) );
				continue;
			}
		}
		break;
	}
}

bool ContainsWord( const char* begin, const char* word )
{
	const char* found = ::strstr( begin, word );
	if ( found == NULL )
	{
		return ( false );
	}

	// check to make sure it's not part of a larger word
	if ( found != begin )
	{
		if ( ::isalpha( found[ -1 ] ) )
		{
			return ( false );
		}
	}
	if ( ::isalpha( *(found + ::strlen( word )) ) )
	{
		return ( false );
	}

	// must be a word
	return ( true );
}

static const char* DEV_WORDS[] =
{
	"dev",
	"tst",
	"debug",
	"hud",
	"anim",
	"temp",
	"tmp",
};

bool ContainsDevText( const char* in )
{
	// lowercase
	gpstring lowerIn( in );
	lowerIn.to_lower();

	// remove special inserts
	RemoveEmbedded( lowerIn, "<", ">" );
	RemoveEmbedded( lowerIn, "/*", "*/" );

	// no underscores allowed
	if ( ::strchr( lowerIn, '_' ) != NULL )
	{
		return ( true );
	}

	// break it down into words and scan those
	{
		const char** i, ** ibegin = DEV_WORDS, ** iend = ARRAY_END( DEV_WORDS );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( ContainsWord( lowerIn, *i ) )
			{
				return ( true );
			}
		}
	}

	// scan for illegal characters that are not friendly to loc people (high byte)
	{
		for ( const char* i = in ; *i ; ++i )
		{
			if ( *i & 0x80 )
			{
				return ( true );
			}
		}
	}

	// scan for >1 space after certain punctuation
	{
		for ( const char* i = in ; *i ; ++i )
		{
			switch ( *i )
			{
				case ( '!' ):
				case ( '?' ):
				case ( ':' ):
				case ( '.' ):
				case ( ',' ):
				{
					if ( (i[ 1 ] == ' ') && (i[ 2 ] == ' ') )
					{
						return ( true );
					}
				}
				break;
			}
		}
	}

	// must not be
	return ( false );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class Extractor implementation

void Extractor :: Init( const char* delimiter, const char* start, bool multDelims )
{
	gpassert( (delimiter != NULL) && (*delimiter != '\0') );

	m_Delimiter    = delimiter;
	m_DelimiterLen = multDelims ? 1 : ::strlen( delimiter );
	m_Current      = start;
	m_Multiple     = multDelims;
}

bool Extractor :: GetNextString( const char*& begin, const char*& end, bool trimWhitespace )
{
	// if trimming whitespace, then w/s delimiters are not allowed
	gpassert( !trimWhitespace || (m_Delimiter[ ::strcspn( m_Delimiter, ALL_WHITESPACE ) ] == '\0') );

	if ( !IsMore() )
	{
		return ( false );
	}

	// search - if multiple delimiters in one string, each character is
	//  considered a possible delimiter, otherwise the entire string is
	//  considered a single delimiter
	const char* found;
	if ( m_Multiple )
	{
		found = m_Current + ::strcspn( m_Current, m_Delimiter );
		if ( *found == '\0' )
		{
			found = NULL;
		}
	}
	else
	{
		found = ::strstr( m_Current, m_Delimiter );
	}

	// not found?
	if ( found == NULL )
	{
		begin = m_Current;      // no match, so the rest of the string is the final token.
		m_Current = NULL;       // reset the extractor so next call to GetNextString() == false

		if ( trimWhitespace )   // adjust whitespace
		{
			while ( ::isspace( *begin ) )           // kill leading
			{
				++begin;
			}
			end = begin + ::strlen( begin ) - 1;    // get end
			while ( ::isspace( *end ) )
			{
				--end;
			}
			++end;                                  // skip final w/s
		}
		else
		{
			end = begin + ::strlen( begin );        // get end
		}
	}
	else
	{
		// check for current location starting with delimiter
		if ( found != m_Current )
		{
			// adjust for whitespace
			const char* next = found;
			--found;
			if ( trimWhitespace )
			{
				while ( ::isspace( *m_Current ) )
				{
					++m_Current;
				}
				while ( ::isspace( *found ) )
				{
					--found;
				}
			}

			// extract the substring containing the token
			begin     = m_Current;
			end       = found + 1;
			m_Current = next;
		}

		// skip over the delimiter
		m_Current += m_DelimiterLen;
	}

	return ( true );
}

bool Extractor :: GetNextString( gpstring& str, bool trimWhitespace )
{
	const char* begin, * end;
	if ( GetNextString( begin, end, trimWhitespace ) )
	{
		str.assign( begin, end - begin );
		return ( true );
	}
	else
	{
		str.erase();
	}

	return ( false );
}

bool Extractor :: GetNextInt( int& data, bool* success, int radix )
{
	if ( success != NULL )
	{
		*success = true;
	}

	const char* begin, * end;
	if ( GetNextString( begin, end ) )
	{
		if ( begin == end )
		{
			if ( success != NULL )
			{
				*success = false;
			}
		}
		else
		{
			// $$ optz: make this use alloca instead

			data = strtol( gpstring( begin, end - begin ), NULL, radix );
		}
		return ( true );
	}

	return ( false );
}

bool Extractor :: GetNextFloat( float& data, bool* success )
{
	if ( success != NULL )
	{
		*success = true;
	}

	const char* begin, * end;
	if ( GetNextString( begin, end ) )
	{
		if ( begin == end )
		{
			if ( success != NULL )
			{
				*success = false;
			}
		}
		else
		{
			// $$ optz: make this use alloca instead

			data = (float)::atof( gpstring( begin, end - begin ).c_str() );
		}
		return ( true );
	}

	return ( false );
}

bool Extractor :: SkipNext( void )
{
	const char* temp1, * temp2;
	return ( GetNextString( temp1, temp2 ) );
}

int Extractor :: GetStrings( stdx::fast_vector <gpstring> & strings, bool trimWhitespace )
{
	gpstring str;
	size_t old = strings.size();

	while ( GetNextString( str, trimWhitespace ) )
	{
		strings.push_back( str );
	}

	return ( strings.size() - old );
}

int Extractor :: GetInts( stdx::fast_vector <int> & ints, bool* success, int radix )
{
	int i;
	size_t old = ints.size();

	while ( GetNextInt( i, success, radix ) )
	{
		ints.push_back( i );
	}

	return ( ints.size() - old );
}

int Extractor :: GetFloats( stdx::fast_vector <float> & floats, bool* success )
{
	float i;
	size_t old = floats.size();

	while ( GetNextFloat( i, success ) )
	{
		floats.push_back( i );
	}

	return ( floats.size() - old );
}

//////////////////////////////////////////////////////////////////////////////
// class CommandLineExtractor implementation

CommandLineExtractor :: CommandLineExtractor( const char* commandLine )
{
	Init( commandLine );
}

CommandLineExtractor :: ~CommandLineExtractor( void )
{
	// this space intentionally left blank...
}

void CommandLineExtractor :: Init( const char* commandLine )
{
	// tokenize
	TokenizeCommandLine( m_Tokens, commandLine );

	// remove first param (which is the EXE name)
	if ( !m_Tokens.empty() )
	{
		m_Tokens.erase( m_Tokens.begin() );
	}
}

CommandLineExtractor::ConstIter CommandLineExtractor :: Begin( void ) const
{
	return ( m_Tokens.begin() );
}

CommandLineExtractor::ConstIter CommandLineExtractor :: End( void ) const
{
	return ( m_Tokens.end() );
}

CommandLineExtractor::ConstIter CommandLineExtractor :: FindTag( const char* token ) const
{
	ConstIter i, begin = Begin(), end = End();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->same_with_case( token ) )
		{
			break;
		}
	}

	return ( i );
}

bool CommandLineExtractor :: HasTag( const char* token ) const
{
	return ( FindTag( token ) != End() );
}

gpstring CommandLineExtractor :: FindPostTag( const char* token, const char* defValue ) const
{
	gpstring entry;

	ConstIter found = FindTag( token );
	if ( found != End() )
	{
		++found;
		if ( (found != End()) && ((*found)[ 0 ] != '-') )
		{
			entry = *found;
		}
	}

	if ( entry.empty() && (defValue != NULL) )
	{
		entry = defValue;
	}

	return ( entry );
}

int CommandLineExtractor :: FindPostTags( TokenColl& coll, const char* token ) const
{
	int oldSize = coll.size();

	ConstIter found = FindTag( token );
	if ( found != End() )
	{
		for ( ++found ; (found != End()) && ((*found)[ 0 ] != '-') ; ++found )
		{
			coll.push_back( *found );
		}
	}

	return ( coll.size() - oldSize );
}

//////////////////////////////////////////////////////////////////////////////
// class LineFinder implementation

LineFinder :: LineFinder( const char* buffer )
{
	gpassert( buffer != NULL );

	m_First = true;
	m_Next  = buffer;
	m_Begin = NULL;
	m_End   = NULL;
}

bool LineFinder :: GetNext( void )
{
	if ( (*m_Next == '\0') && !m_First )
	{
		return ( false );
	}

	// first is necessary to catch buffer of ""
	m_First = false;

	// find the end of line
	m_Begin = m_Next;
	m_End  = m_Next + ::strcspn( m_Next, "\r\n" );

	// skip newline
	m_Next = m_End;
	if ( *m_Next == '\r' )
	{
		++m_Next;
	}
	if ( *m_Next == '\n' )
	{
		++m_Next;
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class FieldWidthFinder implementation

void FieldWidthFinder :: AddColumn( const gpstring& name, bool setMinWidthToName )
{
	m_Fields.push_back( Field( name, setMinWidthToName ) );
}

void FieldWidthFinder :: Stretch( int fieldIndex, const gpstring& name )
{
	gpassert( (fieldIndex >= 0) && (fieldIndex < scast <int> ( m_Fields.size() )) );
	m_Fields[ fieldIndex ].Stretch( name );
}

gpstring FieldWidthFinder :: MakeHeader( char padChar, char divideChar, int fieldDivideExtra ) const
{
	// here's the buffer
	gpstring str;

	// for each field...
	FieldColl::const_iterator i, begin = m_Fields.begin(), end = m_Fields.end();
	for ( i = begin ; i != end ; ++i )
	{
		// append it
		str.append( MakeField( i - begin, i->m_Name, padChar, divideChar, fieldDivideExtra ) );
	}

	// done
	return ( str );
}

gpstring FieldWidthFinder :: MakeField( int fieldIndex, const gpstring& text, char padChar, char divideChar, int fieldDivideExtra ) const
{
	gpassert( (fieldIndex >= 0) && (fieldIndex < scast <int> ( m_Fields.size() )) );

	// here's the buffer
	gpstring str;

	// print out divider if any for later fields
	if ( (fieldIndex != 0) && (padChar != '\0') && (fieldDivideExtra > 0) )
	{
		str.append( fieldDivideExtra, padChar );

		// if border, add it and more extra
		if ( divideChar != '\0' )
		{
			str.append( 1, divideChar );
			str.append( fieldDivideExtra, padChar );
		}
	}

	// print out field name
	int width = min_t( m_Fields[ fieldIndex ].m_Width, scast <int> ( text.length() ) );
	str.append( text, 0, width );
	width = m_Fields[ fieldIndex ].m_Width - width;

	// print out padding if any
	if ( (padChar != '\0') && (width > 0) )
	{
		str.append( width, padChar );
	}

	// done
	return ( str );
}

//////////////////////////////////////////////////////////////////////////////
// class FieldWidthFinder::Field implementation

FieldWidthFinder::Field :: Field( const gpstring& name, bool setMinWidthToName )
	: m_Name( name )
{
	m_Width = setMinWidthToName ? m_Name.length() : 0;
}

void FieldWidthFinder::Field :: Stretch( const gpstring& name )
{
	maximize( m_Width, scast <int> ( name.length() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class FieldQueryMaker implementation

void FieldQueryMaker :: AddField( const gpstring& name )
{
	if ( m_Rows.empty() || (scast <int> ( m_Rows.back().m_Fields.size() ) == GetColumnCount()) )
	{
		m_Rows.push_back( Row() );
	}

	Row& row = m_Rows.back();
	Stretch( row.m_Fields.size(), name );
	row.m_Fields.push_back( name );
}

gpstring FieldQueryMaker :: MakeRow( int rowIndex, char padChar, char divideChar, int fieldDivideExtra ) const
{
	gpassert( (rowIndex >= 0) && (rowIndex < scast <int> ( m_Rows.size() )) );

	// here's the buffer
	gpstring str;

	// for each field...
	const Row& row = m_Rows[ rowIndex ];
	Row::FieldColl::const_iterator i, begin = row.m_Fields.begin(), end = row.m_Fields.end();
	for ( i = begin ; i != end ; ++i )
	{
		str.append( MakeField( i - begin, *i, padChar, divideChar, fieldDivideExtra ) );
	}

	// done
	return ( str );
}

//////////////////////////////////////////////////////////////////////////////
// helper comparison templates

template <typename T>
struct LessByName
{
	// ptr
	bool operator () ( const T* e1, const T* e2 ) const		{  return ( ::stricmp( e1->m_Name, e2->m_Name ) < 0 );  }
	bool operator () ( const T* e, const char* name ) const	{  return ( ::stricmp( e->m_Name, name ) < 0 );  }
	bool operator () ( const char* name, const T* e ) const	{  return ( ::stricmp( name, e->m_Name ) < 0 );  }

	// ref
	bool operator () ( const T& e1, const T& e2 ) const		{  return ( ::stricmp( e1.m_Name, e2.m_Name ) < 0 );  }
	bool operator () ( const T& e, const char* name ) const	{  return ( ::stricmp( e.m_Name, name ) < 0 );  }
	bool operator () ( const char* name, const T& e ) const	{  return ( ::stricmp( name, e.m_Name ) < 0 );  }
};

template <typename T>
struct LessByEnum
{
	bool operator () ( const T* e1, const T* e2 ) const		{  return ( e1->m_Enum < e2->m_Enum );  }
	bool operator () ( const T* e, DWORD d ) const			{  return ( e->m_Enum < d );  }
	bool operator () ( DWORD d, const T* e ) const			{  return ( d < e->m_Enum );  }
};

// threshold after which we bsearch instead of lsearch
static const int BSEARCH_THRESHOLD = 4;

//////////////////////////////////////////////////////////////////////////////
// class BitEnumStringConverter implementation

BitEnumStringConverter :: BitEnumStringConverter( const Entry entries[], int count, DWORD defEnum )
{
	m_DefEnum = defEnum;
	m_HasDefEnum = true;

	Init( false, entries, count );
}

BitEnumStringConverter :: BitEnumStringConverter( const Entry entries[], int count )
{
	m_DefEnum = 0;
	m_HasDefEnum = false;

	Init( false, entries, count );
}

BitEnumStringConverter :: BitEnumStringConverter( bool translate, const Entry entries[], int count, DWORD defEnum )
{
	m_DefEnum = defEnum;
	m_HasDefEnum = true;

	Init( translate, entries, count );
}

BitEnumStringConverter :: BitEnumStringConverter( bool translate, const Entry entries[], int count )
{
	m_DefEnum = 0;
	m_HasDefEnum = false;

	Init( translate, entries, count );
}

const char* BitEnumStringConverter :: ToString( DWORD e ) const
{
	const char* str = NULL;

	if ( ::IsPower2( e ) )
	{
		// indexed
		const Entry* entry = m_IndexColl[ ::GetShift( e ) ];
		gpassert( entry != NULL );
		str = entry->m_Name;
	}
	else if ( e == 0 )
	{
		// zero enum
		gpassert( m_ZeroName != NULL );
		str = m_ZeroName;
	}
	else if ( m_SpecialColl.size() > BSEARCH_THRESHOLD )
	{
		// bsearch special
		EntryColl::const_iterator found = stdx::binary_search(
				m_SpecialColl.begin(), m_SpecialColl.end(), e, LessByEnum <Entry> () );
		gpassert( found != m_SpecialColl.end() );
		str = (*found)->m_Name;
	}
	else
	{
		// lsearch special
		EntryColl::const_iterator i, begin = m_SpecialColl.begin(), end = m_SpecialColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( (*i)->m_Enum == e )
			{
				str = (*i)->m_Name;
				break;
			}
		}
		gpassert( i != end );
	}

	return ( m_Translate ? gptranslate( str ) : str );
}

bool BitEnumStringConverter :: FromString( const char* str, DWORD& e ) const
{
	gpassert( str != NULL );
	bool success = false;

	// check for empty
	if ( *str == '\0' )
	{
		// use default
		if ( m_HasDefEnum )
		{
			success = true;
			e = m_DefEnum;
		}
	}
	else if ( !m_SortedColl.empty() )
	{
		// bsearch the array
		EntryColl::const_iterator found = stdx::binary_search(
				m_SortedColl.begin(), m_SortedColl.end(), str, LessByName <Entry> () );
		if ( found != m_SortedColl.end() )
		{
			e = (*found)->m_Enum;
			success = true;
		}
	}
	else
	{
		// lsearch the array
		EntryColl::const_iterator i, begin = m_IndexColl.begin(), end = m_IndexColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( ::stricmp( (*i)->m_Name, str ) == 0 )
			{
				e = (*i)->m_Enum;
				success = true;
				break;
			}
		}
	}

	// done
	return ( success );
}

gpstring BitEnumStringConverter :: ToFullString( DWORD e, char delim ) const
{
	// early out for single bit
	if ( ::IsPower2( e ) )
	{
		// indexed
		const Entry* entry = m_IndexColl[ ::GetShift( e ) ];
		gpassert( entry != NULL );
		return ( m_Translate ? gptranslate( entry->m_Name ) : entry->m_Name );
	}

	// early out for zero
	if ( e == 0 )
	{
		if ( m_ZeroName != NULL )
		{
			return ( m_Translate ? gptranslate( m_ZeroName ) : m_ZeroName );
		}
		else
		{
			return ( gpstring::EMPTY );
		}
	}

	// here's the builder
	gpstring str;

	// and the delimiting string
	char localDelim[] = { ' ', delim, ' ', 0 };

	// ok first check and remove the specials (search in backwards order to get
	// the more inclusive ones first)
	{
		EntryColl::const_reverse_iterator i, begin = m_SpecialColl.rbegin(), end = m_SpecialColl.rend();
		for ( i = begin ; i != end ; ++i )
		{
			DWORD test = (*i)->m_Enum;
			if ( (test & e) == test )
			{
				// cool - add it and mask it out
				if ( !str.empty() )
				{
					str += localDelim;
				}
				str += m_Translate ? gptranslate( (*i)->m_Name ) : (*i)->m_Name;
				e &= ~test;
			}
		}
	}

	// now do the bit entries
	{
		for ( DWORD i = FindFirstEnum( e ) ; i != 0 ; i = FindNextEnum( i, e ) )
		{
			const Entry* entry = m_IndexColl[ ::GetShift( i ) ];
			gpassert( entry != NULL );

			if ( !str.empty() )
			{
				str += localDelim;
			}
			str += m_Translate ? gptranslate( entry->m_Name ) : entry->m_Name;
		}
	}

	// done
	return ( str );
}

bool BitEnumStringConverter :: FromFullString( const char* str, DWORD& e, char delim ) const
{
	e = 0;

	// if no delims, do it normally
	if ( ::strchr( str, delim ) == NULL )
	{
		return ( FromString( str, e ) );
	}

	// remove preceding w/s
	while ( ::isspace( *str ) )
	{
		++str;
	}

	// nothing!
	if ( *str == '\0' )
	{
		// considered success only if we have a zero enum
		return ( m_ZeroName != NULL );
	}

	// $$ optz - get rid of local copy and update FromString() to take a length

	// copy local so we can modify it
	int len = ::strlen( str );
	char* localStr = (char*)::_alloca( len + 1 );
	::memcpy( localStr, str, len + 1 );
	str = localStr;

	// extract components
	char localDelim[] = { delim, 0 };
	Extractor extractor( localDelim, str );
	const char* begin = NULL, * end = NULL;
	while ( extractor.GetNextString( begin, end ) )
	{
		// zero-terminate
		*(char*)end = '\0';

		DWORD data;
		if ( FromString( begin, data ) )
		{
			// add in component
			e |= data;
		}
		else
		{
			// failed!
			return ( false );
		}
	}

	// must be ok then
	return ( true );
}

void BitEnumStringConverter :: Init( bool translate, const Entry entries[], int count )
{
	gpassert( count > 0 );

	m_Entries   = entries;
	m_Count     = count;
	m_Translate = translate;

	// build bit index
	const Entry* i, * begin = m_Entries, * end = m_Entries + m_Count;
	for ( i = begin ; i != end ; ++i )
	{
		gpassert( (i->m_Name != NULL) && (translate || (*i->m_Name != '\0')) );

		if ( ::IsPower2( i->m_Enum ) )
		{
			// put it in the index
			int index = ::GetShift( i->m_Enum );
			m_IndexColl.resize( max_t( index + 1, (int)m_IndexColl.size() ), NULL );
			gpassert( m_IndexColl[ index ] == NULL );
			m_IndexColl[ index ] = i;
		}
		else if ( i->m_Enum == 0 )
		{
			// it's the zero!
			gpassert( m_ZeroName == NULL );
			m_ZeroName = i->m_Name;
		}
		else
		{
			// put it in the special area
			m_SpecialColl.push_back( i );
		}
	}

	// always sort the special coll
	stdx::sort( m_SpecialColl, LessByEnum <Entry> () );

	// if more than a few elements in entire collection, set up for bsearch
	if ( m_Count > BSEARCH_THRESHOLD )
	{
		// copy it first
		for ( i = begin ; i != end ; ++i )
		{
			m_SortedColl.push_back( i );
		}

		// now sort
		stdx::sort( m_SortedColl, LessByName <Entry> () );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class EnumStringConverter implementation

EnumStringConverter :: EnumStringConverter( const char* strings[], int begin, int end, int defEnum )
{
	m_DefEnum = defEnum;
	m_HasDefEnum = true;

	Init( strings, begin, end );
}

EnumStringConverter :: EnumStringConverter( const char* strings[], int begin, int end )
{
	m_DefEnum = 0;
	m_HasDefEnum = false;

	Init( strings, begin, end );
}

const char* EnumStringConverter :: ToString( int e ) const
{
	// check range first
	if ( (e >= m_Begin) && (e < m_End) )
	{
		// all clear, use it
		return ( m_Strings[ e - m_Begin ] );
	}

	// shouldn't get here unless it's a default out of the main range!
	gpassert( m_HasDefEnum && (e == m_DefEnum) );

	// blank is ok, when fromstring() called it will return default
	return ( "" );
}

bool EnumStringConverter :: FromString( const char* str, int& e ) const
{
	gpassert( str != NULL );
	bool success = false;

	// check for empty
	if ( *str == '\0' )
	{
		// use default
		if ( m_HasDefEnum )
		{
			success = true;
			e = m_DefEnum;
		}
	}
	else if ( m_IndexColl.empty() )
	{
		// lsearch
		const char** i, ** begin = m_Strings, ** end = begin + (m_End - m_Begin);
		for ( i = begin ; i != end ; ++i )
		{
			if ( ::stricmp( str, *i ) == 0 )
			{
				e = i - begin + m_Begin;
				success = true;
				break;
			}
		}
	}
	else
	{
		// bsearch
		EntryColl::const_iterator found = stdx::binary_search(
				m_IndexColl.begin(), m_IndexColl.end(), str, LessByName <Entry> () );
		if ( found != m_IndexColl.end() )
		{
			e = found->m_Enum + m_Begin;	// rip.bid
			success = true;
		}
	}

	// if not found then always force default
	if ( !success && m_HasDefEnum )
	{
		e = m_DefEnum;
	}

	// done
	return ( success );
}

void EnumStringConverter :: Init( const char* strings[], int begin, int end )
{
	gpassert( (end - begin) > 0 );

	m_Strings = strings;
	m_Begin   = begin;
	m_End     = end;

	// build sorted index if past threshold
	if ( (m_End - m_Begin) > BSEARCH_THRESHOLD )
	{
		// build set
		const char** i, ** begin = m_Strings, ** end = begin + (m_End - m_Begin);
		for ( i = begin ; i != end ; ++i )
		{
			Entry entry;
			entry.m_Name = *i;
			entry.m_Enum = i - begin;
			m_IndexColl.push_back( entry );
		}

		// now sort it
		stdx::sort( m_IndexColl, LessByName <Entry> () );
	}
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace stringtool

//////////////////////////////////////////////////////////////////////////////

