//////////////////////////////////////////////////////////////////////////////
//
// File     :  StringTool.h (GPCore)
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Summary  :  This is a misc string utility collection.  Feel free to add to it.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __STRINGTOOL_H
#define __STRINGTOOL_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"
#include <vector>
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////////
// string.h extensions

// same as strcspn except searches backwards - returns offset of last element
//  in 'string' that contains a character from the set of 'control', or -1 if
//  not found
size_t strrcspn( const char* string, const char* control );
size_t wcsrcspn( const wchar_t* string, const wchar_t* control );

// same as strcspn except has max char count - returns offset of first element
//  in 'string' that contains a character from the set of 'control', or one past
//  the end if not found.
size_t strncspn( const char* string, const char* control, size_t count );

// same as strncspn except it does the opposite - finds the first character not
//  in the set. returns offset of first element in 'string' that does not
//  contain a character from the set of 'control', or one past the end if not
//  found.
size_t strncspn_not( const char* string, const char* control, size_t count = -1 );

// same as strchr except has max char count - returns pointer to found char
//  or NULL if not found
char* strnchr( const char* string, char c, size_t count );

// special "advancing" case of snprintf used for sprintf'ing multiple times
//  to a buffer of a given max length. useful for building logs. returns true
//  if there is more room in the output buffer. "buffer" is always advanced to
//  the end of the characters that it outputs (not including null term) and
//  "count" is decremented by the number of characters written (not including
//  null term). both should be passed in each time. there is no failure code.
bool advance_snprintf( char*& buffer, int& count, const char* format, ... );

// special case of vsnprintf that will try to print locally to a stack allocated
//  buffer (passed in) and if it fails, it will use new[] instead, growing the
//  array until it's big enough to fit the string (up to a reasonable limit in
//  case the large string problem is actually a bug). returns the number of
//  bytes printed. note that if the string had to be new'd, be sure to delete[]
//  it after done with it.
//
// note on the dynamicBuffer pointer - this will be set to NULL if unused, and
//  if non-NULL be sure to delete[] it after using it. if you pass in NULL for
//  this value, then it will only use the staticBuffer, and put "..." at the end
//  if it's too big to fit there.
int dynamic_vsnprintf(					// returns # chars written, not including null term
		char*          staticBuffer,	// stack-alloc'd buffer
		int            staticBufferLen,	// length of stack-alloc'd buffer
		char**         dynamicBuffer,	// pointer to pointer to dynamic buffer
		const char*    format,			// formatting string for vsnprintf
		va_list        args );			// args for vsnprintf
int dynamic_vsnprintf(
		wchar_t*       staticBuffer,
		int            staticBufferLen,
		wchar_t**      dynamicBuffer,
		const wchar_t* format,
		va_list        args );

template <const int STATIC_SIZE = 2000, typename CHAR = char>
class auto_dynamic_vsnprintf_t
{
public:
	auto_dynamic_vsnprintf_t( const CHAR* format, va_list args, bool useDynamicIfNeeded = true )
	{
		m_DynamicBuffer = NULL;
		m_Length = dynamic_vsnprintf(
				m_StaticBuffer,
				STATIC_SIZE,
				useDynamicIfNeeded ? &m_DynamicBuffer : NULL,
				format,
				args );
	}

	auto_dynamic_vsnprintf_t( CHAR* staticBuffer, int staticBufferLen, const CHAR* format, va_list args, bool useDynamicIfNeeded = true )
	{
		m_DynamicBuffer = NULL;
		m_Length = dynamic_vsnprintf(
				staticBuffer,
				staticBufferLen,
				useDynamicIfNeeded ? &m_DynamicBuffer : NULL,
				format,
				args );
	}

   ~auto_dynamic_vsnprintf_t( void )		{  delete [] ( m_DynamicBuffer );  }

	const CHAR* c_str( void ) const			{  return ( m_DynamicBuffer ? m_DynamicBuffer : m_StaticBuffer );  }
	int length( void ) const				{  return ( m_Length );  }

	operator const CHAR* ( void ) const		{  return ( c_str() );  }

private:
	CHAR  m_StaticBuffer[ STATIC_SIZE ];
	CHAR* m_DynamicBuffer;
	int   m_Length;
};

typedef auto_dynamic_vsnprintf_t <2000, char   > auto_dynamic_vsnprintf;
typedef auto_dynamic_vsnprintf_t <2000, wchar_t> auto_dynamic_vsnwprintf;

// same as memchr except backwards, point it at start of memory block, returns
//  pointer to last occurrence of 'c' in memory, or NULL if not found.
void* memrchr( const void* buf, int c, size_t count );

// same as memrchr except it does the opposite - finds the first character not
//  equal to 'c'.
void* memrchr_not( const void* buf, int c, size_t count );

// same as _str(lwr|upr) except stores the results in a new string. make sure
// there's enough room in dest! each returns a pointer to dst.
char* strlwr_copy( char* dst, const char* src );
char* strupr_copy( char* dst, const char* src );

// extensions
inline bool iswcsym ( wchar_t c )  {  return ( iswalnum( c ) || (c == L'_') );  }
inline bool iswcsymf( wchar_t c )  {  return ( iswalpha( c ) || (c == L'_') );  }

//////////////////////////////////////////////////////////////////////////////

namespace stringtool  {  // begin of namespace stringtool

// Constants.

	// all possible whitespace characters
	extern const char* ALL_WHITESPACE;

// Character traits.

	// returns true if it's a character that uses one of the 'standard' escapes
	bool IsEscapeCharacter( char c );

	// returns true if it's a letter that follows a \ for one of the 'standard' escapes
	bool IsEscapeCharacterLetter( char c );

	// return the equivalent escape character for this 'standard' escape code if any
	char GetEscapeCharacter( char c );

	// return the equivalent letter for this 'standard' escape code if any
	char GetEscapeCharacterLetter( char c );

// Overrides.

	// no octal! also doesn't use errno, which is EXPENSIVE - uses tls and ::Get/SetLastError() (slow)
	long          strtol ( const char* str, const char** end = NULL, int radix = 0, int* err = NULL );
	unsigned long strtoul( const char* str, const char** end = NULL, int radix = 0, int* err = NULL );
	double        strtod ( const char* str, const char** end = NULL, int* err = NULL );

//////////////////////////////////////////////////////////////////////////////

						 /*** FUNCTION DECLARATIONS ***/

//////////////////////////////////////////////////////////////////////////////

// intelligent max-length string copy. returns the number of chars copied (not
//  including the null). use this instead of brain-dead "strncpy" function. pass
//  in the source string length if you know it, otherwise it will autodetect.
int CopyString( char* dest, const char* src, int maxDestChars, int srcLength = -1 );
int CopyString( char* dest, const gpstring& src, int maxDestChars, int srcLength = -1 );
int CopyString( wchar_t* dest, const wchar_t* src, int maxDestChars, int srcLength = -1 );
int CopyString( wchar_t* dest, const gpwstring& src, int maxDestChars, int srcLength = -1 );

void PadFrontToLength( gpstring & in, char pad_value, unsigned int to_length );
void PadBackToLength( gpstring & in, char pad_value, unsigned int to_length );
void PadBack( gpstring & in, const char* pad_with, unsigned int pad_iterations );

void RemoveLeadingWhiteSpace( gpstring & s );
void RemoveTrailingWhiteSpace( gpstring & s );
void RemoveBorderingWhiteSpace( gpstring & s );

void RemoveLeadingWhiteSpace( gpwstring & s );
void RemoveTrailingWhiteSpace( gpwstring & s );
void RemoveBorderingWhiteSpace( gpwstring & s );

void RemoveLeadingWhiteSpace( const char*& ib, const char* ie );
void RemoveTrailingWhiteSpace( const char* ib, const char*& ie );
void RemoveBorderingWhiteSpace( const char*& ib, const char*& ie );

void ReplaceAllChars( gpstring & in_out, char replace, char replace_with );
bool ReplaceRootPath( gpstring const & path, gpstring const & oldRoot, gpstring const & newRoot, gpstring & newPath );
bool IsAbsolutePath( const char * path );

// translate value from string
bool Get( const char * in, unsigned int & out );
bool Get( const char * in, int & out );
bool Get( const char * in, unsigned long & out );
bool Get( const char * in, long & out );
bool Get( const char * in, __int64 & int64 );
bool Get( const char * in, float & out );
bool Get( const char * in, double & out );
bool Get( const char * in, bool & out );
bool Get( const char * in, gpstring & out );

// translate value from wstring
bool Get( const wchar_t * in, unsigned int & out );
bool Get( const wchar_t * in, int & out );
bool Get( const wchar_t * in, unsigned long & out );
bool Get( const wchar_t * in, long & out );
bool Get( const wchar_t * in, __int64 & int64 );
bool Get( const wchar_t * in, float & out );
bool Get( const wchar_t * in, double & out );
bool Get( const wchar_t * in, bool & out );
bool Get( const wchar_t * in, gpwstring & out );

// get string as first n delimited values, string includes Nth delimiter
bool GetDelimitedValue( const char* in, char delimiter, unsigned int num, const char*& begin, const char*& end );
bool GetDelimitedValue( const char* in, char delimiter, unsigned int num, gpstring & out, bool removeInnerSpace = true );

// call Get() on the delimited value
template <typename T>
bool GetDelimitedValue( const char* in, char delimiter, unsigned int num, T& out )
{
	const char* begin, * end;
	bool rc = GetDelimitedValue( in, delimiter, num, begin, end );
	if ( rc )
	{
		int count = end - begin;
		char* buf = (char*)_alloca( count + 1 );
		::memcpy( buf, begin, count );
		buf[ count ] = '\0';
		rc = Get( buf, out );
	}
	return ( rc );
}

// more helpers
unsigned int GetNumDelimitedStrings    ( const char* in, char delimiter );
inline bool  GetFrontDelimitedString   ( const char* in, char delimiter, gpstring & out )  {  return ( GetDelimitedValue( in, delimiter, 0, out ) );  }
bool         GetBackDelimitedString    ( const char* in, char delimiter, gpstring & out );
bool         RemoveFrontDelimitedString( gpstring const & in, char delimiter, gpstring & out );

// handle escape sequences
bool     ContainsEscapeCharacters( const char* in );
void     EscapeCharactersToTokens( gpstring const & in, gpstring & out );
gpstring EscapeCharactersToTokens( gpstring const & in );
void     EscapeTokensToCharacters( gpstring const & in, gpstring & out );
gpstring EscapeTokensToCharacters( gpstring const & in );

// scanners
#if !GP_RETAIL
bool ContainsDevText( const char* in );
#endif // !GP_RETAIL

// will set said string to value
void Set( unsigned int x,	gpstring & out, bool hexFormat = false, bool append = false );
void Set( unsigned long x,	gpstring & out, bool hexFormat = false, bool append = false );
void Set( __int64 x,		gpstring & out, bool hexFormat = false, bool append = false );
void Set( int x,			gpstring & out, bool hexFormat = false, bool append = false );
void Set( float x,			gpstring & out, bool hexFormat = false, bool append = false );
void Set( double x,			gpstring & out, bool hexFormat = false, bool append = false );
void Set( bool x,			gpstring & out, bool hexFormat = false, bool append = false );
void Set( const char * x,	gpstring & out, bool hexFormat = false, bool append = false );

template <typename T>
inline void Setx( T x, gpstring& out, bool append = false )
	{  Set( x, out, true, append );  }

template <typename T>
inline void Append( T x, gpstring& out, bool hexFormat = false )
	{  Set( x, out, hexFormat, true );  }

template <typename T>
inline void Appendx( T x, gpstring& out )
	{  Set( x, out, true, true );  }


// only adds backslash if necessary
void AppendTrailingBackslash ( gpstring&  s );
void AppendTrailingBackslash ( gpwstring& s );
void RemoveTrailingBackslash ( gpstring&  s );
void RemoveTrailingBackslash ( gpwstring& s );
void RemoveLeadingBackslash  ( gpstring&  s );
void IncludeTrailingBackslash( gpstring&  s, bool include );
void IncludeTrailingBackslash( gpwstring& s, bool include );

// only adds extension if there isn't one
void AppendExtension( gpstring& s, const char* ext );

// special: give it a local gpstring and it will copy into there w/ ext if it
//  doesn't have it already, setting the return to point to it, otherwise it
//  will just leave it alone and return 's'.
const char* AppendExtension( gpstring& local, const char* s, const char* ext );

// replaces/adds extension
void ReplaceExtension( gpstring& s, const char* ext );

// removes any extension
void RemoveExtension( gpstring& s );

// remove double quotes around the string
void RemoveBorderingQuotes( const char*& ib, const char*& ie );
void RemoveBorderingQuotes( gpstring& s );
void RemoveBorderingQuotes( gpwstring& s );

// other string conversion functions
bool     SetLastErrorText( DWORD error, gpstring& s );											// given error -> text
bool     SetLastErrorText( gpstring& s );														// GetLastError() -> text
gpstring GetLastErrorText( DWORD error );														// given error -> text
gpstring GetLastErrorText( void );																// GetLastError() -> text
bool     ExpandEnvironmentStrings( const char* var, gpstring& s );								// expand %PATH% type stuff
gpstring ExpandEnvironmentStrings( const char* var );											// expand %PATH% type stuff
bool     GetEnvironmentVariable( const char* name, gpstring& s, bool autoExpand = true );		// env var -> text
gpstring GetEnvironmentVariable( const char* name, bool autoExpand = true );					// env var -> text

// parsing functions
int            ReadLine( const void* mem, int available, gpstring & s, bool* foundEol = NULL );	// get the next '\n'-terminated line from the given memory block - returns offset to next line, sets optional foundEol to true if found end of line
const char*    NextField( const char*& iter, char delim = ',' );								// find the next 'delim'-delimited field in the string and move the pointer there (NULL is considered a terminator and delimiter) - returns end of last
const char*    NextField( const char*& iter, const char* delim );								// same as above except multiple terminators are allowed - returns the char it found, or '\0' if none
const char*    NextPathComponent( const char*& iter );											// fast-forwards to next path (returns end of last - may point to '\0' if at end)
const wchar_t* NextPathComponent( const wchar_t*& iter );										// fast-forwards to next path (returns end of last - may point to '\0' if at end)
const char*    NextCommandLineToken( const char*& iter );										// finds the start of the next command line token - properly handles quoted filenames

// find next column from current (column is 1-based btw to match editor convention)
int GetNextTabColumn( int current, int tabSize = 4 );

// get padding required to go to next tab column (column is 1-based btw to match editor convention)
int GetNextTabPadding( int current, int tabSize = 4 );

// convert tabs to spaces in the string (start column is 1-based btw to match editor convention)
void ConvertTabsToSpaces( gpstring& str, int tabSize = 4, int startColumn = 1 );

// find out how many digits this number will be when printed
int GetWidthInDigits( int value );

// pattern matching
bool HasDosWildcards( const char* pattern );													// checks to see if it has DOS-style wildcards
bool HasDosWildcards( const wchar_t* pattern );													// checks to see if it has DOS-style wildcards
bool IsDosWildcardMatch( const char* pattern, const char* str, bool caseSensitive = false );	// matches DOS-style '*' and '?' wildcards only, the rest is case-insensitive depending on flag

// simple tokenizer meant for command lines...pretty basic and stupid right
//  now. returns # tokens found.
int TokenizeCommandLine( stdx::fast_vector <gpstring> & tokens, const char* cmdLine = NULL, bool removeQuotes = true );

// returns the string version of the given fourcc, optionally reversing the byte order
gpstring MakeFourCcString( int fourCc, bool reverse = false );

// conversions for microsoft types
void ToString  ( gpstring& out, const wchar_t* data, int len = -1 );
bool FromString( const char* str, wchar_t* data, int maxLen );
void ToString  ( gpstring& out, const GUID& data );
bool FromString( const char* str, GUID& data );

//////////////////////////////////////////////////////////////////////////////

						  /*** CLASS DECLARATIONS ***/

//////////////////////////////////////////////////////////////////////////////
// class Extractor declaration

// this is an extremely simple parser.  the delimiter is what separates
//  tokens, which are retrieved one at a time via GetNextString().

// $$$ FUTURE: implement a general purpose parser along the lines of sscanf()
//             and printf - a simple finite state automaton (see output.c in
//             VC++\crt\src for sample). there are many times when we want the
//             convenience of being able to define a combo grammar/lexeme in
//             string format without the pain and suffering of constructing a
//             new lex/yacc setup just to parse a damn simple thing.

class Extractor
{
public:
	SET_NO_INHERITED( Extractor );

	Extractor( const char* delimiter, const char* start, bool multDelims = false )
			   {  Init( delimiter, start, multDelims );  }
	Extractor( void )
			   {  m_Delimiter = NULL;  m_Current = NULL;  m_Multiple = false;  }
   ~Extractor( void )
			   {  }

	void Init ( const char* delimiter, const char* start, bool multDelims = false );
	void Reset( const char* start )
				{  m_Current = start;  }

	// these return true if something was extracted

	bool GetNextString( const char*& begin, const char*& end, bool trimWhitespace = true );
	bool GetNextString( gpstring& str, bool trimWhitespace = true );
	bool GetNextInt   ( int     & data, bool* success = NULL, int radix = 0 );
	bool GetNextFloat ( float   & data, bool* success = NULL );
	bool SkipNext     ( void );

	// these return how many things were extracted

	int GetStrings( stdx::fast_vector <gpstring> & strings, bool trimWhitespace = true );
	int GetInts   ( stdx::fast_vector <int     > & ints   , bool* success = NULL, int radix = 0 );
	int GetFloats ( stdx::fast_vector <float   > & floats , bool* success = NULL );

	const char* GetRemaining( void ) const
							  {  return ( (m_Current == NULL) ? "" : m_Current );  }
	bool        IsMore      ( void ) const
							  {  return ( (m_Current != NULL) && (*m_Current != '\0') );  }

private:
	const char* m_Delimiter;
	int         m_DelimiterLen;
	const char* m_Current;
	bool        m_Multiple;

	SET_NO_COPYING( Extractor );
};

//////////////////////////////////////////////////////////////////////////////
// class CommandLineExtractor declaration

class CommandLineExtractor
{
public:
	SET_NO_INHERITED( CommandLineExtractor );

	typedef stdx::fast_vector <gpstring> TokenColl;
	typedef TokenColl::const_iterator ConstIter;

	CommandLineExtractor( const char* commandLine = NULL );
   ~CommandLineExtractor( void );

	void Init( const char* commandLine = NULL );

	ConstIter Begin       ( void ) const;
	ConstIter End         ( void ) const;
	ConstIter FindTag     ( const char* token ) const;
	bool      HasTag      ( const char* token ) const;
	gpstring  FindPostTag ( const char* token, const char* defValue = NULL ) const;
	int       FindPostTags( TokenColl& coll, const char* token ) const;

private:
	TokenColl m_Tokens;

	SET_NO_COPYING( CommandLineExtractor );
};

//////////////////////////////////////////////////////////////////////////////
// class LineFinder declaration

// this class just retrieves the start and end of each line in the buffer

class LineFinder
{
public:
	SET_NO_INHERITED( LineFinder );

	LineFinder( const char* buffer );
   ~LineFinder( void )  {  }

	// returns true if there is data to output
	bool GetNext( void );

	// simple query
	const char* GetBegin ( void ) const  {  return ( m_Begin );  }
	const char* GetEnd   ( void ) const  {  return ( m_End   );  }
	int         GetLength( void ) const  {  return ( m_End - m_Begin );  }

private:
	bool        m_First;
	const char* m_Next;

	const char* m_Begin;
	const char* m_End;

	SET_NO_COPYING( LineFinder );
};

//////////////////////////////////////////////////////////////////////////////
// class FieldWidthFinder declaration

class FieldWidthFinder
{
public:
	SET_NO_INHERITED( FieldWidthFinder );

	FieldWidthFinder( void )  {  }
   ~FieldWidthFinder( void )  {  }

	void     AddColumn ( const gpstring& name, bool setMinWidthToName = true );
	void     Stretch   ( int fieldIndex, const gpstring& name );
	gpstring MakeHeader( char padChar = ' ', char divideChar = '|', int fieldDivideExtra = 1 ) const;
	gpstring MakeField ( int fieldIndex, const gpstring& text, char padChar = ' ', char divideChar = '|', int fieldDivideExtra = 1 ) const;

	int GetColumnCount( void ) const  {  return ( scast <int> ( m_Fields.size() ) );  }

private:
	struct Field
	{
		gpstring m_Name;
		int      m_Width;

		Field( const gpstring& name, bool setMinWidthToName );
		void Stretch( const gpstring& name );
	};

	typedef stdx::fast_vector <Field> FieldColl;
	FieldColl m_Fields;

	SET_NO_COPYING( FieldWidthFinder );
};

//////////////////////////////////////////////////////////////////////////////
// class FieldQueryMaker declaration

// $ these two classes really could stand to be replaced with a better
//   db-style table/query set thingy.

class FieldQueryMaker : public FieldWidthFinder
{
public:
	SET_INHERITED( FieldQueryMaker, FieldWidthFinder );

	FieldQueryMaker( void )  {  }
   ~FieldQueryMaker( void )  {  }

	void     AddField( const gpstring& name );
	gpstring MakeRow ( int rowIndex, char padChar = ' ', char divideChar = '|', int fieldDivideExtra = 1 ) const;

	int GetRowCount( void ) const  {  return ( scast <int> ( m_Rows.size() ) );  }

private:
	struct Row
	{
		typedef stdx::fast_vector <gpstring> FieldColl;
		FieldColl m_Fields;
	};

	typedef stdx::fast_vector <Row> RowColl;
	RowColl m_Rows;

	SET_NO_COPYING( FieldQueryMaker );
};

//////////////////////////////////////////////////////////////////////////////
// class BitEnumStringConverter declaration

class BitEnumStringConverter
{
public:
	SET_NO_INHERITED( BitEnumStringConverter );

	struct Entry
	{
		const char* m_Name;
		DWORD       m_Enum;
	};

	BitEnumStringConverter( const Entry entries[], int count, DWORD defEnum );
	BitEnumStringConverter( const Entry entries[], int count );
	BitEnumStringConverter( bool translate, const Entry entries[], int count, DWORD defEnum );
	BitEnumStringConverter( bool translate, const Entry entries[], int count );

	// optionally translate the "to" functions
	void SetTranslate  ( bool set = true )			{  m_Translate = set;  }
	void ClearTranslate( void )						{  SetTranslate( false );  }

	// bit at a time
	const char* ToString  ( DWORD e ) const;
	bool        FromString( const char* str, DWORD& e ) const;

	// full var at a time
	gpstring    ToFullString  ( DWORD e, char delim = '|' ) const;
	bool        FromFullString( const char* str, DWORD& e, char delim = '|' ) const;

private:
	void Init( bool translate, const Entry entries[], int count );

	typedef stdx::fast_vector <const Entry*> EntryColl;

	// spec
	const Entry* m_Entries;			// original map
	int          m_Count;			// size of that map
	DWORD        m_DefEnum;			// default enum to use (i.e. "none")
	bool         m_HasDefEnum;		// has default enum?
	const char*  m_ZeroName;		// name of zero enum (NULL if none)
	bool         m_Translate;		// translate strings?

	// indexes
	EntryColl    m_IndexColl;		// indexes into m_Entries, one for each bit
	EntryColl    m_SpecialColl;		// special entries that are not single bits, sorted by enum
	EntryColl    m_SortedColl;		// entry collection sorted by name - empty if not bsearch

	SET_NO_COPYING( BitEnumStringConverter );
};

//////////////////////////////////////////////////////////////////////////////
// class EnumStringConverter declaration

class EnumStringConverter
{
public:
	SET_NO_INHERITED( EnumStringConverter );

	EnumStringConverter( const char* strings[], int begin, int end, int defEnum );
	EnumStringConverter( const char* strings[], int begin, int end );
	EnumStringConverter( const char* strings[] );

	const char* ToString  ( int e ) const;
	bool        FromString( const char* str, int& e ) const;

private:
	void Init( const char* strings[], int begin, int end );

	struct Entry
	{
		const char* m_Name;
		int         m_Enum;
	};

	typedef stdx::fast_vector <Entry> EntryColl;

	// spec
	const char** m_Strings;			// original map
	int          m_Begin;			// begin of enum set
	int          m_End;				// past end of enum set
	int          m_DefEnum;			// default enum to use (i.e. "none")
	bool         m_HasDefEnum;		// has default enum?

	// indexes
	EntryColl    m_IndexColl;		// index into m_Strings by name (empty if not needed)

	SET_NO_COPYING( EnumStringConverter );
};

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace stringtool

#endif  // __STRINGTOOL_H

//////////////////////////////////////////////////////////////////////////////
