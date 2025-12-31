
//////////////////////////////////////////////////////////////////////////////
//
// File     :  <yy Lexer>.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains lex-generated lexer code. Class design loosely based
//             on original (c) 1991 MKS prototype code. Algorithmic code is
//             nearly identical. Class is yyScanner, though prototype name can
//             be changed through a sed script.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#undef  yy_state_t
#define yy_state_t unsigned int

//////////////////////////////////////////////////////////////////////////////

// include requirements: gpcoll, stdinc/core, stringtool

//////////////////////////////////////////////////////////////////////////////
// class yyScanner declaration

class yyScanner
{
public:
	SET_NO_INHERITED( yyScanner );

	enum eMessageType
	{
		MESSAGE_INFO,		// informational
		MESSAGE_WARNING,	// simple warning
		MESSAGE_ERROR,		// nonfatal error
		MESSAGE_FATAL,		// fatal error
	};

// Construction.

	         yyScanner( int bufferSize = 100 );			// default token & pushback size is 100 bytes
	virtual ~yyScanner( void );							// destructor

	virtual void Reset( void );							// reset scanner
	virtual void Init ( const_mem_ptr mem );			// initialize scanner source memory

// Action.

	virtual int  Scan    ( void );						// do a scan
	virtual void Message ( eMessageType type,			// internally called on exception (OVERRIDE THIS)
						   const char* msg, ... );
	virtual void NextLine( int line );					// internally called on line advance (OVERRIDE THIS)

// Query.

	// state query
	int            GetLine  ( void ) const  {  return ( m_LineNum );  }
	int            GetColumn( void ) const  {  return ( m_ColNum  );  }
	const YYSTYPE& GetLValue( void ) const  {  return ( m_LValue  );  }
	bool           IsEof    ( void ) const  {  return ( m_RawIter >= m_RawEnd );  }
	const BYTE*    GetRaw   ( void ) const  {  return ( m_RawBegin );  }

// Options.

	void SetTabStop( int tabStop )  {  m_TabStop = tabStop;  }

#	if GP_DEBUG
	void SetTrace( bool trace = true )  {  m_Trace = trace;  }
#	endif // GP_DEBUG

protected:

// Private methods.

	// internal utility
	inline int  Get            ( void );				// get next character
	inline void NextColumn     ( int c );				// advance to next column
	inline void SetupForUser   ( void );				// set up m_Text for user
	inline void SetupForScanner( void );				// set up m_Text for scanner
	       void InternalReset  ( void );				// reset the parser

	// user-callable stream methods
	int Input  ( void );								// get-input
	int PutBack( int c );								// put back a character

	// user-callable parsing utility methods
	int SkipComment( const char* endMatch, bool eofBad = true );	// skip comment input, plus is eof bad here?
	int MapEscapes ( int delim, int esc, bool doEscapes = true );	// map C escapes

// Scanner state data.

	YYSTYPE m_LValue;						// current token

	int m_Start;							// start state
	int m_End;								// end of pushback
	int m_LastChar;							// previous char

	int m_LineNum;							// line number (1-based)
	int m_ColNum;							// column number (1-based)
	int m_StartLine;						// line number for start of token (1-based)
	int m_StartCol;							// column number for start of token (1-based)
	int m_TextLen;							// m_Text token length

	char m_TextSave;						// saved m_Text[m_TextLen]

// Buffers.

	stdx::fast_vector <char>       m_Text;	// text buffer
	stdx::fast_vector <yy_state_t> m_State;	// state buffer

	const BYTE* m_RawBegin;					// start of raw data source
	const BYTE* m_RawEnd;					// off-end of raw data source
	const BYTE* m_RawIter;					// where in raw data we are

// Options.

	int m_TabStop;							// tab stop used for error reporting (default is 4)

#	if GP_DEBUG
	bool m_Trace;							// false if not tracing
#	endif // GP_DEBUG

	SET_NO_COPYING( yyScanner );
};

//////////////////////////////////////////////////////////////////////////////
// class yyScanner inline implementation

inline int yyScanner :: Get( void )
{
	return ( IsEof() ? EOF : *m_RawIter++ );
}

inline void yyScanner :: NextColumn( int c )
{
	if ( c == '\n' )
	{
		// advance to next line
		m_ColNum = 1;
		NextLine( m_LineNum );
	}
	else if ( c == '\t' )
	{
		// advance to next tab stop
		m_ColNum += m_TabStop - ((m_ColNum - 1) % m_TabStop);
	}
	else
	{
		++m_ColNum;
	}
}

inline void yyScanner :: SetupForUser( void )
{
	m_TextSave = m_Text[ m_TextLen ];
	m_Text[ m_TextLen ] = '\0';
}

inline void yyScanner :: SetupForScanner( void )
{
	m_Text[ m_TextLen ] = m_TextSave;
}

//////////////////////////////////////////////////////////////////////////////

@ END OF HEADER @

//////////////////////////////////////////////////////////////////////////////
//
// File     :  <yy Lexer>.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

@ GLOBAL DECLARATIONS @

//////////////////////////////////////////////////////////////////////////////
// macros

#define MAKE_NAME( x ) #x

#if GP_DEBUG
#define YY_DEBUG( fmt, a1, a2 )  { if ( m_Trace )  {  gpgenericf(( fmt, a1, a2 ));  }  }
#else
#define YY_DEBUG( fmt, a1, a2 )
#endif // GP_DEBUG

// do *NOT* redefine the following:
#define BEGIN  m_Start =
#define REJECT goto yy_reject
#define MORE   goto yy_more

// remove unnecessary warnings
#pragma warning ( disable : 4102 )	// unused label (potentially lots of 'em)
#pragma warning ( disable : 4702 )	// unreachable code (if YYERROR never used we get this)

//////////////////////////////////////////////////////////////////////////////
// class yyScanner implementation

yyScanner :: yyScanner( int bufferSize )
{
	InternalReset();

	m_Text .resize( bufferSize + 1 );			// text buffer
	m_State.resize( bufferSize + 1 );			// state buffer

	m_TabStop = 4;

#	if GP_DEBUG
	m_Trace = false;
#	endif // GP_DEBUG
}

yyScanner :: ~yyScanner( void )
{
	// this space intentionally left blank...
}

void yyScanner :: Reset( void )
{
	InternalReset();
}

void yyScanner :: Init( const_mem_ptr mem )
{
	Reset();

	m_RawBegin = rcast <const BYTE*> ( mem.mem );
	m_RawEnd   = m_RawBegin + mem.size;
	m_RawIter  = m_RawBegin;
}

// The actual lex scanner
//
// m_Text  [ 0 ... m_TextLen - 1 ]      = the current token
// m_Text  [ m_TextLen ... m_End - 1 ]  = pushed-back characters
// m_States[ 0 ... m_TextLen - 1 ]      = the states corresponding to m_Text
//
// When the user action routine is active, m_TextSave contains
// m_Text[ m_TextLen ], which is set to '\0'.

int yyScanner :: Scan( void )
{
	int c, i, yybase;
	unsigned yyst;       // state
	int yyfmin, yyfmax;  // yy_la_act indices of final states
	int yyoldi, yyoleng; // base i, m_TextLen before look-ahead
	int yyeof;           // 1 if eof has already been read

@ LOCAL DECLARATIONS @

	yyeof = 0;
	i = m_TextLen;
	SetupForScanner();

yy_again:

	m_TextLen = i;
	if ( i > 0 )
	{
		// determine previous char
		m_LastChar = m_Text[ i - 1 ];

		// scan previous token
		while ( i > 0 )
		{
			// fix m_LineNum
			if ( m_Text[ --i ] == YYNEWLINE )
			{
				++m_LineNum;
			}
		}
	}

	// adjust pushback
	m_End -= m_TextLen;
	if ( m_End > 0 )
	{
		::memmove( &*m_Text.begin(), &*(m_Text.begin() + m_TextLen), m_End );
	}
	i = 0;

yy_contin:

	yyoldi = i;
	yyst = yy_begin[ m_Start + ((m_LastChar == YYNEWLINE) ? 1 : 0) ];
	m_State[ i ] = yy_state_t( yyst );

	// run the state machine until it jams
	do
	{
		YY_DEBUG( "<state %d, i = %d>\n", yyst, i );

		if ( i >= scast <int> ( m_Text.size() - 1 ) )
		{
			Message( MESSAGE_FATAL, "token buffer overflow" );
		}

		// get input char
		if ( i < m_End )
		{
			// get pushback char
			c = m_Text[ i ];
		}
		else if ( !yyeof && ((c = Get()) != EOF) )
		{
			m_End = i + 1;
			m_Text[ i ] = scast <char> ( c );
		}
		else
		{
			c = EOF;               // just to make sure...
			m_Text[ i ] = '\0';    // seal off the token
			if ( i == yyoldi )     // no token
			{
				// no more...
				yyeof = 0;
				return ( 0 );
			}
			else
			{
				// don't re-read EOF
				yyeof = 1;
				break;
			}
		}

		YY_DEBUG( "<input %d = 0x%02x>\n", c, c );

		// look up next state
		while (   ((yybase = yy_base[yyst] + (unsigned char)( c )) > yy_nxtmax)
			   || (yy_check[yybase] != yy_state_t( yyst )) )
		{
			if ( yyst == yy_endst )
			{
				goto yy_jammed;
			}
			yyst = yy_default[ yyst ];
		}
		yyst = yy_next[ yybase ];

	yy_jammed:
		m_State[ ++i ] = yy_state_t( yyst );

	}	while ( yyst != yy_endst );

	YY_DEBUG( "<stopped %d, i = %d>\n", yyst, i );

	if ( yyst != yy_endst )
	{
		++i;
	}

yy_search:

	// search backward for a final state
	while ( --i > yyoldi )
	{
		yyst = m_State[ i ];
		if ( (yyfmin = yy_final[ yyst ]) < (yyfmax = yy_final[ yyst + 1 ]) )
		{
			goto yy_found;  // found final state(s)
		}
	}

	// no match, default action
	i = yyoldi + 1;
	PutBack( m_Text[ yyoldi ] );
	goto yy_again;

yy_found:

	YY_DEBUG( "<final state %d, i = %d>\n", yyst, i );

	yyoleng = i;         // save length for REJECT

	// pushback look-ahead RHS, handling trailing context
	if ( (c = int( yy_la_act[ yyfmin ] >> 9) - 1 ) >= 0 )
	{
		unsigned char* bv = yy_look + (c * YY_LA_SIZE);
		static unsigned char bits[ 8 ] =
		{
			1 << 0, 1 << 1, 1 << 2, 1 << 3,
			1 << 4, 1 << 5, 1 << 6, 1 << 7
		};

		for ( ; ; )
		{
			// no /
			if ( --i < yyoldi )
			{
				i = yyoleng;
				break;
			}

			yyst = m_State[ i ];
			if ( bv[ unsigned( yyst ) / 8 ] & bits[ unsigned( yyst ) % 8 ] )
			{
				break;
			}
		}
	}

	// prep for user
	m_TextLen = i;
	SetupForUser();

	// update location state
	m_StartLine = m_LineNum;
	m_StartCol  = m_ColNum;
	for ( const char* p = &*m_Text.begin() ; *p != '\0' ; ++p )
	{
		NextColumn( *p );
	}

	// perform action
	switch ( yy_la_act[ yyfmin ] & 0777 )
	{

@ ACTION CODE @

	}

	SetupForScanner();

	i = m_TextLen;
	goto yy_again;  // action fell though

yy_reject:

	SetupForScanner();

	// restore original m_Text
	i = yyoleng;
	if ( ++yyfmin < yyfmax )
	{
		// another final state, same length
		goto yy_found;
	}
	else
	{
		// try shorter m_Text
		goto yy_search;
	}

yy_more:

	SetupForScanner();

	i = m_TextLen;
	if ( i > 0 )
	{
		m_LastChar = m_Text[ i - 1 ];
	}
	goto yy_contin;
}

void yyScanner :: Message( eMessageType type, const char* msg, ... )
{
	// out leader
	gpdebuggerf(( MAKE_NAME( yyScanner ) " L%d: ", type ));

	// out message
	auto_dynamic_vsnprintf printer( msg, va_args( msg ) );
	gpdebugger( printer );
	gpdebugger( "\n" );
}

void yyScanner :: NextLine( int /*line*/ )
{
	// this space intentionally left blank...
}

int yyScanner :: Input( void )
{
	int c;
	if ( m_End > m_TextLen )
	{
		--m_End;
		::memmove( &*(m_Text.begin() + m_TextLen), &*(m_Text.begin() + m_TextLen + 1), m_End - m_TextLen );
		c = m_TextSave;
		SetupForUser();
	}
	else
	{
		c = Get();
	}
	m_LastChar = c;
	if ( c == YYNEWLINE )
	{
		++m_LineNum;
	}
	return ( c );
}

int yyScanner :: PutBack( int c )
{
	if ( m_End >= scast <int> ( m_Text.size() - 1 ) )
	{
		Message( MESSAGE_FATAL, "push-back buffer overflow" );
	}
	else
	{
		if ( m_End > m_TextLen )
		{
			m_Text[ m_TextLen ] = m_TextSave;
			::memmove( &*(m_Text.begin() + m_TextLen + 1), &*(m_Text.begin() + m_TextLen), m_End - m_TextLen );
			m_Text[ m_TextLen ] = 0;
		}
		++m_End;
		m_TextSave = scast <char> ( c );
		if ( c == YYNEWLINE )
		{
			--m_LineNum;
		}
	}
	return ( c );
}

// $$ future: make c-style comments nestable.

int yyScanner :: SkipComment( const char* endMatch, bool eofBad )
{
	int c = EOF;
	for ( const char* cp = endMatch ; *cp != '\0' ; )
	{
		c = Input();
		NextColumn( c );
		if ( c == EOF )
		{
			// only print this warning if the match isn't a newline
			if ( eofBad && !((endMatch[ 0 ] == '\n') && (endMatch[ 1 ] == '\0')) )
			{
				Message( MESSAGE_WARNING, "end of file in comment" );
			}
			break;
		}
		if ( c != *cp++ )
		{
			cp = endMatch;
			if ( c == *cp )
			{
				++cp;
			}
		}
	}

	// restore last newline
	if ( c == '\n' )
	{
		PutBack( c );
	}

	return ( c );
}

int yyScanner :: MapEscapes( int delim, int esc, bool doEscapes )
{
	// $ this code based on mks/libmks/yymapch.c

	int c, octv, n;

// Early bailouts.

	c = Input();
	if ( c == delim )
	{
		NextColumn( c );
		return ( EOF );
	}
	if ( (c <= 0) || (c == '\n') )
	{
		if ( c != EOF )
		{
			PutBack( c );
		}
		return ( -2 );
	}
	if ( (c != esc) || !doEscapes )
	{
		NextColumn( c );
		return ( c );
	}

// Process escape code.

	NextColumn( c );
	c = Input();
	switch ( c )
	{

	// Simple escapes.

		case ( 'a' ):  c = '\a';  break;  // alert/bell
		case ( 'b' ):  c = '\b';  break;  // backspace
		case ( 'v' ):  c = '\v';  break;  // vertical tab
		case ( 't' ):  c = '\t';  break;  // horizontal tab
		case ( 'n' ):  c = '\n';  break;  // line feed (newline)
		case ( 'f' ):  c = '\f';  break;  // form feed
		case ( 'r' ):  c = '\r';  break;  // carriage return

	// Normal characters.

		case ('?'):  case ('\"'):  case ('\''):  break;

	// Hex character.

		case ('x'):
		{
			NextColumn( c );
			for ( n = 1, octv = 0 ; n <= 2 ; ++n )
			{
				c = Input();
				if ( (c >= '0') && (c <= '9') )
				{
					NextColumn( c );
					c -= '0';
				}
				else if ( (c >= 'a') && (c <= 'f') )
				{
					NextColumn( c );
					c = (c - 'a') + 10;
				}
				else if ( (c >= 'A') && (c <= 'F') )
				{
					NextColumn( c );
					c = (c - 'A') + 10;
				}
				else
				{
					break;
				}
				octv = (octv * 16) + c;
			}
			PutBack( c );

			if ( n == 1 )
			{
				// no higits, it's ok
				return ( 'x' );
			}
			if ( octv > 255 )
			{
				Message( MESSAGE_WARNING, "hex character constant too large, clamping to 0xFF" );
				octv = 255;
			}
			return ( octv );
		}

#	if YYLEX_SUPPORT_OCTAL			// octal is no longer used these days

	// Octal character.

		case ('0'): case ('1'): case ('2'): case ('3'):
		case ('4'): case ('5'): case ('6'): case ('7'):
		{
			NextColumn( c );
			octv = c - '0';
			for ( n = 1 ; ((c = Input()) >= '0') && (c <= '7') && (n <= 3) ; ++n )
			{
				NextColumn( c );
				octv = octv * 010 + (c - '0');
			}
			PutBack( c );
			if ( octv > 255 )
			{
				Message( MESSAGE_WARNING, "octal character constant too large, clamping to 0513 (0xFF)" );
				octv = 255;
			}
			return ( octv );
		}

#	endif // 0

	// $$ FUTURE: add binary support via 0b1110001010 style thing

	// String continuation.

		case ('\r'):
		{
			Input();
		}
		case ('\n'):
		{
			NextColumn( '\n' );
			return ( MapEscapes( delim, esc ) );
		}
	}

	NextColumn( (c == '\n') ? 'n' : c );	// don't pass \n or it will reset the counter!!
	return ( c );
}

void yyScanner :: InternalReset( void )
{
	m_Start     = 0;
	m_End       = 0;
	m_LastChar  = YYNEWLINE;

	m_LineNum   = 1;
	m_ColNum    = 1;
	m_StartCol  = 1;
	m_StartLine = 1;
	m_TextLen   = 0;

	m_TextSave  = 0;

	m_RawBegin  = NULL;
	m_RawEnd    = NULL;
	m_RawIter   = NULL;
}

//////////////////////////////////////////////////////////////////////////////

@ end of yylex.cpp @
