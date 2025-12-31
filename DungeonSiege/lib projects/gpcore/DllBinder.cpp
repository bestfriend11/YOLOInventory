//////////////////////////////////////////////////////////////////////////////
//
// File     :  DllBinder.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "DllBinder.h"

#include "FileSysUtils.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// class DllBinder implementation

DllBinder :: DllBinder( const gpstring& dllName )
	: m_DllName( dllName )
{
	m_DllHandle = NULL;
}

DllBinder :: ~DllBinder( void )
{
	if ( m_DllHandle != NULL )
	{
		::FreeLibrary( m_DllHandle );
	}
}

bool DllBinder :: Load( bool warnOnFail, bool dataOnly )
{
	// bail early if already done
	if ( m_DllHandle != NULL )
	{
		return ( true );
	}

	// first load the library
	m_DllHandle = ::LoadLibraryEx( m_DllName, NULL, dataOnly ? LOAD_LIBRARY_AS_DATAFILE : 0 );
	if ( m_DllHandle == NULL )
	{
		if ( warnOnFail )
		{
			gperrorf(( "DllBinder: unable to load/find '%s' ('%s')\n",
					   m_DllName.c_str(), stringtool::GetLastErrorText().c_str() ));
		}
		return ( false );
	}

	// now map all the procs
	EntryColl::iterator i, begin = m_Procs.begin(), end = m_Procs.end();
	for ( i = begin ; i != end ; ++i )
	{
		FARPROC proc = ::GetProcAddress( m_DllHandle, i->m_Name );
		if ( (proc != NULL) || i->m_Optional )
		{
			i->m_Proc->m_Proc = proc;
		}
		else
		{
			if ( warnOnFail )
			{
				// just abort the whole thing - it's an all-or-nothing deal
				gperrorf(( "DllBinder: unable to map function '%s' in '%s' ('%s')\n",
							  i->m_Name, m_DllName.c_str(), stringtool::GetLastErrorText().c_str() ));
			}
			::FreeLibrary( m_DllHandle );
			m_DllHandle = NULL;
			return ( false );
		}
	}

	// update name to match
	m_DllName = FileSys::GetModuleFileName( m_DllHandle );

	// got them all
	return ( true );
}

bool DllBinder :: Load( const gpstring& dllName, bool warnOnFail, bool dataOnly )
{
	// bail early if already done
	if ( m_DllHandle != NULL )
	{
		return ( true );
	}

	// attempt load with this dll name
	m_DllName = dllName;
	return ( Load( warnOnFail, dataOnly ) );
}

void DllBinder :: AddProc( DllProc* proc, const char* name, bool optional )
{
	m_Procs.push_back( Entry( proc, name, optional ) );
}

//////////////////////////////////////////////////////////////////////////////
