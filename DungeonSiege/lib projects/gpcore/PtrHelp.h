//////////////////////////////////////////////////////////////////////////////
//
// File     :  PtrHelp.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains functions and classes for manipulating pointers.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __PTRHELP_H
#define __PTRHELP_H

//////////////////////////////////////////////////////////////////////////////

#include "GPCore.h"

//////////////////////////////////////////////////////////////////////////////
// class RefPtr <T> declaration

template <typename T>
class RefPtr
{
public:
	SET_NO_INHERITED( RefPtr );
	typedef T DataType;

			 RefPtr( void )							{  m_Object = NULL;  }
	explicit RefPtr( const T& obj )					{  PrivateAssign( obj );  }
			 RefPtr( const ThisType& obj )			{  PrivateAssign( obj );  }
			~RefPtr( void )							{  PrivateFree();  }

// Access.

	bool IsNull( void ) const						{  return ( m_Object == NULL );  }

	DataType*       Get         ( void )			{  return ( m_Object );  }
	const DataType* GetConst    ( void ) const		{  return ( m_Object );  }

	DataType*       operator -> ( void )			{  gpassert( !IsNull() );  return ( Get() );  }
	const DataType* operator -> ( void ) const		{  gpassert( !IsNull() );  return ( GetConst() );  }

	DataType&       operator *  ( void )			{  gpassert( !IsNull() );  return ( *Get() );  }
	const DataType& operator *  ( void ) const		{  gpassert( !IsNull() );  return ( *GetConst() );  }

	operator bool( void ) const						{  return ( !IsNull() );  }

// Assignment.

	void Assign( void )								{  PrivateFree();  PrivateAssign();  }
	void Assign( const T& obj )						{  PrivateFree();  PrivateAssign( obj );  }
	void Assign( const ThisType& obj )				{  if ( m_Object != obj.m_Object )  {  PrivateFree();  PrivateAssign( obj );  }  }

	void Free( void )								{  PrivateFree();  }

	ThisType& operator = ( const T& obj )			{  Assign( obj );  return ( *this );  }
	ThisType& operator = ( const ThisType& obj )	{  Assign( obj );  return ( *this );  }

	bool operator == ( const ThisType& obj ) const	{  return ( m_Object == obj.m_Object );  }
	bool operator != ( const ThisType& obj ) const	{  return ( m_Object != obj.m_Object );  }

private:

	#include "gpmem_new_off.h"

	void PrivateAssign( void )
	{
		m_Object = (T*)( new BYTE[ sizeof( T ) + sizeof( WORD ) ] + sizeof( WORD ) );
		new ( m_Object ) T;
		RefCount() = 1;
	}

	void PrivateAssign( const T& obj )
	{
		m_Object = (T*)( new BYTE[ sizeof( T ) + sizeof( WORD ) ] + sizeof( WORD ) );
		new ( m_Object ) T( obj );
		RefCount() = 1;
	}

	#include "gpmem_new_on.h"

	void PrivateAssign( const ThisType& obj )
	{
		m_Object = obj.m_Object;
		if ( m_Object != NULL )
		{
			gpassert( RefCount() > 0 );
			gpassert( RefCount() != 0xFFFF );
			++RefCount();
		}
	}

	void PrivateFree( void )
	{
		if ( m_Object != NULL )
		{
			gpassert( RefCount() > 0 );
			if ( --RefCount() == 0 )
			{
				m_Object->~T();
				delete ( ((BYTE*)( m_Object )) - sizeof( WORD ) );
			}
			m_Object = NULL;
		}
	}

	WORD& RefCount( void )
	{
		gpassert( m_Object != NULL );
		return ( ((WORD*)( m_Object ))[ -1 ] );
	}

	T* m_Object;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __PTRHELP_H

//////////////////////////////////////////////////////////////////////////////
