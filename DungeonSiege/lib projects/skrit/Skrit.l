/* ///////////////////////////////////////////////////////////////////////////
//
// File     :  Skrit.l
// Author(s):  Scott Bilas
//
// Summary  :  Contains the lexeme for the Skrit language.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////// */

%{

//////////////////////////////////////////////////////////////////////////////

#include "StdHelp.h"
#include "StringTool.h"

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// local helpers

// access derived type
inline Scanner* GetScanner( BaseScanner* obj )
	{  return ( (Scanner*)obj );  }
inline const Scanner* GetScanner( const BaseScanner* obj )
	{  return ( (const Scanner*)obj );  }
#define SCANNER (*GetScanner( this ))

//////////////////////////////////////////////////////////////////////////////
// string-to-keyword mapping

struct Keyword
{
	const char* m_Keyword;
	int         m_Token;
};

bool operator < ( const Keyword& l, const char*    r )  {  return ( stricmp( l.m_Keyword, r ) < 0 );  }
bool operator < ( const char*    l, const Keyword& r )  {  return ( stricmp( l, r.m_Keyword ) < 0 );  }
bool operator < ( const Keyword& l, const Keyword& r )  {  return ( stricmp( l.m_Keyword, r.m_Keyword ) < 0 );  }

Keyword sKeywords[] =
{
	{  "if",         T_IF,          },
	{  "else",       T_ELSE,        },
	{  "while",      T_WHILE,       },
	{  "break",      T_BREAK,       },
	{  "continue",   T_CONTINUE,    },
	{  "forever",    T_FOREVER,     },
	{  "return",     T_RETURN,      },

	{  "bool",       T_BOOL,        },
	{  "int",        T_INT,         },
	{  "float",      T_FLOAT,       },
	{  "string",     T_STRING,      },
	{  "void",       T_VOID,        },
	{  "shared",     T_SHARED,      },
	{  "property",   T_PROPERTY,    },
	{  "hidden",     T_HIDDEN,      },

	{  "sitnspin",   T_SITNSPIN,    },
	{  "abort",      T_ABORT,       },
	{  "msec",       T_MSEC,        },
	{  "frames",     T_FRAMES,      },
	{  "doc",        T_DOC,         },

	{  "event",      T_EVENT,       },
	{  "trigger",    T_TRIGGER,     },
	{  "at",         T_AT,          },

	{  "__LINE__",   T_MACRO_LINE,  },
	{  "__NAME__",   T_MACRO_NAME,  },

	{  "state",      T_STATE,       },
	{  "poll",       T_POLL,        },
	{  "transition", T_TRANSITION,  },
	{  "and",        T_ANDT,        },

	{  "startup",    T_STARTUP,     },
	{  "setstate",   T_SETSTATE,    },

	{  "true",       T_TRUE,        },
	{  "false",      T_FALSE,       },
	{  "null",       T_NULL,        },

	{  "this",       T_THIS,        },
	{  "vm",         T_VM,          },
	{  "owner",      T_OWNER,       },

	{  "include",    T_INCLUDE,     },
	{  "option",     T_OPTION,      },
	{  "only",       T_ONLY,        },
};

//////////////////////////////////////////////////////////////////////////////

%}


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** MACROS **
	*/

digit10      ([0-9])
digit16      ([0-9A-Fa-f])
exp          ([eE][-+]?{digit10}+)
whitespace   ([ \t\v\r]+)
identifier   ([a-z_A-Z][a-z_A-Z0-9]*)


%%


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** BEGIN OF PRODUCTIONS **
	*/

	/*	ParseNode usage documentation:

			m_Token    - token code (defined in .y)
			m_Int      - int constants
			m_Float    - float constants
			m_Location - start of token

		Special:

			m_Int can also be the offset into the string table for a string
			literal.

			m_Int can also be the offset into the identifier table for a
			user or system identifier.
	*/


						/* comments */

"/*"                    {
							SkipComment( "*/" );
							if ( SCANNER.IsPreprocessing() && (GetLine() != SCANNER.m_PreprocessLine) )
							{
								return ( '\n' );
							}
						}

"//"                    {
							SkipComment( "\n", false );
							if ( SCANNER.IsPreprocessing() )
							{
								return ( '\n' );
							}
						}

"*/"                    {
							Message( MESSAGE_ERROR, "'*/': found comment end with no start" );
						}

						/* preprocessor */

^{whitespace}?"#"		return ( T_PREPROCESSOR );

						/* identifiers */

{identifier}\$?         return ( SCANNER.ScanIdentifier() );

						/* symbols */

"<="                    return ( T_LE         );
">="                    return ( T_GE         );
"=="                    return ( T_EQ         );
"~="                    return ( T_AEQ        );		// "approximately equal"
"!="                    return ( T_NE         );
"<>"                    return ( T_NE         );
"|"                     return ( T_BOR        );
"^"                     return ( T_BXOR       );
"&"                     return ( T_BAND       );
"|="                    return ( T_ABOR       );
"^="                    return ( T_ABXOR      );
"&="                    return ( T_ABAND      );
"||"                    return ( T_OR         );
"&&"                    return ( T_AND        );
"->"                    return ( T_ARROW      );
"+="                    return ( T_AADD       );		// assign add
"-="                    return ( T_ASUB       );		// assign subtract
"*="                    return ( T_AMUL       );		// assign multiply
"/="                    return ( T_ADIV       );		// assign divide
"%="                    return ( T_AMOD       );		// assign modulus
"**"                    return ( T_POW        );		// pow()
"**="                   return ( T_APOW       );		// assign pow()
"[["                    return ( T_ONLY_BEGIN );
"]]"                    return ( T_ONLY_END   );

[-()=;:,.!~+/%*<>{}?]   return ( *m_Text.begin() );

						/* integer constants */

{digit10}+              return ( SCANNER.ScanIntConstant() );
0[xX]{digit16}+         return ( SCANNER.ScanIntConstant() );

						/* float constants */

[0-9]+\.[0-9]*{exp}?    |
[0-9]*\.[0-9]+{exp}?    |
[0-9]+{exp}             return ( SCANNER.ScanFloatConstant() );

						/* string constants */

\"                      return ( SCANNER.ScanStringConstant() );
'                       return ( SCANNER.ScanCharConstant() );

						/* unsupported */

"++"					|
"--"					{
							Message( MESSAGE_ERROR, "'%s' is unsupported in this version of the compiler, sorry", m_Text.begin() );
						}

						/* everything else */

{whitespace}            ;  /* whitespace - do nothing */

\n                      {
							// newline - do nothing unless we are preprocessing
							if ( SCANNER.IsPreprocessing() )
							{
								return ( '\n' );
							}
						}

\0                      {
							Message( MESSAGE_WARNING, "'\\0': found a null in stream" );
						}

.                       {
							//$$$ this can give more info than THAT...print out token text by searching for its end
							Message( MESSAGE_ERROR, "unrecognized token" );
						}


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** END OF PRODUCTIONS **
	*/


%%


//////////////////////////////////////////////////////////////////////////////
// class Scanner implementation

Scanner :: Scanner( Compiler* compiler )
	: m_Compiler( compiler )
{
	// make sure the list is sorted
	static bool sSorted = false;
	if ( !sSorted )
	{
		std::sort( sKeywords, ARRAY_END( sKeywords ) );
		sSorted = true;
	}

	// clear vars
	m_PreprocessLine = 0;
	m_DoEscapes = true;
	m_SkipMode = SKIP_NONE;
}

Scanner :: ~Scanner( void )
{
	// this space intentionally left blank...
}

void Scanner :: Reset( void )
{
	Inherited::Reset();
	m_Strings.clear();
	m_PreprocessLine = 0;
	m_DoEscapes = true;
	m_SkipMode = SKIP_NONE;
}

int Scanner :: Scan( void )
{
	// scan
	int rc = Inherited::Scan();

	// note where we were
	m_LValue.m_Location = Location( this, m_StartLine, m_StartCol );
	m_LValue.m_Token = rc;

	// special preprocessor handling
	if ( !IsPreprocessing() && (rc == T_PREPROCESSOR) )
	{
		if ( Preprocess() )
		{
			// done with preprocessing, re-scan to get next token
			rc = m_Compiler->GetScanner()->Scan();
		}
		else
		{
			// eof
			rc = 0;
		}
	}
	else if ( m_SkipMode == SKIP_KEEP )
	{
		// if skip-keep, then pass everything through except the end which we chuck
		if ( rc == T_ONLY_END )
		{
			// clear it
			ClearSkipMode();

			// get next token (recursively)
			rc = Scan();
		}
	}
	else if ( m_SkipMode == SKIP_CODE )
	{
		// keep skipping until we run out or we hit end
		while ( rc != 0 )
		{
			if ( rc == T_ONLY_END )
			{
				// clear it
				ClearSkipMode();

				// get next token (recursively)
				rc = Scan();
				break;
			}
			rc = Inherited::Scan();
		}
	}

	// done
	return ( rc );
}

bool Scanner :: SetSkipMode( bool skipCode )
{
	if ( m_SkipMode != SKIP_NONE )
	{
		return ( false );
	}

	m_SkipMode = skipCode ? SKIP_CODE : SKIP_KEEP;
	return ( true );
}

void Scanner :: ClearSkipMode( void )
{
	gpassert( m_SkipMode != SKIP_NONE );
	m_SkipMode = SKIP_NONE;
}

static const int MP_ANY = -1;
static const int MP_SKIP_WS = -2;

// $ terminate each of these with a 0, second param is bool for whether or not
//   to do escapes, -1 for "take any". note that doEscapes should be turned off
//   for path parsing (because it includes backslashes).
static const int s_IncludeRule[] =  {  T_INCLUDE, false /*doEscapes*/, T_STRINGCONSTANT, '\n', 0  };
static const int s_OptionRule [] =  {  T_OPTION,  true  /*doEscapes*/, MP_ANY, T_SYSID, '\n', 0  };
static const int s_OnlyRule   [] =  {  T_ONLY,    true  /*doEscapes*/, '(', MP_ANY, ')', MP_SKIP_WS, T_ONLY_BEGIN, 0  };

// these are our rules
static const int* s_MicroRules[] =  {  s_IncludeRule, s_OptionRule, s_OnlyRule,  };

bool Scanner :: Preprocess( void )
{
	gpassert( !IsPreprocessing() );

	// we're preprocessing
	m_PreprocessLine = m_StartLine;

	// this will store our temporary local stack
	stdx::fast_vector <ParseNode> valueStack;

	// figure out which microrule we've got
	int rc = Scan();
	bool success = false;
	const int** ir, ** irbegin = s_MicroRules, ** irend = ARRAY_END( s_MicroRules );
	for ( ir = irbegin ; ir != irend ; ++ir )
	{
		// match?
		if ( **ir == rc )
		{
			// get whether or not to do escape chars
			bool doEscapes = !!(*ir)[ 1 ];

			// microparse
			m_DoEscapes = doEscapes;
			bool scan = true;
			const int* it = *ir + 2;
			for ( ; ; )
			{
				if ( scan )
				{
					rc = Scan();
				}
				scan = true;

				if ( (*it == rc) || (*it == MP_ANY) )
				{
					if ( rc != '\n' )
					{
						valueStack.push_back( m_LValue );
					}
				}
				else if ( *it == MP_SKIP_WS )
				{
					if ( rc == '\n' )
					{
						// just skip it
						continue;
					}
					scan = false;
				}
				else
				{
					// abort
					ir = irend;
					break;
				}

				++it;

				if ( *it == 0 )
				{
					success = true;
					break;
				}
			}
			m_DoEscapes = true;

			// done
			break;
		}
	}

	// if not terminator, then skip to end
	if ( !success && (rc != 0) )
	{
		if ( rc != '\n' )
		{
			Message( MESSAGE_ERROR, "'#': found unexpected token '%s' in preprocessor statement", m_Text.begin() );
			if ( SkipComment( "\n", false ) == EOF )
			{
				rc = 0;
			}
			ir = irend;
		}
		else if ( ir == irend )
		{
			Message( MESSAGE_ERROR, "'#': found unexpected end-of-line in preprocessor statement" );
		}
	}

	// eof??
	if ( rc == 0 )
	{
		Message( MESSAGE_ERROR, "'#': unexpected end of file/stream found in preprocessor statement" );
		ir = irend;
	}

	// ok?
	if ( ir != irend )
	{
		// process #include
		if ( **ir == T_INCLUDE )
		{
			// attempt to open the file for including
			m_Compiler->IncludeDirective( valueStack[ 0 ] );
		}
		else if ( **ir == T_OPTION )
		{
			// get enabling
			int token = valueStack[ 0 ].m_Token;
			if ( (token == '+') || (token == '-') )
			{
				// attempt to set options
				m_Compiler->OptionDirective( valueStack[ 1 ], token == '+' );
			}
			else
			{
				// bad
				Message( valueStack[ 0 ].m_Location, MESSAGE_ERROR, "'#option': expecting '+' or '-'" );
			}
		}
		else if ( **ir == T_ONLY )
		{
			// attempt to switch modes
			m_Compiler->OnlyDirective( valueStack[ 1 ] );
		}
	}

	// done
	m_PreprocessLine = 0;
	return ( rc != 0 );
}

int Scanner :: ScanIdentifier( void )
{
	// detect sys/user
	if ( m_Text[ m_TextLen - 1 ] != '$' )
	{
		// find the item as a keyword
		const Keyword* found = stdx::binary_search( sKeywords, ARRAY_END( sKeywords ), &*m_Text.begin() );
		if ( found != ARRAY_END( sKeywords ) )
		{
			switch ( found->m_Token )
			{
				case ( T_TRUE ):
				{
					m_LValue.m_Token = T_BOOLCONSTANT;
					m_LValue.m_Int = 1;
				}	break;
				case ( T_FALSE ):
				{
					m_LValue.m_Token = T_BOOLCONSTANT;
					m_LValue.m_Int = 0;
				}	break;
				case ( T_MACRO_LINE ):
				{
					m_LValue.m_Token = T_INTCONSTANT;
					m_LValue.m_Int = m_LValue.m_Location.m_Line;
				}	break;
				case ( T_MACRO_NAME ):
				{
					m_LValue.m_Token = T_STRINGCONSTANT;
#					if !GP_ERROR_FREE
					m_LValue.m_Int   = m_Compiler->AddStringConstant( m_Name );
#					else // !GP_ERROR_FREE
					m_LValue.m_Int   = m_Compiler->AddStringConstant( "?" );
#					endif // !GP_ERROR_FREE
				}	break;
				default:
				{
					m_LValue.m_Token = found->m_Token;
				}
			}
		}
		else
		{
			// store as-is - parser will manage it after it has some context
			m_LValue.m_Int   = AddCurrentString();
			m_LValue.m_Token = T_SYSID;
		}

		return ( m_LValue.m_Token );
	}
	else
	{
		m_LValue.m_Int = AddCurrentString();
		return ( m_LValue.m_Token = T_USERID );
	}
}

int Scanner :: ScanIntConstant( void )
{
	if ( m_Compiler->GetOptForceFloatConstants() )
	{
		return ( ScanFloatConstant() );
	}

	int err = 0;
	m_LValue.m_Int = stringtool::strtol( &*m_Text.begin(), NULL, 0, &err );

	if ( err == ERANGE )
	{
		Message( MESSAGE_WARNING, "integer constant is out of range, using 0 instead" );
		m_LValue.m_Int = 0;
	}

	return ( T_INTCONSTANT );
}

int Scanner :: ScanFloatConstant( void )
{
	int err = 0;
	m_LValue.m_Float = scast <float> ( stringtool::strtod( &*m_Text.begin(), NULL, &err ) );

	if ( err == ERANGE )
	{
		Message( MESSAGE_WARNING, "float point constant is out of range, using 0.0 instead" );
		m_LValue.m_Float = 0;
	}

	return ( T_FLOATCONSTANT );
}

int Scanner :: ScanStringConstant( void )
{
	// set up buffer
	char buffer[ 512 ];
	buffer[ ELEMENT_COUNT( buffer ) - 1 ] = '\0';
	gpstring target;
	bool usedString = false;

	// get some vars
	char* ptr = buffer;
	int c, i = 0;

	// copy characters over until either the buffer is full or we are done
	// (possibly via error). note that the "full" buffer includes room for null.
	while ( (c = MapEscapes( '\"', '\\', m_DoEscapes )) >= 0 )
	{
		*ptr = scast <char> ( c );

		// buffer full?
		if ( i >= (ELEMENT_COUNT( buffer ) - 1) )
		{
			target += buffer;		// append text
			ptr = buffer;			// and reset vars
			i = 0;
			usedString = true;		// used the string
		}
		else
		{
			++ptr;
			++i;
		}
	}

	// now terminate the string
	*ptr = '\0';

	// unterminated string
	if ( c == -2 )
	{
		Message( MESSAGE_WARNING,
				 "string constant started on (%d,%d) has no closing quote on this line",
				 m_StartLine, m_StartCol );
	}

	// add the string
	if ( usedString )
	{
		m_LValue.m_Int = m_Compiler->AddStringConstant( target, target.length() );
	}
	else
	{
		m_LValue.m_Int = m_Compiler->AddStringConstant( buffer, ptr - buffer );
	}
	return ( T_STRINGCONSTANT );
}

int Scanner :: ScanCharConstant( void )
{
	// get some vars
	m_LValue.m_Int = 0;
	char* ptr = (char*)&m_LValue.m_Int;

	// copy characters over until the buffer is full
	for ( int c, i = 0 ; (c = MapEscapes( '\'', '\\' )) >= 0 ; ++i )
	{
		if ( i == 4 )
		{
			// buffer full?
			Message( MESSAGE_WARNING, "character constant is too long" );
		}
		else if ( i < 4 )
		{
			// shift and save
			m_LValue.m_Int <<= 8;
			*ptr = scast <char> ( c );
		}
	}

	// unterminated string
	if ( c == -2 )
	{
		Message( MESSAGE_WARNING,
				 "character constant started on (%d,%d) has no closing quote on this line",
				 m_StartLine, m_StartCol );
	}

	// done
	return ( T_INTCONSTANT );
}

UINT Scanner :: AddCurrentString( void )
{
	return ( AppendToBuffer( m_Strings, const_mem_ptr( &*m_Text.begin(), m_TextLen + 1 ) ) );
}

int GetKeywordCount( void )
{
	return ( ELEMENT_COUNT( sKeywords ) );
}

const char* GetKeyword( int index )
{
	gpassert( (index >= 0) && (index < GetKeywordCount()) );
	return ( sKeywords[ index ].m_Keyword );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
