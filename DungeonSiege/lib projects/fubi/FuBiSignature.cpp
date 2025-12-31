//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSignature.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiSignature.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helper function implementations

template <class T>
void OutMessage( T::eMessageType type, const char* source, const char* msg, va_list args )
{
#	if !GP_ERROR_FREE

	if ( type == T::MESSAGE_INFO )
	{
		return;
	}

// Store message.

	char buffer[ 1000 ];

	int count = ::sprintf( buffer, "FuBi::%s level %d: ", source, type );
	gpassert( count > 0 );
	int rc = ::_vsnprintf( buffer + count, ELEMENT_COUNT( buffer ) - count - 3, msg, args );
	if ( rc == -1 )
	{
		rc = ELEMENT_COUNT( buffer ) - count - 3;
	}
	count += rc;
	::strcpy( buffer + count, "\n" );

// Out message.

	switch ( type )
	{
		case ( T::MESSAGE_WARNING ):
		case ( T::MESSAGE_ERROR ):
		{
			gpwarning( buffer );
		}	break;

		case ( T::MESSAGE_FATAL ):
		{
			gperror( buffer );
		}	break;
	}

#	endif // !GP_ERROR_FREE
}

//////////////////////////////////////////////////////////////////////////////
// class SignatureScanner implementation

SignatureScanner:: SignatureScanner( void )
{
	m_Perfect = true;
}

SignatureScanner :: ~SignatureScanner( void )
{
	// this space intentionally left blank...
}

void SignatureScanner :: Reset( void )
{
	Inherited::Reset();
	m_Perfect = true;
}

void SignatureScanner :: Message( eMessageType type, const char* msg, ... )
{
	OutMessage <ThisType> ( type, "SignatureScanner", msg, va_args( msg ) );
	if ( type >= MESSAGE_WARNING )
	{
		m_Perfect = false;
	}
}

//////////////////////////////////////////////////////////////////////////////
// class SignatureParser implementation

SignatureParser :: SignatureParser( void )
{
	m_FunctionSpec = NULL;
	m_Perfect = true;
}

SignatureParser :: ~SignatureParser( void )
{
	// this space intentionally left blank...
}

void SignatureParser :: Reset( void )
{
	Inherited::Reset();
	m_FunctionSpec = NULL;
	m_TraitType = TypeSpec();
	m_Perfect = true;
	m_Scanner.Reset();
}

SignatureParser::eParse SignatureParser :: Parse( const char* text, FunctionSpec& spec )
{
	gpassert( text != NULL );

	Reset();

	eParse rc = PARSE_IGNORE;

	// ignore empty exports and embedded string literals (generally used by the
	// compiler when it has exported variables due to inlining of an exported
	// function)

	if ( (*text != '\0') && (::strchr( text, '`' ) == NULL) )
	{
		m_FunctionSpec = &spec;
		m_Scanner.Init( const_mem_ptr( text, ::strlen( text ) ) );

		switch ( Inherited::Parse() )
		{
			case ( 0 ):
			{
				if ( m_Perfect && m_Scanner.IsPerfect() )
				{
					rc = PARSE_OK;
				}
				else
				{
					rc = PARSE_ERROR;
				}
			}	break;

			case ( 1 ):  rc = PARSE_ERROR ;  break;
			case ( 2 ):  rc = PARSE_IGNORE;  break;
		}
	}
	m_FunctionSpec = NULL;

	return ( rc );
}

void SignatureParser :: Message( eMessageType type, const char* msg, ... )
{
	OutMessage <ThisType> ( type, "SignatureParser", msg, va_args( msg ) );
	if ( type >= MESSAGE_WARNING )
	{
		m_Perfect = false;
	}
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
