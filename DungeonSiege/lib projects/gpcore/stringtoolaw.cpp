//////////////////////////////////////////////////////////////////////////////
//
// File     :  StringToolAW.cpp
// Author(s):  Scott Bilas
//
// Summary  :  This is a special inline file meant to be #included multiple
//             times by StringTool.cpp ONLY. Use to define ANSI and Unicode
//             versions of functions easily.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if !defined( STRINGTOOLA ) && !defined( STRINGTOOLW )
#error ! Do NOT include this file! It's meant for StringTool.cpp only!
#endif

#ifdef STRINGTOOLA

#define XCHAR char
#define XSTRING gpstring
#define XC( x ) x
#define XVSNPRINTF _vsnprintf
#define XSTRCSPN strcspn

#else

#define XCHAR wchar_t
#define XSTRING gpwstring
#define XC( x ) L ## x
#define XVSNPRINTF _vsnwprintf
#define XSTRCSPN wcscspn

#endif

//////////////////////////////////////////////////////////////////////////////

int dynamic_vsnprintf(
		XCHAR*       staticBuffer,
		int          staticBufferLen,
		XCHAR**      dynamicBuffer,
		const XCHAR* format,
		va_list      args )
{
	gpassert( staticBuffer != NULL );
	gpassert( format != NULL );

	int rc = 0;
	if ( staticBufferLen > 0 )
	{
		// vsnprintf will return >= 0 even if the null term did not fit. so we have
		// to pretend the output buffer is too small.
		rc = XVSNPRINTF( staticBuffer, staticBufferLen - 1, format, args );

		// success?
		if ( rc != -1 )
		{
			// terminate just in case it was exact fit
			staticBuffer[ rc ] = XC( '\0' );

			// clear out the dynamic pointer if it exists, as we're not using it
			if ( dynamicBuffer != NULL )
			{
				*dynamicBuffer = NULL;
			}

			// all done
			return ( rc );
		}
	}

	// ok have to go with the dynamic route
	if ( dynamicBuffer != NULL )
	{
		// start empty
		*dynamicBuffer = NULL;

		// grow at 50% starting at a reasonable size
		int dynamicSize = max_t( staticBufferLen + staticBufferLen / 2, 1000 );

		// loop until it fits
		for ( ; ; )
		{
			// alloc space
			*dynamicBuffer = new XCHAR[ dynamicSize ];

			// try to write
			rc = XVSNPRINTF( *dynamicBuffer, dynamicSize - 1, format, args );
			if ( rc != -1 )
			{
				// cool it fits
				(*dynamicBuffer)[ rc ] = XC( '\0' );
				break;
			}

			// increase size and sanity check
			dynamicSize += dynamicSize / 2;
			gpassert( dynamicSize < (1024 * 1024) );

			// start over
			delete [] ( *dynamicBuffer );
		}
	}
	else
	{
		// or not... ok well make the end '...' and then stop
		if ( staticBufferLen > 0 )
		{
			staticBuffer[ staticBufferLen - 1 ] = XC( '\0' );
			if ( staticBufferLen > 3 )
			{
				staticBuffer[ staticBufferLen - 2 ] = XC( '.' );
				staticBuffer[ staticBufferLen - 3 ] = XC( '.' );
				staticBuffer[ staticBufferLen - 4 ] = XC( '.' );
			}

			// we wrote almost the entire buffer
			rc = staticBufferLen - 1;
		}
	}

	// done
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////

namespace stringtool  {  // begin of namespace stringtool

//////////////////////////////////////////////////////////////////////////////

int CopyString( XCHAR* dest, const XSTRING& src, int maxDestChars, int srcLength )
{
	if ( srcLength == -1 )
	{
		srcLength = src.length();
	}
	return ( CopyString( dest, src.c_str(), maxDestChars, srcLength ) );
}

void RemoveLeadingWhiteSpace( XSTRING & in )
{
	in.erase( 0, in.find_first_not_of( XC( " \t" ) ) );
}

void RemoveTrailingWhiteSpace( XSTRING & in )
{
	size_t found = in.find_last_not_of( XC( " \t" ) );
	in.erase( found == XSTRING::npos ? 0 : (found + 1) );
}

void RemoveBorderingWhiteSpace( XSTRING & in )
{
	RemoveTrailingWhiteSpace( in );
	RemoveLeadingWhiteSpace( in );
}

void AppendTrailingBackslash( XSTRING & s )
{
	if( s.empty() )
	{
		return;
	}

	XCHAR c = *s.rbegin();

	if ( (c != XC( '\\')) && (c != XC( '/' )) )
	{
		s += XC( '\\' );
	}
}

void IncludeTrailingBackslash( XSTRING& s, bool include )
{
	if ( include )
	{
		AppendTrailingBackslash( s );
	}
	else
	{
		RemoveTrailingBackslash( s );
	}
}

void RemoveTrailingBackslash( XSTRING & s )
{
	// funky while-loop code is necessary - may be multiple backslashes

	if ( !s.empty() )
	{
		const XCHAR* b = s.begin();
		const XCHAR* i = b + s.length() - 1;

		while ( (*i == XC( '\\' )) || (*i == XC( '/' )) )
		{
			--i;
		}

		s.erase( i - b + 1 );
	}
}

void RemoveLeadingBackslash( XSTRING & s )
{
	// funky while-loop code is necessary - may be multiple backslashes

	if ( !s.empty() )
	{
		const XCHAR* b = s.begin();
		const XCHAR* i = b;

		while ( (*i == XC( '\\' )) || (*i == XC( '/')) )
		{
			++i;
		}

		s.erase( 0, i - b );
	}
}

void RemoveBorderingQuotes( XSTRING& s )
{
	if ( !s.empty() && (*s.rbegin() == XC( '\"' )) )
	{
		s.erase( s.length() - 1 );
	}
	if ( !s.empty() && (*s.begin() == XC( '\"' )) )
	{
		s.erase( 0, 1 );
	}
}

const XCHAR* NextPathComponent( const XCHAR*& iter )
{
	gpassert( iter != NULL );

	const XCHAR* end = iter;
	for (  ; *iter ; ++iter, ++end )
	{
		if ( (*iter == XC( '\\' )) || (*iter == XC( '/' )) )
		{
			for ( ++iter ; (*iter == XC( '\\' )) || (*iter == XC( '/' )) ; ++iter )
			{
				// $ skip to end of delim (may be multiple slashes, which are legal to pile up)
			}
			break;
		}
	}

	return ( end );
}

bool HasDosWildcards( const XCHAR* pattern )
{
	gpassert( pattern != NULL );
	return ( pattern[ XSTRCSPN( pattern, XC( "*?" ) ) ] != XC( '\0' ) );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace stringtool

//////////////////////////////////////////////////////////////////////////////

#undef XCHAR
#undef XSTRING
#undef XC
#undef XVSNPRINTF
#undef XSTRCSPN

//////////////////////////////////////////////////////////////////////////////
