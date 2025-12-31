//////////////////////////////////////////////////////////////////////////////
//
// File     :  DrWatson.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains reporting stuff for MS DrWatson tool.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __DRWATSON_H
#define __DRWATSON_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"

namespace FileSys
{
	class StreamWriter;
}

//////////////////////////////////////////////////////////////////////////////

namespace DrWatson  {

// Options.

	enum eOptions
	{
		OPTION_NONE         = 0,			// clear options
		OPTION_ENABLE       = 1 << 0,		// enable DW if on by default in retailoptions.h
		OPTION_FORCE_ENABLE = 1 << 1,		// force-enable DW no matter what
	};

	void SetOptions  ( eOptions options, bool set = true );
	void ClearOptions( eOptions options );
	bool TestOptions ( eOptions options );

	MAKE_ENUM_BIT_OPERATORS( eOptions );

// Setup.

	enum eInfoFlags
	{
		INFO_NONE		= 0,
		INFO_FULL		= 1 << 0,			// gather all info, we're doing a mondo crash report
		INFO_NO_MALLOC	= 1 << 1,			// we're in an out-of-memory condition, do not alloc new memory
	};

	MAKE_ENUM_BIT_OPERATORS( eInfoFlags );

	struct Info
	{
		// in
		eInfoFlags         m_InfoFlags;		// flags for tuning what kind of info to gather
		const SeException* m_Sex;			// crash info goes here (might be null!)

		// out
		FileSys::StreamWriter* m_Writer;	// put output info here (might be null)

		Info( void )
		{
			m_InfoFlags = INFO_NONE;
			m_Sex       = NULL;
			m_Writer    = NULL;
		}
	};

	// the cb should create a new file, dump info into it, and then call
	// RegisterFileForReport() with the file to use.
	typedef CBFunctor1 <Info&> InfoCb;

	// registration API
	void RegisterFileForReport( const char* fileName );
	void RegisterInfoCallback( InfoCb cb, bool front = true );

	// callback API
	void ExecuteInfoCallbacks( Info& info );

	// output files go here - note that trailing backslash always appended
	void            InitOutputDirectory( const char* path = NULL );
	const gpstring& GetOutputDirectory ( void );

// Filter.

	LONG WINAPI ExceptionFilter( const EXCEPTION_POINTERS *pep );

} // end of namespace DrWatson

//////////////////////////////////////////////////////////////////////////////

#endif  // __DRWATSON_H

//////////////////////////////////////////////////////////////////////////////
