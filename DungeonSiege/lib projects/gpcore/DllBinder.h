//////////////////////////////////////////////////////////////////////////////
//
// File     :  DllBinder.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes for playing with modules (DLLs). Pirated from
//             BC++ 5 module.cpp and module.h (OWL 5).
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __DLLBINDER_H
#define __DLLBINDER_H

//////////////////////////////////////////////////////////////////////////////

#include "GPCore.h"
#include <vector>

//////////////////////////////////////////////////////////////////////////////
// class DllBinder declaration

struct DllProc;

class DllBinder
{
public:
	SET_NO_INHERITED( DllBinder );

	DllBinder( const gpstring& dllName = gpstring::EMPTY );
   ~DllBinder( void );

	// call to make sure the DLL is mapped and loaded. returns false on fail.
	bool Load( bool warnOnFail = true, bool dataOnly = false );
	bool Load( const gpstring& dllName, bool warnOnFail = true, bool dataOnly = false );

	// get instance
	HMODULE GetDllHandle( void ) const					{  return ( m_DllHandle );  }
	operator HMODULE( void ) const						{  return ( GetDllHandle() );  }

	// get name
	const gpstring& GetDllName( void ) const			{  return ( m_DllName );  }

protected:

	// call once per proc to track
	void AddProc( DllProc* proc, const char* name, bool optional = false );

private:

	struct Entry
	{
		DllProc*    m_Proc;
		const char* m_Name;
		bool        m_Optional;

		Entry( DllProc* proc, const char* name, bool optional )	{  m_Proc = proc;  m_Name = name;  m_Optional = optional;  }
		Entry( void )											{  ::ZeroObject( *this );  }
	};

	typedef std::vector <Entry> EntryColl;

	// spec
	gpstring  m_DllName;				// name of the DLL to map
	EntryColl m_Procs;					// our mapped procs

	// state
	HMODULE m_DllHandle;				// handle for this DLL (NULL if not mapped)

	SET_NO_COPYING( DllBinder );
};

//////////////////////////////////////////////////////////////////////////////
// struct DllProc declaration

struct DllProc
{
	DllProc( void ) : m_Proc( NULL )	{  }
	FARPROC m_Proc;

	operator FARPROC( void ) const		{  return ( m_Proc );  }
};

//////////////////////////////////////////////////////////////////////////////
// WINAPI (stdcall) declarations

struct DllProcV0 : DllProc
{
	typedef void (WINAPI* PROC)( void );
	void operator () ( void ) const  {  ((PROC)( m_Proc ))();  }
};

template <typename R>
struct DllProc0 : DllProc
{
	typedef R (WINAPI* PROC)( void );
	R operator () ( void ) const  {  return ( ((PROC)( m_Proc ))() );  }
};

template <typename P1>
struct DllProcV1 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1 );
	void operator () ( P1 p1 ) const  {  ((PROC)( m_Proc ))( p1 );  }
};

template <typename R, typename P1>
struct DllProc1 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1 );
	R operator () ( P1 p1) const  {  return ( ((PROC)( m_Proc ))( p1 ) );  }
};

template <typename P1, typename P2>
struct DllProcV2 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2 );
	void operator () ( P1 p1, P2 p2) const  {  ((PROC)( m_Proc ))( p1, p2 );  }
};

template <typename R, typename P1, typename P2>
struct DllProc2 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2 );
	R operator () ( P1 p1, P2 p2 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2 ) );  }
};

template <typename P1, typename P2, typename P3>
struct DllProcV3 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3 );
	void operator () ( P1 p1, P2 p2, P3 p3 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3 );  }
};

template <typename R, typename P1, typename P2, typename P3>
struct DllProc3 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3 );
	R operator () ( P1 p1, P2 p2, P3 p3 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4>
struct DllProcV4 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4>
struct DllProc4 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5>
struct DllProcV5 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
struct DllProc5 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct DllProcV6 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct DllProc6 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct DllProcV7 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct DllProc7 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct DllProcV8 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct DllProc8 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct DllProcV9 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct DllProc9 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
struct DllProcV10 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
struct DllProc10 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
struct DllProcV11 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
struct DllProc11 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12>
struct DllProcV12 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12>
struct DllProc12 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12, typename P13>
struct DllProcV13 : DllProc
{
	typedef void (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12, typename P13>
struct DllProc13 : DllProc
{
	typedef R (WINAPI* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 ) );  }
};

//////////////////////////////////////////////////////////////////////////////
// CDECL declarations

struct CdeclProcV0 : DllProc
{
	typedef void (__cdecl* PROC)( void );
	void operator () ( void ) const  {  ((PROC)( m_Proc ))();  }
};

template <typename R>
struct CdeclProc0 : DllProc
{
	typedef R (__cdecl* PROC)( void );
	R operator () ( void ) const  {  return ( ((PROC)( m_Proc ))() );  }
};

template <typename P1>
struct CdeclProcV1 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1 );
	void operator () ( P1 p1 ) const  {  ((PROC)( m_Proc ))( p1 );  }
};

template <typename R, typename P1>
struct CdeclProc1 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1 );
	R operator () ( P1 p1) const  {  return ( ((PROC)( m_Proc ))( p1 ) );  }
};

template <typename P1, typename P2>
struct CdeclProcV2 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2 );
	void operator () ( P1 p1, P2 p2) const  {  ((PROC)( m_Proc ))( p1, p2 );  }
};

template <typename R, typename P1, typename P2>
struct CdeclProc2 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2 );
	R operator () ( P1 p1, P2 p2 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2 ) );  }
};

template <typename P1, typename P2, typename P3>
struct CdeclProcV3 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3 );
	void operator () ( P1 p1, P2 p2, P3 p3 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3 );  }
};

template <typename R, typename P1, typename P2, typename P3>
struct CdeclProc3 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3 );
	R operator () ( P1 p1, P2 p2, P3 p3 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4>
struct CdeclProcV4 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4>
struct CdeclProc4 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5>
struct CdeclProcV5 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
struct CdeclProc5 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct CdeclProcV6 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct CdeclProc6 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct CdeclProcV7 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct CdeclProc7 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct CdeclProcV8 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct CdeclProc8 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct CdeclProcV9 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct CdeclProc9 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
struct CdeclProcV10 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10>
struct CdeclProc10 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
struct CdeclProcV11 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11>
struct CdeclProc11 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12>
struct CdeclProcV12 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12>
struct CdeclProc12 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12 ) );  }
};

template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12, typename P13>
struct CdeclProcV13 : DllProc
{
	typedef void (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 );
	void operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 ) const  {  ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 );  }
};

template <typename R, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename P10, typename P11, typename P12, typename P13>
struct CdeclProc13 : DllProc
{
	typedef R (__cdecl* PROC)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 );
	R operator () ( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12, P13 p13 ) const  {  return ( ((PROC)( m_Proc ))( p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13 ) );  }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __DLLBINDER_H

//////////////////////////////////////////////////////////////////////////////
