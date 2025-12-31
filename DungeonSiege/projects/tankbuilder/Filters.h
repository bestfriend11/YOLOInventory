//////////////////////////////////////////////////////////////////////////////
//
// File     :  Filters.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains transformation filters for the tank builder.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILTERS_H
#define __FILTERS_H

//////////////////////////////////////////////////////////////////////////////

#include "TankBuilder.h"
#include <list>

//////////////////////////////////////////////////////////////////////////////

void RegisterTankFilters( void );

//////////////////////////////////////////////////////////////////////////////
// class ImgFilter declaration

class ImgFilter : public Tank::Builder::Filter
{
public:
	SET_INHERITED( ImgFilter, Tank::Builder::Filter );

	enum /*FOURCC*/
	{
		FILTER_RAW_MIPMAPS_STRIPTOP = 'RawS',
		FILTER_RAW_MIPMAPS          = 'RawM',
		FILTER_RAW_PLAIN            = 'Raw0',
	};

	ImgFilter( DWORD type )			{  m_Type = type;  }
	virtual ~ImgFilter( void )		{  }

private:
	typedef Tank::Builder::Buffer Buffer;

	virtual bool OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer );

	// local data
	DWORD  m_Type;
	Buffer m_TempBuffer;

	// dup tracking

	struct ImgEntry
	{
		// if non-square, crc's go normal, flip vert, flip horiz, flip vert
		// if square, crc's go four cw rots, then flip vert, then four more cw rots

		gpstring m_Name;
		DWORD    m_Crcs[ 8 ];			// successive crc's
		bool     m_Square;				// true if includes rotations (only possible on square ones)

		ImgEntry( void )
		{
			::ZeroObject( m_Crcs );
			m_Square = false;
		}
	};

	typedef std::list <ImgEntry> ImgColl;
	typedef std::multimap <DWORD, ImgColl::iterator> ImgDupDb;

	static bool     ms_TrackDupes;
	static ImgDupDb ms_ImgColl;
	static ImgDupDb ms_ImgDupDb;

	SET_NO_COPYING( ImgFilter );
};

//////////////////////////////////////////////////////////////////////////////
// class DollarScanner declaration

class DollarScanner : public Tank::Builder::Filter
{
public:
	SET_INHERITED( DollarScanner, Tank::Builder::Filter );
	
	enum /*FOURCC*/
	{
		FILTER_DEFAULT = '$$$',
	};

	DollarScanner( void )				{  }
	virtual ~DollarScanner( void )		{  }
	
private:
	typedef Tank::Builder::Buffer Buffer;

	virtual bool OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer );

	SET_NO_COPYING( DollarScanner );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __FILTERS_H

//////////////////////////////////////////////////////////////////////////////
