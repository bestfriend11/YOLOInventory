//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiDefs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains definitions and high level API functions to support
//             general function binding. See FuBi.* for implementation.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBIDEFS_H
#define __FUBIDEFS_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"

//////////////////////////////////////////////////////////////////////////////
// Usage documentation

/*	+++																	+++
	+++								EXPORTING							+++
	+++																	+++

	*** Exporting Functions to FuBi ***

		To export a function for Skrit or RPC's, just tag it with FUBI_EXPORT,
		like this:

			FUBI_EXPORT void MyFunction( const char* text );

		Note that class methods can also be exported.

	*** Exporting Variables to FuBi ***

		Skrit is able to expose pairs of Set/Get functions as simple variables
		to scripts (this concept copied from JavaBeans). In order for this to
		work right, you need to provide two functions, like this:

			FUBI_EXPORT int  GetSomeVariable( void ) const;
			FUBI_EXPORT void SetSomeVariable( int );

		These will be exposed as a single variable called "SomeVariable" and is
		a member of whatever class these are members of. An easier way to do
		this directly is to use the FUBI_VARIABLE() macros, like this:

			FUBI_VARIABLE( int, m_, SomeVariable , "This is just a variable" );

		This will export Set and Get functions in addition to declaring the
		variable itself at the current access level of the class.

	*** Exporting Classes to FuBi ***

		FuBi supports passing pointers to just about anything - internally it's
		just a DWORD. However, in order for the system to be safe for scripting,
		it must be able to add references to those pointers so that a manager
		knows that there are still outstanding pointers. This will prevent major
		crashes - scripting dereferencing objects that have already been deleted
		etc. This is a serious problem.

		To make a class safe for scripting, implement a manager for the class
		that is based on the ResHandleMgr stuff. See docs in ResHandle.h for
		more info. Then, use the FUBI_MANAGED_CLASS() macro to publish the fact
		that this class is accessible using the standard ResHandle stuff. The
		scripting system, upon receiving a pointer to a managed class, will call
		AddRef() on it while it keeps it, and Release() when it's done.

	*** Exporting Singletons to FuBi ***

		Normally class pointers cannot be passed over the network through an RPC
		call. However, for a guaranteed singleton class, they can. If a class
		has been derived from Singleton, use the FUBI_SINGLETON_CLASS() macro to
		publish it to FuBi.

	*** Exporting Classes to FuBi for RPC support ***

		Class pointers cannot ordinarily be passed over the network for RPC
		calls, either as the "this" pointer in a method call or as a parameter
		in another function. However, if you provide functions to convert those
		pointers to and from an ID or handle or other DWORD "network cookie",
		the FuBi RPC functions will automatically do the conversions back and
		forth and that class will be available for RPC, either as a method call
		or as a parameter. This assumes that the networked system is
		synchronized, where the cookie is a GUID that refers to the same object
		on all machines. Exporting such a class to the RPC system is as simple
		as tagging it as FUBI_RPC_CLASS() in the class definition. Use the
		RPC_THIS_CALL() macros on an RPC class.

		Using the FUBI_RPC_CLASS() macro requires naming and implementation of
		two functions - the "from" and "to". Example:

			static DWORD BodyToNet( Object* o )
				{  return ( o->m_SerialID );  }
			static Object* NetToBody( DWORD d )
				{  return ( g_ObjectMgr.FindObject( d ) );  }

	*** Renaming exported FuBi functions ***

		It's sometimes necessary to export a function to FuBi that has the same
		name as another existing function to avoid a collision. This can be done
		using the FUBI_RENAME macro, like this:

			virtual void SomeFunction( void ) const;
			FUBI_EXPORT void FUBI_RENAME( SomeFunction )( void ) const
				{  SomeFunction();  }

		Generally this will be used for wrapper functions, especially in the
		case of virtual functions, which can't be FuBi-exported directly, but
		instead must be called indirectly through a separate wrapper.

	*** Exporting Enumerations to FuBi ***

		Enums are fully supported automatically through FuBi as types. You can
		declare enums and assign them to each other, pass them to functions,
		etc. However, to support enumerated constants, you must export functions
		that convert the enums to and (possibly) from strings. There are two
		main types of enums: continuous and irregular.

		Continuous enums start at a number (usually 0) and continue assigning
		constants in serial order with no interruptions in the sequence. These
		are the easiest and fastest to support. Use the FUBI_EXPORT_ENUM() macro
		from inside a CPP file to export the enum constants. You must provide a
		"ToString" method that takes your enum and returns a string. The best
		way to implement this is just have a matching const char* array in the
		CPP file that is indexed using the enum as an int. The macro needs the
		first element of the enum and the number of elements in the enum to
		work. Use the SET_BEGIN_ENUM() and SET_END_ENUM() macros to make this
		easiest.

		Irregular enums are typically bitfields but really may be any enum that
		still contains unique values but does not go from start to end without
		breaks. To support this, use the FUBI_EXPORT_IRREGULAR_ENUM() macro.
		For this, you will need to implement "ToString" and "FromString". The
		ToString method must take your enum and return a string. Implement this
		however you like but make sure it's efficient. The best way is probably
		a big switch statement where each case returns a const char*. FromString
		takes a const char* and a reference to your enum, and returns a bool,
		where it's false if the conversion failed (string does not exist) and
		true if it succeeded. On success, the enum will be set to whatever the
		string mapped to. Because FuBi is case-insensitive, be sure to make your
		string comparisons using stricmp() or some other case-insensitive
		function. The best way to implement FromString() on an irregular enum
		is to have an index (like a std::map) that maps strings onto enums,
		or perhaps use a flat array and bsearch over the set.

	*** Exporting keywords to FuBi ***

		Exported keywords are checked against for exported names to make sure
		that function names etc. are not the same as the keywords. Use the
		FUBI_EXPORT_KEYWORDS() macro with a query proc and size to add the
		keywords to the set upon import time.

	*** Exporting a POD (plain old data) type to FuBi ***

		Use the FUBI_EXPORT_POD_TYPE() and FUBI_POD_CLASS() macros to tell FuBi
		that your class/struct is to be considered "plain old data", meaning
		that its constructors and destructor don't do anything (if it has any).
		POD types are treated as simple memory buffers and gain all kinds of
		useful advantages for free, such as automatic Xfer(), pass-by-value or
		-reference for RPC's, and automatic Skrit support. Export the type's
		schema using the macros in FuBiSchema.h and you'll get the bonus
		package.

		Usage: use FUBI_EXPORT_POD_TYPE() outside of class definitions to apply
		to POD types (for example, you could use it on RECT). Use
		FUBI_POD_CLASS() from within a class, struct or trait definition.

	*** Exporting a "pointer class" type to FuBi ***

		Use the FUBI_EXPORT_POINTER_CLASS() macro to tell FuBi that your type is
		only ever used as a pointer - it's a "pointer class". Usually this is
		meant for handles, using the classic trick of declaring a tag struct and
		then typedefing the handle to be a pointer to that unique struct to give
		it type safety. Much better than doing typedef DWORD MyHandleType.

	+++																	+++
	+++							DOCUMENTATION							+++
	+++																	+++

	*** FuBi Documentation ***

		All functions, classes, and variables can be documented with FuBi. The
		purpose of documenting these is to help out scripting. A doc'd function
		can be queried for what it does, and the parameters it takes. Some FuBi
		macros take a "DOCS" parameter, which is a general doc on the entire
		function or variable, and a "PARAMS" parameter, which is a comma- or
		space- delimited list of names for each of the variables that the
		function takes. NOTE: pay attention to the parameter documentation and
		where the commas actually are! Example usage:

			FUBI_EXPORT float Foo( float f, int i );
			FUBI_MEMBER_DOC ( Foo,      "f,"   "i", "Do something with 'f' then 'i'." );

		The FUBI_DOC and FUBI_MEMBER_DOC macros suffer a minor limitation wrt
		overloaded function names, and must be postfixed with some unique id to
		differentiate them (generally an int is ok), like this:

			FUBI_EXPORT float Foo( float f );
			FUBI_MEMBER_DOC(  Foo$0,    "f", "First overloaded function." );
			FUBI_EXPORT float Foo( float f, int i );
			FUBI_MEMBER_DOC(  Foo$1,    "f,"   "i", "First overloaded function." );

	*** FuBi Parameter Documentation ***

		As part of the parameter documentation (the "f,i" in the example above)
		modifiers and verifiers can be added to help specify context to be used
		by clients such as the editor and the console. These can be things such
		as range limiters (they're inclusive btw):

			FUBI_EXPORT float Foo( float f, int i );
			FUBI_MEMBER_DOC(  Foo, "f[-1.0;1.0],i[0;10]", "Do something." );

		In this example, the 'f' parameter would be allowed to go from -1.0 to
		1.0, and the 'i' parameter can be from 0 to 10. For strings, the context
		is specified through a function for choosing or verifying the data, and
		is just given by name:

			FUBI_EXPORT float Foo( const char* name );
			FUBI_MEMBER_DOC(  Foo, "name[SOUND]", "Play a sound." );

		In this example, 'name' is handled by the 'SOUND' constraint, which will
		be able to verify that the user has chosen a sound and/or provides them
		with a list of sound files.

		$$$ doc how to build a constraint $$$

		Note that whitespace doesn't matter at all within the parameter doc
		string.

	*** FuBi Attributes ***

		Each function has a bitfield dedicated for attributes that can be set
		by an engineer at export time. There are two types of attributes for a
		FuBi function: permissions and membership.

		Permissions:

			Permissions concerns what the dispatcher and compiler will
			permit to be executed.

			SERVER		This command must have been sent from the server. RPC's
						dispatched for this command will be ignored (and
						probably generate an error) if they do not come from the
						server.

			DEV			This command is for development builds only. The script
						compiler and RPC dispatcher will ignore calls to dev
						functions in retail builds.

			RETRY		This command is RPC-dispatched and will retry according
						to the net transport's rules.

		Membership:

			Memberships are all about lists. If a function is a member of
			a given list it appears in whatever interface exposes it,
			otherwise it's just not there. Note that this doesn't prevent
			client code from calling it directly (at least, not currently),
			it's just for exposing things to the end user. The memberships
			tagged as [DEFAULT] here are active by default but can explicitly
			disabled.

			ALL			This command is available from everywhere.

			GAME		This command is available for the game to use [DEFAULT].

			CONSOLE		This command is available from the console's nice helper
						tools [DEFAULT].

			EDITOR		This command is available from the editor's nice GUI.

			TRIAL		This is a trial function - do not rely on it yet!

		Setting a function's attributes is done through the FUBI_SET and
		FUBI_MEMBER_SET macros, like this:

			FUBI_EXPORT float Foo( float f, int i );
			FUBI_MEMBER_SET( Foo, +SERVER -GAME +DEV );

		Follow the name of the function with +TAG and -TAG, where 'TAG' is
		either a permission or membership. Use the '+' to turn something on and
		'-' to turn something off. All permissions are turned off by default, so
		only '+' is available, but the [DEFAULT] memberships can be explicitly
		turned off with '-'.

		Note that FUBI_SET and FUBI_MEMBER_SET have the same overloaded name
		problem as the FUBI_DOC and FUBI_MEMBER_DOC macros. See the "FuBi
		documentation" for more info.

	*** RPC Support ***

		To add RPC support to a function, call FUBI_RPC_CALL from inside the
		function implementation. Pass in the name of the function and the
		address of the target for the RPC call. The address may be
		RPC_TO_SERVER, RPC_TO_ALL, RPC_TO_OTHERS, or RPC_TO_LOCAL, or the DWORD
		ID of the targetted player. Use RPC_TO_LOCAL to execute the function
		directly and not call it as an RPC. Example:

			void MyFunction( const char* text )
			{
				FUBI_RPC_CALL( MyFunction, GetRemoteId() );

				... play with text ...
			}

		In this example, if GetRemoteId() is not RPC_TO_LOCAL, FuBi will pack up
		the parameters and send them through the network transport to the remote
		machine with that ID, which will then call its own MyFunction() directly
		(bypassing the GetRemoteId() check completely). Then it will immediately
		return from the function, skipping the implementation. If GetRemoteId()
		returns RPC_TO_LOCAL, then it will not do the RPC work but instead
		execute the function.

		Important: in order for RPC's to begin functioning, you must make sure
		that the systems are synchronized - they must be the same FuBi version
		and have the exact same digest. Call the Synchronize() function for each
		client machine, upon receiving the sync spec from the server, and call
		the SetSynchronize() function on the server to mark it as the
		synchronization target. Use the OPTION_NO_REQUIRE_SYNC option to disable
		this check for testing purposes.

		Some general notes on RPCs:

			Don't worry about packing parameters closer together - leave them
			DWORD aligned, even if they're char's etc. Most functions don't take
			shorts etc. and it's not worth the extra effort to "compress" the
			parameters this way by choosing shorts over int's - just use int's.
			Plus the net pipe will probably do compression anyway.

			Let the RPC packer do its work, don't try to construct memory
			buffers for it. It will do a nice job of packing parameters with a
			minimum of memory copying for you.

			The "simple" function types can be passed directly without
			additional processing. Simple types are things like ints, shorts,
			chars, floats, etc. Complex types that require reprocessing are
			handles, cookies, strings, etc. Simple functions requires only a
			single memcpy of the parameters on the stack, which are conveniently
			in left-to-right order for increasing memory addresses. This is
			faster than a complex function call which requires additional
			reprocessing and packing of the 'special' parameters.

			To pass memory blocks around (void* + size) use the mem_ptr or
			const_mem_ptr classes, which are simple wrappers that FuBi RPC
			understands.

			The network transport and pipe will do ALL validity checking. Don't
			do any of that on the dispatcher end - assume anything incoming is
			perfect.

	+++																	+++
	+++								RULES								+++
	+++																	+++

	*** General Restrictions ***

		FuBi is fairly general purpose. These are specifically supported:

			Functions with external linkage - this includes nonstatic global
			functions and static or nonstatic class member functions. They may
			be overloaded.

			Any number of parameters per function, including variable argument
			(...) lists. Exported functions can also return parameters.

			All built-in types (char, int, float, etc.) are supported for
			parameters or return values.

			Strings (const char*, gpstring, std::gp_string) are specifically
			supported through some special case code. Note that gpstring and
			std::gp_string must be passed by reference.

			Pointers and references to user-defined types (classes, structs etc)
			are ok as parameters and return values, though they are potentially
			unsafe (see "Exporting Classes" above).

			Handles based on ResHandle can be passed around by value just fine.
			In fact, that's the only safe way to pass around references to
			objects...

		There are also some important restrictions:

			__fastcall is not supported as a calling convention.

			Don't prefix any names with FUBI_ or allow any names to contain '$'
			in the middle of the name - these are reserved as special tags that
			FuBi uses for documentation and internal functions attached to
			exports.

			SiegeSkrit is case-insensitive and FuBi has to accommodate this. Do
			not export two functions with the same name but different case (for
			example, Test() and TEST()).

			For consistency reasons, do not use underscores in any exported
			names - some_function_call() is bad, SomeFunctionCall() is good.

			If a SetXXX and GetXXX pair exists but the parameters do not match
			as described above in "Exporting Variables" it will print an error.

			"Complicated" exports won't even parse properly and you'll get an
			error from FuBi about how it isn't able to understand some function
			you're trying to export. This will typically happen with templates
			or functions that take function pointers as parameters. Not
			supported. Specific templates (such as callbacks) will be supported
			as they become necessary - let me know.

			Unions are not supported. Never will be.

			Passing complex types by value is not supported. If you export a
			function that takes a GUID by value, you'll get an error. Pass it
			by reference and it'll be okay. Specific support can be added to
			allow passing these by value - support for RECT, SIZE, and POINT is
			already in there as an example. Let me know of more cases as you
			need them (very easy to add).

			Virtual functions are supported but are very unsafe. A virtual
			function exported to FuBi will get called directly. The vtbl is not
			consulted to figure out which version of the function to call. To
			prevent Bad Things happening from this I issue a warning for
			exported virtual functions. Work around this by exporting a non-
			virtual function that just redirects the call to a virtual function.
			Then the compiler will do the proper vtbl work. Use FUBI_RENAME to
			give it the same name as the virtual function from Skrit's point of
			view.

			Sometimes the DbgHelp.dll symbol undecoration code just bugs out.
			This happens more often on Win9x for some reason. Let me know of any
			weird cases.

			Exports of constructors, destructors and overloaded operators are
			not supported.

			Exporting functions from nested classes and namespaces is not
			supported, mainly because SiegeSkrit doesn't need this complication.

			Data exports (exporting a variable directly rather than using
			FUBI_VARIABLE) will always get warnings. Don't export data.

		$$$ Planned but not currently implemented $$$:

			Support for default parameters. For example, func( int i = 0 ).
			SiegeSkrit will support better than C++ default params by allowing
			you to leave out intermediate params rather than just the last ones.

	*** RPC-Specific Restrictions ***

		In order for a function to support RPC's there are a few more important
		restrictions:

			Pointers are generally not supported. There is no way to sync up
			memory across the network automatically. However, FuBi singleton
			classes (FUBI_SINGLETON_CLASS), const char*, and mem_ptr and
			const_mem_ptr are special-cased. Also, if a pointer can be converted
			to a network-friendly DWORD handle and back again on the other end
			(perhaps via a GUID or other database lookup on synchronized
			systems) then pointer passing will be automatically supported. Use
			the FUBI_RPC_CLASS() macro to expose this capability to FuBi.

			Non-retryable functions cannot return anything other than 'void'.

			Retryable functions cannot return anything other than 'FuBiCookie'.

			Vararg not supported.
*/

//////////////////////////////////////////////////////////////////////////////
// macro definitions

// Function macros.

	// general purpose fubi exporter
#	define FUBI_EXPORT DECL_DLL_EXPORT
#	define FEX FUBI_EXPORT

	// note that the weird helper function here is required to avoid exporting
	// all kinds of extra stuff if those constants were to be embedded in the
	// exported function. strange but it works...

#	define FUBI_DOC( NAME, PARAMS, DOCS ) \
		inline const FuBi::FunctionDocs& NAME##$QueryDocsHelper( void ) \
			{  static const FuBi::FunctionDocs sDocs( PARAMS, DOCS );  return ( sDocs );  } \
		FUBI_EXPORT inline const FuBi::FunctionDocs& __cdecl NAME##$QueryDocs( void ) \
			{  return ( NAME##$QueryDocsHelper() );  }

#	define FUBI_SET( NAME, FLAGS ) \
		inline void NAME##$QueryFlagsHelper( FuBi::FunctionFlagBuilder& builder ) \
			{  using namespace FuBi::FunctionSpecFlags;  builder | FLAGS;  } \
		FUBI_EXPORT inline void __cdecl NAME##$QueryFlags( FuBi::FunctionFlagBuilder& builder ) \
			{  NAME##$QueryFlagsHelper( builder );  }

#	define FUBI_MEMBER_DOC( NAME, PARAMS, DOCS ) \
		static const FuBi::FunctionDocs& NAME##$QueryDocsHelper( void ) \
			{  static const FuBi::FunctionDocs sDocs( PARAMS, DOCS );  return ( sDocs );  } \
		FUBI_EXPORT static const FuBi::FunctionDocs& __cdecl NAME##$QueryDocs( void ) \
			{  return ( NAME##$QueryDocsHelper() );  }

#	define FUBI_MEMBER_SET( NAME, FLAGS ) \
		static void NAME##$QueryFlagsHelper( FuBi::FunctionFlagBuilder& builder ) \
			{  using namespace FuBi::FunctionSpecFlags;  builder FLAGS;  } \
		FUBI_EXPORT static void __cdecl NAME##$QueryFlags( FuBi::FunctionFlagBuilder& builder ) \
			{  NAME##$QueryFlagsHelper( builder );  }

#	define FUBI_RENAME( NAME ) FUBI_RENAME_##NAME

// Variable macros.

	// this will generate: GetFoo(), SetFoo(), and add a m_Foo, where "Foo" is NAME,
	//  the type is T, and the prefix on the variable is m_.

#	define FUBI_VARIABLE( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT T Get##NAME( void ) const  {  return ( PREFIX##NAME );  } \
		FUBI_EXPORT void Set##NAME( T v  )  {  PREFIX##NAME = v;  } \
		FUBI_MEMBER_DOC( Get##NAME, "", DOCS ); \
		T PREFIX##NAME

#	define FUBI_VARIABLE_BYREF( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT const T& Get##NAME( void ) const  {  return ( PREFIX##NAME );  } \
		FUBI_EXPORT void Set##NAME( const T& v  )  {  PREFIX##NAME = v;  } \
		FUBI_MEMBER_DOC( Get##NAME, "", DOCS ); \
		T PREFIX##NAME

#	define FUBI_VARIABLE_READONLY( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT T Get##NAME( void ) const  {  return ( PREFIX##NAME );  } \
		FUBI_MEMBER_DOC( Get##NAME, "", DOCS ); \
		T PREFIX##NAME

#	define FUBI_VARIABLE_BYREF_READONLY( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT const T& Get##NAME( void ) const  {  return ( PREFIX##NAME );  } \
		FUBI_MEMBER_DOC( Get##NAME, "", DOCS ); \
		T PREFIX##NAME

#	define FUBI_VARIABLE_WRITEONLY( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT void Set##NAME( T v  )  {  PREFIX##NAME = v;  } \
		FUBI_MEMBER_DOC( Set##NAME, "", DOCS ); \
		T PREFIX##NAME

#	define FUBI_VARIABLE_BYREF_WRITEONLY( T, PREFIX, NAME, DOCS ) \
		FUBI_EXPORT void Set##NAME( const T& v  )  {  PREFIX##NAME = v;  } \
		FUBI_MEMBER_DOC( Set##NAME, "", DOCS ); \
		T PREFIX##NAME

// Class macros.

	// class doco

#	define FUBI_CLASS_DOC( T, DOCS ) \
		FUBI_MEMBER_DOC( CLASSDOCO, "", DOCS )

	// inheritance

#	define FUBI_CLASS_INHERIT_IMPL( T, BASE, NAME ) \
		FUBI_EXPORT static int __cdecl NAME( BASE* ) \
			{  return ( (int)(BASE*)(T*)1 - (int)(T*)1 );  }

#	define FUBI_CLASS_INHERIT( T, BASE ) \
		FUBI_CLASS_INHERIT_IMPL( T, BASE, FUBI_Inheritance )

#	define FUBI_CLASS_INHERIT2( T, BASE1, BASE2 ) \
		FUBI_CLASS_INHERIT_IMPL( T, BASE1, FUBI_Inheritance$0 ) \
		FUBI_CLASS_INHERIT_IMPL( T, BASE2, FUBI_Inheritance$1 )

#	define FUBI_CLASS_INHERIT3( T, BASE1, BASE2, BASE3 ) \
		FUBI_CLASS_INHERIT2( T, BASE1, BASE2 ) \
		FUBI_CLASS_INHERIT_IMPL( T, BASE3, FUBI_Inheritance$2 )

#	define FUBI_CLASS_INHERIT4( T, BASE1, BASE2, BASE3, BASE4 ) \
		FUBI_CLASS_INHERIT3( T, BASE1, BASE2, BASE3 ) \
		FUBI_CLASS_INHERIT_IMPL( T, BASE4, FUBI_Inheritance$3 )

	// managed classes

#	define FUBI_MANAGED_CLASS_IMPL( T ) \
		typedef ResHandle <T> FUBI_Handle; \
		typedef ResHandleMgr <FUBI_Handle> FUBI_HandleMgr; \
		FUBI_EXPORT static bool __cdecl FUBI_DoesHandleMgrExist( void ) \
			{  return ( FUBI_HandleMgr::DoesSingletonExist() );  } \
		FUBI_EXPORT static UINT __cdecl FUBI_HandleAddRef( FUBI_Handle handle ) \
			{  return ( FUBI_HandleMgr::GetSingleton().AddRef( handle ) );  } \
		FUBI_EXPORT static UINT __cdecl FUBI_HandleRelease( FUBI_Handle handle ) \
			{  return ( FUBI_HandleMgr::GetSingleton().Release( handle ) );  } \
		FUBI_EXPORT static bool __cdecl FUBI_HandleIsValid( FUBI_Handle handle ) \
			{  return ( FUBI_HandleMgr::GetSingleton().IsValid( handle ) );  } \
		FUBI_EXPORT static T* __cdecl FUBI_HandleGet( FUBI_Handle handle ) \
			{  return ( FUBI_HandleMgr::GetSingleton().Get( handle ) );  } \

#	define FUBI_MANAGED_CLASS( T, DOCS ) \
		FUBI_MANAGED_CLASS_IMPL( T ) \
		FUBI_CLASS_DOC( T, DOCS )

#	define FUBI_MANAGED_NESTED_CLASS( T, NAME, DOCS ) \
		FUBI_MANAGED_CLASS_IMPL( T ) \
		FUBI_CLASS_DOC( NAME, DOCS )

	// singleton classes

#	define FUBI_SINGLETON_CLASS_IMPL( T ) \
		FUBI_EXPORT static T* __cdecl FUBI_GetClassSingleton( void ) \
			{  return ( &GetSingleton() );  }

#	define FUBI_SINGLETON_CLASS( T, DOCS ) \
		FUBI_SINGLETON_CLASS_IMPL( T ) \
		FUBI_CLASS_DOC( T, DOCS )

#	define FUBI_SINGLETON_NESTED_CLASS( T, NAME, DOCS ) \
		FUBI_SINGLETON_CLASS_IMPL( T ) \
		FUBI_CLASS_DOC( NAME, DOCS )

	// rpc classes

#	define FUBI_RPC_CLASS_IMPL( T, TO_FUNC, FROM_FUNC ) \
		FUBI_EXPORT static DWORD __cdecl FUBI_InstanceToNet( T* instance ) \
			{  return ( TO_FUNC( instance ) );  } \
		FUBI_EXPORT static T* __cdecl FUBI_NetToInstance( DWORD netCookie, FuBiCookie* fubiCookie ) \
			{  return ( FROM_FUNC( netCookie, fubiCookie ) );  }

#	define FUBI_RPC_CLASS( T, TO_FUNC, FROM_FUNC, DOCS ) \
		FUBI_RPC_CLASS_IMPL( T, TO_FUNC, FROM_FUNC ) \
		FUBI_CLASS_DOC( T, DOCS )

#	define FUBI_RPC_NESTED_CLASS( T, NAME, TO_FUNC, FROM_FUNC, DOCS ) \
		FUBI_RPC_CLASS_IMPL( T, TO_FUNC, FROM_FUNC ) \
		FUBI_CLASS_DOC( NAME, DOCS )

	// POD classes

#	define FUBI_POD_CLASS( T ) \
		FUBI_EXPORT static size_t __cdecl FUBI_PodGetSize( void ) \
			{  return ( sizeof( T ) );  }

// RPC macros.

	/* important: it is necessary to pass in the name of the function for
				  rpc lookup because the linker will collapse multiple functions
				  with identical code down to the same entry point. this is
				  common with empty or other stub type functions. the name
				  is necessary to differentiate the proper function.
	*/

	// helper for param skipping - 4 bytes each
#	define FUBI_PARAM_SKIP( COUNT ) \
		paramStart = rcast <BYTE*> ( paramStart ) + ((COUNT) * 4);
#	define FUBI_NO_PARAM_SKIP \
		;

	// how far to search for RPC tag - this is actually 0x27 or so but give a
	// little bit more for safety.
	const int FUBI_RPC_MAX_SEARCH = 0x40;

	// divider
#	define FUBI_EMBED_TAG_MOV 0xB8		/* mov eax, [dword constant] */

	// basic embedded tags
#	define FUBI_EMBED_TAG_0 0x46		/* 'F' */
#	define FUBI_EMBED_TAG_1 0x75		/* 'u' */
#	define FUBI_EMBED_TAG_2 0x62		/* 'b' */
#	define FUBI_EMBED_TAG_3 0x69		/* 'i' */

	// special RPC tags
#	define FUBI_RPC_TAG_0 0x46			/* 'F' */
#	define FUBI_RPC_TAG_1 0x52			/* 'R' */
#	define FUBI_RPC_TAG_2 0x70			/* 'p' */
#	define FUBI_RPC_TAG_3 0x63			/* 'c' */

	// the full tag
	extern const BYTE FUBI_EMBEDDED_RPC_TAG[];

	// helper for embedded tagging - include initial MOVs to keep the
	// disassembler from getting confused. sequence is 0xB8 'Fubi' 0xB8 'abcd'
	// in reverse byte order.
#	define FUBI_EMBED_TAG( a, b, c, d )											\
		{																		\
			__asm jmp $+15														\
			__asm _emit FUBI_EMBED_TAG_MOV										\
			__asm _emit FUBI_EMBED_TAG_3										\
			__asm _emit FUBI_EMBED_TAG_1										\
			__asm _emit FUBI_EMBED_TAG_2										\
			__asm _emit FUBI_EMBED_TAG_0										\
			__asm _emit FUBI_EMBED_TAG_MOV										\
			__asm _emit d														\
			__asm _emit c														\
			__asm _emit b														\
			__asm _emit a														\
		}

	// helper for "which function am i?" code - puts it into s_FunctionSpec
#	define FUBI_FIND_FUNCTION( FUNC_NAME, RESOLVE_PROC, PARAM_SKIP )			\
		/* get the n'th parameter of the function */							\
		void* paramStart;														\
		{																		\
			/* this is the start of the stack frame, skipping the old ebp */	\
			/* and return addr */												\
			__asm mov eax, ebp													\
			__asm add eax, 8													\
			__asm mov paramStart, eax											\
		}																		\
		PARAM_SKIP;																\
																				\
		/* get the correct function spec and cache it */						\
		static const FunctionSpec* s_FunctionSpec = RESOLVE_PROC( GetEIP(),		\
				FUNC_NAME, ELEMENT_COUNT( FUNC_NAME ) - 1 );					\
		gpassert( s_FunctionSpec != NULL );

	// special tag to say "this function is an RPC" - 'FRpc'
#	define FUBI_RPC_TAG()														\
	FUBI_EMBED_TAG( FUBI_RPC_TAG_0, FUBI_RPC_TAG_1,								\
					FUBI_RPC_TAG_2, FUBI_RPC_TAG_3 );							\
	FuBi::AutoRpcTagBase FUBI_AutoRpcTagBase;									\
	FuBi::AutoRpcTagBase* FUBI_AutoRpcTagBasePtr = &FUBI_AutoRpcTagBase

	// implementation for an RPC call
#	define FUBI_RPC_CALL_IMPL( FUNC_NAME, THIS_PARAM, RPC_ADDRESS, RETURN )		\
	FUBI_EMBED_TAG( FUBI_RPC_TAG_0, FUBI_RPC_TAG_1,								\
					FUBI_RPC_TAG_2, FUBI_RPC_TAG_3 );							\
	FuBi::AutoRpcTag FUBI_AutoRpcTag( FUBI_AutoRpcTagBasePtr );					\
	{																			\
		using namespace FuBi;													\
																				\
		/* get function spec */													\
		FUBI_FIND_FUNCTION( FUNC_NAME, ResolveRpc, FUBI_NO_PARAM_SKIP );		\
																				\
		/* send RPC */															\
		DWORD rpcAddress = FUBI_AutoRpcTag.m_IsDispatching						\
						 ? RPC_TO_LOCAL : (RPC_ADDRESS);						\
		if ( FuBi::RpcTestMacro( rpcAddress, s_FunctionSpec ) )					\
		{																		\
			Cookie cookie = RPC_FAILURE;										\
			if ( s_FunctionSpec != NULL )										\
			{																	\
				cookie = SendRpc( s_FunctionSpec, (void*)(THIS_PARAM),			\
								  paramStart, rpcAddress );						\
			}																	\
			if ( rpcAddress != RPC_TO_ALL )										\
			{																	\
				RETURN;															\
			}																	\
		}																		\
	}

	// return helpers
#	define FUBI_RPC_RETURN \
		gpassert( cookie == RPC_SUCCESS ); return
#	define FUBI_RPC_RETRY_RETURN \
		gpassert( cookie == RPC_SUCCESS ); return cookie;

	// non-retrying RPC calls
#	define FUBI_RPC_CALL( FUNC_NAME, RPC_ADDRESS ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, NULL, RPC_ADDRESS, FUBI_RPC_RETURN )
#	define FUBI_RPC_CALL_PEEKADDRESS( FUNC_NAME ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, NULL, FUBI_TAKE_RPC_PEEKADDRESS, FUBI_RPC_RETURN )
#	define FUBI_RPC_THIS_CALL( FUNC_NAME, RPC_ADDRESS ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, this, RPC_ADDRESS, FUBI_RPC_RETURN )
#	define FUBI_RPC_THIS_CALL_PEEKADDRESS( FUNC_NAME ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, this, FUBI_TAKE_RPC_PEEKADDRESS, FUBI_RPC_RETURN )

	// retrying RPC calls
#	define FUBI_RPC_CALL_RETRY( FUNC_NAME, RPC_ADDRESS ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, NULL, RPC_ADDRESS, FUBI_RPC_RETRY_RETURN )
#	define FUBI_RPC_CALL_RETRY_PEEKADDRESS( FUNC_NAME ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, NULL, FUBI_TAKE_RPC_PEEKADDRESS, FUBI_RPC_RETRY_RETURN )
#	define FUBI_RPC_THIS_CALL_RETRY( FUNC_NAME, RPC_ADDRESS ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, this, RPC_ADDRESS, FUBI_RPC_RETRY_RETURN )
#	define FUBI_RPC_THIS_CALL_RETRY_PEEKADDRESS( FUNC_NAME ) \
		FUBI_RPC_CALL_IMPL( #FUNC_NAME, this, FUBI_TAKE_RPC_PEEKADDRESS, FUBI_RPC_RETRY_RETURN )

	// default global "this" is null
	inline void* FUBI_GetThisParam( void )  {  return ( NULL );  }

// Type system macros.

	// this will cause FuBi to replace one name with another. first param must
	// be a quoted string, second param must not be quoted (needed for unique
	// static variable name, sorry). note that this funky template stuff is
	// necessary to be able to use the macro in an H file.
#	define FUBI_REPLACE_NAME( oldName, newName ) \
		struct FuBiNameReplaceSpec_##newName  { \
			FuBi::NameReplaceSpec m_Spec; \
			FuBiNameReplaceSpec_##newName( void ) : m_Spec( oldName, #newName )  {  }  }; \
		template struct ::GlobalStatic <FuBiNameReplaceSpec_##newName>

	// this will export a "normal" enum (all in one continuous series, like
	// 3, 4, 5, etc ) to FuBi and requires tostring conversion and a first entry
	// and count
#	define FUBI_EXPORT_ENUM_IMPL( enumName, enumType, toFunc, begin, count ) \
		struct FuBiEnumExporter_##enumName  { \
			FuBi::EnumExporterT <enumType> m_Spec; \
			FuBiEnumExporter_##enumName( void ) : m_Spec( #enumName, toFunc, begin, count )  {  }  }; \
		template struct ::GlobalStatic <FuBiEnumExporter_##enumName>

#	define FUBI_EXPORT_ENUM( enumName, begin, count ) \
		FUBI_EXPORT_ENUM_IMPL( enumName, enumName, ToString, begin, count )

#	define FUBI_EXPORT_ENUM_EX( enumName, readableName, begin, count ) \
		FUBI_EXPORT_ENUM_IMPL( readableName, enumName, ToString, begin, count )

	// this will export an "irregular" enum (has breaks in the series) to FuBi
	// and requires fromstring conversion, and optional tostring conversion for
	// convenience and doco
#	define FUBI_EXPORT_IRREGULAR_ENUM_IMPL( enumName, toFunc, fromFunc, tofFunc, fromfFunc ) \
		struct FuBiEnumExporter_##enumName  { \
			FuBi::EnumExporterT <enumName> m_Spec; \
			FuBiEnumExporter_##enumName( void ) : m_Spec( #enumName, toFunc, fromFunc, tofFunc, fromfFunc )  {  }  }; \
		template struct ::GlobalStatic <FuBiEnumExporter_##enumName>

#	define FUBI_EXPORT_IRREGULAR_ENUM( enumName ) \
		FUBI_EXPORT_IRREGULAR_ENUM_IMPL( enumName, ToString, FromString, NULL, NULL )

#	define FUBI_EXPORT_BITFIELD_ENUM( enumName ) \
		FUBI_EXPORT_IRREGULAR_ENUM_IMPL( enumName, ToString, FromString, ToFullString, FromFullString )

	// this will allow adding keywords to the FuBi set
#	define FUBI_EXPORT_KEYWORDS( proc, count ) \
		struct FuBiKeywordExporter_##proc  { \
			FuBi::KeywordExporter m_Spec; \
			FuBiKeywordExporter_##proc( void ) : m_Spec( proc, count )  {  }  }; \
		template struct ::GlobalStatic <FuBiKeywordExporter_##proc>

	// this will tell FuBi that your type is plain old data
#	define FUBI_EXPORT_POD_TYPE_EX( type, name ) \
		struct FuBiPodExporter_##name  { \
			FuBi::PodExporter m_Spec; \
			FuBiPodExporter_##name( void ) : m_Spec( #name, sizeof( type ) )  {  }  }; \
		template struct ::GlobalStatic <FuBiPodExporter_##name>

	// this will tell FuBi that your type is plain old data (auto-name)
#	define FUBI_EXPORT_POD_TYPE( type ) \
		FUBI_EXPORT_POD_TYPE_EX( type, type )

	// this will tell FuBi that your type is only ever used as a pointer
#	define FUBI_EXPORT_POINTER_CLASS( type ) \
		struct FuBiPointerClassExporter_##type  { \
			FuBi::PointerClassExporter m_Spec; \
			FuBiPointerClassExporter_##type( void ) : m_Spec( #type )  {  }  }; \
		template struct ::GlobalStatic <FuBiPointerClassExporter_##type>

// Other.

	// this will force a class to be included in the final linked EXE. the
	// problem with relying on the export table for FuBi imports is that the
	// linker may choose to throw out all kinds of stuff if it's never
	// referenced from C++ code. this happens a lot with libraries linked into
	// an EXE. the workaround (and this is a HACK) is to stick a
	// FUBI_FORCE_LINK_CLASS( MyClass ) tag in your class's H file, and then
	// make sure that this H file is #included by something in one of the main
	// project's CPP files. note that your class's dtor must be public and
	// cannot be auto-generated or inline.
#	define FUBI_FORCE_LINK_CLASS( cls ) \
		GLOBALSTATIC_BEGIN_FORCELINK( ForceLink, cls ); \
		((cls*)0)->~cls(); \
		GLOBALSTATIC_END_FORCELINK( ForceLink, cls )

//////////////////////////////////////////////////////////////////////////////
// FuBi API declarations

namespace FuBi
{

// Forward declarations.

	// enums
	enum eVarType;

	// structs
	struct ClassSpec;
	struct EnumSpec;
	struct FunctionSpec;
	struct ConstraintSpec;
	struct ParamSpec;
	struct TypeSpec;
	struct VariableSpec;
	struct TraitSpec;

	// classes
	class Store;
	class Record;
	class HeaderSpec;
	class ClassHeaderSpec;
	class PersistContext;
	class BitPacker;
	class StoreHeaderSpec;
	class SysExports;			// singleton - gFuBySysExports

	// used for function doco
	struct FunctionDocs
	{
		const char* m_ParamDocs;
		const char* m_FunctionDocs;

		FunctionDocs( const char* paramDocs, const char* functionDocs )
		{
			m_ParamDocs    = paramDocs;
			m_FunctionDocs = functionDocs;
		}
	};

	// cookie type for retryable rpc's
	DECLARE_HANDLE( Cookie );

	// string conversion
	const char* ToString( Cookie cookie );

// Constants.

	// errors
	const UINT INVALID_FUNCTION_SERIAL = scast <UINT> ( -1 );
	const int  INVALID_COLUMN_INDEX = -1;

	// strings
	extern const char EVENT_NAMESPACE    [];		// namespace "FuBiEvent"
	extern const char TYPE_SKRITOBJECT   [];		// class "SkritObject"
	extern const char TYPE_SKRITVM       [];		// class "SkritMachine"
	extern const char TYPE_SIEGEPOSDATA  [];		// struct "SiegePosData"
	extern const char TYPE_SIEGEPOS      [];		// struct "SiegePos"
	extern const char TYPE_QUAT          [];		// struct "Quat"
	extern const char TYPE_VECTOR3       [];		// struct "vector_3"
	extern const char TYPE_FUBICOOKIE    [];		// struct "FuBiCookie"

// x86 utility.

	DWORD GetEIP( void );
	DWORD GetST0( void );

// Remote procedure call methods.

	// resolution/packing (used by macros)
	const FunctionSpec* ResolveRpc( DWORD EIP, const char* funcName, int funcNameLen );
	Cookie              SendRpc   ( const FunctionSpec* spec, void* thisParam, const void* firstParam, DWORD toAddress );

	// helpers
	int GetCurrentRpcPacketSize( bool take = false );

	// do not call this! internal use only!
	bool RpcTestMacro( DWORD addr, const FunctionSpec* spec );

// Callback methods.

	const FunctionSpec* LookupEvent ( const void* addr, const char* funcName, int funcNameLen );
	const FunctionSpec* LookupEvent ( const void* addr, const char* funcName );
	const FunctionSpec* ResolveEvent( DWORD EIP, const char* funcName, int funcNameLen );
	const FunctionSpec* ResolveCall ( DWORD EIP, const char* funcName, int funcNameLen );

// Name replacement.

	struct NameReplaceSpec : GlobalRegistrar <NameReplaceSpec>
	{
		const char* m_OldName;
		const char* m_NewName;

		typedef std::pair <const char*, const char*> ReplacePair;
		typedef stdx::fast_vector <ReplacePair> ReplaceColl;

		NameReplaceSpec( const char* oldName, const char* newName )
		{
			m_OldName = oldName;
			m_NewName = newName;
		}
		static const char* GetReplacement( const char* oldName );

		static void GetUnusedReplacements( ReplaceColl& strings );
	};

// Remote procedure calls.

	// constants
	const DWORD RPC_INVALID_ADDR		= (DWORD)( -1 );	// $ this does not correspond to a DPID
	const DWORD RPC_TO_ALL				= (DWORD)(  0 );	// == DPNID_ALL_PLAYERS_GROUP
	const DWORD RPC_TO_OTHERS			= (DWORD)( -2 );	// $ this does not correspond to a DPID
	const DWORD RPC_TO_LOCAL_DEFAULT	= (DWORD)( -3 );	// default local address set by default upon starting (all machines are servers initially)
	const DWORD RPC_TO_SERVER_LOCAL		= (DWORD)( -3 );	// address for the server if it's local
	const DWORD RPC_TO_SERVER_REMOTE	= (DWORD)( -4 );	// address for the server if it's remote
	const DWORD RPC_TEST_ADDR_START		= (DWORD)(  1 );	// just a nice starting point for a non-dplay test app to assign machine id's

	// "constants" - treat as constant, modify ONLY by net pipe
	extern const DWORD& RPC_TO_LOCAL;
	extern const DWORD& RPC_TO_SERVER;
	extern const DWORD& RPC_CALLER;

	// address changing
	void SetCallerMachineId ( DWORD machineId );
	void SetRpcIdsToDefaults( void );

	// machine stats
	void SetServerIsRemote( void );
	void SetServerIsLocal ( void );
	bool IsServerLocal    ( void );
	bool IsMachineRemote  ( DWORD machineId );

	// cookie constants
	const Cookie RPC_SUCCESS        = (Cookie)(-1);		// successful rpc request
	const Cookie RPC_FAILURE        = (Cookie)(-2);		// failure to exec - try again
	const Cookie RPC_SUCCESS_IGNORE = (Cookie)(-3);		// successful, but because we aborted it
	const Cookie RPC_FAILURE_IGNORE = (Cookie)(-4);		// failure, but don't retry, we want to abort

	// checker
	inline bool IsSpecialCookie( Cookie cookie )
		{  return ( (cookie == RPC_SUCCESS) || (cookie == RPC_FAILURE) || (cookie == RPC_SUCCESS_IGNORE) || (cookie == RPC_FAILURE_IGNORE) );  }

	// special rpc base tag for allowing aborts of dispatches
	struct AutoRpcTagBase
	{
		bool m_IsDispatching;
		DECL_THREAD static int ms_PeekAddress;

		AutoRpcTagBase( void );

	   ~AutoRpcTagBase( void )
		{
			ms_PeekAddress = RPC_INVALID_ADDR;
		}

		static void SetPeekAddress( DWORD addr )
		{
			gpassert( ms_PeekAddress == RPC_INVALID_ADDR );
			ms_PeekAddress = addr;
		}

		static DWORD TakePeekAddress( void )
		{
 			gpassert( ms_PeekAddress != RPC_INVALID_ADDR );
			DWORD addr = ms_PeekAddress;
			ms_PeekAddress = RPC_INVALID_ADDR;
			return ( addr );
		}
	};

	// special rpc stack building tag for verification/optimization
	struct AutoRpcTag
	{
		bool m_IsDispatching;

		AutoRpcTag( const AutoRpcTagBase* base = NULL );
	   ~AutoRpcTag( void );
	};

	// for am-i-in-a-dispatched-rpc requests
#	define FUBI_IN_RPC_DISPATCH (FUBI_AutoRpcTag.m_IsDispatching)
#	define FUBI_GET_RPC_PACKET_SIZE FuBi::GetCurrentRpcPacketSize()
#	define FUBI_TAKE_RPC_PACKET_SIZE FuBi::GetCurrentRpcPacketSize( true )
#	define FUBI_SET_RPC_PEEKADDRESS( x ) FuBi::AutoRpcTagBase::SetPeekAddress( x )
#	define FUBI_TAKE_RPC_PEEKADDRESS FuBi::AutoRpcTagBase::TakePeekAddress()

	// verify structure for checking an rpc stack
	struct RpcVerifySpec
	{
		const FunctionSpec* m_Function;				// function called
		DWORD               m_Address;				// address for destination of RPC
		const_mem_ptr       m_Params;				// parameters being passed to function (NULL for void func, note that 'size' is FULL PACKET SIZE, 0 if unavailable)
		void*               m_This;					// address of object if any

		RpcVerifySpec( void )
		{
			m_Function = NULL;
			m_Address  = RPC_INVALID_ADDR;
			m_This     = NULL;
		}

		void SetAsNestDetectCancel( void )
		{
			m_Function = (const FunctionSpec*)1;
			m_Address  = RPC_TO_ALL;
			m_This     = NULL;
		}

		bool IsNestDetectCancel( void ) const
		{
			return ( (int)m_Function == 1 );
		}

		gpstring BuildQualifiedNameAndParams( bool isRpcPacked, bool allowBinaryOut = false, int skipParams = 0 ) const;
	};

	// collection type for inspecting the stack
	typedef stdx::fast_vector <RpcVerifySpec> RpcVerifyColl;

	// collection type for holding lots of addresses
	typedef stdx::fast_vector <DWORD> AddressColl;

	// type for specifying rebroadcasting options
	enum eBroadcastType
	{
		BROADCAST_ABORT,				// abort the rpc, detected to be unnecessary
		BROADCAST_NORMAL,				// ship out rpc as requested
		BROADCAST_REDIRECT,				// redirect rpc based on addresses
		BROADCAST_DEFER,				// defer shipping out rpc (addresses are irrelevant)
	};

	inline bool IsNormal  ( eBroadcastType type )  {  return ( (type == BROADCAST_NORMAL) || (type == BROADCAST_DEFER) );  }
	inline bool IsRedirect( eBroadcastType type )  {  return ( type == BROADCAST_REDIRECT );  }
	inline bool IsDeferred( eBroadcastType type )  {  return ( type == BROADCAST_DEFER );  }

	// callback for rebroadcasting
	struct IRebroadcaster
	{
		// called by FuBi to verify sends and dispatches of RPC's
		virtual bool OnVerifySendRpc    ( const RpcVerifyColl& /*specs*/, const AddressColl& /*addresses*/, bool /*isRpcPacked*/ )  {  return ( true );  }
		virtual bool OnVerifyDispatchRpc( bool /*preCall*/, const RpcVerifySpec& /*spec*/ )  {  return ( true );  }

		// called by FuBi to retarget broadcast-type RPC's
		virtual eBroadcastType OnProcessBroadcastRpc( const RpcVerifySpec& /*spec*/, AddressColl& /*addresses*/ )  {  return ( BROADCAST_NORMAL );  }

		// called by FuBi to adjust transport singleton
		virtual void OnBeginRedirect( const RpcVerifyColl& /*specs*/ )  {  }
		virtual void OnEndRedirect  ( void )  {  }

		// called by FuBi for stats reporting
		virtual void OnSendRpc    ( const FuBi::FunctionSpec* /*spec*/, DWORD /*thisPtr*/, DWORD /*packetSize*/ )  {  }
		virtual void OnDispatchRpc( const FuBi::FunctionSpec* /*spec*/, DWORD /*thisPtr*/, DWORD /*packetSize*/ )  {  }
	};

	// special auto-class for declaring a nest-detect cancellation scope
	struct AutoRpcNestDetectCancel
	{
		AutoRpcNestDetectCancel( void );
	   ~AutoRpcNestDetectCancel( void );
	};

// Membership.

	// custom namespace to make our macros more convenient
	namespace FunctionSpecFlags
	{
		// $ these must sync with FunctionSpec::eFlags

		enum ePermissions
		{
			SERVER,			// this command must have been sent from the server
			DEV,			// only allow this command in development builds (!GP_RETAIL)
			RETRY,			// this is a "retryable" function - retry until it succeeds
		};

		enum eMembership
		{
			ALL,			// enabled for everybody (same as setting all the other bits)
			GAME,			// just enabled for game
			CONSOLE,		// just enabled for the development console
			EDITOR,			// just enabled for SiegeEditor
			TRIAL,			// this is a trial function - do not rely on it yet!
		};

	};

	struct FunctionFlagBuilder
	{
		enum eCommand
		{
			COMMAND_ADD,
			COMMAND_REMOVE,
		};

		typedef FunctionSpecFlags::ePermissions ePermissions;
		typedef stdx::fast_vector <ePermissions> PermissionColl;

		typedef FunctionSpecFlags::eMembership  eMembership;
		typedef std::pair <eMembership, eCommand> Membership;
		typedef stdx::fast_vector <Membership>  MembershipColl;

		PermissionColl m_Permissions;
		MembershipColl m_Memberships;

		FunctionFlagBuilder& operator + ( ePermissions flag )
			{  m_Permissions.push_back( flag );  return ( *this );  }

		FunctionFlagBuilder& operator + ( eMembership  flag )
			{  m_Memberships.push_back( Membership( flag, COMMAND_ADD ) );  return ( *this );  }
		FunctionFlagBuilder& operator - ( eMembership  flag )
			{  m_Memberships.push_back( Membership( flag, COMMAND_REMOVE ) );  return ( *this );  }
	};

// Traits.

	// xfer format for string output (suggested only)
	enum eXfer
	{
		XFER_NORMAL,		// output as normal
		XFER_HEX,			// output as hex
		XFER_FOURCC,		// output as FOURCC
		XFER_QUOTED,		// output as quoted string
	};

	// modes for data transfer
	enum eXMode
	{
		XMODE_QUIET,		// all defaults all the time, no errors
		XMODE_RELAXED,		// only gives errors for "severe" problems - data conversion errors, bad property names...
		XMODE_STRICT,		// gives errors for anything

		// special
		XMODE_DEFAULT = XMODE_STRICT,	// this is the default (which is 'strict')
	};

	// query on enum
#	if !GP_RETAIL
	inline bool ShouldReportConversionErrors( eXMode mode )  {  return ( mode >= XMODE_RELAXED );  }
	inline bool ShouldReportNameErrors      ( eXMode mode )  {  return ( mode >= XMODE_RELAXED );  }
#	endif // !GP_RETAIL

	// error reporting
#	if !GP_RETAIL
	ReportSys::Context& GetXferErrors( void );
#	define gXferErrors FuBi::GetXferErrors()
#	endif // !GP_RETAIL

	// basic prototypes
	typedef void (__cdecl *ToStringProc  )( gpstring&, const void*, eXfer );
	typedef bool (__cdecl *FromStringProc)( const char*, void* );
	typedef void (__cdecl *CopyVarProc   )( void*, const void* );

	// traits prototype - instances must implement commented-out functions
	template <typename T> struct Traits
	{
	//  static eFlags      GetFlags          ( void );
	//  static int         GetSize           ( void );
	//  static eVarType    GetVarType        ( void );
	//  static gpstring    ToString          ( const T& obj, eXfer xfer = XFER_NORMAL );
	//  static bool        FromString        ( const char* str, T& obj );
	//  static const char* GetInternalVarName( void );
	//  static const char* GetExternalVarName( void );
	};

	// construct a trait by type
	template <typename T>
	const Traits <T> & GetTraits( const T& )
	{
		static Traits <T> s_Traits;
		return ( s_Traits );
	}

	// helpers
	eVarType GetTypeByName( const char* name );

	// traits base class - provides types
	struct Trait
	{
		enum eFlags
		{
			FLAG_NONE      =      0,			// nothing special
			FLAG_INTEGER   = 1 << 0,			// integer type
			FLAG_FLOAT     = 1 << 1,			// float type
			FLAG_POD       = 1 << 2,			// plain-old-data - no ctor/dtor that does anything useful (applies to members too)
			FLAG_FORCETEXT = 1 << 3,			// force text conversions on this (good for enums)
		};

		Trait( void )  {  }
		SET_NO_COPYING( Trait );
	};

	MAKE_ENUM_BIT_OPERATORS( Trait::eFlags );

// Enum exporting.

	// enum-specialized prototypes
	typedef const char* (__cdecl *EnumToStringProc      )( DWORD );
	typedef bool        (__cdecl *EnumFromStringProc    )( const char* str, DWORD& );
	typedef gpstring    (__cdecl *EnumToFullStringProc  )( DWORD );
	typedef bool        (__cdecl *EnumFromFullStringProc)( const char* str, DWORD& );

	// the exporter
	struct EnumExporter		// don't inherit from registrar - this class is instantiated more than just globally
	{
		const char*            m_Name;
		ToStringProc           m_ToStringProc;
		FromStringProc         m_FromStringProc;
		EnumToStringProc       m_EnumToStringProc;
		EnumFromStringProc     m_EnumFromStringProc;
		EnumToFullStringProc   m_EnumToFullStringProc;
		EnumFromFullStringProc m_EnumFromFullStringProc;
		DWORD                  m_Begin;
		int                    m_Count;
		EnumSpec*              m_Parent;
		EnumExporter*          m_Next;
		static EnumExporter*   ms_Root;

		EnumExporter( void )  {  ZeroObject( *this );  }

		EnumExporter( const char* name, ToStringProc to, FromStringProc from, EnumToStringProc eto, EnumFromStringProc efrom, EnumToFullStringProc efto = NULL, EnumFromFullStringProc effrom = NULL )
		{
			m_Name                   = name;
			m_ToStringProc           = to;
			m_FromStringProc         = from;
			m_EnumToStringProc       = eto;
			m_EnumFromStringProc     = efrom;
			m_EnumToFullStringProc   = efto;
			m_EnumFromFullStringProc = effrom;
			m_Begin                  = 0;
			m_Count                  = 0;
			m_Parent                 = NULL;
			m_Next                   = ms_Root;
			ms_Root                  = this;
		}

		EnumExporter( const char* name, ToStringProc to, FromStringProc from, EnumToStringProc eto, DWORD begin, int count )
		{
			m_Name                   = name;
			m_ToStringProc           = to;
			m_FromStringProc         = from;
			m_EnumToStringProc       = eto;
			m_EnumFromStringProc     = NULL;
			m_EnumToFullStringProc   = NULL;
			m_EnumFromFullStringProc = NULL;
			m_Begin                  = begin;
			m_Count                  = count;
			m_Parent                 = NULL;
			m_Next                   = ms_Root;
			ms_Root                  = this;
		}

		bool IsContinuous( void ) const  {  return ( m_Count > 0 );  }
		bool HasConstants( void ) const  {  return ( m_FromStringProc != NULL );  }
	};

	DECL_SELECT_ANY EnumExporter* EnumExporter::ms_Root = NULL;

	// helper method
	bool EnumConstantFromString( const char* str, const EnumSpec* spec, DWORD& val, bool fastOnly = false );

	// type-specific exporter
	template <typename T>
	struct EnumExporterT : EnumExporter, Singleton <EnumExporterT <T> >
	{
		typedef T Type;
		typedef EnumExporterT <T> ThisType;
		typedef Singleton <EnumExporterT <T> > SingleType;

		typedef const char* (__cdecl *TToStringProc      )( T );
		typedef bool        (__cdecl *TFromStringProc    )( const char*, T& );
		typedef gpstring    (__cdecl *TToFullStringProc  )( T );
		typedef bool        (__cdecl *TFromFullStringProc)( const char*, T& );

		EnumExporterT( const char* name, TToStringProc to, TFromStringProc from, TToFullStringProc fto, TFromFullStringProc ffrom )
			: EnumExporter( name,
							ToString,
							FromString,
							rcast <EnumToStringProc      > ( to ),
							rcast <EnumFromStringProc    > ( from ),
							rcast <EnumToFullStringProc  > ( fto ),
							rcast <EnumFromFullStringProc> ( ffrom ) )  {  }

		EnumExporterT( const char* name, TToStringProc to, T begin, int count )
			: EnumExporter( name,
							ToString,
							FromString,
							rcast <EnumToStringProc> ( to ),
							scast <DWORD> ( begin ),
							count )  {  }

		static void __cdecl ToString( gpstring& out, const void* obj, eXfer /*xfer*/ = XFER_NORMAL )
		{
			ThisType& exp = SingleType::GetSingleton();
			out = (*exp.m_EnumToStringProc)( *rcast <const DWORD*> ( obj ) );
		}
		static bool __cdecl FromString( const char* str, void* obj )
		{
			ThisType& exp = SingleType::GetSingleton();
			if ( exp.m_FromStringProc != NULL )
			{
				return ( (*exp.m_EnumFromStringProc)( str, *rcast <DWORD*> (obj ) ) );
			}
			else
			{
				return ( EnumConstantFromString( str, exp.m_Parent, *rcast <DWORD*> ( obj ), true ) );
			}
		}
		static void __cdecl ToFullString( gpstring& out, const void* obj, eXfer xfer = XFER_NORMAL )
		{
			ThisType& exp = SingleType::GetSingleton();
			if ( exp.m_EnumToFullStringProc != NULL )
			{
				out = (*exp.m_EnumToFullStringProc)( *rcast <const DWORD*> ( obj ) );
			}
			else
			{
				return ( ToString( out, obj, xfer ) );
			}
		}
		static bool __cdecl FromFullString( const char* str, void* obj )
		{
			ThisType& exp = SingleType::GetSingleton();
			if ( exp.m_FromFullStringProc != NULL )
			{
				return ( (*exp.m_EnumFromFullStringProc)( str, *rcast <DWORD*> (obj ) ) );
			}
			else
			{
				return ( FromString( str, exp.m_Parent, *rcast <DWORD*> ( obj ), true ) );
			}
		}
	};

// POD trait exporting.

	struct PodExporter : GlobalRegistrar <PodExporter>
	{
		const char* m_Name;
		size_t      m_Size;

		PodExporter( const char* name, size_t size )
		{
			m_Name = name;
			m_Size = size;
		}
	};

// PointerClass exporting.

	struct PointerClassExporter : GlobalRegistrar <PointerClassExporter>
	{
		const char* m_Name;

		PointerClassExporter( const char* name )
		{
			m_Name = name;
		}
	};

// Keyword exporting.

	struct KeywordExporter : GlobalRegistrar <KeywordExporter>
	{
		typedef const char* (*QueryProc)( int index );

		QueryProc m_QueryProc;
		int       m_Count;

		KeywordExporter( QueryProc proc, int count )
		{
			gpassert( proc != NULL );
			gpassert( count > 0 );

			m_QueryProc = proc;
			m_Count     = count;
		}
	};

// Misc.

	enum eNamingConvention
	{
		NAMING_DATA,
		NAMING_CODE,
		NAMING_DATA_OR_CODE,
	};

	// check the string to see if it's a valid sysid - data convention is
	// lower_case_naming, whereas code convention is UpperCaseNaming or
	// CONSTANT_CASE_NAMING.
	bool IsValidSysId( const char* name, eNamingConvention naming = NAMING_CODE );

// Parameter constraint exporting.

	struct ConstraintExporter
	{
		//$$$ strings only
	};

	//$$$ RegisterConstraint()...

// enum eVarType declaration

	enum eVarType
	{
		VAR_UNKNOWN = -1,

		// standard types

		SET_BEGIN_ENUM( VAR_, 0 ),

		VAR_SCHAR,						// signed char
		VAR_UCHAR,						// unsigned char
		VAR_SHORT,						// short
		VAR_USHORT,						// unsigned short
		VAR_INT,						// int
		VAR_UINT,						// unsigned int
		VAR_INT64,						// int64
		VAR_UINT64,						// unsigned int64
		VAR_FLOAT,						// float
		VAR_DOUBLE,						// double
		VAR_VOID,						// void
		VAR_BOOL,						// bool

		SET_END_ENUM( VAR_ ),

		// mapped types - update these if we ever move to win64

	#	ifdef _CHAR_UNSIGNED
		VAR_CHAR       = VAR_UCHAR,		// char
	#	else
		VAR_CHAR       = VAR_SCHAR,		// char
	#	endif

		// ansi/ms
		VAR_SSHORT     = VAR_SHORT,		// signed short
		VAR_SINT       = VAR_INT,		// signed int
		VAR_LONG       = VAR_INT,		// long
		VAR_SLONG      = VAR_LONG,		// signed long
		VAR_ULONG      = VAR_UINT,		// unsigned long
		VAR_INT8       = VAR_SCHAR,		// int8
		VAR_SINT8      = VAR_SCHAR,		// signed int8
		VAR_UINT8      = VAR_UCHAR,		// unsigned int8
		VAR_INT16      = VAR_SSHORT,	// int16
		VAR_SINT16     = VAR_SSHORT,	// signed int16
		VAR_UINT16     = VAR_USHORT,	// unsigned int16
		VAR_INT32      = VAR_SINT,		// int32
		VAR_SINT32     = VAR_SINT,		// signed int32
		VAR_UINT32     = VAR_UINT,		// unsigned int32
		VAR_SINT64     = VAR_INT64,		// signed int64
		VAR_LONGDOUBLE = VAR_DOUBLE,	// long double - same as double in 32-bit code (no longer 80 bit)

		// windows
		VAR_BYTE       = VAR_UCHAR,		// Win32 "BYTE"
		VAR_WORD       = VAR_USHORT,	// Win32 "WORD"
		VAR_DWORD      = VAR_ULONG,		// Win32 "DWORD"

		// these require special processing

		SET_BEGIN_ENUM( VAR_SPECIAL_, 0x1000 ),

		VAR_MEM_PTR,					// mem_ptr
		VAR_CONST_MEM_PTR,				// const_mem_ptr
		VAR_STRING,						// gpstring/std::gp_string
		VAR_WSTRING,					// gpwstring/std::gp_wstring

		SET_END_ENUM( VAR_SPECIAL_ ),

		// enum - these are enums exported from the game

		VAR_ENUM = 0x2000,				// map types to these on the fly

		// user types - everything else

		VAR_USER = 0x3000,				// map types to these on the fly - they correspond to class entries in SysExports

		// special tags
		VAR_ENUM_END = VAR_USER,
	};

	MAKE_ENUM_MATH_OPERATORS( eVarType );

}	// end of FuBi namespace

//////////////////////////////////////////////////////////////////////////////
// non-FuBi namespace

// Misc helpers.

	namespace FuBiEvent  {  }
#	define DECLARE_EVENT namespace FuBiEvent

#	if GP_RETAIL
#	define CHECK_SERVER_ONLY
#	else // !GP_RETAIL
#	define CHECK_SERVER_ONLY \
	if ( !FuBi::IsServerLocal() ) \
	{ \
		gperrorf(( "This can only be called on the server, server machineId = 0x%08x, local machineId = 0x%08x", \
				   RPC_TO_SERVER, RPC_TO_LOCAL )); \
	}
#	endif // !GP_RETAIL

// Globals.

	// constants
	using FuBi::RPC_INVALID_ADDR;
	using FuBi::RPC_TO_SERVER;
	using FuBi::RPC_TO_ALL;
	using FuBi::RPC_TO_OTHERS;
	using FuBi::RPC_TO_LOCAL;
	using FuBi::RPC_CALLER;

	// cookies
	typedef FuBi::Cookie FuBiCookie;
	using FuBi::RPC_SUCCESS;
	using FuBi::RPC_FAILURE;
	using FuBi::RPC_SUCCESS_IGNORE;
	using FuBi::RPC_FAILURE_IGNORE;
	FUBI_REPLACE_NAME( "FuBiCookie__", FuBiCookie );

	// special
	extern FuBi::AutoRpcTagBase* FUBI_AutoRpcTagBasePtr;

//////////////////////////////////////////////////////////////////////////////

#endif  // __FUBIDEFS_H

//////////////////////////////////////////////////////////////////////////////
