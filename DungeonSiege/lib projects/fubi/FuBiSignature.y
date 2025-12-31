/* ///////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSignature.y
// Author(s):  Scott Bilas
//
// Summary  :  Contains the production rules for FuBi signature parsing.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////// */

%{

//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiSignature.h"
#include "FuBi.h"

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// local helpers

// generated constants will give these warnings (ok to ignore)
#pragma warning ( disable : 4309 )	// 'initializing' : truncation of constant value
#pragma warning ( disable : 4305 )	// 'initializing' : truncation from 'const int' to 'short'

// access derived type
inline SignatureParser* GetParser( SignatureBaseParser* obj )
	{  return ( (SignatureParser*)obj );  }
inline const SignatureParser* GetParser( const SignatureBaseParser* obj )
	{  return ( (const SignatureParser*)obj );  }
#define PARSER  (*GetParser( this ))
#define SCANNER (PARSER.m_Scanner)
#define SPEC    (*PARSER.m_FunctionSpec)

#if GP_DEBUG
#define DEBUG_MESSAGE( x ) Message x
#else // GP_DEBUG
#define DEBUG_MESSAGE( x )
#endif // GP_DEBUG

#define YYIGNORE YYRETURN( 2 )

//////////////////////////////////////////////////////////////////////////////

%}


	  /*/////////////////////////////////////////////////////////////////////////
	 // ** TOKENS **
	*/

/* KEYWORD TOKENS */

	/* calling convention specifiers */
%token T_CDECL
%token T_FASTCALL
%token T_STDCALL
%token T_THISCALL

	/* misc specifiers */
%token T_STATIC
%token T_VIRTUAL
%token T_CONST
%token T_TEMPLATE

	/* types and type modifiers */
%token T_CHAR T_SHORT T_INT T_LONG
%token T_SIGNED T_UNSIGNED
%token T_FLOAT T_DOUBLE
%token T_VOID
%token T_BOOL
%token T_INT8 T_INT16 T_INT32 T_INT64

	/* class keys */
%token T_CLASS T_STRUCT T_UNION T_ENUM

	/* access specifiers */
%token T_PUBLIC T_PRIVATE T_PROTECTED

/* TYPE SYSTEM TOKENS */

%token T_RESHANDLE T_RESHANDLEFIELDS
%token T_MEM_PTR T_CONST_MEM_PTR T_GPBSTRING T_STD T_BASICSTRING T_GPBASICSTRING

/* OTHER TOKENS */

	/* misc */
%token T_IDENTIFIER
%token T_INTCONSTANT
%token T_SCOPE
%token T_VARARG
%token T_ILLEGAL


%%


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** BEGIN OF PRODUCTIONS **
	*/

	/* note: generally try to use left-recursion (it's more efficient).
			 see pg 197 in the bird book or MKS yacc support page for why.

			 exprlist:  exprlist ',' expr ;  -- left recursion
			 exprlist:  expr ',' exprlist ;  -- right recursion

	   note: don't bother with error recovery at all here. we're only going
			 to be parsing output from UnDecorateSymbolName(), which will not
			 generate bogus data. instead, it may generate syntax we're not
			 prepared for, in which case either the function signature should
			 be changed (because it's too complex for FuBi anyway) or this
			 grammar should be updated to allow it. prefer the first method -
			 i don't see any point in supporting pass-by-value of nested
			 classes, templates, or arrays... */

declaration
	:	access_opt modifier_opt return_type call_method full_name full_param_list func_const_opt
		{
#			if ( GP_DEBUG )
			{
				static const char FUBI_PREFIX[]   = "FUBI_";
				static const UINT FUBI_PREFIX_LEN = ELEMENT_COUNT( FUBI_PREFIX ) - 1;

				// check for underscores in function name and issue warning
				// (only if not FUBI managed function)
				if (   !gFuBiSysExports.TestOptions( SysExports::OPTION_NO_STRICT )
					&& (SPEC.m_Name.find( '_' ) != gpstring::npos)
					&& !SPEC.m_Name.same_with_case( FUBI_PREFIX, FUBI_PREFIX_LEN ) )
				{
					if ( SPEC.m_Parent != NULL )
					{
						Message( MESSAGE_WARNING, "consistency warning - method name '%s::%s' should not have underscores",
								 SPEC.m_Parent->m_Name.c_str(), SPEC.m_Name.c_str() );
					}
					else
					{
						Message( MESSAGE_WARNING, "consistency warning - function name '%s' should not have underscores",
								 SPEC.m_Name.c_str() );
					}
				}
			}
#			endif // GP_DEBUG
		}
	|	T_ILLEGAL
		{
			DEBUG_MESSAGE(( MESSAGE_WARNING, "unable to undecorate symbol '%s'", SCANNER.GetRaw() ));
			YYIGNORE;		// bad dbghelp?
		}
	|	/* NULL */
		{
			DEBUG_MESSAGE(( MESSAGE_WARNING, "nothing to parse!" ));
			YYIGNORE;
		}
	;

access_opt
	:	T_PRIVATE   ':'			/* leave all these alone */
	|	T_PROTECTED ':'
	|	T_PUBLIC    ':'
	|	/* NULL */
	;

modifier_opt
	:	T_STATIC
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_STATIC_MEMBER;
		}
	|	T_VIRTUAL
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "method is virtual - vtbl calls not supported" ));
			YYABORT;
		}
	|	/* NULL */
	;

return_type
	:	type
		{
			SPEC.m_ReturnType = $1.m_Type;
		}
	|	/* NULL */
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "ctor/dtor/operator exports not supported" ));
			YYABORT;
		}
	;

call_method
	:	T_CDECL
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_CALL_CDECL;
		}
	|	T_FASTCALL
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_CALL_FASTCALL;

			// any point in supporting this?
			DEBUG_MESSAGE(( MESSAGE_FATAL, "__fastcall call type not currently supported" ));
			YYABORT;
		}
	|	T_STDCALL
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_CALL_STDCALL;
		}
	|	T_THISCALL
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_CALL_THISCALL;
		}
	|	/* NULL */
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "unrecognized calling convention" ));  // something new?
			YYABORT;
		}
	;

full_name
	:	qualified_id T_SCOPE T_IDENTIFIER
		{
			// only if not a trait
			if ( !(SPEC.m_Flags & FunctionSpec::FLAG_TRAIT) )
			{
				// probably a member function
				SPEC.m_Flags |= FunctionSpec::FLAG_MEMBER;

				// take class
				SPEC.m_Parent = gFuBiSysExports.GetClass( $$.m_String );
				if ( !SPEC.m_Parent->m_Name.same_with_case( $$.m_String ) )
				{
					DEBUG_MESSAGE(( MESSAGE_FATAL, "class name collision detected between '%s' and '%s' (exports are case insensitive)",
									SPEC.m_Parent->m_Name.c_str(), $$.m_String.c_str() ));
					YYABORT;
				}

				// check for underscores, issue warning
				if ( !gFuBiSysExports.TestOptions( SysExports::OPTION_NO_STRICT ) )
				{
					if ( $$.m_String.find( '_' ) != gpstring::npos )
					{
						// see if it is destined to be replaced (but dont replace
						// it yet - let the postprocessors do that)
						const char* replace = NameReplaceSpec::GetReplacement( $$.m_String );
						if ( (replace == NULL) || (::strchr( replace, '_' ) != NULL) )
						{
							DEBUG_MESSAGE(( MESSAGE_WARNING, "consistency warning - class name '%s' should not have underscores",
											$$.m_String.c_str() ));
						}
					}
				}
			}

			// shift name over to take function name
			SPEC.m_Name = $3.m_String;
		}
	|	T_IDENTIFIER
		{
			SPEC.m_Name = $1.m_String;
		}
	;

full_param_list
	:	'(' param_list ')'
	|	/* NULL */
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "not a function export - is it data? data not supported" ));
			YYABORT;
		}
	;

func_const_opt
	:	T_CONST
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_CONST;
		}
	|	/* NULL */
	;

type
	:	general_type type_const_opt pointer_opt
		{
			$$ = $1;
			$$.m_Type.m_Flags |= $2.m_Type.m_Flags;
			$$.m_Type.m_Flags |= $3.m_Type.m_Flags;
		}
	|	error
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "unable to parse type - update fubi signature grammar" ));
			YYABORT;
		}
	;

general_type
	:	builtin_type
	|	custom_type
	|	user_type
	;

type_const_opt
	:	T_CONST
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_CONST;
		}
	|	/* NULL */
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_NONE;
		}
	;

pointer_opt
	:	'*'
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_POINTER;
		}
	|	'*' '*'
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_POINTER_POINTER;
		}
	|	'&'
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_REFERENCE;
		}
	|	/* NULL */
		{
			$$.m_Type.m_Flags = TypeSpec::FLAG_NONE;
		}
	;

param_list
	:	param_list ',' type
		{
			if ( $3.m_Type != VAR_UNKNOWN )
			{
				SPEC.m_ParamSpecs.push_back( ParamSpec( $3.m_Type ) );
			}
		}
	|	type
		{
			if ( ($1.m_Type != VAR_VOID) && ($1.m_Type != VAR_UNKNOWN) )
			{
				SPEC.m_ParamSpecs.push_back( ParamSpec( $1.m_Type ) );
			}
		}
	;

user_type
	:	user_type_class qualified_id
		{
			switch ( $1.m_Token )
			{
				case ( T_CLASS ):
				case ( T_STRUCT ):
				{
					if ( $2.m_String.same_with_case( "SkritHObject" ) )
					{
						$$.m_Type = gFuBiSysExports.FindSkritObject()->m_Type;
						$$.m_Type.m_Flags = TypeSpec::FLAG_HANDLE;
					}
					else
					{
						$$.m_Type = gFuBiSysExports.GetClass( $2.m_String )->m_Type;
					}
				}	break;
				case ( T_ENUM ):
				{
					$$.m_Type = gFuBiSysExports.GetEnum( $2.m_String )->m_Type;
				}	break;
				default:
				{
					gpassert( 0 );		// should never get here
				}
			}
		}
	;

user_type_class
	:	T_CLASS
	|	T_STRUCT
	|	T_UNION
		{
			DEBUG_MESSAGE(( MESSAGE_FATAL, "union types not supported" ));
			YYABORT;
		}
	|	T_ENUM
	;

qualified_id
	:	qualified_id T_SCOPE T_IDENTIFIER
		{
			// handle nested/namespaced types by pasting them together
			$$.m_String += $3.m_String;

			// see if it is meant for replacement
			const char* replace = NameReplaceSpec::GetReplacement( $$.m_String );
			if ( replace != NULL )
			{
				// use the replacement version
				$$.m_String = replace;
			}
		}
	|	qualified_id T_SCOPE T_IDENTIFIER '<' general_type '>'
		{
			// special support for traits
			if ( $1.m_String.same_with_case( "FuBi" ) && $3.m_String.same_with_case( "Traits" ) )
			{
				if ( $5.m_Type.m_Flags == TypeSpec::FLAG_NONE )
				{
					SPEC.m_Flags |= FunctionSpec::FLAG_TRAIT;
					PARSER.m_TraitType = $5.m_Type;
				}
				else
				{
					DEBUG_MESSAGE(( MESSAGE_FATAL, "Can only export undecorated traits (no pointers, references, etc.)" ));
				}
			}
			else
			{
				DEBUG_MESSAGE(( MESSAGE_FATAL, "Template grammar only supported for FuBi::Traits" ));
			}
		}
	|	T_IDENTIFIER
	;

custom_type
	:	T_STRUCT T_MEM_PTR
		{
			$$.m_Type = VAR_MEM_PTR;
			$$.m_Type.m_Flags = TypeSpec::FLAG_NONE;
		}
	|	T_STRUCT T_CONST_MEM_PTR
		{
			$$.m_Type = VAR_CONST_MEM_PTR;
			$$.m_Type.m_Flags = TypeSpec::FLAG_NONE;
		}
	|	handle_type
		{
			$$.m_Type = gFuBiSysExports.GetClass( $$.m_String )->m_Type;
			$$.m_Type.m_Flags = TypeSpec::FLAG_HANDLE;
		}
	;

handle_type
	:	T_CLASS T_RESHANDLE '<' user_type_class qualified_id ',' handle_fields_type '>'
		{
			$$ = $5;
		}
	;

handle_fields_type
	:	user_type_class qualified_id
	|	T_STRUCT T_RESHANDLEFIELDS '<' T_INTCONSTANT '>'
		/* ... other general field types ... */
	;

builtin_type
	:	T_FLOAT
		{
			$$.m_Type = VAR_FLOAT;
		}
	|	T_VOID
		{
			$$.m_Type = VAR_VOID;
		}
	|	T_BOOL
		{
			$$.m_Type = VAR_BOOL;
		}
	|	T_VARARG
		{
			SPEC.m_Flags |= FunctionSpec::FLAG_VARARG;
			$$.m_Type = VAR_UNKNOWN;
		}
	|	T_CLASS T_GPBSTRING '<' T_CHAR ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' '>'
		{
			/* class gpbstring <char, struct std::char_traits <char>, class std::allocator <char> > */
			$$.m_Type = VAR_STRING;
		}
	|	T_CLASS T_STD T_SCOPE T_BASICSTRING '<' T_CHAR ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' '>'
		{
			/* class std::basic_string <char, struct std::char_traits <char>, class std::allocator <char> > */
			$$.m_Type = VAR_STRING;
		}
	|	T_CLASS T_STD T_SCOPE T_GPBASICSTRING '<' T_CHAR ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_CHAR '>' '>'
		{
			/* class std::gp_basic_string <char, struct std::char_traits <char>, class std::allocator <char> > */
			$$.m_Type = VAR_STRING;
		}
	|	T_CLASS T_GPBSTRING '<' T_UNSIGNED T_SHORT ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' '>'
		{
			/* class gpbstring <wchar_t, struct std::char_traits <wchar_t>, class std::allocator <wchar_t> > */
			$$.m_Type = VAR_WSTRING;
		}
	|	T_CLASS T_STD T_SCOPE T_BASICSTRING '<' T_UNSIGNED T_SHORT ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' '>'
		{
			/* class std::basic_string <wchar_t, struct std::char_traits <wchar_t>, class std::allocator <wchar_t> > */
			$$.m_Type = VAR_WSTRING;
		}
	|	T_CLASS T_STD T_SCOPE T_GPBASICSTRING '<' T_UNSIGNED T_SHORT ',' T_STRUCT T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' ',' T_CLASS T_STD T_SCOPE T_IDENTIFIER '<' T_UNSIGNED T_SHORT '>' '>'
		{
			/* class std::gp_basic_string <wchar_t, struct std::char_traits <wchar_t>, class std::allocator <wchar_t> > */
			$$.m_Type = VAR_WSTRING;
		}
	|	builtin_type_int
		{
			$$ = $1;

			if ( $$.m_TypeFlags & ScanStruct::FLAG_SPECIAL )
			{
				switch ( $$.m_Type.m_Type )
				{
					case ( VAR_INT8 ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
						{
							$$.m_Type = VAR_SINT8;
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
						{
							$$.m_Type = VAR_UINT8;
						}
					}	break;

					case ( VAR_INT16 ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
						{
							$$.m_Type = VAR_SINT16;
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
						{
							$$.m_Type = VAR_UINT16;
						}
					}	break;

					case ( VAR_INT32 ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
						{
							$$.m_Type = VAR_SINT32;
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
						{
							$$.m_Type = VAR_UINT32;
						}
					}	break;

					case ( VAR_INT64 ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
						{
							$$.m_Type = VAR_SINT64;
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
						{
							$$.m_Type = VAR_UINT64;
						}
					}	break;
				}
			}
			else
			{
				switch ( $$.m_Type.m_Type )
				{
					case ( VAR_CHAR ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
						{
							$$.m_Type = VAR_SCHAR;
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
						{
							$$.m_Type = VAR_UCHAR;
						}
					}	break;

					case ( VAR_DOUBLE ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_LONG )
						{
							$$.m_Type = VAR_LONGDOUBLE;
						}
					}	break;

					case ( VAR_UNKNOWN ):
					{
						if ( $$.m_TypeFlags & ScanStruct::FLAG_SHORT )
						{
							if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
							{
								$$.m_Type = VAR_SSHORT;
							}
							else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
							{
								$$.m_Type = VAR_USHORT;
							}
							else
							{
								$$.m_Type = VAR_SHORT;
							}
						}
						else if ( $$.m_TypeFlags & ScanStruct::FLAG_LONG )
						{
							if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
							{
								$$.m_Type = VAR_SLONG;
							}
							else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
							{
								$$.m_Type = VAR_ULONG;
							}
							else
							{
								$$.m_Type = VAR_LONG;
							}
						}
						else
						{
							if ( $$.m_TypeFlags & ScanStruct::FLAG_SIGNED )
							{
								$$.m_Type = VAR_SINT;
							}
							else if ( $$.m_TypeFlags & ScanStruct::FLAG_UNSIGNED )
							{
								$$.m_Type = VAR_UINT;
							}
							else
							{
								$$.m_Type = VAR_INT;
							}
						}
					}	break;
				}
			}
		}
	;

builtin_type_int
	:	builtin_type_int builtin_type_int_modifier
		{
			$$.m_TypeFlags = $1.m_TypeFlags | $2.m_TypeFlags;
			if ( $1.m_Type == VAR_UNKNOWN )
			{
				$$.m_Type = $2.m_Type;
			}
			else
			{
				$$.m_Type = $1.m_Type;
			}
		}
	|	builtin_type_int_modifier
	;

builtin_type_int_modifier
	:	T_CHAR
		{
			$$.m_Type   = VAR_CHAR;
			$$.m_TypeFlags = 0;
		}
	|	T_SIGNED
		{
			$$.m_Type   = VAR_UNKNOWN;
			$$.m_TypeFlags = ScanStruct::FLAG_SIGNED;
		}
	|	T_UNSIGNED
		{
			$$.m_Type   = VAR_UNKNOWN;
			$$.m_TypeFlags = ScanStruct::FLAG_UNSIGNED;
		}
	|	T_SHORT
		{
			$$.m_Type   = VAR_UNKNOWN;
			$$.m_TypeFlags = ScanStruct::FLAG_SHORT;
		}
	|	T_INT
		{
			$$.m_Type   = VAR_UNKNOWN;
			$$.m_TypeFlags = ScanStruct::FLAG_INT;
		}
	|	T_LONG
		{
			$$.m_Type   = VAR_UNKNOWN;
			$$.m_TypeFlags = ScanStruct::FLAG_LONG;
		}
	|	T_DOUBLE
		{
			$$.m_Type   = VAR_DOUBLE;
			$$.m_TypeFlags = 0;
		}
	|	T_INT8
		{
			$$.m_Type   = VAR_INT8;
			$$.m_TypeFlags = ScanStruct::FLAG_SPECIAL;
		}
	|	T_INT16
		{
			$$.m_Type   = VAR_INT16;
			$$.m_TypeFlags = ScanStruct::FLAG_SPECIAL;
		}
	|	T_INT32
		{
			$$.m_Type   = VAR_INT32;
			$$.m_TypeFlags = ScanStruct::FLAG_SPECIAL;
		}
	|	T_INT64
		{
			$$.m_Type   = VAR_INT64;
			$$.m_TypeFlags = ScanStruct::FLAG_SPECIAL;
		}
	;


	  /*/////////////////////////////////////////////////////////////////////////
	 // ** END OF PRODUCTIONS **
	*/


%%


//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// these are useful when debugging for the watch window

#if GP_DEBUG
FuBi::YYSTYPE& fx$( void  )  {  return ( FuBi::x$() );  }
FuBi::YYSTYPE& fx$( int i )  {  return ( FuBi::x$( i ) );  }
const char* ftoken( int token )  {  return ( FuBi::token( token ) );  }
const char* ftoken( FuBi::YYSTYPE& x )  {  return ( FuBi::token( x.m_Token ) );  }
const char* token( FuBi::YYSTYPE& x )  {  return ( FuBi::token( x.m_Token ) );  }
#endif

//////////////////////////////////////////////////////////////////////////////
