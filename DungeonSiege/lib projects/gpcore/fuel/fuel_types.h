#pragma once
/*========================================================================

  Types used by Fuel

------------------------------------------------------------------------*/
#ifndef __FUEL_TYPES_H
#define __FUEL_TYPES_H

//#define DEBUG_FUEL 1
//#define DEBUG_FUEL_CACHE 1

class TextFuelDB;
class BinaryFuelDB;
class FuelBlock;
class FastFuelHandle;
class FastFuelFindHandle;
class FuelBlock;
class FuelDB;
class TextFuelDB;
class BinaryFuelDB;
class FuelHandle;
struct SiegePos;
struct Quat;


namespace FileSys
{
	class StreamWriter;
};

namespace BinFuel
{
	struct Header;
	struct DictionaryHeader;
};

enum eFuelValueProperty;

extern const DWORD BINARY_FUEL_VERSION;
#define BINARY_FUEL_ID 'DQL.'
extern const DWORD MAX_NUM_BINARY_DBS;

#include <list>
#include <map>
#include "gpcoll.h"

typedef std::vector< BYTE > FuelByteBuffer;
typedef std::list<FuelBlock*> fuel_block_list;
typedef stdx::fast_vector< BinFuel::Header * > HeaderColl;

#include "stringtool.h"
#include "FuBiDefs.h"


namespace BinFuel
{
	struct Header;

	/*
		BlockHeader
		KeyValueInfo 0
		KeyValueInfo ...
		KeyValueInfo N
		ChildInfo 0
		ChildInfo ...
		ChildInfo N
		DataBlock
		...
		BlockHeader - child 0
		...
		BlockHeader - child N
	*/

	extern BinFuel::DictionaryHeader * GetDictionary();
	extern void SetDictionaryPath( const char * path );
	extern void FreeDictionary();


	////////////////////////////////////////////////////////////////////////////////
	//	enums


	enum eBlockProperties
	{
		// static
		BP_NAME_IN_DICTIONARY		= 1 << 0,
		BP_TYPE_IN_DICTIONARY		= 1 << 1,
		BP_IS_DIRECTORY				= 1 << 2,
		// dynamic	
		BP_IS_DEV_ONLY				= 1 << 3,
		BP_IS_STUB					= 1 << 4,
	};

	MAKE_ENUM_BIT_OPERATORS( eBlockProperties );

	enum eChildProperties
	{
		// static
		CP_IS_DIRECTORY				= 1 << 0,
		// dynamic
		CP_OFFSET_IS_POINTER		= 1 << 1,
		CP_IS_STUB					= 1 << 2,
	};

	MAKE_ENUM_BIT_OPERATORS( eChildProperties );

	enum eKeyValueProperty
	{
		KVP_KEY_IN_DICTIONARY		= 1 << 0,
		KVP_VALUE_IN_DICTIONARY		= 1 << 1,
		
		KVP_VALUE_NORMAL_INT		= 1 << 2,
		KVP_VALUE_HEX_INT			= 1 << 3,

		KVP_VALUE_FLOAT				= 1 << 4,
		KVP_VALUE_DOUBLE			= 1 << 5,
		KVP_VALUE_BOOL				= 1 << 6,
		KVP_VALUE_STRING			= 1 << 7,

		KVP_VALUE_SIEGE_POS			= 1 << 8,
		KVP_VALUE_QUAT				= 1 << 9,
		KVP_VALUE_VECTOR_3			= 1 << 10,

		KVP_VALUE_INT				= KVP_VALUE_NORMAL_INT | KVP_VALUE_HEX_INT,
		KVP_VALUE_COMPLEX_NUMERIC	= KVP_VALUE_SIEGE_POS | KVP_VALUE_QUAT | KVP_VALUE_VECTOR_3 | KVP_VALUE_DOUBLE,
		KVP_VALUE_NUMERIC			= KVP_VALUE_COMPLEX_NUMERIC | KVP_VALUE_INT | KVP_VALUE_FLOAT | KVP_VALUE_BOOL,

	};

	MAKE_ENUM_BIT_OPERATORS( eKeyValueProperty );

	
	////////////////////////////////////////////////////////////////////////////////
	//	structs

#pragma pack ( push, 1 )

	////////////////////////////////////////
	//	struct BinaryFileHeader

	struct BinaryFileHeader
	{
		enum eProperty
		{
			FP_INCLUDES_DEV_BLOCKS_NOW		= 1 << 0,
			FP_INCLUDES_DEV_BLOCKS_NORMALLY	= 1 << 1,
		};

		
		BinaryFileHeader()
		{									
			ZeroObject( *this );
			//m_Debug					= '@HF@';
			m_Id					= BINARY_FUEL_ID;
			m_Version				= BINARY_FUEL_VERSION;
		}
		//DWORD m_Debug;
		DWORD m_Id;
		DWORD m_Version;
		DWORD m_Properties;
		DWORD m_ChildPathChecksum;
	};

	////////////////////////////////////////
	//	struct DictionaryHeader

	struct DictionaryHeader
	{
		DictionaryHeader()
		{
			ZeroObject( *this );
			m_Debug					= '@HD@';
			m_Id					= BINARY_FUEL_ID;
			m_Version				= BINARY_FUEL_VERSION;
		}
		DWORD m_Debug;
		DWORD m_Id;
		DWORD m_Version;
		DWORD m_NumStrings;
	};

	////////////////////////////////////////
	//	struct Header

	struct CommonHeaderData
	{
		inline bool IsStub() const			{ return ( m_Properties & BP_IS_STUB ) != 0; }
		inline bool IsDirectory() const		{ return ( m_Properties & BP_IS_DIRECTORY ) != 0; }
		inline bool IsDirectory()			{ return ( m_Properties & BP_IS_DIRECTORY ) != 0; }
		inline bool IsDevOnly() const		{ return ( m_Properties & BP_IS_DEV_ONLY ) != 0; }
		inline bool IsEmpty() const			{ return ( m_NumKeys == 0 ) && ( m_NumChildren == 0 ); }

		inline Header * GetDirectory()
		{
			for ( CommonHeaderData * i = this ; i && !i->IsDirectory() ; i = (CommonHeaderData*)i->m_Parent )
			{
			}
			return( (Header*)i );
		}

		DWORD	 m_CacheId;				 
		Header * m_Parent;				
		DWORD    m_NameOffset;			
		DWORD    m_TypeOffset;		
		DWORD    m_NumKeys;			
		DWORD    m_NumChildren;		
		BYTE	 m_Properties;
	};
		
	struct Header : public CommonHeaderData
	{
		Header()
		{
			ZeroObject( *this );
			//m_Debug = '@HB@';
		}
	};

	struct DirHeader : public CommonHeaderData
	{
		DirHeader()
		{
			ZeroObject( *this );
			//m_Debug = '@HB@';
		}
		DWORD m_NumStubChildren;
		DWORD m_StaticChecksum;
		DWORD m_Size;
	};

	////////////////////////////////////////
	//	struct ChildInfo

	struct ChildInfo
	{
		ChildInfo()
		{
			ZeroObject( *this );
			//m_Debug = '@IC@';
		}

		inline bool IsStub() const			{ return ( m_Properties & CP_IS_STUB ) != 0; }
		inline bool IsDirectory()	const	{ return( ( m_Properties & CP_IS_DIRECTORY ) != 0 ); }

		//DWORD	m_Debug;
		DWORD	m_BlockHeaderOffset;
		BYTE	m_Properties;
	};

	////////////////////////////////////////
	//	struct KeyValueInfo

	struct KeyValueInfo
	{
		KeyValueInfo()
		{
			ZeroObject( *this );
			//m_Debug = '@IK@';
		}

		//DWORD m_Debug;
		DWORD m_KeyOffset;
		DWORD m_ValueOrOffset;	// or the value itself if it's an int, float or bool
		SHORT m_Properties;
	};

#pragma pack ( pop )

	////////////////////////////////////////////////////////////////////////////////
	//	data storage helpers

	inline DWORD Store( const void* mem, size_t size, FuelByteBuffer & out, DWORD bufferStartOffset )
	{
		gpassert( (mem != NULL) || (size == 0) );

		DWORD sizeBefore = out.size();

		if ( size != 0 )
		{
			const BYTE* memb = rcast <const BYTE*> ( mem );
			out.insert( out.end(), memb, memb + size );
		}
		return sizeBefore - bufferStartOffset;
	}

	inline DWORD Store( const char * mem, FuelByteBuffer & out, DWORD bufferStartOffset )
	{
		DWORD sizeBefore = out.size();
		Store( mem, strlen( mem ), out, bufferStartOffset );
		out.push_back( 0 );
		return sizeBefore - bufferStartOffset;
	}

	template <typename T>
	DWORD Store( const T& obj, FuelByteBuffer & out, DWORD bufferStartOffset )
	{
		DWORD sizeBefore = out.size();
		out.insert( out.end(), rcast <const BYTE*> ( &obj ), rcast <const BYTE*> ( &obj + 1 ) );
		return sizeBefore - bufferStartOffset;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	data retrieval helpers

	const char * GetDictionaryString( BinFuel::DictionaryHeader * dict, DWORD n );

	
	////////////////////////////////////////////////////////////////////////////////
	//	
	inline KeyValueInfo * GetKeyValueInfo( Header const * header, DWORD n )
	{
		return (KeyValueInfo *)	(
								((BYTE*)header) +
								( header->IsDirectory() ? sizeof( DirHeader ) : sizeof( Header ) ) +
								(sizeof( KeyValueInfo ) * n )
								);
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline ChildInfo * GetChildInfo( Header const * header, DWORD n )
	{
		return (ChildInfo *)	(
								((BYTE*)header) +
								( header->IsDirectory() ? sizeof( DirHeader ) : sizeof( Header ) ) +
								( header->m_NumKeys * sizeof( KeyValueInfo ) ) +
								( sizeof( ChildInfo ) * n )
								);
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline const char * GetName( Header const * block )
	{
		if( block->m_Properties & BP_IS_STUB )
		{
			return ((gpstring*)block->m_NameOffset)->c_str();
		}
		else if( block->m_Properties & BP_NAME_IN_DICTIONARY )
		{
			gpassert( 0 );
			return NULL;
		}
		else
		{
			return (const char*)( ((const char*)block) + block->m_NameOffset );
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline const char * GetType( Header const * block )
	{
		if( block->m_Properties & BP_IS_STUB )
		{
			return gpstring::EMPTY.c_str();
		}
		else if( block->m_Properties & BP_NAME_IN_DICTIONARY )
		{
			gpassert( 0 );
			return NULL;
		}
		else
		{
			return (const char*)( ((const char*)block) + block->m_TypeOffset );
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	Header * GetChild( Header const * parent, DWORD n );

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline const char * GetChildName( Header const * parent, DWORD n )
	{
		return GetName( GetChild( parent, n ) );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline const char * GetChildType( Header const * parent, DWORD n )
	{
		return GetType( GetChild( parent, n ) );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline Header * BSearchGetChildNamed( Header const * parent, const char * name, DWORD min, DWORD max, DWORD * location = NULL )
	{
		if( parent->m_NumChildren == 0 )
		{
			return NULL;
		}

		for( ; ; )
		{
			// special-case last iteration of search

			if( max == ( min + 1 ) )
			{
				if( stricmp( GetChildName( parent, min ), name ) == 0 )
				{
					if( location )
					{
						*location = min;
					}
					return GetChild( parent, min );
				}
				else if( stricmp( GetChildName( parent, max ), name ) == 0 )
				{
					if( location )
					{
						*location = max;
					}
					return GetChild( parent, max );
				}
				else
				{
					return NULL;
				}
			}
			else if( min == max )
			{
				if( stricmp( GetChildName( parent, min ), name ) == 0 )
				{
					if( location )
					{
						*location = max;
					}
					return GetChild( parent, max );
				}
				else
				{
					return NULL;
				}
			}

			// divide, compare and repeat

			DWORD middle = (min+max) >> 1;

			DWORD equality = stricmp( GetChildName( parent, middle ), name );

			if( equality == 0 )
			{
				if( location )
				{
					*location = middle;
				}
				return GetChild( parent, middle );
			}
			else if( equality == -1 )
			{
				min = middle;
				//max = max;
			}
			else if( equality == 1 )
			{
				//min = min;
				max = middle;
				// recurse
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline Header * GetChildNamed( Header const * parent, const char * childName, DWORD ignoreProperties = 0 )
	{
		if( parent->m_NumChildren == 0 )
		{
			return NULL;
		}

		Header * header = BSearchGetChildNamed( parent, childName, 0, parent->m_NumChildren - 1 );

		if( header && ( ignoreProperties && (header->m_Properties & ignoreProperties)) )
		{
			return NULL;
		}
		return header;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline Header * BSearchGetChildTyped( Header const * parent, const char * type, DWORD min, DWORD max )
	{
		if( parent->m_NumChildren == 0 )
		{
			return NULL;
		}

		for( ; ; )
		{
			// special-case last iteration of search

			if( max == ( min + 1 ) )
			{
				if( stricmp( GetChildType( parent, min ), type ) == 0 )
				{
					return GetChild( parent, min );
				}
				else if( stricmp( GetChildType( parent, max ), type ) == 0 )
				{
					return GetChild( parent, max );
				}
				else
				{
					return NULL;
				}
			}
			else if( min == max )
			{
				if( stricmp( GetChildType( parent, min ), type ) == 0 )
				{
					return GetChild( parent, max );
				}
				else
				{
					return NULL;
				}
			}

			// divide, compare and repeat

			DWORD middle = (min+max) >> 1;
			if( stricmp( GetChildType( parent, middle ), type ) == 0 )
			{
				return GetChild( parent, middle );
			}
			else if( stricmp( GetChildType( parent, middle ), type ) == -1 )
			{
				min = middle;
				//max = max;
			}
			else if( stricmp( GetChildType( parent, middle ), type ) == 1 )
			{
				//min = min;
				max = middle;
				// recurse
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline Header * GetChildTyped( Header const * parent, const char * type, DWORD ignoreProperties = 0 )
	{
		// fast version
		if( parent->m_NumChildren == 0 )
		{
			return NULL;
		}
		Header * header = BSearchGetChildTyped( parent, type, 0, parent->m_NumChildren - 1 );
		if( header && ignoreProperties && ( header->m_Properties & ignoreProperties) )
		{
			return NULL;
		}
		else
		{
			return header;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline eKeyValueProperty GetKeyValueProperties( Header * header, DWORD n )
	{
		gpassert( n <= header->m_NumKeys );
		return scast<eKeyValueProperty>( (DWORD)( (((BYTE*)header) + GetKeyValueInfo( header, n )->m_Properties) ) );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	const char * GetKeyName( Header const * header, DWORD n );

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline KeyValueInfo * BSearchGetKeyValueInfoForKey( Header const * block, const char * name, DWORD min, DWORD max, DWORD * location = NULL )
	{
		if( block->m_NumKeys == 0 )
		{
			return NULL;
		}

		for( ; ; )
		{
			// special-case last iteration of search

			if( max == ( min + 1 ) )
			{
				if( stricmp( GetKeyName( block, min ), name ) == 0 )
				{
					if( location )
					{
						*location = min;
					}
					return GetKeyValueInfo( block, min );
				}
				else if( stricmp( GetKeyName( block, max ), name ) == 0 )
				{
					if( location )
					{
						*location = max;
					}
					return GetKeyValueInfo( block, max );
				}
				else
				{
					return NULL;
				}
			}
			else if( min == max )
			{
				if( stricmp( GetKeyName( block, min ), name ) == 0 )
				{
					if( location )
					{
						*location = max;
					}
					return GetKeyValueInfo( block, max );
				}
				else
				{
					return NULL;
				}
			}

			// divide, compare and repeat

			DWORD middle = (min+max) >> 1;
			if( stricmp( GetKeyName( block, middle ), name ) == 0 )
			{
				if( location )
				{
					*location = middle;
				}
				return GetKeyValueInfo( block, middle );
			}
			else if( stricmp( GetKeyName( block, middle ), name ) == -1 )
			{
				min = middle;
				//max = max;
			}
			else if( stricmp( GetKeyName( block, middle ), name ) == 1 )
			{
				//min = min;
				max = middle;
				// recurse
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline KeyValueInfo * GetKeyValueInfoForKey( Header const * block, const char * name, DWORD * location = NULL )
	{
		if( block->m_NumKeys == 0 )
		{
			return NULL;
		}
		return BSearchGetKeyValueInfoForKey( block, name, 0, block->m_NumKeys-1, location );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	BYTE * GetValueDataPtr( Header const * block, KeyValueInfo * info );

	////////////////////////////////////////////////////////////////////////////////
	//
	inline BYTE * GetValueDataPtrForKey( Header * block, const char * name )
	{
		KeyValueInfo * info = GetKeyValueInfoForKey( block, name );
		if( info )
		{
			return GetValueDataPtr( block, info );
		}
		return NULL;
	}

	
	////////////////////////////////////////////////////////////////////////////////
	//
	inline DWORD GetDictStringOffset( BinFuel::DictionaryHeader * dict, DWORD n )
	{
		return *(((DWORD*)(dict+1)) + n);
	}

	
	////////////////////////////////////////////////////////////////////////////////
	//
	inline const char * GetDictionaryString( BinFuel::DictionaryHeader * dict, DWORD n )
	{
		return ((const char *)dict) + GetDictStringOffset( dict, n );
	}

	
	////////////////////////////////////////////////////////////////////////////////
	//	
	inline DWORD BSearchGetDictionaryString( DictionaryHeader * dict, const char * name )
	{
		if ( dict == NULL )
		{
			gpfatal( "Fuel binary dictionary missing!!!\n" );
		}

		if( dict->m_NumStrings == 0 )
		{
			gpassert( 0 );
			return NULL;
		}

		DWORD min = 0;
		DWORD max = dict->m_NumStrings-1;

		for( ; ; )
		{
			// special-case last iteration of search

			if( max == ( min + 1 ) )
			{
				if( strcmp( GetDictionaryString( dict, min ), name ) == 0 )
				{
					return min;
				}
				else if( strcmp( GetDictionaryString( dict, max ), name ) == 0 )
				{
					return max;
				}
				else
				{
					return 0;
				}
			}
			else if( min == max )
			{
				if( strcmp( GetDictionaryString( dict, min ), name ) == 0 )
				{
					return max;
				}
				else
				{
					return 0;
				}
			}

			// divide, compare and repeat

			DWORD middle = (min+max) >> 1;

			DWORD equality = strcmp( GetDictionaryString( dict, middle ), name );

			if( equality == 0 )
			{
				return middle;
			}
			else if( equality == -1 )
			{
				min = middle;
				//max = max;
			}
			else if( equality == 1 )
			{
				//min = min;
				max = middle;
				// recurse
			}
		};
	}

	void Dump( BinFuel::Header const * block, bool verbose = true, ReportSys::ContextRef ctx = NULL );
};


#endif


