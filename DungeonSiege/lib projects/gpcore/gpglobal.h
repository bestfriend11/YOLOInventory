//////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
// File     :  GPGlobal.h
// Author(s):  <Original Author Unknown>, Scott Bilas
//
// Summary  :  GPGlobal is a repository for various global functions and
//             other useful frequently-used utilities.
//
// Copyright © 1999 Gas Powered Games.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef GPGLOBAL_H
#define GPGLOBAL_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// generic macros

// add to a class definition to make the object uncopyable
#define SET_NO_COPYING(type)            \
	private:                            \
		type(const type&);              \
		type& operator = (const type&)

// add to a class definition to make the object uncopyable - easier for templates
#define SET_NO_COPYINGX(type)           \
	private:                            \
		type(const ThisType&);          \
		type& operator = (const ThisType&)

// "assert" expression is true at compile-time. this macro will cause errors
// C2466 and C2086 when it is used on a zero expression. the errors are only
// there to point you at the failed assertion, and mean nothing by themselves.
// use the COMPILER_ASSERT when you have conditions that must be true and are
// built from constants. for example, COMPILER_ASSERT( sizeof( int ) == 4 )
// would be useful for verifying platform.
#define COMPILER_ASSERT(x) typedef int $_This_Is_A_Compile_Time_Assert_$_For_Docs_See_GPGLOBAL_H[!!(x)]

// force the typedef to true initially so we get the second error (C2086) that
// should point the engineer at this file for the docs.
COMPILER_ASSERT( 1 );

// force breakpoint without function call (i.e. DebugBreak goes on top of the stack)
#define BREAKPOINT() _asm { int 3 }

// it's "my"! use this to tag variables (purely for notation) that are "owned"
// and must have delete called on 'em.
#define my

// found this neat hack in CUJ online. use it to permit allocating variables
// in a scope without having to name them.
#define CONCATENATE_DIRECT( s1, s2 ) s1##s2
#define CONCATENATE( s1, s2 )        CONCATENATE_DIRECT( s1, s2 )
#define ANONYMOUS_VARIABLE(str)      CONCATENATE( str, __LINE__ )

// This one's from Bruce Dawson -sb
//
// This disgusting macro forces VisualC++ to use the ANSI C++ rules
// for the scoping of variables declared inside for statements.
// Trust me - it works. Using this evil macro makes it much easier
// to write portable code. This will be unnecessary starting with
// version 7.0 of VC++
#define	for2 if (0) ; else for

// avoids the normal two-step va_list + va_start() for vprintf()
#ifdef _M_IX86
#define va_args( param )  ( (va_list)&(param) + _INTSIZEOF( param ) )
#endif // _M_IX86

// for lazy programmers
#define rcast reinterpret_cast
#define dcast dynamic_cast
#define scast static_cast
#define ccast const_cast
#define DECL_DLL_EXPORT __declspec ( dllexport )
#define DECL_DLL_IMPORT __declspec ( dllimport )
#define DECL_SELECT_ANY __declspec ( selectany )
#define DECL_NAKED      __declspec ( naked )

// 3dsmax uses delayed DLL loading which requires
// Dynamic TLS (Thread Local Storage). To try and
// work around things I am simply ignoring DECL_THREAD
// and hoping that nothing in the exporter actually
// ends up re-entering the code
// --biddle
#if GP_SIEGEMAX
#define DECL_THREAD
#else
#define DECL_THREAD     __declspec ( thread )
#endif

// form: dstVar = force_cast <dstType> (srcVar);
//
// ex: float f;
//     int i = force_cast <int> (f);

template <typename TO, typename FROM>
inline TO& force_cast( FROM& data )
{
	COMPILER_ASSERT( sizeof( TO ) == sizeof( FROM ) );
	return ( *(TO*)&data );
}

template <typename TO, typename FROM>
inline void force_cast_copy( TO& to, FROM& from )
{
	to = force_cast <TO> ( from );
}

//////////////////////////////////////////////////////////////////////////////
// build-specific macros

// Wrap this around debug-only code
#if GP_DEBUG
#   define GPDEBUG_ONLY( f ) f
#else
#   define GPDEBUG_ONLY( f )
#endif

// Wrap this around dev-only code (non-retail)
#if GP_RETAIL
#   define GPDEV_ONLY( f )
#   define GPDEV_PARAM( f )
#else
#   define GPDEV_ONLY( f ) f
#   define GPDEV_PARAM( f ) , f
#endif

// Wrap this around retail-only code
#if GP_RETAIL
#   define GPRETAIL_ONLY( f ) f
#else
#   define GPRETAIL_ONLY( f )
#endif

#define EMPTYSTATEMENT typedef int EmptyStatement_

//////////////////////////////////////////////////////////////////////////////
// compile safety macros

// GP_ImplementLater$$$ - code will run ok but there's stuff missing that
//                        is eventually needed for the retail build.
//
// GP_Unimplemented$$$  - code will NOT run ok if you hit this stuff.

#if GP_RETAIL

	// force a compile-time failure - do not allow a build if something is
	// still unimplemented (use release mode instead if it's "ok for now")

#	if GP_FULLY_IMPLEMENTED
#   define GP_ImplementLater$$$()  COMPILER_ASSERT( 0 )
#   define GP_Unimplemented$$$()  COMPILER_ASSERT( 0 )
#	else // GP_FULLY_IMPLEMENTED
#   define GP_ImplementLater$$$()
#   define GP_Unimplemented$$$()
#	endif // GP_FULLY_IMPLEMENTED

#else // GP_DEBUG

#   define GP_ImplementLater$$$()
#   define GP_Unimplemented$$$()  gpassert_raw( 0, "Unimplemented code!", "Unimplemented code!" )

#endif // GP_DEBUG

//////////////////////////////////////////////////////////////////////////////
// array access macros

// from C/C++UJ Sep 98 pg 88
//
// $ note: this code does not work with VC++ 5. the compiler will work
//   with class templates that use constants as template parameters, but i
//   guess it doesn't work with function templates. if fixed in a later
//   version, uncomment this code and replace the macros.
//
// template <typename ELEMENT, size_t COUNT>
// inline size_t ELEMENT_SIZE(ELEMENT (& const)[COUNT])
//   {  return (sizeof(ELEMENT));  }
// template <typename ELEMENT, size_t COUNT>
// inline size_t ELEMENT_COUNT(ELEMENT (& const)[COUNT])
//   {  return (COUNT);  }
// template <typename ELEMENT, size_t COUNT>
// inline T& ELEMENT_LAST(ELEMENT (& array)[COUNT])
//   {  COMPILER_ASSERT(COUNT > 0);  return (array[COUNT - 1]);  }
// template <typename ELEMENT, size_t COUNT>
// inline const T& ELEMENT_LAST(ELEMENT (& const array)[COUNT])
//   {  COMPILER_ASSERT(COUNT > 0);  return (array[COUNT - 1]);  }
// template <typename ELEMENT, size_t COUNT>
// inline T& ARRAY_END(ELEMENT (& array)[COUNT])
//   {  COMPILER_ASSERT(COUNT > 0);  return (array + COUNT);  }
// template <typename ELEMENT, size_t COUNT>
// inline const T& ARRAY_END(ELEMENT (& const array)[COUNT])
//   {  COMPILER_ASSERT(COUNT > 0);  return (array + COUNT);  }

// based on the one-dimensional array, get the size of an element
#define ELEMENT_SIZE( x ) (sizeof( (x)[ 0 ] ))

// based on the one-dimensional array, get the number of elements.
#define ELEMENT_COUNT( x ) (sizeof( x ) / ELEMENT_SIZE( x ))

// get at the last element of the one-dimensional array
#define ELEMENT_LAST( x ) ((x)[ ELEMENT_COUNT( x ) - 1 ])

// get address at the one-past-last element (STL's "end") of the one-dimensional array
#define ARRAY_END( x ) ((x) + ELEMENT_COUNT( x ))

//////////////////////////////////////////////////////////////////////////////
// forward declarations

namespace std
{
	template <typename T> class allocator;
	template <typename T, typename A = allocator <T> > class vector;
}

namespace ReportSys
{
	struct ContextRef;
}

enum eSystemProperty;

//////////////////////////////////////////////////////////////////////////////
// functions

// COM

	// Initializes COM and maintains status
	bool GPCoInitialize();

// Checksums.

	// standard CRC-16 function (adapted from CRC32)
	UINT16 GetCRC16( UINT16 seed, const void* buf, UINT sizeInBytes );
	inline UINT16 GetCRC16( const void* buf, UINT sizeInBytes )
		{  return ( GetCRC16( 0, buf, sizeInBytes ) );  }

	// adds more info to CRC
	inline void AddCRC16( UINT16& seed, const void* buf, UINT sizeInBytes )
		{  seed = GetCRC16( seed, buf, sizeInBytes );  }

	// adds more info to CRC
	template <typename T>
	inline void AddCRC16( UINT16& seed, const T& obj )
		{  AddCRC16( seed, &obj, sizeof( T ) );  }

	// gets a fast, consistent 32-bit checksum for an arbitrary data buffer (copied from zlib)
	UINT32 GetAdler32( const void* buf, UINT sizeInBytes );
	// standard CRC-32 function (copied from zlib)
	UINT32 GetCRC32( UINT32 seed, const void* buf, UINT sizeInBytes );
	inline UINT32 GetCRC32( const void* buf, UINT sizeInBytes )
		{  return ( GetCRC32( 0, buf, sizeInBytes ) );  }

	// adds more info to CRC
	inline void AddCRC32( UINT32& seed, const void* buf, UINT sizeInBytes )
		{  seed = GetCRC32( seed, buf, sizeInBytes );  }

	// adds more info to CRC
	template <typename T>
	inline void AddCRC32( UINT32& seed, const T& obj )
		{  AddCRC32( seed, &obj, sizeof( T ) );  }

// UUencoding/decoding.

	// returns max number of bytes that can be encoded at a time
	int GetUuMaxInSize( void );

	// returns number of characters required in 'out' buffer
	int GetUuEncodeSize( int bytes );

	// uuencode the input line, up to GetUuMaxInSize() bytes at a time, with
	// the results stored in 'out', which will need to be at least
	// GetUuEncodeSize( inBytes ) in size (including null term).
	int UuEncodeLine( const void* in, int inBytes, char* out );

	// uuencode into a string
	void UuEncode( const void* in, int inBytes, gpstring& out );

	// uudecode the line to 'out'
	int UuDecodeLine( const char* in, void* out, const char** next = NULL );

	// uudecode from a string into a buffer, returns -1 if fail, size of decoded otherwise
	int UuDecode( const char* in, void* out, int outSize );

// Encryption/decryption.

	// $ note: the TEA algorithm works a qword at a time, so be sure to zero-
	//   expand the input and output buffers to be qword-aligned.

	// in-place encryption, give it some random dwords as a key or none for default
	void TeaEncrypt( void* data, size_t length );
	void TeaEncrypt( void* data, size_t length, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );
	void TeaEncrypt( gpstring& str );
	void TeaEncrypt( gpstring& str, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );
	void TeaEncrypt( gpstring& out, const gpwstring& in );
	void TeaEncrypt( gpstring& out, const gpwstring& in, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );

	// in-place decryption, give it the same key from encryption func
	void TeaDecrypt( void* data, size_t length );
	void TeaDecrypt( void* data, size_t length, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );
	void TeaDecrypt( gpstring& str );
	void TeaDecrypt( gpstring& str, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );
	void TeaDecrypt( gpwstring& out, const gpstring& in );
	void TeaDecrypt( gpwstring& out, const gpstring& in, DWORD k0, DWORD k1, DWORD k2, DWORD k3 );

// Memory utilities.

	// checks to see if this DWORD is part of invalid memory (NT calls it bad food).
	//  note that our SmartHeap startup code sets SH to use the same codes as VC++
	//  does for our convenience.
	inline bool IsMemoryBadFood( DWORD d )
	{
		return (    ( d == 0xDDDDDDDD )     // crt: dead land (deleted objects)
				 || ( d == 0xCDCDCDCD )     // crt: clean land (new, uninitialized objects)
				 || ( d == 0xFDFDFDFD )     // crt: no man's land (off the end of memory block)
				 || ( d == 0xCCCCCCCC )     // vc++: stack objects apparently initialized with this...?
				 || ( d == 0xFEEEFEEE )     // ? nt internal ?
				 || ( d == 0xBAADF00D ) );  // winnt: bad food (nt internal "not your memory" filler)
	}

	// check for NULL or clobbered pointer
	template <typename T>
	inline bool IsPointerValid( T ptr )
	{
		return ( ptr && !IsMemoryBadFood( (DWORD)ptr ) );
	}

	// use to zero out members of a class - pass in the first and last members of
	//  the set to clear.
	template <typename T1, typename T2>
	inline void ZeroMembers( T1& t1, T2& t2 )
		{  memset( &t1, 0, reinterpret_cast <size_t> ( &t2 ) - reinterpret_cast <size_t> ( &t1 ) + sizeof( t2 ) );  }

	template <typename T>
	inline void ZeroObject( T& object )
		{  memset( &object, 0, sizeof( T ) );  }

	template <typename T>
	inline void CopyObject( T& dst, const T& src )
		{  memcpy( &dst, &src, sizeof( T ) );  }

	template <typename T>
	inline int CompareObjects( const T& l, const T& r )
		{  return ( memcmp( &l, &r, sizeof( T ) ) );  }

	// walk the memory and touch each page - this will fault anything that is on
	// disk (paged out if not used recently or never paged in if from a memory
	// mapped file) and bring it into memory. this takes care of all win95 weird
	// paging behavior. be sure to set the "fromPageFile" parameter so the
	// system knows how to deal with it. if you're touching system memory that's
	// been new'd or whatever, it's probably in the pagefile. if you're touching
	// memory that's from a memory-mapped file then set it to false.
	void MemTouchPages( const void* base, int size, bool fromPageFile );

// Byte swapping.

	inline void ByteSwap( DWORD& d )
	{
		__asm
		{
			mov		eax, dword ptr [d]
			mov		ebx, dword ptr [eax]
			bswap	ebx
			mov		dword ptr [eax], ebx
		}
	}

	inline DWORD GetByteSwap( DWORD d )
	{
		ByteSwap( d );
		return ( d );
	}

	inline void ByteSwap( WORD& w )
	{
		BYTE* b = (BYTE*)&w;
		b[ 0 ] ^= b[ 1 ] ^= b[ 0 ] ^= b[ 1 ];
	}

	inline WORD GetByteSwap( WORD w )
	{
		ByteSwap( w );
		return ( w );
	}

// Time.

	// gets current system time
	double GetSystemSeconds( void );

	// gets adjusted "global" time - this will be adjusted for save game etc.
	double GetGlobalSeconds( void );

	// adjust global time offset here - only call this from save game functions
	void   SetGlobalSecondsAdjust( double adjust );
	double GetGlobalSecondsAdjust( void );

	// converts time into h/m/s (pass in valid seconds)
	void ConvertTime( int& hours, int& minutes, float& seconds );

	// this is a more accurate version of Sleep()
	void AccurateSleep( float seconds );

//////////////////////////////////////////////////////////////////////////////
// random number generators

	// $ algorithms:
	//
	// MT = Mersenne Twister, a fast, accurate, high-res random number (see .cpp
	//      for docs). its only real disadvantage is that the seed cannot be
	//      extracted from the current state of the algorithm. the seed function
	//      is also kind of expensive wrt cpu time. ultimately this means that
	//      the Twisters is difficult to sync on separate machines.
	//
	// NR = From _Numerical Recipes in C_ page 284. Only meant to be used for
	//      stuff that needs to be "kind of" random. Pretty good most of the
	//      time, very fast, and can be re-seeded instantly.

// Helper base.

	template <typename T>
	class RandomGeneratorBase
	{
	public:
		// result is any possible dword
		DWORD RandomDword( void )						{  return ( ((T*)this)->RandomDword() );  }

		// result is any possible byte
		BYTE RandomByte( void )							{  return ( (BYTE)(RandomDword()) );  }

		// result is any possible int
		int RandomInt( void )							{  return ( (int)(RandomDword()) );  }

		// result is in range [0,1]
		float RandomFloat( void )						{  return ( (float)(RandomDword() * 2.3283064370807974e-10f) );  }

		// result is in range [0,1]
		double RandomDouble( void )						{  return ( (double)(RandomDword() * 2.3283064370807974e-10) );  }

		// result is in range [0.0,Max]
		float Random( float maximum )					{  return ( (float)(maximum * RandomFloat()) );  }

		// result is in range [Min,Max]
		float Random( float minRange, float maxRange )	{  return ( minRange + Random( maxRange - minRange ) );  }

		// result is in range [0,Max]
		int Random( int maximum )						{  gpassert( maximum >= 0 );  return ( RandomDword() % (maximum + 1) ); }

		// result is in range [0,Max]
		int Random( UINT32 maximum )					{  gpassert( maximum >= 0 );  return ( RandomDword() % (maximum + 1) ); }

		// result is in range [Min,Max]
		int Random( int minRange, int maxRange )		{  gpassert( minRange <= maxRange );  return ( minRange + Random( maxRange - minRange ) );  }

		// result is in range [Min,Max]
		int Random( UINT32 minRange, UINT32 maxRange )	{  gpassert( minRange <= maxRange );  return ( minRange + Random( maxRange - minRange ) );  }
	};

// Mersenne Twister.

	class RandomGeneratorMT : public RandomGeneratorBase <RandomGeneratorMT>
	{
	public:
		RandomGeneratorMT( void )				{  ::ZeroObject( m_State );  m_Next = NULL;  m_Left = -1;  Randomize();  }

		void  SetSeed    ( DWORD seed );		// you can seed with any uint32, but the best are odd numbers
		void  Randomize  ( void )				{  SetSeed( (GetTickCount() & ~1) + 1 );  }
		DWORD RandomDword( void );

	private:

		enum
		{
			N = 624,							// length of state vector
			M = 397,							// a period parameter
			K = 0x9908B0DFU,					// a magic constant
		};

		DWORD  m_State[ N + 1 ];				// state vector + 1 extra to not violate ANSI C
		DWORD* m_Next;							// next random value is computed from here
		int    m_Left;							// can *m_Next++ this many times before reloading
	};

	// get the global random generator
	RandomGeneratorMT& GetGlobalRng( void );

	// use the global rng
	inline float  Random      ( float maximum )						{  return ( GetGlobalRng().Random( maximum ) );  }
	inline float  Random      ( float minRange, float maxRange )	{  return ( GetGlobalRng().Random( minRange, maxRange ) );  }
	inline int    Random      ( int maximum )						{  return ( GetGlobalRng().Random( maximum ) );  }
	inline int    Random      ( UINT32 maximum )					{  return ( GetGlobalRng().Random( maximum ) );  }
	inline int    Random      ( int minRange, int maxRange )		{  return ( GetGlobalRng().Random( minRange, maxRange ) );  }
	inline int    Random      ( UINT32 minRange, UINT32 maxRange )	{  return ( GetGlobalRng().Random( minRange, maxRange ) );  }
	inline BYTE   RandomByte  ( void )								{  return ( GetGlobalRng().RandomByte() );  }
	inline int    RandomInt   ( void )								{  return ( GetGlobalRng().RandomInt() );  }
	inline DWORD  RandomDword ( void )								{  return ( GetGlobalRng().RandomDword() );  }
	inline float  RandomFloat ( void )								{  return ( GetGlobalRng().RandomFloat() );  }
	inline double RandomDouble( void )								{  return ( GetGlobalRng().RandomDouble() );  }
	inline void   Randomize   ( void )								{  GetGlobalRng().Randomize();  }

// From _Numerical Recipes in C_ page 284.

	class RandomGeneratorNR : public RandomGeneratorBase <RandomGeneratorNR>
	{
	public:
		RandomGeneratorNR( void )				{  Randomize();  }

		void  SetSeed    ( DWORD seed )			{  m_Random = seed;  }
		DWORD GetSeed    ( void ) const			{  return ( m_Random );  }
		void  Randomize  ( void )				{  SetSeed( ::RandomDword() );  }
		DWORD RandomDword( void );

	private:
		DWORD m_Random;
	};

//////////////////////////////////////////////////////////////////////////////
// special "at final exit" functions

// functions added here will execute in FIFO order after all c++ destruction
// and atexit() calls are complete. note that atexit() executes in FILO order.

typedef void (__cdecl *VoidVoidProc)( void );
void AddAtFinalExitProc( VoidVoidProc proc );
bool IsInFinalExit( void );

// this will expose a GetTYPE() function that you can #define away
#define DEFINE_POST_GLOBAL( TYPE, NAME )			\
	static TYPE* g##NAME##Ptr = NULL;				\
	void Destroy##NAME( void )						\
	{												\
		Delete( g##NAME##Ptr );						\
	}												\
	TYPE& Get##NAME( void )							\
	{												\
		if ( g##NAME##Ptr == NULL )					\
		{											\
			g##NAME##Ptr = new TYPE;				\
			AddAtFinalExitProc( Destroy##NAME );	\
		}											\
		return ( *g##NAME##Ptr );					\
	}

// example:
//
// DEFINE_POST_GLOBAL( MyClass, Object )
// #define gObject GetObject()

//////////////////////////////////////////////////////////////////////////////
// class SeException declaration

// $ note: the exception translator must be installed per thread.

class SeException
{
public:
	typedef std::vector <ULONG> AddressColl;

	SeException( const EXCEPTION_POINTERS& info, const DWORD* lastError );
	SeException( void );

	unsigned int GetCode( void ) const
		{  return ( m_Information.ExceptionRecord->ExceptionCode );  }
	const EXCEPTION_POINTERS& GetInformation( void ) const
		{  return ( m_Information );  }
	DWORD GetLastError( void ) const
		{  gpassert( HasLastError() );  return ( m_LastError );  }
	bool HasLastError( void ) const
		{  return ( m_HasLastError );  }

	void CopyLocal        ( const EXCEPTION_POINTERS& info, const DWORD* lastError );
	void CopyLocal        ( const EXCEPTION_POINTERS& info, DWORD lastError );
	void CopyLocal        ( void );
	void AcquireStackTrace( HANDLE hthread );

	void Dump( ReportSys::ContextRef ctx, bool allThreads = false ) const;
	void Dump( bool allThreads = false ) const;

	static void DumpException    ( const EXCEPTION_RECORD& xrec, ReportSys::ContextRef ctx );
	static void DumpContext      ( HANDLE hthread, DWORD threadId, const CONTEXT& regs, ReportSys::ContextRef ctx );
	static void DumpContext      ( HANDLE hthread, DWORD threadId, const CONTEXT& regs, const AddressColl& addresses, ReportSys::ContextRef ctx );
	static void DumpProcessTraits( ReportSys::ContextRef ctx );
	static void DumpThreadTraits ( HANDLE hthread, const DWORD* lastError, ReportSys::ContextRef ctx );

private:
	static void PrivateDumpContext( HANDLE hthread, DWORD threadId, const CONTEXT& regs, const AddressColl* addresses, ReportSys::ContextRef ctx );

	EXCEPTION_POINTERS m_Information;
	EXCEPTION_RECORD   m_LocalRecord;
	CONTEXT            m_LocalContext;
	DWORD              m_LastError;
	bool               m_HasLastError;
	AddressColl        m_StackTrace;
};

// API functions.

	// helper
	const char* ExceptionCodeToString( DWORD code );

	// do the dialog
	int DoModalExceptionDlg( const EXCEPTION_POINTERS& xinfo, const char* headerMsg = NULL, HWND parent = NULL );

	// use for __except clause (pass in GetExceptionInformation())
	LONG WINAPI ExceptionDlgFilter( const EXCEPTION_POINTERS* xinfo, DWORD lastError, const char* headerMsg = NULL, HWND parent = NULL );

// Global exception handler filters.

	// registration
	void RegisterGlobalExceptionFilter  ( LPTOP_LEVEL_EXCEPTION_FILTER filter );
	void UnregisterGlobalExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter );

	// call this from your __except clause
	LONG CALLBACK GlobalExceptionFilter(
			EXCEPTION_POINTERS* xinfo,								// exception info
			LONG fatalReturn = EXCEPTION_EXECUTE_HANDLER,			// if someone throws a fatal, use this return
			LONG defaultReturn = EXCEPTION_CONTINUE_SEARCH,			// if nobody handles anything, use this return
			bool execDefaultHandler = false,						// use the default handler to notify the user?
			SeException* xtrace = NULL );							// optionally store exception info here while stack is still valid

	// returns # SEH's caught so far
	const DWORD& GlobalGetUnfilteredHandledExceptionCount( void );

	// special constant used for fatals
	const int EXCEPTION_GPG_FATAL = 'Fatl';

//////////////////////////////////////////////////////////////////////////////
// power-of-two functions

// return TRUE if the number is an even power of 2 we don't need a loop for
//  this, just some simple boolean math. note that 0 is not a power of 2.
template <typename T>
inline BOOL IsPower2( T n )
{
	return n && ((n & (n-1)) == 0);
}

#pragma warning ( push )
#pragma warning ( disable : 4035 )

// returns the number of shifts required to get the highest set bit into slot 0.
inline int GetShift( int n )
{
	__asm
	{
		mov		edx, [n]
		bsr		eax, edx
	}
}

// return the number of shifts required to get the lowest bit into slot 0.
inline int GetZeroShift( int n )
{
	__asm
	{
		mov		edx, [n]
		bsf		eax, edx
	}
}

#pragma warning ( pop )

// return the next power of two that contains the passed in number
template <typename T>
inline T MakePower2Up( T n )
{
	if ( IsPower2( n ) )
	{
		return ( n );
	}
	return ( T( (n == 0) ? 1 : (2 << GetShift( n )) ) );
}

// return the previous power of two that contains the passed in number - this
//  is useful for finding the most significant bit
template <typename T>
inline T MakePower2Down( T n )
{
	if ( IsPower2( n ) )
	{
		return ( n );
	}
	return ( T( (n == 0) ? 0 : (1 << GetShift(n)) ) );
}

//////////////////////////////////////////////////////////////////////////////
// alignment functions

// note that "alignment" should be a power of 2 for these to work

// variable-alignment
template <typename T> inline bool IsAligned   ( T  value, size_t alignment)  {  return ( !((unsigned int)value & (alignment - 1)) );  }
template <typename T> inline T    GetAlignUp  ( T  value, size_t alignment)  {  return ( (T)(((unsigned int)value + alignment - 1) & ~(alignment - 1)) );  }
template <typename T> inline T    GetAlignDown( T  value, size_t alignment)  {  return ( (T)((unsigned int)value & ~(alignment - 1)) );  }
template <typename T> inline void AlignUp     ( T& value, size_t alignment)  {  value = (T)(((unsigned int)value + alignment - 1) & ~(alignment - 1));  }
template <typename T> inline void AlignDown   ( T& value, size_t alignment)  {  value = (T)((unsigned int)value & ~(alignment - 1));  }

// QWORD-alignment functions
template <typename T> inline bool IsQwordAligned   ( T  value )  {  return ( !((unsigned int)value & 7) );  }
template <typename T> inline T    GetQwordAlignUp  ( T  value )  {  return ( (T)(((unsigned int)value + 7) & ~7) );  }
template <typename T> inline T    GetQwordAlignDown( T  value )  {  return ( (T)((unsigned int)value & ~7) );  }
template <typename T> inline void QwordAlignUp     ( T& value )  {  value = (T)(((unsigned int)value + 7) & ~7);  }
template <typename T> inline void QwordAlignDown   ( T& value )  {  value = (T)((unsigned int)value & ~7);  }
template <typename T> inline int  GetPadToQword    ( T  value )  {  return ( (int)(GetQwordAlignUp( value ) - value) );  }

// DWORD-alignment functions
template <typename T> inline bool IsDwordAligned   ( T  value )  {  return ( !((unsigned int)value & 3) );  }
template <typename T> inline T    GetDwordAlignUp  ( T  value )  {  return ( (T)(((unsigned int)value + 3) & ~3) );  }
template <typename T> inline T    GetDwordAlignDown( T  value )  {  return ( (T)((unsigned int)value & ~3) );  }
template <typename T> inline void DwordAlignUp     ( T& value )  {  value = (T)(((unsigned int)value + 3) & ~3);  }
template <typename T> inline void DwordAlignDown   ( T& value )  {  value = (T)((unsigned int)value & ~3);  }
template <typename T> inline int  GetPadToDword    ( T  value )  {  return ( (int)(GetDwordAlignUp( value ) - value) );  }

// WORD-alignment functions
template <typename T> inline bool IsWordAligned   ( T  value )  {  return ( !((unsigned int)value & 1) );  }
template <typename T> inline T    GetWordAlignUp  ( T  value )  {  return ( (T)(((unsigned int)value + 1) & ~1) );  }
template <typename T> inline T    GetWordAlignDown( T  value )  {  return ( (T)((unsigned int)value & ~1) );  }
template <typename T> inline void WordAlignUp     ( T& value )  {  value = (T)(((unsigned int)value + 1) & ~1);  }
template <typename T> inline void WordAlignDown   ( T& value )  {  value = (T)((unsigned int))value & ~1);  }
template <typename T> inline int  GetPadToWord    ( T  value )  {  return ( (int)(GetWordAlignUp( value ) - value) );  }

//////////////////////////////////////////////////////////////////////////////
// enum macros

// surround an enum "constant array" to set some basic default constants
//  (first, last, end, count). SET_BEGIN_ENUM needs a prefix (usually the
//  enum name is fine) and a starting number. SET_END_ENUM just needs the
//  prefix.
#define SET_BEGIN_ENUM( x, num )        \
	x##BEFORE_BEGIN = num - 1
#define SET_END_ENUM( x )               \
	x##END,                             \
	x##BEGIN = x##BEFORE_BEGIN + 1,     \
	x##LAST  = x##END - 1,              \
	x##COUNT = x##END - x##BEGIN

// this macro allows you to use a typesafe bitflag parameter as a strict enum
// rather than simple an int or dword. the operators need to be defined so you
// can combine and test the component flags.
#define MAKE_ENUM_BIT_OPERATORS( x ) \
	inline x operator | ( x a, x b ) \
		{  return ( scast <x> ( scast <int> ( a ) | scast <int> ( b ) ) ); } \
	inline x operator & ( x a, x b ) \
		{  return ( scast <x> ( scast <int> ( a ) & scast <int> ( b ) ) ); } \
	inline x operator |= ( x& a, x b ) \
		{  return ( a = a | b ); } \
	inline x operator &= ( x& a, x b ) \
		{  return ( a = a & b ); } \
	inline x NOT( x a ) /* stupid compiler won't properly create an operator ~() */ \
		{  return ( scast <x> ( ~(scast <int> ( a )) ) ); }

// this macro allows you to use math operations on enums and still be
// relatively typesafe. add more as you find the need.
#define MAKE_ENUM_MATH_OPERATORS( x ) \
	inline x operator ++ ( x& a )                    /* prefix increment */ \
		{  return ( a = scast <x> ( a + 1 ) );  } \
	inline x operator ++ ( x& a, int )               /* postfix increment */ \
		{  x old = a;  ++a;  return ( old );  } \
	inline x operator -- ( x& a )                    /* prefix decrement */ \
		{  return ( a = scast <x> ( a - 1 ) );  } \
	inline x operator -- ( x& a, int )               /* postfix decrement */ \
		{  x old = a;  --a;  return ( old );  } \
	inline x operator + ( x a, x b ) \
		{  return ( scast <x> ( scast <int> ( a ) + scast <int> ( b ) ) ); } \
	inline x operator - ( x a, x b ) \
		{  return ( scast <x> ( scast <int> ( a ) - scast <int> ( b ) ) ); } \
	inline x operator += ( x& a, x b ) \
		{  return ( a = scast <x> ( a + b ) ); } \
	inline x operator -= ( x& a, x b ) \
		{  return ( a = scast <x> ( a - b ) ); }

// these functions provide a way to iterate over a bitfield-style enum to
//  extract the individual flags.

template <typename T>
inline T FindFirstEnum( T data )
{
	return ( (T)( ((DWORD)data == 0) ? 0 : (1 << GetZeroShift( (DWORD)data )) ) );
}

template <typename T>
inline T FindNextEnum( T iter, T data )
{
	gpassert( (DWORD)iter != 0 );
	iter = (T)( (DWORD)data & ~((((DWORD)iter - 1) << 1) + 1) );
	return ( FindFirstEnum( iter ) );
}

//////////////////////////////////////////////////////////////////////////////
// inheritance macros

// guaranteed to be bad to use as an Inherited typedef
struct CannotCallInherited  {  };
#define BAD_INHERIT CannotCallInherited

// helper macros
#define SET_INHERITS(m, t, t1, t2, t3, t4)          \
	typedef m  ThisType;   typedef t  Inherited;    \
	typedef t1 Inherited1; typedef t2 Inherited2;   \
	typedef t3 Inherited3; typedef t4 Inherited4;
#define SET_INHERITS_NO_THISTYPE(t, t1, t2, t3, t4) \
						   typedef t  Inherited;    \
	typedef t1 Inherited1; typedef t2 Inherited2;   \
	typedef t3 Inherited3; typedef t4 Inherited4;

// this is a bonehead catcher that you should use to knock out the
//  InheritedX typedefs.  if you call InheritedX::Anything() it'll
//  puke on you.
#define SET_NO_INHERITED(m) \
	SET_INHERITS(m, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT)
#define SET_NO_INHERITED_NO_THISTYPE() \
	SET_INHERITS_NO_THISTYPE(BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT)

#define SET_INHERITED(m, t) \
	SET_INHERITS(m, t, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT)
#define SET_INHERITED2(m, t1, t2) \
	SET_INHERITS(m, BAD_INHERIT, t1, t2, BAD_INHERIT, BAD_INHERIT)
#define SET_INHERITED_NO_THISTYPE(t) \
	SET_INHERITS_NO_THISTYPE(t, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT, BAD_INHERIT)

// use for multiple inheritance
#define SET_INHERITED1_NO_THISTYPE(t) \
	SET_INHERITED_NO_THISTYPE(t)

// use for multiple inheritance
#define SET_INHERITED2_NO_THISTYPE( t1, t2 ) \
	SET_INHERITS_NO_THISTYPE( BAD_INHERIT, t1, t2, BAD_INHERIT, BAD_INHERIT )

//////////////////////////////////////////////////////////////////////////////
// FTOL enhancements

// this is the inline version, about 2x faster
inline int FTOL( float f )
{
	int i;
	__asm
	{
		fld   [ f ]
		fistp [ i ]
	}
	return ( i );
}

// double version
inline int FTOL( double d )
{
	int i;
	__asm
	{
		fld   [ d ]
		fistp [ i ]
	}
	return ( i );
}

//////////////////////////////////////////////////////////////////////////////
// timer query

inline LARGE_INTEGER ReadRDTSCTimer( void )
{
	LARGE_INTEGER ret_time;
	__asm
	{
		rdtsc
		mov ret_time.LowPart,eax
		mov ret_time.HighPart,edx
	}
	return ret_time;
}

//////////////////////////////////////////////////////////////////////////////
// min/max templates

// windows may have already defined this (bad microsoft!)
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template <typename T> inline
const T& max( const T& x, const T& y )
	{  return ( x < y ? y : x );  }

template <typename T> inline
const T& min( const T& x, const T& y )
	{  return ( y < x ? y : x );  }

template <typename T, typename PRED> inline
const T& max( const T& x, const T& y, PRED p )
	{  return ( p( x, y ) ? y : x );  }

template <typename T, typename PRED> inline
const T& min( const T& x, const T& y, PRED p )
	{  return ( p( y, x ) ? y : x );  }

// these are special versions meant for when you are using temporaries

template <typename T, typename U> inline
T max_t( const T& x, const U& y )
	{  return ( x < y ? y : x );  }

template <typename T, typename U, typename PRED> inline
T max_t( const T& x, const U& y, PRED p )
	{  return ( p( x, y ) ? y : x );  }

template <typename T, typename U> inline
T min_t( const T& x, const U& y )
	{  return ( y < x ? y : x );  }

template <typename T, typename U, typename PRED> inline
T min_t( const T& x, const U& y, PRED p )
	{  return ( p( y, x ) ? y : x );  }

//////////////////////////////////////////////////////////////////////////////
// clamping templates

// these return true if the value had to be clamped

template <typename T, typename U> inline
bool clamp_min( T& val, const U& min_val )
{
	if ( val < min_val )
	{
		val = min_val;
		return ( true );
	}
	return ( false );
}

template <typename T, typename U> inline
bool clamp_max( T& val, const U& max_val )
{
	if ( val > max_val )
	{
		val = max_val;
		return ( true );
	}
	return ( false );
}

template <typename T, typename U, typename V> inline
bool clamp_min_max( T& val, const U& min_val, const V& max_val )
{
	if ( val < min_val )
	{
		val = min_val;
		return ( true );
	}
	else if ( val > max_val )
	{
		val = max_val;
		return ( true );
	}
	return ( false );
}

template <typename T, typename U, typename V> inline
bool wrap_min_max( T& val, const U& min_val, const V& max_val )
{
	if ( val < min_val )
	{
		val = max_val;
		return ( true );
	}
	else if ( val > max_val )
	{
		val = min_val;
		return ( true );
	}
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// extremes templates

template <typename T, typename U> inline
T& maximize( T& val, const U& test_val )
{
	if ( val < test_val )
	{
		val = test_val;
	}
	return ( val );
}

template <typename T, typename U> inline
T& minimize( T& val, const U& test_val )
{
	if ( val > test_val )
	{
		val = test_val;
	}
	return ( val );
}

template <typename T, typename PRED> inline
void swap_if( T& l, T& r, const PRED& pred )
{
	if ( pred( r, l ) )
	{
		std::swap( l, r );
	}
}

template <typename T> inline
void swap_if( T& l, T& r )
{
	if ( r < l )
	{
		std::swap( l, r );
	}
}

//////////////////////////////////////////////////////////////////////////////
// 32-bit "canonical" color manipulation

#pragma pack ( push, 1 )
struct gpcolor
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;

	gpcolor( void )													{  }
	gpcolor( DWORD d )												{  *(DWORD*)this = d;  }
	gpcolor( int r_, int g_, int b_, int a_ = 255 )					{  set( r_, g_, b_, a_ );  }
	gpcolor( float r_, float g_, float b_, float a_ = 1.0f )		{  set( r_, g_, b_, a_ );  }

	void set( int r_, int g_, int b_, int a_ = 255 )				{  r = (BYTE)r_;  g = (BYTE)g_;  b = (BYTE)b_;  a = (BYTE)a_;  }
	void set( float r_, float g_, float b_, float a_ = 1.0f )		{  r = (BYTE)FTOL( r_ * 255.0f);  g = (BYTE)FTOL( g_ * 255.0f);  b = (BYTE)FTOL( b_ * 255.0f);  a = (BYTE)FTOL( a_ * 255.0f);  }

	operator DWORD ( void ) const									{  return ( *(DWORD*)this );  }
	operator DWORD& ( void )										{  return ( *(DWORD*)this );  }
};
#pragma pack ( pop )

//////////////////////////////////////////////////////////////////////////////
// class GlobalRegistrar <T> declaration

// usage: just derive your class from this as:
//
//            MyClass : public GlobalRegistrar <MyClass>.
//
//        when you instantiate one of your classes it will insert itself at the
//        front of the list. iterate through YourClass::ms_Root to get at all
//        the instances. obviously this class registers only and never
//        deregisters - it's purely meant for tracking of static objects
//        registered globally.

template <typename T>
struct GlobalRegistrar
{
	T*         m_Next;
	static T*  ms_Root;
	static int ms_Count;

	GlobalRegistrar( void )
	{
		m_Next  = ms_Root;
		ms_Root = (T*)this;
		++ms_Count;
	}
};

template <typename T> T*  GlobalRegistrar <T>::ms_Root  = NULL;
template <typename T> int GlobalRegistrar <T>::ms_Count = 0;

//////////////////////////////////////////////////////////////////////////////
// class Singleton <T> declaration

// this is a simple singleton tracking class. it asserts on attempts to deref
//  an invalid singleton and if you instantiate more than one. auto-unregisters
//  the object on destruction.

// usage - only one step!:
//
//     class MyClass : public Singleton <MyClass>			<-- derive from Singleton <YourClassNameHere>
//     {
//     public:
//         void Foo( void );
//         ...
//     };
//
//     void SomeFunction( void )
//     {
//         MyClass::GetSingleton().Foo();					<-- easy, eh?
//     }
//
// for more convenience, you can always do something like this:
//
//     #define gMyClass MyClass::GetSingleton()
//
// ...then it's like a global: gMyClass.Foo()...
//

template <typename T>
class Singleton
{
protected:
	Singleton( void )
	{
		gpassert( ms_Singleton == NULL );
		int offset = (int)(T*)1 - (int)(Singleton <T> *)(T*)1;
		ms_Singleton = (T*)((int)this + offset);
	}
   ~Singleton( void )  {  gpassert( ms_Singleton != NULL );  ms_Singleton = NULL;  }

public:
	static T&   GetSingleton      ( void )  {  gpassert( ms_Singleton != NULL );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != NULL );  }

private:
	static T* ms_Singleton;

	SET_NO_COPYING( Singleton <T> );
};

template <typename T> T* Singleton <T>::ms_Singleton = NULL;

//////////////////////////////////////////////////////////////////////////////
// class ManualSingleton <T> declaration

template <typename T>
class ManualSingleton
{
protected:
	ManualSingleton( bool push = true )
	{
		if ( push )
		{
			PushSingleton();
		}
	}

   ~ManualSingleton( void )
	{
		PopSingleton();
	}

	void PushSingleton( void )
	{
		int offset = (int)(T*)1 - (int)(ManualSingleton <T> *)(T*)1;
		ms_SingletonStack.push_back( (T*)((int)this + offset) );
	}

	void PopSingleton( void )
	{
		int offset = (int)(T*)1 - (int)(ManualSingleton <T> *)(T*)1;
		T* ptr = (T*)((int)this + offset);

		SingletonStack::iterator i, ibegin = ms_SingletonStack.begin(), iend = ms_SingletonStack.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( *i == ptr )
			{
				ms_SingletonStack.erase( i );
				break;
			}
		}
	}

public:
	static T&   GetSingleton      ( void )  {  gpassert( DoesSingletonExist() );  return ( *ms_SingletonStack.back() );  }
	static T*   GetSingletonPtr   ( void )  {  return ( DoesSingletonExist() ? ms_SingletonStack.back() : NULL );  }
	static bool DoesSingletonExist( void )  {  return ( !ms_SingletonStack.empty() );  }

private:
	typedef std::vector <T*> SingletonStack;
	static SingletonStack ms_SingletonStack;

	SET_NO_COPYING( ManualSingleton <T> );
};

template <typename T> std::vector <T*> ManualSingleton <T>::ms_SingletonStack;

//////////////////////////////////////////////////////////////////////////////
// class ThreadedSingleton <T> declaration

// purpose: allows Singleton usage across multiple threads

template <typename T>
class ThreadedSingleton
{
protected:
	ThreadedSingleton( void )
	{
		gpassert( ms_Singleton == NULL );
		int offset = (int)(T*)1 - (int)(Singleton <T> *)(T*)1;
		ms_Singleton = (T*)((int)this + offset);
	}

   ~ThreadedSingleton( void )  {  gpassert( ms_Singleton != NULL );  ms_Singleton = NULL;  }

public:
	static T&   GetSingleton      ( void )  {  gpassert( ms_Singleton != NULL );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != NULL );  }

private:
	DECL_THREAD static T* ms_Singleton;

	SET_NO_COPYING( ThreadedSingleton <T> );
};

template <typename T> T* ThreadedSingleton <T>::ms_Singleton = NULL;

//////////////////////////////////////////////////////////////////////////////
// class AutoSingleton <T> declaration

// purpose: a Singleton that automatically instantiates itself (statically).
//          nothing special about this class - it just provides the same
//          interface as a normal Singleton.

template <typename T>
class AutoSingleton
{
protected:
	AutoSingleton( void )
	{
		gpassert( ms_Singleton == NULL );
		ms_Singleton = &ms_SingletonInstance;
	}
   ~AutoSingleton( void )
	{
		gpassert( ms_Singleton != NULL );
		ms_Singleton = NULL;
	}

public:
	static T&   GetSingleton      ( void )  {  gpassert( ms_Singleton != NULL );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != NULL );  }

private:
	static T* ms_Singleton;
	static T ms_SingletonInstance;

	SET_NO_COPYING( AutoSingleton <T> );
};

template <typename T> T* AutoSingleton <T>::ms_Singleton = NULL;
template <typename T> T AutoSingleton <T>::ms_SingletonInstance;

//////////////////////////////////////////////////////////////////////////////
// class OnDemandSingleton <T> declaration

// purpose: this is a Singleton that is instantiated on demand and destroyed
//          automatically at app shutdown. do not instantiate 'T' manually.
//          this is a "traditional" singleton class that you see in textbooks.

template <typename T>
T& GetOnDemandSingleton( void )
{
	static T s_Singleton;
	return ( s_Singleton );
}

template <typename T>
class OnDemandSingleton
{
protected:
	OnDemandSingleton( void )
	{
		gpassert( !ms_SingletonExists );
		ms_SingletonExists = true;
	}
   ~OnDemandSingleton( void )
	{
		gpassert( ms_SingletonExists );
		ms_SingletonExists = false;
	}

public:
	static T&   GetSingleton      ( void )	{  return ( GetOnDemandSingleton <T> () );  }
	static T*   GetSingletonPtr   ( void )  {  return ( &GetSingleton() );  }
	static bool DoesSingletonExist( void )  {  return ( ms_SingletonExists );  }

private:
	static bool ms_SingletonExists;
};

template <typename T>
bool OnDemandSingleton <T>::ms_SingletonExists = false;

//////////////////////////////////////////////////////////////////////////////
// class RefCountSingleton <T> declaration

// purpose: this is a Singleton that is instantiated on demand and destroyed
//          automatically when all outlying handles have been released. use
//          this class indirectly through RefCountSingleton::Handle. do not
//          instantiate 'T' manually.

template <typename T>
class RefCountSingleton
{
public:
	static void AddRef( void )
	{
		gpassert( ms_RefCount >= 0 );
		if ( ms_RefCount++ == 0 )
		{
			gpassert( ms_Singleton == NULL );
			ms_Singleton = new T;
		}
	}

	static void Release( void )
	{
		if ( --ms_RefCount == 0 )
		{
			gpassert( ms_Singleton != NULL );
			Delete ( ms_Singleton );
		}
		gpassert( ms_RefCount >= 0 );
	}

	class Handle
	{
	public:
		typedef RefCountSingleton <T> Owner;

		Handle( void )           {  T::AddRef();  }
		Handle( const Handle& )  {  T::AddRef();  }
	   ~Handle( void )           {  T::Release();  }

		T& operator *  ( void ) const  {  return ( Owner::GetSingleton() );  }
		T* operator -> ( void ) const  {  return ( &Owner::GetSingleton() );  }
	};

private:
	friend class Handle;

	static T&   GetSingleton      ( void )  {  gpassert( ms_Singleton != NULL );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != NULL );  }

	static T*  ms_Singleton;
	static int ms_RefCount;
};

template <typename T> T*  RefCountSingleton <T>::ms_Singleton = NULL;
template <typename T> int RefCountSingleton <T>::ms_RefCount = 0;

//////////////////////////////////////////////////////////////////////////////
// helper structs

// used for void*+size parameter passing
struct mem_ptr
{
	typedef void* MemType;

	void*  mem;			// pointer to start of memory
	size_t size;		// size of memory

	mem_ptr( void )               {  mem = NULL;  size = 0;  GPDEBUG_ONLY( assert_valid() );  }
	mem_ptr( void* m, size_t s )  {  mem = m;     size = s;  GPDEBUG_ONLY( assert_valid() );  }

	operator void* ( void ) const  {  return ( mem  );  }
	operator size_t( void ) const  {  return ( size );  }

	void assert_valid( void ) const
	{
		gpassert( !IsMemoryBadFood( (DWORD)mem ) );
	}
};

template <typename T>
inline mem_ptr make_mem_ptr( T& obj )
	{  return ( mem_ptr( &obj, sizeof( T ) ) );  }

// used for void*+size parameter passing
struct const_mem_ptr
{
	typedef const void* MemType;

	const void* mem;	// pointer to start of memory
	size_t      size;	// size of memory

	const_mem_ptr( void )                     {  mem = NULL;  size = 0;  }
	const_mem_ptr( const void* m, size_t s )  {  mem = m;  size = s;  GPDEBUG_ONLY( assert_valid() );  }
	const_mem_ptr( mem_ptr p )                {  mem = p;  size = p;  GPDEBUG_ONLY( assert_valid() );  }

	operator const void* ( void ) const  {  return ( mem  );  }
	operator size_t      ( void ) const  {  return ( size );  }

	void assert_valid( void ) const
	{
		gpassert( !IsMemoryBadFood( (DWORD)mem ) );
		gpassert( size < 1024 * 1024 * 1024 );
	}
};

template <typename T>
inline const_mem_ptr make_const_mem_ptr( const T& obj )
	{  return ( const_mem_ptr( &obj, sizeof( T ) ) );  }

// used for iteration through memory
template <typename T>
struct mem_iter_t : T
{
	mem_iter_t( void )                    {  }
	mem_iter_t( T::MemType m, size_t s )  : T( m, s )  {  }
	mem_iter_t( T p )				      : T( p )  {  }

	template <typename U>
	void get( U& obj )
	{
		obj = *(const U*)mem;
	}

	bool advance( size_t s )
	{
		mem = (BYTE*)mem + s;
		size_t oldSize = size;
		size -= s;
		return ( oldSize >= size );
	}

	DWORD advance_dword( void )
	{
		DWORD d = *(const DWORD*)mem;
		advance( sizeof( DWORD ) );
		return ( d );
	}

	WORD advance_word( void )
	{
		WORD w = *(const WORD*)mem;
		advance( sizeof( WORD ) );
		return ( w );
	}

	BYTE advance_byte( void )
	{
		BYTE b = *(const BYTE*)mem;
		advance( sizeof( BYTE ) );
		return ( b );
	}
};

typedef mem_iter_t <mem_ptr> mem_iter;
typedef mem_iter_t <const_mem_ptr> const_mem_iter;

//////////////////////////////////////////////////////////////////////////////
// bit flag manipulation

template <typename T, typename U>
inline void BitFlagsSet( T& in, U flags )
{
	gpassert( (DWORD)flags != 0 );
	in = (T)( (DWORD)in | (DWORD)flags );
}

template <typename T, typename U>
inline void BitFlagsClear( T& in, U flags )
{
	gpassert( (DWORD)flags != 0 );
	in = (T)( (DWORD)in & ~(DWORD)flags );
}

template <typename T, typename U>
inline bool BitFlagsContainAny( T& in, U flags )
{
	return ( ((DWORD)in & (DWORD)flags) != 0 );
}

template <typename T, typename U>
inline bool BitFlagsContainAll( T& in, U flags )
{
	return ( ((DWORD)in & (DWORD)flags) == (DWORD)flags );
}

template <typename T, typename U>
inline bool BitFlagsAre( T& in, U flags )
{
	return ( (DWORD)in == (DWORD)flags );
}

//////////////////////////////////////////////////////////////////////////////
// struct RecurseLocker declaration

#if !GP_RETAIL

namespace kerneltool
{
	class Critical;
}

struct RecurseTracker
{
	int  m_ReadLock;
	bool m_WriteLock;

	RecurseTracker( void )
	{
		m_ReadLock = 0;
		m_WriteLock = false;
	}
};

#include <map>
typedef std::map <DWORD, RecurseTracker> RecurseTrackerDb;

class RecurseLocker
{
public:
	RecurseLocker( DWORD cookie, const char* name, RecurseTrackerDb& db, bool readOnly );
   ~RecurseLocker( void );

private:
	const char*                m_Name;
	RecurseTrackerDb&          m_Db;
	RecurseTrackerDb::iterator m_Tracker;
	bool                       m_ReadOnly;
	kerneltool::Critical*      m_Critical;
};

#define RECURSE_LOCK_FOR_READ( x ) \
	RecurseLocker recurseLockerR_##x( 0, #x, s_##x##RecurseTrackerDb, true )
#define RECURSE_LOCK_FOR_WRITE( x ) \
	RecurseLocker recurseLockerW_##x( 0, #x, s_##x##RecurseTrackerDb, false )
#define RECURSE_LOCK_INSTANCE_FOR_READ( obj, x ) \
	RecurseLocker recurseLockerR_##x( (DWORD)(obj), #x, s_##x##RecurseTrackerDb, true )
#define RECURSE_LOCK_INSTANCE_FOR_WRITE( obj, x ) \
	RecurseLocker recurseLockerW_##x( (DWORD)(obj), #x, s_##x##RecurseTrackerDb, false )
#define RECURSE_DECLARE( x ) \
	static RecurseTrackerDb s_##x##RecurseTrackerDb;

#else // !GP_RETAIL

#define RECURSE_LOCK_FOR_READ( x )
#define RECURSE_LOCK_FOR_WRITE( x )
#define RECURSE_LOCK_INSTANCE_FOR_READ( obj, x )
#define RECURSE_LOCK_INSTANCE_FOR_WRITE( obj, x )
#define RECURSE_DECLARE( x )

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// general version query declarations

// $ note: this is deprecated. use gpversion instead.

// this is a more readable version of microsoft's wacky version formatting
// that they use in RAID.
struct MSQAVersion
{
	WORD m_VersionExternal;
	WORD m_VersionInternal;
	WORD m_Year;
	WORD m_Month;
	WORD m_Day;
	WORD m_BuildOfDay;

	MSQAVersion( void );

	bool IsZero( void ) const;

	bool operator == ( const MSQAVersion& other ) const;
};

bool     FromString( const char* textVersion, MSQAVersion& out );
gpstring ToString  ( const MSQAVersion& in, bool full );

bool operator < ( const MSQAVersion& l, const MSQAVersion& r );

//////////////////////////////////////////////////////////////////////////////
// gpversion

// $ this is the One True Version structure

struct gpversion
{
	enum eMode
	{
		MODE_SHORT,
		MODE_LONG,
		MODE_MSQA_ONLY_SHORT,
		MODE_MSQA_ONLY_LONG,
		MODE_COMPLETE,
		MODE_AUTO_SHORT,
		MODE_AUTO_LONG,
	};

	WORD m_MajorVersion;			// any # digits
	WORD m_MinorVersion;			// max 2 digits
	WORD m_Revision;				// max 1 digit
	WORD m_BuildNumber;				// max 4 digits
	WORD m_Year;
	WORD m_Month;
	WORD m_Day;
	WORD m_BuildOfDay;

	gpversion( void );

	bool InitFromResources( const char* fileName = NULL );

	bool IsZero    ( void ) const;
	bool IsMSQAOnly( void ) const;

	bool operator == ( const gpversion& other ) const;
	bool operator == ( const MSQAVersion& other ) const;
};

bool        FromString   ( const char* textVersion, gpversion& out );
gpstring    ToString     ( const gpversion& in, gpversion::eMode mode );
gpstring    ToString     ( const MSQAVersion& in, gpversion::eMode mode );
MSQAVersion ToMSQAVersion( const gpversion& in );
gpversion   ToGpVersion  ( const MSQAVersion& in );

bool operator < ( const gpversion& l, const gpversion& r );

//////////////////////////////////////////////////////////////////////////////
// DirectX helper declarations

// Version checking.

	// This function returns
	//
	//     0 - No DirectX installed
	//     1 - DirectX version 1 installed
	//     2 - DirectX 2 installed
	//     3 - DirectX 3 installed
	//     5 - At least DirectX 5 installed.
	//     6 - At least DirectX 6 installed.
	//
	// Note that this code is intended as a general guideline. Your app will
	// probably be able to simply query for functionality (via COM's
	// QueryInterface) for one or two components.
	//
	// Also note:
	//
	//     "if (dxVer != 5 ) return FALSE;" is BAD.
	//     "if (dxVer < 5 ) return FALSE;" is MUCH BETTER.
	//
	// to ensure your app will run on future releases of DirectX.

	DWORD GetDxVersion( void );

// Error handling/reporting.

	// looks up a plaintext version of the DirectX/COM error code for you,
	// returns NULL if error not in text database
	const char* LookupDxErrorMessage( HRESULT code );

	// this will give a valid error message no matter what - if it's a DX error
	// then it will convert properly otherwise it will decompose the error
	// according to Win32 error bit convention.
	void     MakeDxErrorMessage( gpstring& out, HRESULT code, const char* file = NULL, int line = 0, bool includeExtra = false );
	gpstring MakeDxErrorMessage( HRESULT code, const char* file = NULL, int line = 0 );

	// this will return the string version of the dplay message, or NULL if non-
	// existent.
	const char* LookupDPlayMessage( DWORD messageId );

	// Wrap all DirectX calls in either TRYDX() or TRYDX_SUCCEED() - use the
	// SUCCEED version to cause a fatal if the HRESULT is a failure code (an
	// example of a good time to use this is when creating the primary surface).
	// The functions will log error info (with optional file and line info) on a
	// failure case.

#	if GP_RETAIL

	HRESULT TryDx( HRESULT result, bool fatalOnFail = false );

#	define TRYDX( rc )			TryDx( rc )
#	define TRYDX_SUCCEED( rc )	TryDx( rc, true )

#	else

	HRESULT TryDx( HRESULT result, const char* file, int line, bool fatalOnFail = false );

#	define TRYDX( rc )			TryDx( rc, __FILE__, __LINE__ )
#	define TRYDX_SUCCEED( rc )	TryDx( rc, __FILE__, __LINE__, true )

#	endif

	// special macro that returns 'true' if successful dx call, else it reports
	// the error and returns false
#	define CHECKDX( rc )		SUCCEEDED( TRYDX( rc ) )

//////////////////////////////////////////////////////////////////////////////
// namespace SysInfo declaration

namespace SysInfo  {  // begin of namespace SysInfo

// Basic functions.

	const char*          GetComputerName       ( void );
	const char*          GetUserName           ( void );
	const SYSTEM_INFO&   GetSystemInfo         ( void );
	const OSVERSIONINFO& GetOsVersionInfo      ( void );
	const DWORD&         GetTotalPhysicalMemory( void );
	gpstring             GetIPAddress          ( void );
	double				 GetCPUSpeedInHertz    ( void );
	double				 GetCPUSeconds         ( void );
	bool				 IsSIMDSupported       ( void );
	bool                 IsDebuggerPresent     ( void );

	// these will return the current EXE version by querying the
	//  VS_VERSION_INFO in the resources. data is cached so only the
	//  first call is expensive.

	const gpversion& GetExeFileVersion    ( void );
	gpstring         GetExeFileVersionText( gpversion::eMode mode );
	gpstring         MakeExeSessionInfo   ( void );

	inline int  GetProcessorCount( void )  {  return ( (int)GetSystemInfo().dwNumberOfProcessors );  }
	inline bool IsMultiProcessor ( void )  {  return ( GetProcessorCount() > 1 );  }
	inline bool IsWin9x          ( void )  {  return ( GetOsVersionInfo().dwPlatformId == VER_PLATFORM_WIN32_WINDOWS );  }

// Inline helpers.

	inline DWORD GetSystemPageSize( void )
		{  return ( GetSystemInfo().dwPageSize );  }
	inline DWORD GetSystemAllocationGranularity( void )
		{  return ( GetSystemInfo().dwAllocationGranularity );  }
	inline bool  IsOsNt( void )
		{  return ( GetOsVersionInfo().dwPlatformId == VER_PLATFORM_WIN32_NT );  }
	inline bool  IsOsWin9x( void )
		{  return ( !IsOsNt() );  }

}  // end of namespace SysInfo

//////////////////////////////////////////////////////////////////////////////

#endif // GPGLOBAL_H

//////////////////////////////////////////////////////////////////////////////
