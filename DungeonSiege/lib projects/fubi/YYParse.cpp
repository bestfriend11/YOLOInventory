$@
$H

$L#line 5 "$P"

//////////////////////////////////////////////////////////////////////////////
//
// File     :  <yy Parser>.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains yacc-generated parser code. Class design loosely based
//             on original (c) 1991 MKS prototype code. Algorithmic code is
//             nearly identical. Class is yyParser, though prototype name can
//             be changed as command line option.
//
// Note     :  As you change this file, keep the #line directives up to date.
//             They should always be one greater than the line they're on.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

// include requirements: gpcoll, stdinc/core, stringtool

//////////////////////////////////////////////////////////////////////////////
// class $pParser declaration

// note: YYSYNC can be #defined to cause parse() to attempt to always hold a
//       lookahead token

// note: for debugging, use x$() and x$( int ) to dereference the $$ and $1,
//       $2 etc in the watch window. also use token() to get token string.

class $pParser
{
public:
	SET_NO_INHERITED( $pParser );

	enum eMessageType
	{
		MESSAGE_INFO,		// informational
		MESSAGE_WARNING,	// simple warning
		MESSAGE_ERROR,		// nonfatal error
		MESSAGE_FATAL,		// fatal error
	};

// Construction.

	         $pParser( int bufferSize = 150 );			// default size of the stacks
	virtual ~$pParser( void );							// destructor

	virtual void Reset( void );							// reset parser

// Action.

	virtual int  Parse        ( void );					// parse with given scanner
	virtual void Message      ( eMessageType type,		// internally called on exception (OVERRIDE THIS)
								const char* msg, ... );
	virtual void InternalError( eMessageType type,		// error occurred reported internally
								const char* msg );

// Query.

	// ordinary
	int IsRecovering( void ) const  {  return ( m_Error != 0 );  }

	// override implementation
	virtual int            Scan        ( void ) = 0;		// scan the next token (generally used internally)
	virtual const YYSTYPE& GetLValue   ( void ) const = 0;	// get current lvalue
	virtual bool           IsEof       ( void ) const = 0;	// at eof?

protected:

// Constants.

	enum
	{
		ERROR_CODE      = 256,			// YACC 'error' value
		MIN_STATE_COUNT = 20,			// not useful to be too small!
	};

// Private methods.

	// internal utility
	void ClearError   ( void )  {  m_Error =    0;  }
	void ClearInput   ( void )  {  m_Char  =   -1;  }
	void InternalReset( void )  {  m_Reset = true;  }

// State data.

	bool m_Reset;								// if set, reset state next parse

	short m_TableIndex;							// table index
	short m_CurrentState;						// current state

	stdx::fast_vector <short>   m_StateStack;	// states stack
	stdx::fast_vector <YYSTYPE> m_ValueStack;	// values stack

	short*   m_StateStackTop;					// top of state stack
	YYSTYPE* m_ValueStackTop;					// top of value stack
	YYSTYPE  m_SavedLValue;						// saved m_LValue;
	YYSTYPE  m_Value;							// $$
	YYSTYPE* m_SavedValueStackTop;				// $n

	int m_Char;									// current token
	int m_ErrorCount;							// (cumulative) error count
	int m_Error;								// error flag

	SET_NO_COPYING( $pParser );
};

//////////////////////////////////////////////////////////////////////////////
// debug struct declarations

#define YYDEBUG GP_DEBUG

#if YYDEBUG

struct $pNamedType
{
	const char* m_Name;  // printable name
	int m_Token;         // token #
	int m_Type;          // token type
};

struct $pTypedRules
{
	const char* m_Name;  // compressed rule string
	int m_Type;          // rule result type
};

#endif // YYDEBUG

//////////////////////////////////////////////////////////////////////////////

$E

$L#line 139 "$P"

//////////////////////////////////////////////////////////////////////////////
//
// File     :  <yy Parser>.cpp (GK3: Scripting.Parser)
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// macros

#if GP_DEBUG
YYSTYPE* g_DbgX$  = NULL;
YYSTYPE* g_DbgX$0 = NULL;
int      g_DbgX$N = 0;
YYSTYPE& x$( void )
{
	return ( *g_DbgX$ );
}
YYSTYPE& x$( int i )
{
	return ( g_DbgX$0[ i - g_DbgX$N ] );
}
const char* token( int token )
{
	const yyNamedType* i, * begin = yyTokenTypes, * end = begin + ELEMENT_COUNT( yyTokenTypes );
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Token == token )
		{
			return ( i->m_Name );
		}
	}
	return ( "<Invalid Token>" );
}
#endif // GP_DEBUG

#define MAKE_NAME( x ) #x

// simulate bitwise negation as if it was done on a two's complement machine.
// this makes the generated code portable to machines with different
// representations of integers (ie. signed magnitude).
#define NEGATE(s)       (-((s)+1))

// use these macros in action code
#define YYERROR         goto yyerrlabel
#define YYRETURN( val ) return ( val )
#define YYACCEPT        YYRETURN( 0 )
#define YYABORT         YYRETURN( 1 )
#define yypvt           m_SavedValueStackTop
#define yyval           m_Value

// unnecessary warnings
#pragma warning ( disable : 4102 )	// unused label (potentially lots of 'em)
#pragma warning ( disable : 4702 )	// unreachable code (if YYERROR never used we get this)

// implementation of common code for yysync
#define YYSCAN() \
	if ( m_Char < 0 ) \
	{ \
		if ( (m_Char = Scan()) < 0 ) \
		{ \
			if ( m_Char == -2 ) \
			{ \
				YYABORT; \
			} \
			m_Char = 0; \
		} \
		m_SavedLValue = GetLValue(); \
	}

//////////////////////////////////////////////////////////////////////////////
// class $pParser implementation

$pParser :: $pParser( int bufferSize )
{
	Reset();

	m_StateStack.resize( bufferSize + 1 );
	m_ValueStack.resize( bufferSize + 1 );
}

$pParser :: ~$pParser( void )
{
	// this space intentionally left blank...
}

void $pParser :: Reset( void )
{
	InternalReset();
}

int $pParser :: Parse( void )
{
	//  note that this code is reentrant; you can return a value and then
	//   resume parsing by recalling parse(). call reset() before parse() if
	//   you want a fresh start.

	const short* yyp;
	const short* yyq;
	int yyj;

$A

	// start new parse
	if ( m_Reset )
	{
		m_ErrorCount    = 0;
		m_Error         = 0;
		m_StateStackTop = &*m_StateStack.begin();
		m_ValueStackTop = &*m_ValueStack.begin();
		m_CurrentState  = YYS0;

		ClearInput();

		m_Reset = false;
	}
	else
	{
		goto yyNext;	// continue saved parse after action
	}

yyStack:

	if ( ++m_StateStackTop >= &*m_StateStack.end() )
	{
		InternalError( MESSAGE_FATAL, "parser stack overflow" );
		YYABORT;
	}

	*m_StateStackTop = m_CurrentState;		// stack current state
	*++m_ValueStackTop = m_Value;			// ... and value

// Look up next action in action table.

yyEncore:

#	ifdef YYSYNC
	YYSCAN();
#	endif

	// simple state
	if ( m_CurrentState >= ELEMENT_COUNT( yypact ) )
	{
		// reduce in any case
		m_TableIndex = scast <short> ( m_CurrentState - YYDELTA );
	}
	else
	{
		if( *(yyp = &yyact[ yypact[ m_CurrentState ] ]) >= 0 )
		{
#			ifndef YYSYNC
			YYSCAN();
#			endif

			// look for a shift on m_Char
			yyq = yyp;
			m_TableIndex = scast <short> ( m_Char );
			while ( m_TableIndex < *yyp++ )
			{
				;   // empty on purpose
			}
			if ( m_TableIndex == yyp[ - 1] )
			{
				m_CurrentState = scast <short> ( NEGATE( yyq[ yyq-yyp ] ) );
				m_Value = m_SavedLValue;		// stack value
				ClearInput();					// clear token

				if ( m_Error )
				{
					--m_Error;		// successful shift
				}
				goto yyStack;
			}
		}

		// ** fell through - take default action **

		// simple state
		if ( m_CurrentState >= ELEMENT_COUNT( yydef ) )
		{
			goto yyError;
		}

		// default == reduce?
		if ( (m_TableIndex = yydef[ m_CurrentState ] ) < 0 )
		{
			// search exception table
			yyp = &yyex[ NEGATE( m_TableIndex ) ];

			#ifndef YYSYNC
			YYSCAN();
			#endif

			while( ((m_TableIndex = *yyp) >= 0) && (m_TableIndex != m_Char) )
			{
				yyp += 2;
			}
			m_TableIndex = yyp[ 1 ];
		}
	}

	yyj = yyrlen[ m_TableIndex ];

	m_StateStackTop -= yyj;						// pop stacks
	m_SavedValueStackTop = m_ValueStackTop;		// save top
	m_ValueStackTop -= yyj;
	m_Value = m_ValueStackTop[ 1 ];				// default action $$ = $1

#	if GP_DEBUG
	g_DbgX$  = &yyval;
	g_DbgX$0 = yypvt;
	g_DbgX$N = yyj;
#	endif // GP_DEBUG

	// perform semantic action
	switch ( m_TableIndex )
	{
		$A
		$L#line 363 "$P"
		case ( YYrACCEPT ):
			YYACCEPT;
		case ( YYrERROR ):
			goto yyError;
	}

// Look up next state in goto table.

yyNext:

	yyp = &yygo[ yypgo[ m_TableIndex ] ];
	yyq = yyp++;
	m_TableIndex = *m_StateStackTop;
	while ( m_TableIndex < *yyp++ )
	{
		;   // empty on purpose
	}
	m_CurrentState = scast <short> ( NEGATE( m_TableIndex == *--yyp ? yyq[ yyq-yyp ] : *yyq ) );

	goto yyStack;

	// come here from YYERROR

yyerrlabel:

	m_Error = 1;
	if ( m_TableIndex == YYrERROR )
	{
		m_StateStackTop--;
		m_ValueStackTop--;
	}

yyError:

	switch ( m_Error )
	{
		// new error
		case ( 0 ):
		{
			++m_ErrorCount;
			m_TableIndex = scast <short> ( m_Char );
			InternalError( MESSAGE_ERROR, IsEof() ? "unexpected end of file/stream" : "syntax error" );
			if ( m_TableIndex != m_Char )
			{
				// user has changed the current token - try again
				++m_Error;
				goto yyEncore;
			}
		}

		// partially recovered
		case ( 1 ):
		case ( 2 ):
		{
			m_Error = 3;		// need 3 valid shifts to recover

			// ** pop states, looking for a shift on `error' **

			for ( ; m_StateStackTop > &*m_StateStack.begin() ; m_StateStackTop--, m_ValueStackTop-- )
			{
				// simple state
				if ( *m_StateStackTop >= ELEMENT_COUNT( yypact ) )
				{
					continue;
				}
				yyp = &yyact[ yypact[ *m_StateStackTop ] ];
				yyq = yyp;

				do
				{
					;   // empty on purpose
				}
				while ( ERROR_CODE < *yyp++ );

				if ( ERROR_CODE == yyp[ -1 ] )
				{
					m_CurrentState = scast <short> ( NEGATE( yyq[ yyq-yyp ] ) );
					goto yyStack;
				}

				// pop stacks; try again
			}
		}	break;	// no shift on error - abort

		// erroneous token after an error - discard it
		case (3):
		{
			// but not EOF
			if ( m_Char == 0 )
			{
				break;
			}
			ClearInput();
			goto yyEncore;		// try again in same state
		}
	}
	YYABORT;
}

void $pParser :: Message( eMessageType type, const char* msg, ... )
{
	// out leader
	gpdebuggerf(( MAKE_NAME( $pParser ) " L%d: ", type ));

	// out message
	auto_dynamic_vsnprintf printer( msg, va_args( msg ) );
	gpdebugger( printer );
	gpdebugger( "\n" );
}

void $pParser :: InternalError( eMessageType type, const char* msg )
{
	Message( type, msg );
}

//////////////////////////////////////////////////////////////////////////////
// extra code

$T

//////////////////////////////////////////////////////////////////////////////
