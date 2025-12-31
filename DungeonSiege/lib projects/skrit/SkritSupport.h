//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritSupport.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains support utility classes for Skrit. Note that these
//             are not in the Skrit namespace and will only be used by the
//             import parser - probably never even need to #include this file!
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __SKRITSUPPORT_H
#define __SKRITSUPPORT_H

//////////////////////////////////////////////////////////////////////////////

#include "DllBinder.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "SkritDefs.h"

struct vector_3;
struct Quat;

//////////////////////////////////////////////////////////////////////////////
// traits

FUBI_DECLARE_TRAITS_IMPL( Skrit::HObject, "SkritHObject", (Skrit::HObject()), PointerClass, FuBi::GetTypeByName( "SkritHObject" ) )
{
#	if GP_DEBUG
	template <typename U>
	static bool Xfer( PersistContext& persist, eXfer xfer, const char* key, T& obj, const U& defValue )
	{
		gpassert( persist.IsRestoring() || obj.IsPersistent() || !obj );
		return ( PointerClassTraitBase <Skrit::HObject> ::Xfer( persist, xfer, key, obj, defValue ) );
	}
#	endif // GP_DEBUG
};

FUBI_DECLARE_XFER_TRAITS( Skrit::HAutoObject )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
	{
		gpassert( persist.IsSaving() || !obj );
		Skrit::HObject localObj = obj.GetHandle();
		bool rc = persist.Xfer( xfer, key, localObj );
		obj = localObj;
		return ( rc );
	}
};

//////////////////////////////////////////////////////////////////////////////
// class Debug declaration

class Debug : public AutoSingleton <Debug>
{
public:
	SET_NO_INHERITED( Debug );

	Debug( void );
   ~Debug( void );

// Output/printing.

	FUBI_EXPORT const char* Format( const char* format, ... );
	FUBI_MEMBER_DOC       ( Format,            "format",
						   "Print formatted output ('printf' style) to a string and return it." );

// Assertions.

	static FUBI_EXPORT void Assert( int testExpr );
		   FUBI_MEMBER_DOC( Assert$0,  "testExpr",
						   "Assert on 'testExpr'." );

	static FUBI_EXPORT void Assert( int testExpr, const char* msg );
		   FUBI_MEMBER_DOC( Assert$1,  "testExpr,"           "msg",
						   "Assert on 'testExpr', print out 'msg' on failure." );

	static FUBI_EXPORT void AssertF( int testExpr, const char* format, ... );
		   FUBI_MEMBER_DOC( AssertF,    "testExpr,"           "format",
						   "Assert on 'testExpr', print out 'format' (printf-style) on failure." );

// Misc.

	static FUBI_EXPORT void Breakpoint( void );
		   FUBI_MEMBER_DOC( Breakpoint, "",
						   "Break into debugger." );

#	if GPSTRING_TRACKING
	static FUBI_EXPORT void DumpStrings( void );
#	endif // GPSTRING_TRACKING
#	if !GP_ERROR_FREE
	static FUBI_EXPORT void DumpGameStack( void );
#	endif // !GP_ERROR_FREE

private:
	char m_Buffer[ 1000 ];

	FUBI_SINGLETON_CLASS( Debug, "Debugging functions." );
	FUBI_FORCE_LINK_CLASS( Debug );
	SET_NO_COPYING( Debug );
};

#define gDebug Debug::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class Help declaration

#if !GP_RETAIL

class Help : public AutoSingleton <Help>
{
public:
	SET_NO_INHERITED( Help );

	Help( void );
   ~Help( void );

	static FUBI_EXPORT void All( void );
		   FUBI_MEMBER_DOC( All, "",
						   "Dump the entire system to the console." );

	static FUBI_EXPORT void Classes( bool includeHidden );
//		   FUBI_MEMBER_DOC( Classes, "",
//						   "List all available classes to the console." );

	static FUBI_EXPORT void Classes( void )
									 {  Classes( false );  }

	static FUBI_EXPORT void Enums( bool includeConstants );
//		   FUBI_MEMBER_DOC( Enums, "",
//						   "List all available enumerations to the console." );

	static FUBI_EXPORT void Enums( void )
									 {  Enums( false );  }

	static FUBI_EXPORT void Globals( void );
		   FUBI_MEMBER_DOC( Globals, "",
						   "List all available global functions to the console." );

	static FUBI_EXPORT void Class( const char* className );
		   FUBI_MEMBER_DOC( Class,            "className",
						   "Get help for a class's functions and variables." );

	static FUBI_EXPORT void Enum( const char* enumName );
		   FUBI_MEMBER_DOC( Enum,            "enumName",
						   "List all the constants available for the given enum." );

	static FUBI_EXPORT void Global( const char* globalName );
		   FUBI_MEMBER_DOC( Global,            "globalName",
						   "Get help for an individual global function." );

private:
	FUBI_SINGLETON_CLASS( Help, "Help support for SiegeSkrit." );
	FUBI_FORCE_LINK_CLASS( Help );
	SET_NO_COPYING( Help );
};

#define gHelp Help::GetSingleton()

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class VTune declaration

#if GP_ENABLE_PROFILING

class VTune : public AutoSingleton <VTune>
{
public:
	SET_NO_INHERITED( VTune );

	VTune( void )  {  }
   ~VTune( void );

	FUBI_EXPORT bool Pause( void );
	FUBI_MEMBER_DOC( Pause, "",
					"Pause VTune's sampling." );

	FUBI_EXPORT bool Resume( void );
	FUBI_MEMBER_DOC( Resume, "",
					"Resume VTune's sampling." );

private:
	class VTuneAPI : public DllBinder
	{
	public:
		VTuneAPI( void );

		DllProc0 <BOOL> VtPauseSampling;
		DllProc0 <BOOL> VtResumeSampling;
	};

	VTuneAPI m_VTuneAPI;

	FUBI_SINGLETON_CLASS( VTune, "VTune support API." );
	FUBI_FORCE_LINK_CLASS( VTune );
	SET_NO_COPYING( VTune );
};

#define gVTune VTune::GetSingleton()

#endif // GP_ENABLE_PROFILING

//////////////////////////////////////////////////////////////////////////////
// namespace Skrit declaration

namespace Skrit
{
	FUBI_EXPORT void Execute           ( const char* skritName, const char* funcName );
	FUBI_EXPORT void Execute           ( const char* skritName );
	FUBI_EXPORT void Command           ( const char* command );

#	if !GP_RETAIL
	FUBI_EXPORT void Dump              ( const char* skritName );
	FUBI_EXPORT void Disassemble       ( const char* skritName );
	FUBI_EXPORT void CheckFile         ( const char* skritName );
	FUBI_EXPORT void CheckCommand      ( const char* command );
	FUBI_EXPORT void DumpCommand       ( const char* command );
	FUBI_EXPORT void DisassembleCommand( const char* command );
#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
// namespace FuBi declaration

namespace FuBi
{
#	if !GP_RETAIL
	FUBI_EXPORT void DumpPrototypes( void );
#	endif // !GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
// namespace Math declaration

namespace Math
{

// Random numbers.

	FUBI_EXPORT float RandomFloat( float maxFloat );
	FUBI_DOC        ( RandomFloat$0,		"maxFloat",
					 "Returns a randomly generated float between 0.0 and 'maxFloat'." );

	FUBI_EXPORT float RandomFloat( float minFloat, float maxFloat );
	FUBI_DOC        ( RandomFloat$1,      "minFloat,"     "maxFloat",
					 "Returns a randomly generated float between 'minFloat' and 'maxFloat'." );

	FUBI_EXPORT int   RandomInt( int maxInt );
	FUBI_DOC        ( RandomInt$0,		"maxInt",
					 "Returns a randomly generated integer between 0 and 'maxInt'." );

	FUBI_EXPORT int   RandomInt( int minInt, int maxInt );
	FUBI_DOC        ( RandomInt$1,    "minInt,"   "maxInt",
					 "Returns a randomly generated integer between 'minInt' and 'maxInt'." );

// Trig.

	FUBI_EXPORT float Sin( float f );
	FUBI_DOC        ( Sin,      "f",
					 "Returns the sine of 'f', which must be in radians." );

	FUBI_EXPORT float Cos( float f );
	FUBI_DOC        ( Cos,      "f",
					 "Returns the cosine of 'f', which must be in radians." );

	FUBI_EXPORT float Abs( float f );
	FUBI_DOC        ( Abs,      "f",
					 "Returns the absolute value of 'f'." );

	FUBI_EXPORT int   Round( float f );
	FUBI_DOC        ( Round,      "f",
					 "Returns 'f' rounded to the nearest integer." );

	FUBI_EXPORT float RoundToFloat( float f );
	FUBI_DOC        ( RoundToFloat,      "f",
					 "Returns 'f' rounded to the nearest integer (returned as a float)." );

	FUBI_EXPORT float Floor( float f );
	FUBI_DOC        ( Floor,      "f",
					 "Returns the floor of 'f'." );

	FUBI_EXPORT float Ceil( float f );
	FUBI_DOC        ( Ceil,      "f",
					 "Returns the ceil of 'f'." );

	FUBI_EXPORT float RadiansToDegrees( float f );
	FUBI_DOC        ( RadiansToDegrees,      "f",
					 "Converts 'f' from radians to degrees." );

	FUBI_EXPORT float DegreesToRadians( float f );
	FUBI_DOC        ( DegreesToRadians,      "f",
					 "Converts 'f' from degrees to radians." );

// Vectors.

	FUBI_EXPORT float Length( const vector_3& v );
	FUBI_DOC        ( Length,				"v",
					 "Returns length of v." );

	FUBI_EXPORT float Length2( const vector_3& v );
	FUBI_DOC        ( Length2,				"v",
					 "Returns squared length of v." );

// Filters.

	FUBI_EXPORT int   MaxInt( int a, int b );
	FUBI_DOC		( MaxInt$0, "a, b",
					  "Returns the maximum of two numbers." );

	FUBI_EXPORT int   MinInt( int a, int b );
	FUBI_DOC		( MinInt$0, "a, b",
					  "Returns the minimum of two numbers." );

	FUBI_EXPORT float MaxFloat( float a, float b );
	FUBI_DOC		( MaxFloat$0, "a, b",
					  "Returns the maximum of two numbers." );

	FUBI_EXPORT float MinFloat( float a, float b );
	FUBI_DOC		( MinFloat$0, "a, b",
					  "Returns the minimum of two numbers." );

	FUBI_EXPORT float FilterSmoothStep( float a, float b, float x );
	FUBI_DOC        ( FilterSmoothStep$0,    "a[0;1],  b[0;1],  x[0;1]",
					 "Run a smoothing filter on 'x', with 'a' and 'b' defining "
					 "the beginning and ending of the filter along x." );

	FUBI_EXPORT float FilterSmoothStep( float x );
	FUBI_DOC        ( FilterSmoothStep$1,    "x[0;1]",
					 "Run a smoothing filter on 'x'." );
// Constants

	FUBI_EXPORT float Pi( void );
	FUBI_DOC		( Pi, "", "Mathematical constant we all know and love." );

	FUBI_EXPORT float PiHalf( void );
	FUBI_DOC		( PiHalf, "", "Mathematical constant we all know and love * 0.5." );

// Conversions

	FUBI_EXPORT int   ToInt( float f );
	FUBI_DOC        ( ToInt, "f", "Convert a float to an int." );

	FUBI_EXPORT float ToFloat( int i );
	FUBI_DOC        ( ToFloat, "i", "Convert an int to a float." );

	FUBI_EXPORT float FromPercent( float f );
}

//////////////////////////////////////////////////////////////////////////////
// class Report declaration

class Report
{

// Reporting.

	// name replacement necessary to avoid ctor naming problem
	static FEX void FUBI_RENAME( ReportF )( ReportSys::Context* context, const char* format, ... );
	static FEX void FUBI_RENAME( Report  )( ReportSys::Context* context, const char* msg );
	static FEX void FUBI_RENAME( ReportF )( const char* contextName, const char* format, ... );
	static FEX void FUBI_RENAME( Report  )( const char* contextName, const char* msg );

	static FEX void GenericF   ( const char* format, ... );
	static FEX void Generic    ( const char* msg );

	static FEX void PerfF      ( const char* format, ... );
	static FEX void Perf       ( const char* msg );

	static FEX void PerfLogF   ( const char* format, ... );
	static FEX void PerfLog    ( const char* msg );

	static FEX void TestLogF   ( const char* format, ... );
	static FEX void TestLog    ( const char* msg );

	static FEX void MessageBoxF( const char* format, ... );
	static FEX void MessageBox ( const char* msg );

	static FEX void ErrorBoxF  ( const char* format, ... );
	static FEX void ErrorBox   ( const char* msg );

	static FEX void SScreenF   ( const char* format, ... );
	static FEX void SScreen    ( const char* msg );
	static FEX void SScreenF   ( DWORD machineId, const char* format, ... );
	static FEX void SScreen    ( DWORD machineId, const char* msg );
	static FEX void RCScreen   ( DWORD machineId, const char* msg );
	static FEX void ScreenF    ( const char* format, ... );
	static FEX void Screen     ( const char* msg );

	static FEX void DebuggerF  ( const char* format, ... );
	static FEX void Debugger   ( const char* msg );

	static FEX void WarningF   ( const char* format, ... );
	static FEX void Warning    ( const char* msg );

	static FEX void ErrorF     ( const char* format, ... );
	static FEX void Error      ( const char* msg );

	static FEX void FatalF     ( const char* format, ... );
	static FEX void Fatal      ( const char* msg );
	static FEX void Fatal      ( void );

// Context control.

	static FEX void Enable   ( const char* context );
	static FEX void Disable  ( const char* context );
	static FEX void Toggle   ( const char* context );
	static FEX bool IsEnabled( const char* context );

	static FEX void AddNewSinkToContext( const char* context, const char* name, const char* sinkFactory, const char* sinkParams );
	static FEX void AddNewSinkToContext( const char* context, const char* name, const char* sinkFactory );
	static FEX void AddNewSinkToContext( const char* context, const char* sinkFactory );
	static FEX void AddNewSinkToSink   ( const char* sink, const char* name, const char* sinkFactory );
	static FEX void AddNewSinkToSink   ( const char* sink, const char* sinkFactory );

// Misc.

	static FEX const char* TranslateMsg( const char* message );
	static FEX const char* Translate( const char* message );

	static FEX void DumpFile( const char* fileName );
};

//////////////////////////////////////////////////////////////////////////////
// maker declarations

FUBI_EXPORT vector_3& MakeVector( float x, float y, float z );
FUBI_DOC            ( MakeVector,      "x,"     "y,"     "z",
					 "Returns a vector composed of x, y, and z. This is a "
					 "temporary and will change the next time this function is "
					 "called so do not store the results." );

FUBI_EXPORT const vector_3&
					  GetZeroVector( void );
FUBI_DOC            ( GetZeroVector, "",
					 "Returns a vector 0, 0, 0 - do not change this value." );

FUBI_EXPORT Quat&     MakeQuat( float x, float y, float z, float w );
FUBI_DOC            ( MakeQuat,      "x,"     "y,"     "z,"     "w",
					 "Returns a Quat composed of x, y, z, and w. This is a "
					 "temporary and will change the next time this function is "
					 "called so do not store the results." );

FUBI_EXPORT const char*
					  MakeFourCcString( int fourCc );
FUBI_DOC			( MakeFourCcString$0,  "fourCc",
					 "Returns the string version of the given fourCc." );

FUBI_EXPORT const char*
					  MakeFourCcString( int fourCc, bool reverse );
FUBI_DOC			( MakeFourCcString$1,  "fourCc,"    "reverse",
					 "Returns the string version of the given fourCc, "
					 "optionally reversing the byte order." );

FUBI_EXPORT DWORD     MakeColor  ( float red, float green, float blue, float alpha );
FUBI_EXPORT DWORD     MakeColor  ( float red, float green, float blue );
FUBI_EXPORT DWORD     MakeColor  ( const vector_3& color, float alpha );
FUBI_EXPORT DWORD     MakeColor  ( const vector_3& color );
FUBI_EXPORT gpstring& MakeStringF( const char* format, ... );

//////////////////////////////////////////////////////////////////////////////
// decomposer declarations

FUBI_EXPORT float GetRed  ( DWORD intColor );
FUBI_DOC		( GetRed,        "intColor",
				 "Returns the red level of a color based on its integer version." );

FUBI_EXPORT float GetGreen( DWORD intColor );
FUBI_DOC		( GetGreen,      "intColor",
				 "Returns the green level of a color based on its integer version." );

FUBI_EXPORT float GetBlue ( DWORD intColor );
FUBI_DOC		( GetBlue,       "intColor",
				 "Returns the blue level of a color based on its integer version." );

FUBI_EXPORT float GetAlpha( DWORD intColor );
FUBI_DOC		( GetAlpha,      "intColor",
				 "Returns the alpha level of a color based on its integer version." );

//////////////////////////////////////////////////////////////////////////////
// renderer support declarations

FUBI_EXPORT void StartActiveTextureLogging();
FUBI_DOC       ( StartActiveTextureLogging, "",
				"Starts an internal log of every texture used to render." );

FUBI_EXPORT void StopActiveTextureLogging( const char* fileName );
FUBI_DOC       ( StopActiveTextureLogging,			  "fileName",
				"Stops the internal texture log and dumps it to the given file." );

FUBI_EXPORT void SetTextureLogSaturation( const char* fileName, float saturation );
FUBI_DOC       ( SetTextureLogSaturation,			 "fileName,"     "saturation",
				"Takes a saved texture log and scales the saturation level of each texture in that log by the given scale (0.0 - 1.0)." );

FUBI_EXPORT void RevertSaturation();
FUBI_DOC       ( RevertSaturation, "",
				"Reverts all saturated textures back to their original state." );

//////////////////////////////////////////////////////////////////////////////
// class String declaration

class String
{
	static FEX bool            IsEmpty        ( const char* str );
	static FEX const gpstring& GetFileNameOnly( const gpstring& str );
	static FEX gpstring&       Append         ( gpstring& str, const char* extra );
	static FEX gpstring&       AppendF        ( gpstring& str, const char* format, ... );
	static FEX gpstring&       Assign         ( gpstring& str, const char* extra );
	static FEX gpstring&       AssignF        ( gpstring& str, const char* format, ... );
	static FEX gpstring&       Left           ( gpstring& str, int len );
	static FEX gpstring&       Mid            ( gpstring& str, int pos, int len );
	static FEX gpstring&       Mid            ( gpstring& str, int pos );
	static FEX gpstring&       Right          ( gpstring& str, int len );
	static FEX bool            SameNoCase     ( const char* l, const char* r );
	static FEX bool            SameNoCase     ( const char* l, const char* r, int len );
	static FEX bool            SameWithCase   ( const char* l, const char* r );
	static FEX bool            SameWithCase   ( const char* l, const char* r, int len );

	static FEX int       GetNumDelimitedValues( const char* in, char delimiter );
	static FEX int       GetNumDelimitedValues( const char* in )								{  return ( GetNumDelimitedValues( in, ',' ) );  }
	static FEX gpstring& GetDelimitedString   ( const char* in, int index, char delimiter, const char* defValue );
	static FEX gpstring& GetDelimitedString   ( const char* in, int index, char delimiter )		{  return ( GetDelimitedString( in, index, delimiter, "" ) );  }
	static FEX gpstring& GetDelimitedString   ( const char* in, int index )						{  return ( GetDelimitedString( in, index, ',' ) );  }
	static FEX bool      GetDelimitedBool     ( const char* in, int index, char delimiter, bool defValue );
	static FEX bool      GetDelimitedBool     ( const char* in, int index, char delimiter )		{  return ( GetDelimitedBool( in, index, delimiter, false ) );  }
	static FEX bool      GetDelimitedBool     ( const char* in, int index )						{  return ( GetDelimitedBool( in, index, ',' ) );  }
	static FEX int       GetDelimitedInt      ( const char* in, int index, char delimiter, int defValue );
	static FEX int       GetDelimitedInt      ( const char* in, int index, char delimiter )		{  return ( GetDelimitedInt( in, index, delimiter, 0 ) );  }
	static FEX int       GetDelimitedInt      ( const char* in, int index )						{  return ( GetDelimitedInt( in, index, ',' ) );  }
	static FEX float     GetDelimitedFloat    ( const char* in, int index, char delimiter, float defValue );
	static FEX float     GetDelimitedFloat    ( const char* in, int index, char delimiter )		{  return ( GetDelimitedFloat( in, index, delimiter, 0 ) );  }
	static FEX float     GetDelimitedFloat    ( const char* in, int index )						{  return ( GetDelimitedFloat( in, index, ',' ) );  }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __SKRITSUPPORT_H

//////////////////////////////////////////////////////////////////////////////
