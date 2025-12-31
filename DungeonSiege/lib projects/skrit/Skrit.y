/* ///////////////////////////////////////////////////////////////////////////
//
// File     :  Skrit.y
// Author(s):  Scott Bilas
//
// Summary  :  Contains the production rules for the Skrit compiler.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////// */

%{

//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "Skrit.h"
#include "SkritStructure.h"
#include "SkritByteCode.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////

namespace Skrit  {  // begin of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// documentation

/*	Future:

	be able to support inline latent triggers. for example:

		Goid goid$ = godb.sclonego();
		wait frame;
		goid$.Animate( ... );
		wait( timeout = 100msec) trigger OnGoHandleMessage( WE_ANIM_DONE ) and if ( AIQuery.EnemiesNear() );
		wait time( 100msec);

		... etc

	the point is to be able to insert triggers inline in the code. sort of
	micro state transitions. upon hitting a wait block, exit and leave the
	IP and stack where it is. then set the blocking event - have a set of
	builtins like frame, time, etc. on top of that support inline static and
	dynamic triggers that are waited on until we can proceed in the code.
	be able to support a timeout or something as well.
*/

//////////////////////////////////////////////////////////////////////////////
// local helpers

// Disable warnings for generated constants.

#	pragma warning ( disable : 4309 )	// 'initializing' : truncation of constant value
#	pragma warning ( disable : 4305 )	// 'initializing' : truncation from 'const int' to 'short'

// Access derived type.

	inline Compiler* GetCompiler( BaseParser* obj )
		{  return ( (Compiler*)obj );  }
	inline const Compiler* GetCompiler( const BaseParser* obj )
		{  return ( (const Compiler*)obj );  }
#	define COMPILER (*GetCompiler( this ))
#	define FUNCTION (COMPILER.m_CurrentFunction)
#	define PACK     (*COMPILER.m_Pack)
#	define OVERRIDE (COMPILER.m_OverrideLocation)

// Errors.

#	define  YYCHECK_USERID( FROM ) \
			if ( FROM.m_Token == T_SYSID ) \
 			{ \
				OVERRIDE = FROM.m_Location; \
				Message( MESSAGE_ERROR, "'%s': found sysid, expecting userid - did you forget the '$'?", \
						 FROM.ResolveString() ); \
				YYERROR; \
			}

#	define  YYCHECK_SYSID( FROM ) \
			if ( FROM.m_Token == T_USERID ) \
			{ \
				OVERRIDE = FROM.m_Location; \
				Message( MESSAGE_ERROR, "'%s': found userid, expecting sysid - did you accidentally add a '$'?", \
						 FROM.ResolveString() ); \
				YYERROR; \
			}


//////////////////////////////////////////////////////////////////////////////

%}


	  /*/////////////////////////////////////////////////////////////////////////
	 // ** TOKENS **
	*/

/* KEYWORD TOKENS */

	/* program flow */
%token T_IF T_ELSE
%token T_WHILE T_BREAK T_CONTINUE T_FOREVER
%token T_RETURN

	/* types */
%token T_BOOL T_INT T_FLOAT T_STRING T_VOID
%token T_SHARED T_PROPERTY T_HIDDEN

	/* general */
%token T_SITNSPIN T_ABORT T_MSEC T_FRAMES T_DOC

	/* game interface */
%token T_EVENT T_AT T_TRIGGER

	/* macros */
%token T_MACRO_LINE T_MACRO_NAME

	/* operators */
%token T_LE T_GE T_EQ T_AEQ T_NE
%token T_BOR T_BXOR T_BAND
%token T_OR T_AND T_ARROW
%token T_ABOR T_ABXOR T_ABAND
%token T_AADD T_ASUB T_AMUL T_ADIV T_AMOD
%token T_POW T_APOW

	/* state machine */
%token T_STATE T_POLL T_TRANSITION T_ANDT
%token T_STARTUP T_SETSTATE

/* MISC TOKENS */

	/* preprocessor */
%token T_PREPROCESSOR T_INCLUDE T_OPTION T_ONLY T_ONLY_BEGIN T_ONLY_END

	/* constants */
%token T_BOOLCONSTANT T_INTCONSTANT T_FLOATCONSTANT T_STRINGCONSTANT
%token T_TRUE T_FALSE T_NULL
%token T_ENUMCONSTANT

	/* identifiers */
%token T_SYSID T_USERID T_THIS T_VM T_OWNER

	/* used internally */
%token T_FUNCCALL
%token T_DEREFERENCE
%token T_VARIABLE
%token T_EVAL T_TEMPEVAL
%token T_DUMMY


	  /*/////////////////////////////////////////////////////////////////////////
	 // ** YACC DIRECTIVES **
	*/

%start skrit

	/* these are precedence rules to resolve conflicts */
	/* for more info see MKS Lex & Yacc Page 333 */

%right T_ELSE

%left ','
%left '=' T_ABOR T_ABXOR T_ABAND T_AADD T_ASUB T_AMUL T_ADIV T_AMOD T_APOW
%right '?' ':'
%left T_OR
%left T_AND
%left T_BOR
%left T_BXOR
%left T_BAND
%left T_EQ T_NE T_AEQ
%left '<' T_LE '>' T_GE
%left '+' '-'
%left '*' '/' '%'
%right P_CAST
%left T_POW
%right '!' '~' P_UPLUS P_UMINUS


%%


	  /*/////////////////////////////////////////////////////////////////////////
	 // ** SIEGESKRIT GRAMMAR **
	*/

	/*	important note on this grammar: it's cheapo. i'm not interested in
		making a c++ compiler here. there's a lot of special-case code that
		only works a certain way. if you're interested in a general-purpose
		compiler where a function is a "type" then check out lcc or something.
		on the other hand, a lot of this special case code is driving me crazy.
		look for a redesign in skrit v2. this is about 10x better than gk3's
		sheep, but still has a long way to go.

		note: generally try to use left-recursion (it's more efficient).
			  see pg 197 in the bird book or MKS yacc support page for why.

			  exprlist:  exprlist ',' expr ;  -- left recursion
			  exprlist:  expr ',' exprlist ;  -- right recursion

		note: $$ = $1 is ALWAYS applied for EVERY action before it gets called.
	*/

	  /* ////////////////////////////////////////////////////////////////////////
	 // ** SKRIT **
	*/

	/*	No special tag meanings here */

skrit
	:	skrit_global_list
		{
			COMPILER.FinishCompile();
		}
	|	/* NULL */
	;

skrit_global_list
	:	skrit_global
	|	skrit_global_list skrit_global
	;

skrit_global
	:	T_SHARED  {  COMPILER.m_GlobalShared = true;  }  global_variable_decl  {  COMPILER.m_GlobalShared   = false;  COMPILER.m_GlobalHidden = false;  }
	|	property_prefix  {  COMPILER.m_GlobalProperty = true;  }  global_variable_decl  {  COMPILER.m_GlobalProperty = false;  COMPILER.m_GlobalHidden = false;  }
	|	global_variable_decl
	|	state_decl
		{
			// all done with the state
			COMPILER.m_GlobalState.ResetLocal();
		}
	|	transition_decl
	|	function_decl
		{
			// may be all done with the state
			if ( FUNCTION.m_LocalState )
			{
				COMPILER.m_GlobalState.ResetLocal();
			}

			// all done with the function
			FUNCTION.Reset();

			// back to global
			COMPILER.LeaveToGlobalScope( $$, Compiler::FREESCOPE_NOEMIT );
		}
	|	option
	|	error
	;

property_prefix
	:	T_PROPERTY  {  COMPILER.m_GlobalHidden = false;  }
	|	T_HIDDEN T_PROPERTY  {  COMPILER.m_GlobalHidden = true;  }
	|	T_PROPERTY T_HIDDEN  {  COMPILER.m_GlobalHidden = true;  }
	;

	  /* ////////////////////////////////////////////////////////////////////////
	 // ** OPTIONS **
	*/

option
	:	T_OWNER '=' sys_type ';'
		{
			if ( COMPILER.m_OwnerUsedLine == 0 )
			{
				COMPILER.m_OwnerUsedLine = $1.m_Location.m_Line;
				COMPILER.m_OwnerType = $3.m_Type.m_Type;
			}
			else
			{
				Message( MESSAGE_ERROR, "'owner': cannot reset owner type - first used/set at line '%d'",
						 COMPILER.m_OwnerUsedLine );
			}
		}
	;

	  /* ////////////////////////////////////////////////////////////////////////
	 // ** STATE MACHINE **
	*/

state_decl
	:	state_begin state_block
		{
			if ( COMPILER.m_GlobalState.m_NestedLevel != 0 )
			{
				--COMPILER.m_GlobalState.m_NestedLevel;
			}
		}
	;

state_begin
	:	state_startup_opt T_STATE id state_poll_opt
		{
			YYCHECK_USERID( $3 );

			const char* stateName = $3.ResolveString();
			Compiler::GlobalState& globalState = COMPILER.m_GlobalState;

			// do not allow nested states
			if ( globalState.m_Current == STATE_INDEX_NONE )
			{
				Compiler::Symbol* stateSym = COMPILER.AddOrFindState( stateName, $3.m_Location, true );
				if ( stateSym != NULL )
				{
					// update state info
					globalState.m_Current = stateSym->m_Index;
					Compiler::State& state = COMPILER.m_States[ globalState.m_Current ];

					// take care of startup state
					Raw::State* stateHeader = COMPILER.GetStateHeader();
					if ( $1.m_Token == T_STARTUP )
					{
						// warn about duplicate startup chosen by user
						if ( globalState.m_ChoseStartup )
						{
							Message( MESSAGE_WARNING, "'%s': startup state is overriding state '%s' already marked for startup",
									 stateName, COMPILER.GetStateSymbol( stateHeader->m_InitialState )->m_Name.c_str() );
						}

						// new state
						stateHeader->m_InitialState = scast <WORD> ( globalState.m_Current );
						globalState.m_SetStartup   = true;
						globalState.m_ChoseStartup = true;
					}

					// take care of poll state
					if ( $4.m_Token == T_POLL )
					{
						// warn about existing poll setting
						if ( state.m_Config.m_PollPeriod >= 0.0f )
						{
							char oldPeriod[ 30 ], newPeriod[ 30 ];
							if ( state.m_Config.m_PollPeriod == 0.0f )
							{
								::strcpy( oldPeriod, "<always>" );
							}
							else
							{
								::sprintf( oldPeriod, "%f seconds", state.m_Config.m_PollPeriod );
							}
							if ( $4.m_Float == 0.0f )
							{
								::strcpy( oldPeriod, "<always>" );
							}
							else
							{
								::sprintf( oldPeriod, "%f seconds", $4.m_Float );
							}
							Message( MESSAGE_WARNING, "'%s': poll period of %s overriding old setting of %s",
									 stateName, oldPeriod, newPeriod );
						}

						// set it
						state.m_Config.m_PollPeriod = $4.m_Float;
						state.m_NeedsSaving = true;
					}
				}
			}
			else
			{
				++globalState.m_NestedLevel;
				Message( MESSAGE_ERROR, "'%s::%s': nested states not supported",
						 COMPILER.GetCurrentStateSymbol()->m_Name.c_str(), stateName );
			}
		}
	;

state_startup_opt
	:	T_STARTUP
	|	/* NULL */
		{
			$$.m_Token = T_DUMMY;
		}
	;

state_block
	:	'{' skrit_global_list '}'
	|	'{' '}'
	|	';'
	;

state_poll_opt
	:	T_POLL '(' T_INTCONSTANT ')'
		{
			$$.m_Float = scast <float> ( $3.m_Int );
		}
	|	T_POLL '(' T_FLOATCONSTANT ')'
		{
			$$.m_Float = $3.m_Float;
		}
	|	T_POLL '(' T_INTCONSTANT T_MSEC ')'
		{
			$$.m_Float = scast <float> ( $3.m_Int ) / 1000.0f;
		}
	|	T_POLL
		{
			$$.m_Float = 0.0f;
		}
	|	/* NULL */
		{
			$$.m_Token = T_DUMMY;
		}
	;

transition_decl
	:	T_TRANSITION transition_block
	|	T_TRANSITION transition_item
	;

transition_block
	:	'{' transition_list '}'
	|	'{' '}'
	;

transition_list
	:	transition_item
	|	transition_list transition_item
	|	transition_list error
		{
			// clear error transition state
			COMPILER.m_Transition.Reset();
		}
	;

transition_item
	:	transition_item_impl
		{
			// all done with the transition, submit it
			if ( COMPILER.m_Transition.IsValid() )
			{
				COMPILER.EmitTransition();
			}
			COMPILER.m_Transition.Reset();
		}
	;

transition_item_impl
	:	id T_ARROW id ':' transition_condition_list transition_response
		{
			YYCHECK_USERID( $1 );
			YYCHECK_USERID( $3 );

			// look up states
			const char* fromName = $1.ResolveString();
			const char* toName   = $3.ResolveString();
			int fromStateNum = COMPILER.AddOrFindState( fromName, $1.m_Location )->m_SymbolIndex;		// keep index only as adding new symbol may resize vector
			Compiler::Symbol* toState   = COMPILER.AddOrFindState( toName,   $3.m_Location );
			Compiler::Symbol* fromState = &COMPILER.m_Symbols[ fromStateNum ];
			if ( (fromState != NULL) && (toState != NULL) )
			{
				// store info
				COMPILER.m_Transition.m_FromState = fromState->m_Index;
				COMPILER.m_Transition.m_ToState   = toState  ->m_Index;
			}
		}
	|	T_ARROW id ':' transition_condition_list transition_response
		{
			YYCHECK_USERID( $2 );

			// future: consider possibility of allowing no "from" so that it
			//         would become like a default "all" case, and apply the
			//         same transition out to every single state. probably
			//         want to use a new keyword for that though, something
			//         like "any".

			// check for default state
			int fromState = COMPILER.m_GlobalState.m_Current;
			if ( fromState == STATE_INDEX_NONE )
			{
				fromState = 0;
				Message( MESSAGE_ERROR, "'transition': no current state to use for default 'from' state" );
			}

			// look up state
			const char* toName = $2.ResolveString();
			Compiler::Symbol* toState = COMPILER.AddOrFindState( toName, $2.m_Location );
			if ( toState != NULL )
			{
				// store info
				COMPILER.m_Transition.m_FromState = fromState;
				COMPILER.m_Transition.m_ToState   = toState->m_Index;
			}
		}
	|	';'	/* ignore extra semicolons */
	;

transition_condition_list
	:	transition_condition
	|	transition_condition_list T_ANDT transition_condition
	|	transition_condition_list T_ANDT error
	;

transition_condition
	:	id
		{
			$$.m_Next = NULL;
			COMPILER.EmitStaticCondition( $$ );
		}
	|	id '(' ')'
		{
			$$.m_Next = NULL;
			COMPILER.EmitStaticCondition( $$ );
		}
	|	id '(' transition_param_list ')'
		{
			$$.m_Next = new ParseNode( $3 );
			COMPILER.EmitStaticCondition( $$ );
		}
	|	id '(' error ')'
		{
			$$.m_Next = NULL;
			COMPILER.EmitStaticCondition( $$ );
		}
	|	transition_if_begin expr ')'
		{
			// return the expr like a function
			COMPILER.Emit( Op::CODE_RETURN, $3 );

			// make sure it is an int expr
			if ( ($2.m_Type == FuBi::VAR_INT) || ($2.m_Type == FuBi::VAR_BOOL) )
			{
				switch ( $2.m_Token )
				{
					case ( T_BOOLCONSTANT ):
					case ( T_INTCONSTANT ):
					{
						OVERRIDE = $2.m_Location;
						Message( MESSAGE_WARNING, "'if-expr': conditional expression is constant" );
					}	break;
				}

				// note that the result of the if-statement will be left on the
				//  expr stack for the machine to read back.
			}
			else
			{
				OVERRIDE = $2.m_Location;
				Message( MESSAGE_ERROR, "'if-expr': bool/int type expected, found '%s'",
						 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
			}
		}
	;

transition_if_begin
	:	T_IF '('
		{
			Compiler::ConditionDynamic cond;
			cond.m_CodeOffset = PACK.m_ByteCode.size();
			COMPILER.m_Transition.m_Dynamics.push_back( cond );
		}
	;

transition_param_list
	:	transition_param_item
		{
			$$.m_Next = NULL;
		}
	|	transition_param_list ',' transition_param_item
		{
			$$.AddLinkEnd( new ParseNode( $3 ) );
		}
	|	transition_param_list ',' error
		{
			$$.DeleteChain();
		}
	|	transition_param_list error
		{
			$$.DeleteChain();
		}
	;

transition_param_item
	:	global_variable_constant
	;

transition_response
	:	'=' transition_response_begin statement_list_opt '}'
		{
			COMPILER.EndFunction( $4 );
			FUNCTION.Reset();
		}
	|	'}'
		{
			Message( MESSAGE_ERROR, "'trans-response': found '}', did you forget a ';'?" );
			YYERROR;
		}
	|	';'
	;

transition_response_begin
	:	'{'
		{
			// transition code is in its own scope
			COMPILER.EnterScope( $$ );
			COMPILER.m_Transition.m_ResponseOffset = PACK.m_ByteCode.size();
		}
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** VARIABLES **
	*/

	/*	Special:

			m_Type is the variable type.

			m_Int is used for id name offsets and constants.
	*/

global_variable_decl
	:	type global_variable_decl_list ';'
	;

global_variable_decl_list
	:	global_variable_decl_item
	|	global_variable_decl_list ',' global_variable_decl_item
	|	global_variable_decl_list ',' error
	;

global_variable_decl_item
	:	id
		{
			YYCHECK_USERID( $1 );
			COMPILER.AddVariable( $1, NULL, NULL );
		}
	|	id T_DOC '=' string_literal
		{
			YYCHECK_USERID( $1 );
			COMPILER.AddVariable( $1, NULL, &$4 );
			if ( !COMPILER.m_GlobalProperty )
			{
				Message( MESSAGE_WARNING, "'global_variable_decl': found 'doc' for non-property variable" );
			}
		}
	|	id '=' global_variable_constant
		{
			YYCHECK_USERID( $1 );
			COMPILER.AddVariable( $1, &$3, NULL );
		}
	|	id '=' global_variable_constant T_DOC '=' string_literal
		{
			YYCHECK_USERID( $1 );
			COMPILER.AddVariable( $1, &$3, &$6 );
			if ( !COMPILER.m_GlobalProperty )
			{
				Message( MESSAGE_WARNING, "'global_variable_decl': found 'doc' for non-property variable" );
			}
		}
	;

global_variable_constant
	:	global_variable_constant_no_id
	|	id
		{
			if ( !COMPILER.ResolveEnum( $$, true ) )
			{
				YYERROR;
			}
		}
	;

global_variable_constant_no_id
	:	T_BOOLCONSTANT
		{
			$$.m_Type = FuBi::VAR_BOOL;
		}
	|	T_INTCONSTANT
		{
			$$.m_Type = FuBi::VAR_INT;
		}
	|	'-' T_INTCONSTANT
		{
			$$.m_Type = FuBi::VAR_INT;
			$$.m_Int = -$2.m_Int;
		}
	|	T_FLOATCONSTANT
		{
			$$.m_Type = FuBi::VAR_FLOAT;
		}
	|	'-' T_FLOATCONSTANT
		{
			$$.m_Type = FuBi::VAR_FLOAT;
			$$.m_Float = -$2.m_Float;
		}
	|	string_literal
		{
			$$.m_Type.SetCString();
		}
	;

local_variable_decl
	:	type local_variable_decl_list ';'
	;

local_variable_decl_list
	:	local_variable_decl_item
	|	local_variable_decl_list ',' local_variable_decl_item
	|	local_variable_decl_list ',' error
	;

local_variable_decl_item
	:	id
		{
			YYCHECK_USERID( $$ );
			COMPILER.AddVariable( $$, NULL, NULL );
		}
	|	id
		{
			YYCHECK_USERID( $1 );
			COMPILER.AddVariable( $1, NULL, NULL );
		}
  /*+*/ '=' expr
		{
			COMPILER.EmitAssignment( $$, $3, $4 );
		}
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** FUNCTIONS **
	*/

	/*	Special:

			m_Type is the return type of the function, and the types of the
			parameters that it takes.

			m_Int is used for id name offsets.
	*/

function_decl
	:	function_decl_begin statement_list_opt '}'
		{
			if ( FUNCTION.m_NestedLevel == 0 )
			{
				COMPILER.EndFunction( $3 );
			}
			else
			{
				--FUNCTION.m_NestedLevel;
			}

			if ( FUNCTION.m_LocalState && (COMPILER.m_GlobalState.m_NestedLevel != 0) )
			{
				--COMPILER.m_GlobalState.m_NestedLevel;
			}

			// pad a little
			PACK.EmitByte( 0 );
		}
	;

function_decl_begin
	:	function_decl_begin_state
		{
			gpstring funcName( $1.ResolveString() );
			Compiler::Function& function = FUNCTION;

			// check some things
			gpassert( COMPILER.m_Transition.m_Statics.empty() );
			gpassert( COMPILER.m_Transition.m_Dynamics.empty() );

			// do not allow nested functions
			if ( function.m_Export == NULL )
			{
				// event handler?
				int eventSerial = INVALID_FUNCTION_INDEX;
				if ( ($1.m_Token == T_EVENT) || ($1.m_Token == T_TRIGGER) )
				{
					// look up event
					bool success = false;
					if ( COMPILER.m_EventClass != NULL )
					{
						// remove trailing '$'
						gpstring localName( funcName );
						gpassert( *localName.rbegin() == '$' );
						localName.erase( localName.length() - 1 );

						// get its id to bind in the loader
						const FuBi::FunctionSpec* eventSpec = COMPILER.FindFuBiEvent( localName );
						if ( eventSpec != NULL )
						{
							eventSerial = eventSpec->m_SerialID;

							// prefix name with state if not a trigger
							if ( ($1.m_Token != T_TRIGGER) && (COMPILER.m_GlobalState.m_Current != STATE_INDEX_NONE) )
							{
								const gpstring& stateName = COMPILER.GetCurrentStateSymbol()->m_Name;
								funcName = stateName + "." + funcName;
							}

							// done
							success = true;
						}
					}

					// report error
					if ( !success )
					{
						Message( MESSAGE_WARNING, "'%s': function tagged as an event does not exist in FuBi under namespace '%s'",
								 funcName.c_str(), FuBi::EVENT_NAMESPACE );
						if ( $1.m_Token == T_TRIGGER )
						{
							YYERROR;
						}
					}
				}

				// add new function export if not a trigger
				if ( $1.m_Token != T_TRIGGER )
				{
					function.m_Export = COMPILER.AddExport( funcName, $1.m_Location, NULL, 0, eventSerial );
					if ( function.m_Export == NULL )
					{
						YYERROR;
					}

					// remember its return type and starting place
					function.m_Export->m_ReturnType = $1.m_Type.m_Type;
				}
				else
				{
					// remember where the response started
					COMPILER.m_Transition.m_ResponseOffset = PACK.m_ByteCode.size();

					// a 'to' state of nothing means this is a trigger
					COMPILER.m_Transition.m_ToState = STATE_INDEX_NONE;
					COMPILER.m_Transition.m_FromState = COMPILER.m_GlobalState.m_Current;
				}

				// func params must appear in their own scope
				COMPILER.EnterScope( $$ );
			}
			else
			{
				++FUNCTION.m_NestedLevel;
				Message( MESSAGE_ERROR, "'%s::%s': nested functions not supported",
						 function.m_Export->m_Name, funcName.c_str() );
			}
		}
  /*+*/	function_decl_params_and_extra '{'
		{
			// function?
			if ( FUNCTION.m_Export != NULL )
			{
				// verify that the params match up if a bound function
				if (   (FUNCTION.m_NestedLevel == 0)
					&& (FUNCTION.m_Export->m_EventBinding != scast <WORD> ( INVALID_FUNCTION_INDEX ))
					&& !COMPILER.CheckEventBinding() )
				{
					FUNCTION.m_Export->m_EventBinding = scast <WORD> ( INVALID_FUNCTION_INDEX );
				}
			}
			else
			{
				// treat trigger param list as a condition
				$3.m_Int = $1.m_Int;
				$3.m_Token = $1.m_Token;
				COMPILER.EmitStaticCondition( $3, true );

				// all done with the transition, submit it
				COMPILER.EmitTransition();
				COMPILER.m_Transition.Reset();
			}

			// adjust scope
			COMPILER.m_LocalScopes.back()->m_Location = $4.m_Location;
		}
	;

function_decl_begin_state
	:	state_begin function_decl_prefix
		{
			$$ = $2;
			FUNCTION.m_LocalState = true;
		}
	|	function_decl_prefix
	;

function_decl_prefix
	:	event_or_trigger type id
		{
			YYCHECK_USERID( $3 );
			$$ = $3;
			$$.m_Type = $2.m_Type;
			$$.m_Token = $1.m_Token;
		}
	|	type id
		{
			YYCHECK_USERID( $2 );
			$$.m_Int = $2.m_Int;
		}
	|	event_or_trigger T_VOID id
		{
			YYCHECK_USERID( $3 );
			$$ = $3;
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Token = $1.m_Token;
		}
	|	T_VOID id
		{
			YYCHECK_USERID( $2 );
			$$ = $2;
			$$.m_Type = FuBi::VAR_VOID;
		}
	|	event_or_trigger id [ '{' ]
		{
			YYCHECK_USERID( $2 );
			$$ = $2;
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Token = $1.m_Token;
		}
	|	id [ '{' ]
		{
			YYCHECK_USERID( $1 );
			$$.m_Type = FuBi::VAR_VOID;
		}
	;

event_or_trigger
	:	T_EVENT
	|	T_TRIGGER
	;

function_decl_params_and_extra
	:	function_decl_params function_decl_time
		{
			if ( FUNCTION.m_Export == NULL )
			{
				OVERRIDE = $2.m_Location;
				Message( MESSAGE_ERROR, "'at': found timing info on a static trigger" );
			}
			else if ( $$.m_Next != NULL )
			{
				OVERRIDE = $2.m_Location;
				Message( MESSAGE_ERROR, "'at': timed functions may not receive parameters" );
			}
		}
	|	function_decl_params
	|	function_decl_time
		{
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Next = NULL;

			if ( FUNCTION.m_Export == NULL )
			{
				OVERRIDE = $1.m_Location;
				Message( MESSAGE_ERROR, "'at': found timing info on a static trigger" );
			}
		}
	|	/* NULL */
		{
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Next = NULL;
		}
	;

function_decl_params
	:	'(' typed_param_list ')'
		{
			if ( FUNCTION.m_Export == NULL )
			{
				$$.m_Next = new ParseNode( $2 );
			}
		}
	|	'(' ')'
		{
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Next = NULL;
		}
	|	'(' T_VOID ')'
		{
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Next = NULL;
		}
	|	'(' error ')'
		{
			$$.m_Type = FuBi::VAR_VOID;
			$$.m_Next = NULL;
		}
	;

typed_param_list
	:	typed_param_item
		{
			$$.m_Next = NULL;
		}
	|	typed_param_list ',' typed_param_item
		{
			if ( FUNCTION.m_Export == NULL )
			{
				$$.AddLinkEnd( new ParseNode( $3 ) );
			}
		}
	|	typed_param_list ',' error
		{
			if ( FUNCTION.m_Export == NULL )
			{
				$$.DeleteChain();
			}
		}
	|	typed_param_list error
		{
			if ( FUNCTION.m_Export == NULL )
			{
				$$.DeleteChain();
			}
		}
	;

typed_param_item
	:	type id
		{
			if ( FUNCTION.m_Export != NULL )
			{
				if ( $2.m_Token == T_USERID )
				{
					if ( COMPILER.AddVariable( $2, NULL, NULL ) )
					{
						FUNCTION.m_Export = COMPILER.AddExportParam( FUNCTION.m_Export, $$ );
						COMPILER.Emit( Op::CODE_PUSHPRM, $2 );

						Compiler::Symbol* symbol = &COMPILER.m_Symbols.back();
						switch ( symbol->m_SymType )
						{
							case ( Compiler::SYMTYPE_VARIABLE ):  COMPILER.Emit( Op::CODE_STOREVAR, $2 );  break;
							case ( Compiler::SYMTYPE_STRING   ):  COMPILER.Emit( Op::CODE_STORESTR, $2 );  break;
							case ( Compiler::SYMTYPE_HANDLE   ):  COMPILER.Emit( Op::CODE_STOREHDL, $2 );  break;
							default:  gpassert( 0 );		// should never get here
						}

						PACK.EmitWord( symbol->m_Index );
						PACK.EmitByte( symbol->m_Store );
					}
				}
				else
				{
					Message( MESSAGE_ERROR, "'%s': found sysid, expecting userid - did you forget the '$'?",
							 $2.ResolveString() );
				}
			}
			else
			{
				Message( MESSAGE_ERROR, "'param': found type and id, expecting constant - this is a trigger, not an event" );
				YYERROR;
			}
		}
	|	builtin_type
		{
			if ( FUNCTION.m_Export != NULL )
			{
				FUNCTION.m_Export = COMPILER.AddExportParam( FUNCTION.m_Export, $$ );
				COMPILER.Emit( Op::CODE_SKIPPRM, $1 );
			}
			else
			{
				Message( MESSAGE_ERROR, "'param': found type, expecting constant - this is a trigger, not an event" );
				YYERROR;
			}
		}
	|	global_variable_constant_no_id
		{
			if ( FUNCTION.m_Export != NULL )
			{
				Message( MESSAGE_ERROR, "'param': found constant, expecting type and id - this is an event, not a trigger" );
				YYERROR;
			}
		}
	|	id
		{
			// could be an enum or a type (always sysid though)
			YYCHECK_SYSID( $1 );

			if ( COMPILER.ResolveEnum( $$, false ) )
			{
				if ( FUNCTION.m_Export != NULL )
				{
					Message( MESSAGE_ERROR, "'param': found enum constant, expecting type and id - this is an event, not a trigger" );
					YYERROR;
				}
			}
			else if ( COMPILER.ResolveSysType( $$ ) )
			{
				if ( FUNCTION.m_Export != NULL )
				{
					COMPILER.SetGlobalType( $$ );
					FUNCTION.m_Export = COMPILER.AddExportParam( FUNCTION.m_Export, $$ );
					COMPILER.Emit( Op::CODE_SKIPPRM, $1 );
				}
				else
				{
					Message( MESSAGE_ERROR, "'param': found type, expecting constant - this is a trigger, not an event" );
					YYERROR;
				}
			}
			else
			{
				YYERROR;
			}
		}
	;

function_decl_time
	:	function_decl_time_impl
		{
			// save time to runner
			if ( FUNCTION.m_Export->IsTimeInFrames() )
			{
				COMPILER.m_AtTimeFrames = FUNCTION.m_Export->GetTimeInFrames();
			}
			else
			{
				COMPILER.m_AtTimeMsec = FUNCTION.m_Export->GetTimeInMsec();
			}
		}
	;

function_decl_time_impl
	:	T_AT '(' function_decl_time_constant ')'
	|	T_AT '(' T_CONTINUE function_decl_time_constant ')'
		{
			// add time from runner
			if ( FUNCTION.m_Export->IsTimeInFrames() )
			{
				FUNCTION.m_Export->SetTimeInFrames( FUNCTION.m_Export->GetTimeInFrames() + COMPILER.m_AtTimeFrames );
			}
			else
			{
				FUNCTION.m_Export->SetTimeInMsec( FUNCTION.m_Export->GetTimeInMsec() + COMPILER.m_AtTimeMsec );
			}
		}
	;

function_decl_time_constant
	:	T_INTCONSTANT
		{
			FUNCTION.m_Export->SetTimeInMsec( $1.m_Int * 1000 );
		}
	|	T_INTCONSTANT T_MSEC
		{
			FUNCTION.m_Export->SetTimeInMsec( $1.m_Int );
		}
	|	T_INTCONSTANT T_FRAMES
		{
			FUNCTION.m_Export->SetTimeInFrames( $1.m_Int );
		}
	|	T_FLOATCONSTANT
		{
			FUNCTION.m_Export->SetTimeInSeconds( $1.m_Float );
		}
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** STATEMENTS **
	*/

statement_list_opt
	:	statement_list
	|	/* NULL */
	;

statement_list
	:	statement
		{
			COMPILER.TempVarCheckpoint( $1 );
		}
	|	statement_list statement
		{
			COMPILER.TempVarCheckpoint( $1 );
		}
	;

statement
	:	';'
	|	statement_simple
	|	statement_setstate
	|	statement_expr
	|	statement_block
	|	statement_control
	|	statement_assign
	|	function_decl
	|	local_variable_decl
	|	error
	;

statement_simple
	:	T_SITNSPIN   ';'  {  COMPILER.Emit( Op::CODE_SITNSPIN,   $2 );  }
	|	T_ABORT      ';'  {  COMPILER.Emit( Op::CODE_ABORT,      $2 );  }
	;

statement_setstate
	:	statement_setstate_impl
		{
			YYCHECK_USERID( $$ );

			const char* stateName = $$.ResolveString();
			Compiler::Symbol* state = COMPILER.AddOrFindState( stateName, $$.m_Location );
			if ( state != NULL )
			{
				COMPILER.Emit( Op::CODE_SETSTATE, $$ );
				PACK.EmitWord( state->m_Index );
			}
		}
	;

statement_setstate_impl
	:	T_SETSTATE '(' id ')' ';'  {  $$ = $3;  }
	|	T_SETSTATE id ';'          {  $$ = $2;  }
	;

statement_expr
	:	expr ';'
		{
			if ( $1.m_Type != FuBi::VAR_VOID )
			{
				COMPILER.Emit( Op::CODE_POP1, $2 );
			}

			switch ( $$.m_Token )
			{
				case ( T_BOOLCONSTANT ):
				case ( T_INTCONSTANT ):
				case ( T_FLOATCONSTANT ):
				case ( T_STRINGCONSTANT ):
				{
					OVERRIDE = $2.m_Location;
					Message( MESSAGE_WARNING, "'expr': constant/literal has no effect" );
				}	break;
			}
		}
	;

statement_block
	:	'{'
		{
			COMPILER.EnterScope( $1 );
		}
  /*+*/	statement_list_opt '}'
		{
			COMPILER.LeaveScope( $4 );
		}
	;

statement_control
	:	control_if_else
	|	control_while
	|	control_break
	|	control_continue
	|	control_return
	;

statement_assign
	:	expr_postfix statement_assign_op
		{
			if ( $2.m_Token != '=' )
			{
				if ( !COMPILER.ResolvePostFixExpr( $1 ) )
				{
					YYERROR;
				}
			}
		}
  /*+*/ expr ';'
		{
			OVERRIDE = $2.m_Location;
			bool ok = COMPILER.EmitAssignmentOp( $$, $1, $2, $4 );
			OVERRIDE.Reset();
			if ( !ok )
			{
				YYERROR;
			}
		}
	;

statement_assign_op
	:	'='
	|	T_ABOR
	|	T_ABXOR
	|	T_ABAND
	|	T_AADD
	|	T_ASUB
	|	T_AMUL
	|	T_ADIV
	|	T_AMOD
	|	T_APOW
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** FLOW CONTROL **
	*/

	/*	Special:

			m_Type is used for expression types that 'if' takes.

			m_Int is used for opcode offsets based at the start of the entire
			block of bytecode, to be patched later with relative offsets.
	*/

control_if_else
	:	if_part statement_block else_part
		{
			// patch the BRANCHZERO data from the if_part so that it points
			//  to right after the BRANCH + data of the else_part.
			COMPILER.PatchCodeWord( $1.m_Int, $3.m_Int - $1.m_Int );
		}
	|	if_part statement_block %prec T_ELSE
		{
			// current opcode offset is set to the end of statement
			// patch the BRANCHZERO data from the if_part so that it points there
			COMPILER.PatchCodeWord( $1.m_Int, COMPILER.GetCodeOffset() - ($1.m_Int + 2) );
		}
	;

if_part
	:	T_IF expr
		{
			if ( !COMPILER.EmitConditionalBranch( $$, $2 ) )
			{
				YYERROR;
			}
		}
	;

else_part
	:	else_item control_if_else
		{
			// current opcode offset is set to the end of statement
			// patch the BRANCH data from the else_item so that it points there
			COMPILER.PatchCodeWord( $1.m_Int, COMPILER.GetCodeOffset() - ($1.m_Int + 2) );
		}
	|	else_item statement_block
		{
			// current opcode offset is set to the end of statement
			// patch the BRANCH data from the else_item so that it points there
			COMPILER.PatchCodeWord( $1.m_Int, COMPILER.GetCodeOffset() - ($1.m_Int + 2) );
		}
	;

else_item
	:	T_ELSE
		{
			COMPILER.TempVarCheckpoint( $1 );
			COMPILER.Emit( Op::CODE_BRANCH, $1 );
			$$.m_Int = COMPILER.GetCodeOffset();
			PACK.EmitSword( 0 );							// emit temporary branch location (will be resolved at else_part level)
		}
	;

control_while
	:	while_part '{'
		{
			// enter while scope
			COMPILER.EnterScope( $2 );
			COMPILER.GetBackScope()->m_IsLoop = true;

			// add patch point for while conditional (if there is one)
			if ( $1.m_Token != T_FOREVER )
			{
				Compiler::PatchEntry patch;
				patch.m_PatchPoint = $1.m_Int;
				patch.m_Type = Compiler::PATCH_TAIL;
				COMPILER.GetBackScope()->m_PatchPoints.push_back( patch );
			}

			// update $1 int to point to start of while code (used later for back patching)
			if ( $1.m_Next != NULL )
			{
				$1.m_Int = $1.m_Next->m_Int;
				delete ( $1.m_Next );
			}
		}
  /*+*/	statement_list_opt '}'
		{
			// cleanup vars only - we still need the scope for a little longer though
			COMPILER.LeaveScope( $5, Compiler::FREESCOPE_NODELETE );

			// add unconditional branch (to be patched like everything else)
			COMPILER.Emit( Op::CODE_BRANCH, $5 );
			Compiler::PatchEntry patch;
			patch.m_PatchPoint = COMPILER.GetCodeOffset();
			patch.m_Type = Compiler::PATCH_HEAD;
			COMPILER.GetBackScope()->m_PatchPoints.push_back( patch );
			PACK.EmitSword( 0 );

			// ok now that we know our pointers, go up a scope (this will patch too)
			COMPILER.GetBackScope()->m_CodeHead = $1.m_Int;
			COMPILER.GetBackScope()->m_CodeTail = COMPILER.GetCodeOffset();
			COMPILER.LeaveScope( $5, Compiler::FREESCOPE_NOEMIT );
		}
	;

while_part
	:	T_WHILE
		{
			$1.m_Int = COMPILER.GetCodeOffset();
		}
  /*+*/ expr
		{
			int offset = $1.m_Int;
			if ( !COMPILER.EmitConditionalBranch( $$, $3 ) )
			{
				YYERROR;
			}
			$$.m_Next = new ParseNode;
			$$.m_Next->m_Int = offset;
		}
	|	T_FOREVER
		{
			$$.m_Type  = FuBi::VAR_BOOL;
			$$.m_Int   = COMPILER.GetCodeOffset();
			$$.m_Token = T_FOREVER;
			$$.m_Next  = NULL;
		}
	;

control_break
	:	T_BREAK
		{
			if ( !COMPILER.EmitLoopGoto( $1, Compiler::PATCH_TAIL, "break" ) )
			{
				YYERROR;
			}
		}
	;

control_continue
	:	T_CONTINUE
		{
			if ( !COMPILER.EmitLoopGoto( $1, Compiler::PATCH_HEAD, "continue" ) )
			{
				YYERROR;
			}
		}
	;

control_return

	/*	note on return:

		right now the RETURN statement ends up causing problems with extra
		generated code. it should flip a bit when hit that causes all further
		code generation to turn off. give a warning when it tries to generate
		it too so the user knows that this code will not ever be executed.
		anyway this should avoid problems with the if(){return}else{return}
		causing both extra instructions to be generated and the final warning
		about no return instruction. it needs to analyze the code paths like
		the vc++ compiler so it can give better warnings.
	*/

	:	T_RETURN ';'
		{
			COMPILER.LeaveToGlobalScope( $1, Compiler::FREESCOPE_NODELETE );

			// set return type
			$$.m_Type = COMPILER.GetFunctionReturn();

			// check for compatibility
			if ( $$.m_Type != FuBi::VAR_VOID )
			{
				Message( MESSAGE_ERROR, "'return': function must return a value of type '%s'",
						 gFuBiSysExports.FindNiceTypeName( $$.m_Type ) );
			}

			COMPILER.Emit( Op::CODE_RETURN0, $2 );
		}
	|	T_RETURN expr ';'
		{
			COMPILER.LeaveToGlobalScope( $1, Compiler::FREESCOPE_NODELETE );

			// set return type
			$$.m_Type = COMPILER.GetFunctionReturn();

			// check for void
			if ( $$.m_Type == FuBi::VAR_VOID )
			{
				OVERRIDE = $2.m_Location;
				Message( MESSAGE_ERROR, "'return': cannot return a value from a void function" );
			}
			else if ( $2.m_Token == T_NULL )
			{
				// only support null returns on UDT refs
				if ( !FuBi::IsUser( $$.m_Type.m_Type ) || $$.m_Type.IsPassByValue() )
				{
					OVERRIDE = $2.m_Location;
					Message( MESSAGE_ERROR, "'return': cannot return 'null' for function return type '%s'",
							 gFuBiSysExports.MakeFullTypeName( $$.m_Type ).c_str() );
				}
			}
			else
			{
				// check for compatibility
				FuBi::TypeSpec::eCompare compare = FuBi::TestCompatibility( $2.m_Type, $$.m_Type );
				if ( compare != FuBi::TypeSpec::COMPARE_INCOMPATIBLE )
				{
					// convert if necessary
					if ( compare == FuBi::TypeSpec::COMPARE_COMPATIBLE )
					{
						COMPILER.EmitConversion( "expr", $2.m_Type, $$.m_Type, 0, $2 );
					}
				}
				else
				{
					COMPILER.ReportConversionError( "return", "expression", $2.m_Type, $$.m_Type, $2 );
				}

				// cannot return temporaries!
				if ( $2.m_Token == T_TEMPEVAL )
				{
					Message( MESSAGE_ERROR, "'return': function cannot return temporary variable of type '%s'",
							 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
				}
			}

			COMPILER.TempVarCheckpoint( $3 );
			COMPILER.Emit( Op::CODE_RETURN, $3 );
		}
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** EXPRESSIONS **
	*/

		/*  with expressions, the ordering of emit statements is not important.
			the operations must be performed according to precedence rules, and
			since it's all RPN stack ops this works out exactly the way we need.
		*/

expr
	:	expr_impl

/*  $$$ this multi-action causes conflicts, haven't figure out how to work around it yet
	|	expr ','
		{
			//$$$ warn if the left expr is a constant (as in statement_expr)

			if ( $1.m_Type != FuBi::VAR_VOID )
			{
				COMPILER.Emit( Op::CODE_POP1, $2 );
			}
		}
		expr_impl */
	;

expr_impl
	:	expr_unary
	|	expr_binary
	|	expr_ternary
	;

expr_unary
	:	expr_postfix
		{
			if ( !COMPILER.ResolvePostFixExpr( $$ ) )
			{
				YYERROR;
			}
		}
	|	'+' expr %prec P_UPLUS
		{
			$$ = $2;
			$$.SetNonConstantToEval();

			if ( ($$.m_Type != FuBi::VAR_INT) && ($$.m_Type != FuBi::VAR_FLOAT) )
			{
				Message( MESSAGE_ERROR, "'+': illegal on operands of type '%s'",
						 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
			}
		}
	|	'-' expr %prec P_UMINUS
		{
			$$ = $2;
			$$.SetNonConstantToEval();

			if ( $$.m_Type == FuBi::VAR_INT )
			{
				COMPILER.Emit( Op::CODE_NEGATEI, $1 );
			}
			else if ( $$.m_Type == FuBi::VAR_FLOAT )
			{
				COMPILER.Emit( Op::CODE_NEGATEF, $1 );
			}
			else
			{
				Message( MESSAGE_ERROR, "'-': illegal on operands of type '%s'",
						 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
			}
		}
	|	'~' expr
		{
			$$ = $2;
			$$.m_Token = T_EVAL;

			if (   ($$.m_Type == FuBi::VAR_INT)
				|| FuBi::IsEnum( $$.m_Type.m_Type )
				|| $$.m_Type.IsPointerClass()  )
			{
				COMPILER.Emit( Op::CODE_BNOTI, $1 );
			}
			else
			{
				Message( MESSAGE_ERROR, "'~': illegal on operands of type '%s'",
						 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
			}
		}
	|	'!' expr
		{
			$$ = $2;
			$$.m_Token = T_EVAL;

			if (   ($$.m_Type == FuBi::VAR_INT)
				|| ($$.m_Type == FuBi::VAR_BOOL)
				|| FuBi::IsEnum( $$.m_Type.m_Type )
				|| $$.m_Type.IsPointerClass() )
			{
				COMPILER.Emit( Op::CODE_NOTIB, $1 );
				$$.m_Type = FuBi::VAR_BOOL;
			}
			else
			{
				Message( MESSAGE_ERROR, "'!': illegal on operands of type '%s'",
						 gFuBiSysExports.FindNiceTypeName( $2.m_Type ) );
			}
		}
	;

expr_binary
	:	expr '+'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ADDI,                 "+" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '-'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_SUBTRACTI,            "-" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '*'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_MULTIPLYI,            "*" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '/'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_DIVIDEI,              "/" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '%'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_MODULOI,              "%" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_POW  expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_POWERI,               "**", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_EQ   expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISEQUALI,             "==", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_AEQ  expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISAPPROXEQUALI_DUMMY, "~=", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_NE   expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISNOTEQUALI,          "!=", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '>'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISGREATERI,           ">" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr '<'    expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISLESSI,              "<" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_GE   expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISGREATEREQUALI,      ">=", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_LE   expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ISLESSEQUALI,         "<=", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_BOR  expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_BORI,                 "|" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_BXOR expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_BXORI,                "^" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_BAND expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_BANDI,                "&" , $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_OR   expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ORIB,                 "||", $$, $2, $3 );  OVERRIDE.Reset();  }
	|	expr T_AND  expr  {  OVERRIDE = $2.m_Location;  COMPILER.EmitBinaryOp( Op::CODE_ANDIB,                "&&", $$, $2, $3 );  OVERRIDE.Reset();  }
	;

expr_ternary
	:	expr '?'
		{
			if ( !COMPILER.EmitConditionalBranch( $2, $1 ) )
			{
				YYERROR;
			}
		}
  /*+*/ expr ':'
		{
			COMPILER.Emit( Op::CODE_BRANCH, $5 );
			$5.m_Int = COMPILER.GetCodeOffset();
			PACK.EmitSword( 0 );

			// patch the BRANCHZERO data from the first so that it points to
			//  right after the BRANCH + data of the second.
			COMPILER.PatchCodeWord( $2.m_Int, COMPILER.GetCodeOffset() - ($2.m_Int + 2) );
		}
  /*+*/ expr
		{
			// patch the BRANCH data from the end of the second part so that it
			//  points to right after the end of the third expr.
			COMPILER.PatchCodeWord( $5.m_Int, COMPILER.GetCodeOffset() - ($5.m_Int + 2) );

			// make sure types are the same (or close enough)
			if ( FuBi::TestCompatibility( $4.m_Type, $7.m_Type ) <= FuBi::TypeSpec::COMPARE_PROMOTABLE )
			{
				// take type of first part
				$$ = $4;
				$$.SetNonConstantToEval();
			}
			else
			{
				// error out
				Message( MESSAGE_ERROR, "'?:': two expressions of types '%s' and '%s' not similar enough",
						 gFuBiSysExports.FindNiceTypeName( $4.m_Type ),
						 gFuBiSysExports.FindNiceTypeName( $7.m_Type ) );
			}
		}
	;

expr_postfix
	:	expr_primary
		{
			$$.m_Next = NULL;
		}
	|	expr_postfix '.' id
		{
			YYCHECK_SYSID( $3 );

			// see if $1 is a class if not figured out already
			bool success = false, voidCall = false;
			if ( $1.m_Type == FuBi::VAR_UNKNOWN )
			{
				if ( $1.m_Token == T_SYSID )
				{
					FuBi::eVarType type = gFuBiSysExports.FindType( $1.ResolveString() );
					if ( FuBi::IsUser( type ) )
					{
						success = true;
						$$ = $3;
						$$.m_Next = NULL;
						$$.m_Type = type;
						$$.m_Token = T_DEREFERENCE;
					}
					else if ( type == FuBi::VAR_UNKNOWN )
					{
						// ok so it wasn't a type so it's not a singleton or
						// static method call. try it as a 0-param global
						// function.
						voidCall = true;
					}
				}
				else if ( $1.m_Token == T_USERID )
				{
					Compiler::Symbol* symbol = COMPILER.FindSymbol( $$.ResolveString() );
					if (   (symbol != NULL)
						&& FuBi::IsUser( symbol->m_VarType.m_Type )
						&& Compiler::IsVariable( symbol->m_SymType ) )
					{
						success = true;
						$$ = $3;
						$$.m_Next = NULL;
						$$.m_Type = symbol->m_VarType;
						$$.m_Token = T_DEREFERENCE;

						// retrieve the handle
						switch ( symbol->m_SymType )
						{
							case ( Compiler::SYMTYPE_VARIABLE ) :
							{
								COMPILER.Emit( Op::CODE_LOADVAR, $$ );
								break;
							}
							case ( Compiler::SYMTYPE_STRING ) :
							{
								COMPILER.Emit( Op::CODE_LOADSTR, $$ );
								break;
							}
							case ( Compiler::SYMTYPE_HANDLE ) :
							{
								COMPILER.Emit( Op::CODE_LOADHDL, $$ );
								break;
							}
							default:
							{
								gpassert( 0 );		// should never get here
							}
						}
						PACK.EmitWord( symbol->m_Index );
						PACK.EmitByte( symbol->m_Scope == &COMPILER.m_GlobalScope );		// if global
					}
				}
			}
			// previous expr said this was a class, see if a void call
			else if ( FuBi::IsUser( $1.m_Type.m_Type ) )
			{
				// could be "this" or a ptr-returning function
				if ( ($1.m_Token == T_THIS) || ($1.m_Token == T_VM) || ($1.m_Token == T_OWNER) || ($1.m_Token == T_FUNCCALL) )
				{
					success = true;
					$$ = $3;
					$$.m_Next = NULL;
					$$.m_Type = $1.m_Type;
					$$.m_Token = T_DEREFERENCE;
				}
				else
				{
					gpassert( $1.m_Token == T_DEREFERENCE );
					voidCall = true;
				}
			}

			// calc.func.func style thing
			if ( voidCall )
			{
				if ( !COMPILER.EmitCall( $$ ) )
				{
					YYERROR;
				}

				if ( !$$.m_Type.IsPassByValue() )
				{
					success = true;
					$$.m_Int = $3.m_Int;
					$$.m_Next = NULL;
					$$.m_Token = T_DEREFERENCE;
				}
			}

			// report failure
			if ( !success )
			{
				OVERRIDE = $2.m_Location;
				Message( MESSAGE_ERROR, "'.': left expression is illegal on non-object types ('%s')",
						 gFuBiSysExports.FindNiceTypeName( $1.m_Type ) );
				$$ = $3;
				$$.m_Next = NULL;
			}
		}
	|	expr_postfix '(' ')'
		{
			$$.m_Next = NULL;
			if ( !COMPILER.EmitCall( $$, false, false ) )
			{
				YYERROR;
			}
		}
	|	expr_postfix '(' expr_param_list ')'
		{
			$$.m_Next = new ParseNode( $3 );
			if ( !COMPILER.EmitCall( $$, false, false ) )
			{
				YYERROR;
			}
		}
	|	expr_postfix '(' error ')'
		{
			$$.m_Next = NULL;
			$$.m_Type = FuBi::VAR_UNKNOWN;
		}
	;

expr_param_list
	:	expr_impl
		{
			$$.m_Next = NULL;
		}
	|	expr_param_list ',' expr_impl
		{
			$$.AddLinkEnd( new ParseNode( $3 ) );
		}
	|	expr_param_list ',' error
		{
			$$.DeleteChain();
		}
	|	expr_param_list error
		{
			$$.DeleteChain();
		}
	;

expr_primary
	:	expr_literal
	|	id
		{
			if ( COMPILER.ResolveEnum( $$, false ) )
			{
				COMPILER.EmitPush( $$.m_Raw, $$ );
			}
		}
	|	'(' expr ')'
		{
			$$ = $2;
			$$.SetNonConstantToEval();
		}
	|	builtin_type_impl '(' expr ')'
		{
			COMPILER.EmitCast( $3, $1.m_Type.m_Type );
			$$ = $3;
			$$.SetNonConstantToEval();
		}
	;

	/*
	$$$ this has conflicts...

	|	'(' builtin_type_impl ')' expr %prec P_CAST
		{
			COMPILER.EmitCast( $4, $2.m_Type.m_Type );
			$$ = $4;
			$$.SetNonConstantToEval();
		}
	*/

expr_literal
	:	T_BOOLCONSTANT
		{
			$$.m_Type = FuBi::VAR_BOOL;
			COMPILER.EmitPush( $$.m_Int, $1 );
		}
	|	T_INTCONSTANT
		{
			$$.m_Type = FuBi::VAR_INT;
			COMPILER.EmitPush( $$.m_Int, $1 );
		}
	|	T_FLOATCONSTANT
		{
			$$.m_Type = FuBi::VAR_FLOAT;
			COMPILER.Emit( Op::CODE_PUSH4, $1 );
			PACK.EmitDword( *rcast <DWORD*> ( &($$.m_Float) ) );
		}
	|	T_NULL
		{
			$$.m_Type.m_Type = FuBi::VAR_VOID;
			$$.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
			$$.m_Int = 0;
			COMPILER.EmitPush( 0, $1 );
		}
	|	T_THIS
		{
			$$.m_Type.m_Type = COMPILER.m_SkritObjectType;
			$$.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
			COMPILER.Emit( Op::CODE_PUSHTHIS, $1 );
		}
	|	T_VM
		{
			$$.m_Type.m_Type = COMPILER.m_SkritVmType;
			$$.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
			COMPILER.Emit( Op::CODE_PUSHVM, $1 );
		}
	|	T_OWNER
		{
			if ( COMPILER.m_OwnerUsedLine == 0 )
			{
				COMPILER.m_OwnerUsedLine = $1.m_Location.m_Line;
			}

			if ( COMPILER.m_OwnerType != FuBi::VAR_UNKNOWN )
			{
				$$.m_Type.m_Type = COMPILER.m_OwnerType;
				$$.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
				COMPILER.Emit( Op::CODE_PUSHOWNER, $1 );
			}
			else
			{
				$$.m_Type.m_Type = COMPILER.m_SkritObjectType;
				$$.m_Type.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
				COMPILER.Emit( Op::CODE_PUSHTHIS, $1 );
			}
		}
	|	string_literal
		{
			$$.m_Type.SetCString();
			COMPILER.EmitPush( $$.m_Int, $1 );
			COMPILER.Emit( Op::CODE_OFFTOPCC, $1 );
			PACK.EmitByte( 0 );
		}
/*	$$$ to do: figure out how to negate the constant here without needing to
		emit a NEGATE op. putting it in the grammar adds conflicts... anyway
		the immediate negation is completely stupid, waste of time since we
		know that the math can be done offline. look in LCC book to see how
		they handle grammar stuff for compile-time arithmetic... ALSO if you
		have an INT literal it should automatically get promoted to a FLOAT if
		required for an expression or function call. this requires that the
		PUSH call be deferred until the point of usage...

		this can be done by just doing the PUSH at the expr_primary level, and
		then supporting a minimal math grammar just for constants which will do
		negate, positive, add/mul/div etc. and just modify m_Int etc. - reuse
		this for the variable decls too... */
	;


	  /* ////////////////////////////////////////////////////////////////////////
	 // ** UTILITY **
	*/

id
	:	T_USERID
	|	T_SYSID
	;

type
	:	builtin_type
	|	sys_type
	;

builtin_type
	:	builtin_type_impl
		{
			COMPILER.SetGlobalType( $1 );
		}
	;

builtin_type_impl
	:	T_BOOL
		{
			$$.m_Type = FuBi::VAR_BOOL;
		}
	|	T_INT
		{
			$$.m_Type = FuBi::VAR_INT;
		}
	|	T_FLOAT
		{
			$$.m_Type = FuBi::VAR_FLOAT;
		}
	|	T_STRING
		{
			$$.m_Type.SetString();
		}
	;

sys_type
	:	id
		{
			YYCHECK_SYSID( $$ );

			if ( !COMPILER.ResolveSysType( $$ ) )
			{
				YYERROR;
			}

			COMPILER.SetGlobalType( $$ );
		}
	;

string_literal
	:	T_STRINGCONSTANT
	|	string_literal T_STRINGCONSTANT
		{
			// join strings together - easy, just erase the null separator!
			PACK.m_Strings.erase( PACK.m_Strings.begin() + $2.m_Int - 1 );
		}
	;


	  /* /////////////////////////////////////////////////////////////////////////
	 // ** END OF PRODUCTIONS **
	*/


%%


//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
// these are useful when debugging for the watch window

#if GP_DEBUG
Skrit::YYSTYPE& sx$( void  )  {  return ( Skrit::x$() );  }
Skrit::YYSTYPE& sx$( int i )  {  return ( Skrit::x$( i ) );  }
const char* stoken( int token )  {  return ( Skrit::token( token ) );  }
const char* stoken( Skrit::YYSTYPE& x )  {  return ( Skrit::token( x.m_Token ) );  }
const char* token( Skrit::YYSTYPE& x )  {  return ( Skrit::token( x.m_Token ) );  }
#endif

//////////////////////////////////////////////////////////////////////////////
