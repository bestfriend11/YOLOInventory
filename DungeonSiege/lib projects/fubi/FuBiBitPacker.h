//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiBitPacker.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains a class for serializing bits in and out of a buffer.
//             Meant for super efficient RPC xfers.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBIBITPACKER_H
#define __FUBIBITPACKER_H

//////////////////////////////////////////////////////////////////////////////

#include "GpColl.h"

//////////////////////////////////////////////////////////////////////////////

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// class BitPacker declaration

class BitPacker
{
public:

// Setup.

	BitPacker( void )
	{
		m_Saving        = true;
		m_BitCount      = 0;
		m_RestoreBits   = NULL;
		m_RestoredIndex = 0;
	}

	BitPacker( const_mem_ptr mem )
	{
		m_Saving        = false;
		m_BitCount      = mem.size * 8;
		m_RestoreBits   = (const BYTE*)mem.mem;
		m_RestoredIndex = 0;
	}

   ~BitPacker( void )
	{
#		if !GP_RETAIL
		if ( IsRestoring() && (GetBitsLeft() >= 8) )
		{
			gperror( "Somebody didn't read enough bits, and I don't like leftovers!\n" );
		}
#		endif // !GP_RETAIL
	}

	bool IsSaving   ( void ) const							{  return ( m_Saving );  }
	bool IsRestoring( void ) const							{  return ( !IsSaving() );  }

	const_mem_ptr GetSavedBits( void ) const				{  gpassert( IsSaving() );  return ( const_mem_ptr( &*m_SavedBits.begin(), (m_BitCount >> 3) + ((m_BitCount & 7) ? 1 : 0) ) );  }
	const_mem_ptr ReleaseSavedBits( void*& freeThisPtr )	{  return ( m_SavedBits.release_as_mem( freeThisPtr ) );  }

	int  GetBitCount( void ) const							{  return ( m_BitCount );  }
	bool IsEmpty    ( void ) const							{  return ( GetBitCount() == 0 );  }
	int  GetBitsLeft( void ) const							{  return ( GetBitCount() - m_RestoredIndex );  }
	bool HasBitsLeft( void ) const							{  return ( GetBitsLeft() != 0 );  }

// Xfer operations.

	// raw bit xfer
	template <typename T>
	void XferRaw( T& obj )
	{
		if ( IsSaving() )
		{
			WriteBits( obj );
		}
		else
		{
			ReadBits( obj );
		}
	}

	template <typename T>
	void XferRaw( T& obj, int bits )
	{
		if ( IsSaving() )
		{
			WriteBits( &obj, bits );
		}
		else
		{
			::ZeroObject( obj );
			ReadBits( &obj, bits );
		}
	}

	void XferBits( void* obj, int bits )
	{
		if ( IsSaving() )
		{
			WriteBits( obj, bits );
		}
		else
		{
			ReadBits( obj, bits );
		}
	}

	void XferBytes( void* obj, int bytes )				{  XferBits( obj, bytes * 8 );  }

	// processed xfer
	void XferBit     ( bool& bit )						{  if ( IsSaving() )  {  WriteBit( bit );  }  else  {  bit = ReadBit();  }  }
	void XferBuffer  ( const_mem_ptr& mem, stdx::fast_vector <BYTE> & buffer, int maxLenBytes = 4 );
	void XferString  ( gpstring& str, int maxLenBytes = 4, int bitsPerChar = 7 );
	void XferString  ( gpwstring& str, int maxLenBytes = 4, int bitsPerChar = 7 );
	void XferString  ( const char*& str, gpstring& buffer, int maxLenBytes = 4, int bitsPerChar = 7 );
	void XferStringIf( const char*& str, gpstring& buffer, const char* defValue, int maxLenBytes = 4, int bitsPerChar = 7 );
	void XferFloat   ( float& f );
	void XferFloatIf ( float& f, float defValue );

	// special packed xfer
	void XferCount( DWORD& d, int bytes = 4 );
	void XferCount( int& i, int bytes = 4 )				{  XferCount( *(DWORD*)&i, bytes );  }
	void XferCount( unsigned int& i, int bytes = 4 )	{  XferCount( *(DWORD*)&i, bytes );  }

	template <typename T>
	void XferIf( T& obj, const T& defValue, int bits )
	{
		if ( IsSaving() )
		{
			if ( ::memcmp( &obj, &defValue, sizeof( T ) ) != 0 )
			{
				// changed, write it
				WriteBit( 1 );
				WriteBits( &obj, bits );
			}
			else
			{
				// no change, store 0
				WriteBit( 0 );
			}
		}
		else
		{
			// always copy in default in case we don't overwrite all bits
			obj = defValue;

			// changed, read it
			if ( ReadBit() )
			{
				ReadBits( &obj, bits );
			}
		}
	}

	template <typename T>
	void XferIf( T& obj, const T& defValue )  {  XferIf( obj, defValue, sizeof( T ) * 8 );  }

	template <typename T>
	void XferIf( T& obj )  {  XferIf( obj, Traits <T>::GetDefaultValue() );  }

	template <typename T>
	void XferIf2( T& obj, const T& defValue1, const T& defValue2, int bits )
	{
		if ( IsSaving() )
		{
			if ( ::memcmp( &obj, &defValue1, sizeof( T ) ) != 0 )
			{
				WriteBit( 1 );

				if ( ::memcmp( &obj, &defValue2, sizeof( T ) ) != 0 )
				{
					// changed, write it
					WriteBit( 1 );
					WriteBits( &obj, bits );
				}
				else
				{
					// no change, store 0
					WriteBit( 0 );
				}
			}
			else
			{
				// no change, store 0
				WriteBit( 0 );
			}
		}
		else
		{
			// always copy in default in case we don't overwrite all bits
			obj = defValue1;

			// changed, read it
			if ( ReadBit() )
			{
				// changed still?
				if ( ReadBit() )
				{
					ReadBits( &obj, bits );
				}
				else
				{
					// second default
					obj = defValue2;
				}
			}
		}
	}

	template <typename T>
	void XferIf2( T& obj, const T& defValue1, const T& defValue2 )  {  XferIf2( obj, defValue1, defValue2, sizeof( T ) * 8 );  }

	template <typename T>
	void XferAsBitIf( T& obj, const T& defValue, int bytes )
	{
		if ( IsSaving() )
		{
			if ( obj != 0 )
			{
				WriteBit( 1 );

				if ( (obj != defValue) || (defValue == 0) )
				{
					gpassert( ::IsPower2( (int)obj ) );
					int index = ::GetShift( (int)obj );
					gpassert( index < (bytes * 8) );

					if ( defValue != 0 )
					{
						WriteBit( 1 );
					}
					WriteBits( &index, ::GetShift( bytes ) + 3 );
				}
				else
				{
					WriteBit( 0 );
				}
			}
			else
			{
				WriteBit( 0 );
			}
		}
		else
		{
			if ( ReadBit() )
			{
				if ( (defValue == 0) || ReadBit() )
				{
					int index = 0;
					ReadBits( &index, ::GetShift( bytes ) + 3 );
					obj = (T)(1 << index);
				}
				else
				{
					obj = defValue;
				}
			}
			else
			{
				obj = 0;
			}
		}
	}

	template <typename T>
	void XferAsBitIf( T& obj, const T& defValue )  {  XferAsBit( obj, defValue, sizeof( T ) );  }

	template <typename T>
	void XferAsBit( T& obj, int bytes )  {  XferAsBitIf( obj, 0, bytes );  }

	template <typename T>
	void XferAsBit( T& obj )  {  XferAsBit( obj, sizeof( T ) );  }

// Direct bit operations.

	bool ReadBit( void )
	{
		gpassert( IsRestoring() );

		int byteIndex = m_RestoredIndex >> 3;
		int bitIndex = m_RestoredIndex & 7;
		++m_RestoredIndex;

		if ( m_RestoredIndex <= m_BitCount )
		{
			return ( !!(m_RestoreBits[ byteIndex ] & (1 << (7 - bitIndex))) );
		}
		else
		{
			gperror( "Can't read any more bits, I'm all tapped out!\n" );
			return ( false );
		}
	}

	void ReadBits( void* mem, int bits );

	void ReadBytes( void* mem, int bytes )  {  ReadBits( mem, bytes * 8 );  }

	void ReadBits( mem_ptr mem )  {  ReadBytes( mem.mem, mem.size );  }

	template <typename T>
	void ReadBits( T& obj )  {  ReadBytes( &obj, sizeof( obj ) );  }

	void WriteBit( bool bit )
	{
		gpassert( IsSaving() );
		m_SavedBits.resize( (m_BitCount >> 3) + 1, 0 );
		writeBitQuick( bit );
	}

	void WriteBits( const void* mem, int bits );

	void WriteBytes( const void* mem, int bytes )  {  WriteBits( mem, bytes * 8 );  }

	void WriteBits( const_mem_ptr mem )  {  WriteBytes( mem.mem, mem.size );  }

	template <typename T>
	void WriteBits( const T& obj )  {  WriteBytes( &obj, sizeof( obj ) );  }

private:
	void writeBitQuick( bool bit )
	{
		gpassert( IsSaving() );

		int byteIndex = m_BitCount >> 3;
		int bitIndex = m_BitCount & 7;
		++m_BitCount;

		gpassert( byteIndex < (int)m_SavedBits.size() );
		if ( bit )
		{
			m_SavedBits[ byteIndex ] |= 1 << (7 - bitIndex);
		}
	}

	typedef stdx::fast_vector <BYTE> Bits;

	// general info
	bool m_Saving;					// true if saving, false if restoring
	int  m_BitCount;				// how many bits we've used in data

	// saving
	Bits m_SavedBits;				// our data for saving

	// restoring
	const BYTE* m_RestoreBits;		// our data for restoring
	int         m_RestoredIndex;	// index of current bit when reading
};

//////////////////////////////////////////////////////////////////////////////
// helper macros

// xfer a bool stored as a bitfield in a struct
#define FUBI_XFER_PACKED_BITFIELD_BOOL( PERSIST, VAR )	\
	{													\
		bool FUBI_Var = VAR;							\
		PERSIST.XferBit( FUBI_Var );					\
		VAR = FUBI_Var;									\
	}

// figure out the max # bits required to store an enum (assuming it's only
// composed of positive numbers!!)
inline int GetMaxEnumBits( int begin, int end )
{
	gpassert( begin < end );
	gpassert( begin >= 0 );

	return ( ::GetShift( end - 1 ) + 1 );
}
#define FUBI_MAX_ENUM_BITS( PREFIX )  FuBi::GetMaxEnumBits( (int)PREFIX##BEGIN, (int)PREFIX##END )

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////

#endif  // __FUBIBITPACKER_H

//////////////////////////////////////////////////////////////////////////////
