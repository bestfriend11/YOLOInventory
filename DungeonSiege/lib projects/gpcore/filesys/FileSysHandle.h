//////////////////////////////////////////////////////////////////////////////
//
// File     :  FileSysHandle.h (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Summary  :  Contains the base class for simple file system handles.
//             This file is internal to FileSys.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FILESYSHANDLE_H
#define __FILESYSHANDLE_H

//////////////////////////////////////////////////////////////////////////////

#include "GPGlobal.h"

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// class Handle declaration

// this is a generic API-type file handle. fast and stupid, cheap to pass
//  around as a parameter to functions.

class Handle
{
public:
	SET_NO_INHERITED( Handle );

	// special: for assigning "*this" to null
	struct tagNull_  {  };
	typedef tagNull_* Null_;
	static const Null_ Null;

	Handle( void )   {  m_Handle = NULL;  }
	Handle( Null_ )  {  m_Handle = NULL;  }

	inline Handle( UINT owner, UINT resource, UINT magic );		// user-supplied magic
	inline Handle( UINT owner, UINT resource );					// automagic

	bool IsNull          ( void ) const  {  return ( m_Handle == 0 );                      }
	bool IsHandle        ( void ) const  {  return ( !!m_HandleBit );                      }
	bool IsPointer       ( void ) const  {  return ( !m_HandleBit );                       }
	bool IsValidPointer  ( void ) const  {  return ( IsPointer() && !IsNull() );           }

	UINT GetResourceIndex( void ) const  {  return ( m_ResourceIndex );                    }
	UINT GetOwnerIndex   ( void ) const  {  return ( m_OwnerIndex );                       }
	UINT GetMagic        ( void ) const  {  return ( m_Magic );                            }
	UINT GetHandle       ( void ) const  {  return ( m_Handle );                           }
	UINT GetValidHandle  ( void ) const  {  gpassert( IsHandle() );  return ( m_Handle );  }

	inline void SetOwnerIndex( UINT index );

	template <typename T>
	void SetPointer( T ptr )
	{
		force_cast_copy( m_Handle, ptr );
		gpassert( IsPointer() );
	}

	template <typename T>
	void GetPointer( T& ptr ) const
	{
		gpassert( IsPointer() );
		force_cast_copy( ptr, m_Handle );
	}

	template <typename T>
	void GetValidPointer( T& ptr ) const
	{
		gpassert( IsValidPointer() );
		force_cast_copy( ptr, m_Handle );
	}

	operator UINT ( void ) const  {  return ( GetHandle() );  }

	inline static UINT MakeNextMagic( UINT base );

	friend bool operator != ( Handle l, Handle r );
	friend bool operator == ( Handle l, Handle r );
	friend bool operator <  ( Handle l, Handle r );

	enum
	{
		// sizes to use for bit fields
		MAX_BITS_RESOURCE  = 10,			// LSB (  0 -  9 )
		MAX_BITS_OWNER     = 10,			//     ( 10 - 19 )
		MAX_BITS_MAGIC     = 11,			//     ( 20 - 30 )
		MAX_BITS_HANDLEBIT =  1,			// MSB (      31 )

		// sizes to compare against for asserting dereferences
		MAX_RESOURCE  = ( 1 << MAX_BITS_RESOURCE  ) - 1,
		MAX_OWNER     = ( 1 << MAX_BITS_OWNER     ) - 1,
		MAX_MAGIC     = ( 1 << MAX_BITS_MAGIC     ) - 1,
		MAX_HANDLEBIT = ( 1 << MAX_BITS_HANDLEBIT ) - 1,
	};

#	if GP_DEBUG
	bool AssertValid( UINT owner, UINT resource, UINT magic );
	bool AssertValid( void );
#	endif // GP_DEBUG

private:
	// make sure this thing is a dword in size for efficiency
	COMPILER_ASSERT( (MAX_BITS_RESOURCE + MAX_BITS_OWNER + MAX_BITS_MAGIC + MAX_BITS_HANDLEBIT) == 32 );

	union
	{
		struct
		{
			unsigned m_ResourceIndex : MAX_BITS_RESOURCE;	// index into resource array for owner
			unsigned m_OwnerIndex    : MAX_BITS_OWNER   ;	// index into mgr (owner) array
			unsigned m_Magic         : MAX_BITS_MAGIC   ;	// magic number to check against structure
			unsigned m_HandleBit     : 1                ;	// most significant bit means "i'm a handle" - can't use upper 2G anyway
		};
		DWORD m_Handle;
	};

	static UINT ms_AutoMagic;	// used by automagic handle ctor
};

COMPILER_ASSERT( sizeof( Handle ) == sizeof( DWORD ) );		// all of FileSys depends on this

//////////////////////////////////////////////////////////////////////////////
// class Handle inline implementation

DECL_SELECT_ANY const Handle::Null_ Handle::Null = NULL;
DECL_SELECT_ANY UINT Handle::ms_AutoMagic = 0;

inline Handle :: Handle( UINT owner, UINT resource, UINT magic )
{
	m_OwnerIndex    = owner;
	m_ResourceIndex = resource;
	m_Magic         = magic;
	m_HandleBit     = 1;

	GPDEBUG_ONLY( AssertValid( owner, resource, magic ) );
}

inline Handle :: Handle( UINT owner, UINT resource )
{
	m_OwnerIndex    = owner;
	m_ResourceIndex = resource;
	m_Magic         = ms_AutoMagic = MakeNextMagic( ms_AutoMagic );
	m_HandleBit     = 1;

	GPDEBUG_ONLY( AssertValid( owner, resource, ms_AutoMagic ) );
}

inline void Handle :: SetOwnerIndex( UINT index )
{
	gpassert( index <= MAX_OWNER );
	m_OwnerIndex = index;
}

inline UINT Handle :: MakeNextMagic( UINT base )
{
	if ( ++base > MAX_MAGIC )
	{
		base = 1;
	}
	return ( base );
}

#if GP_DEBUG

inline bool Handle :: AssertValid( UINT owner, UINT resource, UINT magic )
{
	AssertValid();

	// check our own constraints
	gpassert( owner    <= MAX_OWNER    );
	gpassert( resource <= MAX_RESOURCE );
	gpassert( magic    <= MAX_MAGIC    );		// $ note magic must be nonzero

	return (true);
}

inline bool Handle :: AssertValid( void )
{
	// check for invalid memory as "*this"
	gpassert( !IsMemoryBadFood( m_Handle ) );

	return (true);
}

#endif // GP_DEBUG

inline bool operator != ( Handle l, Handle r )
	{  return ( l.m_Handle != r.m_Handle );  }

inline bool operator == ( Handle l, Handle r )
	{  return ( l.m_Handle == r.m_Handle );  }

inline bool operator <  ( Handle l, Handle r )
	{  return ( l.m_Handle < r.m_Handle );  }

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

#endif  // __FILESYSHANDLE_H

//////////////////////////////////////////////////////////////////////////////
