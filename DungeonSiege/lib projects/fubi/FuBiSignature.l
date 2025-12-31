/* ///////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSignature.l
// Author(s):  Scott Bilas
//
// Summary  :  Contains the lexeme for FuBi signature parsing.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////// */

%{

//////////////////////////////////////////////////////////////////////////////

#include "StdHelp.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// local helpers

// access derived type
inline SignatureScanner* GetScanner( SignatureBaseScanner* obj )
	{  return ( (SignatureScanner*)obj );  }
inline const SignatureScanner* GetScanner( const SignatureBaseScanner* obj )
	{  return ( (const SignatureScanner*)obj );  }
#define SCANNER (*GetScanner( this ))

#if GP_DEBUG
#define DEBUG_MESSAGE( x ) Message x
#else // GP_DEBUG
#define DEBUG_MESSAGE( x )
#endif // GP_DEBUG

// found a token
#define TOKENIZE( t )  return ( m_LValue.m_Token = t );

//////////////////////////////////////////////////////////////////////////////
// string-to-keyword mapping

struct Keyword
{
	const char* m_Keyword;
	int m_Token;
};

bool operator < ( const Keyword& l, const char*    r )  {  return ( strcmp( l.m_Keyword, r ) < 0 );  }
bool operator < ( const char*    l, const Keyword& r )  {  return ( strcmp( l, r.m_Keyword ) < 0 );  }
bool operator < ( const Keyword& l, const Keyword& r )  {  return ( strcmp( l.m_Keyword, r.m_Keyword ) < 0 );  }

Keyword sKeywords[] =
{
	{ "__cdecl",         T_CDECL,           },
	{ "__fastcall",      T_FASTCALL,        },
	{ "__stdcall",       T_STDCALL,         },
	{ "__thiscall",      T_THISCALL,        },

	{ "static",          T_STATIC,          },
	{ "virtual",         T_VIRTUAL,         },
	{ "const",           T_CONST,           },
	{ "template",        T_TEMPLATE,        },

	{ "char",            T_CHAR,            },
	{ "short",           T_SHORT,           },
	{ "int",             T_INT,             },
	{ "long",            T_LONG,            },
	{ "signed",          T_SIGNED,          },
	{ "unsigned",        T_UNSIGNED,        },
	{ "float",           T_FLOAT,           },
	{ "double",          T_DOUBLE,          },
	{ "void",            T_VOID,            },
	{ "bool",            T_BOOL,            },
	{ "__int8",          T_INT8,            },
	{ "__int16",         T_INT16,           },
	{ "__int32",         T_INT32,           },
	{ "__int64",         T_INT64,           },

	{ "class",           T_CLASS,           },
	{ "struct",          T_STRUCT,          },
	{ "union",           T_UNION,           },
	{ "enum",            T_ENUM,            },

	{ "public",          T_PUBLIC,          },
	{ "private",         T_PRIVATE,         },
	{ "protected",       T_PROTECTED,       },

	{ "ResHandle",       T_RESHANDLE,       },
	{ "ResHandleFields", T_RESHANDLEFIELDS, },
	{ "mem_ptr",         T_MEM_PTR,         },
	{ "const_mem_ptr",   T_CONST_MEM_PTR,   },
	{ "gpbstring",       T_GPBSTRING,       },
	{ "std",             T_STD,             },
	{ "basic_string",    T_BASICSTRING,     },
	{ "gp_basic_string", T_GPBASICSTRING,   },
};

int FindKeyword( const char* text )
{
	// first sort the list
	static bool sSorted = false;
	if ( !sSorted )
	{
		std::sort( sKeywords, ARRAY_END( sKeywords ) );
		sSorted = true;
	}

	// do a bsearch to find the item
	const Keyword* found = stdx::binary_search( sKeywords, ARRAY_END( sKeywords ), text );
	if ( found == ARRAY_END( sKeywords ) )
	{
		return ( 0 );
	}
	else
	{
		return ( found->m_Token );
	}
}

//////////////////////////////////////////////////////////////////////////////


%}


%%


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** BEGIN OF PRODUCTIONS **
	*/

							/* identifiers */

[a-z_A-Z\$][a-z_A-Z0-9\$]*  {
								// first try keyword
								int token = FindKeyword( &*m_Text.begin() );
								if ( token == 0 )
								{
									// see if it is meant for replacement
									const char* replace = NameReplaceSpec::GetReplacement( &*m_Text.begin() );
									if ( replace == NULL )
									{
										// use it as-is
										m_LValue.m_String = &*m_Text.begin();
										token = T_IDENTIFIER;
									}
									else
									{
										// use the replacement version
										m_LValue.m_String = replace;
										token = T_IDENTIFIER;
									}
								}
								TOKENIZE( token );
							}

							/* constants */

[0-9]*                      {
								m_LValue.m_Int = stringtool::strtol( &*m_Text.begin() );
								TOKENIZE( T_INTCONSTANT );
							}

							/* symbols */

[-(),/%&*<>:]               TOKENIZE( *m_Text.begin() );
"::"                        TOKENIZE( T_SCOPE );
\.\.\.                      TOKENIZE( T_VARARG );
"?"                         TOKENIZE( T_ILLEGAL );

							/* everything else */

[ \t\v\n\r]+                {
								;  /* whitespace - do nothing */
							}

\0                          {
								DEBUG_MESSAGE(( MESSAGE_WARNING, "found a null in stream" ));
							}

.                           {
								DEBUG_MESSAGE(( MESSAGE_WARNING, "unrecognized token" ));
							}


	 /* ////////////////////////////////////////////////////////////////////////
	// ** END OF PRODUCTIONS **
   */


%%


//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
