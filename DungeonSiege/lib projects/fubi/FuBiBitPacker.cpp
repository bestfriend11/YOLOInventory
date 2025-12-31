//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiBitPacker.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiBitPacker.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// class BitPacker implementation

void BitPacker :: XferBuffer( const_mem_ptr& mem, stdx::fast_vector <BYTE> & buffer, int maxLenBytes )
{
	XferCount( mem.size, maxLenBytes );
	if ( mem.size > 0 )
	{
		if ( IsSaving() )
		{
			WriteBits( mem );
		}
		else
		{
			buffer.resize( mem.size );
			mem.mem = &*buffer.begin();
			ReadBytes( &*buffer.begin(), mem.size );
		}
	}
}

static void XferStringMaybe( BitPacker& packer, const char*& str, gpstring& buffer, const char** defValue, int maxLenBytes, int bitsPerChar )
{
	if ( packer.IsSaving() )
	{
		// 1 bit for if it's not null
		if ( str != NULL )
		{
			packer.WriteBit( 1 );

			// 1 bit for not identical
			if ( (defValue == NULL) || (*defValue == NULL) || !::same_with_case( *defValue, str ) )
			{
				if ( defValue != NULL )
				{
					packer.WriteBit( 1 );
				}

				// write the length
				int len = ::strlen( str );
				packer.XferCount( len, maxLenBytes );

				// write the data if not empty (skip upper bit)
				for ( const char* i = str ; *i != '\0' ; ++i )
				{
					packer.WriteBits( i, bitsPerChar );
				}
			}
			else
			{
				packer.WriteBit( 0 );
			}
		}
		else
		{
			packer.WriteBit( 0 );
		}
	}
	else
	{
		// not null?
		if ( packer.ReadBit() )
		{
			// not identical?
			if ( (defValue == NULL) || packer.ReadBit() )
			{
				// get length
				int len = 0;
				packer.XferCount( len, maxLenBytes );

				// now alloc space and read data
				buffer.resize( len, 0 );
				for ( char* i = buffer.begin_split(), * end = i + len ; i != end ; ++i )
				{
					packer.ReadBits( i, bitsPerChar );
				}

				// redirect to buffer
				str = buffer;
			}
			else
			{
				buffer = *defValue;
				str = buffer;
			}
		}
		else
		{
			str = NULL;
		}
	}
}

void BitPacker :: XferString( gpwstring& str, int maxLenBytes, int bitsPerChar )
{
	if ( IsSaving() )
	{
		gpstring stra = ::ToAnsi( str );
		XferString( stra, maxLenBytes, bitsPerChar );
	}
	else
	{
		gpstring stra;
		XferString( stra, maxLenBytes, bitsPerChar );
		str = ::ToUnicode( stra );
	}
}

void BitPacker :: XferString( gpstring& str, int maxLenBytes, int bitsPerChar )
{
	const char* dummy = str;
	XferStringMaybe( *this, dummy, str, NULL, maxLenBytes, bitsPerChar );
}

void BitPacker :: XferString( const char*& str, gpstring& buffer, int maxLenBytes, int bitsPerChar )
{
	XferStringMaybe( *this, str, buffer, NULL, maxLenBytes, bitsPerChar );
}

void BitPacker :: XferStringIf( const char*& str, gpstring& buffer, const char* defValue, int maxLenBytes, int bitsPerChar )
{
	XferStringMaybe( *this, str, buffer, &defValue, maxLenBytes, bitsPerChar );
}

void BitPacker :: XferFloat( float& f )
{
	XferFloatIf( f, 0.0f );
}

void BitPacker :: XferFloatIf( float& f, float defValue )
{
	bool defSame = (defValue == 0) || (defValue == 1);

	if ( IsSaving() )
	{
		// 1 bit for not 0.0
		if ( f != 0 )
		{
			WriteBit( 1 );

			// 1 bit for not 1.0
			if ( f != 1 )
			{
				WriteBit( 1 );

				// 1 bit for not identical
				if ( defSame || (f != defValue) )
				{
					if ( !defSame )
					{
						WriteBit( 1 );
					}

					WriteBits( f );
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
			WriteBit( 0 );
		}
	}
	else
	{
		if ( ReadBit() )
		{
			if ( ReadBit() )
			{
				if ( defSame || ReadBit() )
				{
					ReadBits( f );
				}
				else
				{
					f = defValue;
				}
			}
			else
			{
				f = 1;
			}
		}
		else
		{
			f = 0;
		}
	}
}

void BitPacker :: XferCount( DWORD& d, int bytes )
{
	if ( IsSaving() )
	{
		if ( d != 0 )								// 1 bit for > 0
		{
			WriteBit( 1 );
			if ( d > 0xF )							// 1 bit for > nybble
			{
				WriteBit( 1 );
				if ( bytes > 1 )
				{
					if ( d > 0xFF )					// 1 bit for > byte
					{
						WriteBit( 1 );
						if ( bytes > 2 )
						{
							gpassert( bytes == 4 );
							if ( d > 0xFFFF )		// 1 bit for > word
							{
								WriteBit( 1 );
								WriteBits( &d, 32 );
							}
							else
							{
								WriteBit( 0 );
								WriteBits( &d, 16 );
							}
						}
						else
						{
							gpassert( bytes == 2 );
							gpassert( d <= 0xFFFF );
							WriteBits( &d, 16 );	// just write it
						}
					}
					else
					{
						WriteBit( 0 );
						WriteBits( &d, 8 );			// write byte
					}
				}
				else
				{
					gpassert( bytes == 1 );
					gpassert( d <= 0xFF );
					WriteBits( &d, 8 );				// just write it
				}
			}
			else
			{
				WriteBit( 0 );
				WriteBits( &d, 4 );					// write nybble
			}
		}
		else
		{
			WriteBit( 0 );
		}
	}
	else
	{
		d = 0;
		if ( ReadBit() )							// > 0
		{
			if ( ReadBit() )						// > nybble
			{
				if ( bytes > 1 )
				{
					if ( ReadBit() )				// > byte
					{
						if ( bytes > 2 )
						{
							gpassert( bytes == 4 );
							if ( ReadBit() )		// > word
							{
								ReadBits( &d, 32 );
							}
							else
							{
								ReadBits( &d, 16 );
							}
						}
						else
						{
							gpassert( bytes == 2 );
							ReadBits( &d, 16 );		// read word
						}
					}
					else
					{
						ReadBits( &d, 8 );			// read byte
					}
				}
				else
				{
					gpassert( bytes == 1 );
					ReadBits( &d, 8 );				// read byte
				}
			}
			else
			{
				ReadBits( &d, 4 );					// read nybble
			}
		}
	}
}

void BitPacker :: ReadBits( void* mem, int bits )
{
	gpassert( IsRestoring() );
	gpassert( mem != NULL || bits == 0 );
	gpassert( bits >= 0 );
	gpassert( m_RestoredIndex <= (m_BitCount - bits) );

	// $ not the most efficient thing in the world but wahteva

	int bytes = bits >> 3;
	bits &= 7;

	// read bytes
	while ( bytes-- )
	{
		*(BYTE*)mem = 0;
		for ( BYTE bit = 1 ; bit ; bit <<= 1 )
		{
			if ( ReadBit() )
			{
				*(BYTE*)mem |= bit;
			}
		}

		mem = (BYTE*)mem + 1;
	}

	// read trailing bits
	for ( BYTE bit = 1 ; bits-- ; bit <<= 1 )
	{
		if ( ReadBit() )
		{
			*(BYTE*)mem |= bit;
		}
		else
		{
			*(BYTE*)mem &= ~bit;
		}
	}
}

void BitPacker :: WriteBits( const void* mem, int bits )
{
	gpassert( IsSaving() );
	gpassert( mem != NULL || bits == 0 );
	gpassert( bits >= 0 );

	// $ not the most efficient thing in the world but wahteva

	m_SavedBits.resize( ((m_BitCount + bits - 1) >> 3) + 1, 0 );

	int bytes = bits >> 3;
	bits &= 7;

	// write bytes
	while ( bytes-- )
	{
		BYTE byte = *(const BYTE*)mem;
		mem = (const BYTE*)mem + 1;

		for ( int bit = 8 ; bit-- ;  )
		{
			writeBitQuick( byte & 1 );
			byte >>= 1;
		}
	}

	// write trailing bits
	if ( bits > 0 )
	{
		BYTE byte = *(const BYTE*)mem;
		while ( bits-- )
		{
			writeBitQuick( byte & 1 );
			byte >>= 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
