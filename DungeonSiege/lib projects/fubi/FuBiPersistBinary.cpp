//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiPersistBinary.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiPersistBinary.h"
#include "ReportSys.h"

#define DEBUG_NESTING 0

#if DEBUG_NESTING
#include "ReportSys.h"
#endif // DEBUG_NESTING

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// helper macros

#define LOCAL_COPY_LOWER( name, len )					\
														\
	/* must have valid name */							\
	if ( name == NULL )									\
	{													\
		name = "";										\
	}													\
														\
	/* cache length, alloc space */						\
	int len = ::strlen( name ) + 1;						\
	char* local##name = (char*)::_alloca( len );		\
														\
	/* lowercase it */									\
	::strlwr_copy( local##name, name );					\
	name = local##name;

//////////////////////////////////////////////////////////////////////////////
// namespace TreeBinaryRaw implementation

namespace TreeBinaryRaw  {  // begin of namespace TreeBinaryRaw

void DataHeader :: Init( void )
{
	m_ProductId     = FileSys::PRODUCT_ID;
	m_XferId        = DATA_XFER_ID;
	m_HeaderVersion = DATA_HEADER_VERSION;
	m_Flags         = DATA_FLAG_NONE;
}

void IndexHeader :: Init( void )
{
	m_ProductId     = FileSys::PRODUCT_ID;
	m_XferId        = INDEX_XFER_ID;
	m_HeaderVersion = INDEX_HEADER_VERSION;
	m_Flags         = INDEX_FLAG_NONE;
}

}  // end of namespace TreeBinaryRaw

//////////////////////////////////////////////////////////////////////////////
// class TreeBinaryReader implementation

TreeBinaryReader :: TreeBinaryReader( void )
{
	// this space intentionally left blank...
}

TreeBinaryReader :: ~TreeBinaryReader( void )
{
	// this space intentionally left blank...
}

bool TreeBinaryReader :: Init( const_mem_ptr data, const_mem_ptr index )
{
	using namespace TreeBinaryRaw;

// Check headers.

	if ( data.size < sizeof( DataHeader ) )
	{
		::SetLastError( ERROR_BAD_FORMAT );
		return ( false );
	}

	const DataHeader* dataHeader = (const DataHeader*)data.mem;
	if (   (dataHeader->m_ProductId     != FileSys::PRODUCT_ID)
		|| (dataHeader->m_XferId        != DATA_XFER_ID)
		|| (dataHeader->m_HeaderVersion != DATA_HEADER_VERSION) )
	{
		::SetLastError( ERROR_BAD_FORMAT );
		return ( false );
	}

	if ( index.size < (sizeof( IndexHeader ) + sizeof( DWORD )) )
	{
		::SetLastError( ERROR_BAD_FORMAT );
		return ( false );
	}

	const IndexHeader* indexHeader = (const IndexHeader*)index.mem;
	if (   (indexHeader->m_ProductId     != FileSys::PRODUCT_ID)
		|| (indexHeader->m_XferId        != INDEX_XFER_ID)
		|| (indexHeader->m_HeaderVersion != INDEX_HEADER_VERSION) )
	{
		::SetLastError( ERROR_BAD_FORMAT );
		return ( false );
	}

// Ok read index.

	// first take the data
	m_Data = data;

	// get a runner for the index
	const_mem_iter indexIter = index;
	indexIter.advance( sizeof( IndexHeader ) );

	// get the total block array count
	m_BlockColl.resize( indexIter.advance_dword() );

	// fill entries
	BlockColl::iterator i, ibegin = m_BlockColl.begin(), iend = m_BlockColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Block& block = *i;

		// get num values
		block.m_ValueCount = indexIter.advance_dword();

		// save pointer to value offsets
		block.m_ValueDataOffsets = (const DWORD*)indexIter.mem;

		// advance past them
		indexIter.advance( block.m_ValueCount * sizeof( DWORD ) );

		// get num subblocks
		block.m_BlockCount = indexIter.advance_dword();

		// save pointer to block infos
		block.m_BlockInfos = (const BlockChild*)indexIter.mem;

		// advance past them
		indexIter.advance( block.m_BlockCount * sizeof( BlockChild ) );
	}

	// get a root iter
	m_BlockStack.push_back( &m_BlockColl.front() );

	// done
	return ( true );
}

#if !GP_RETAIL

void TreeBinaryReader :: Dump( const char* rootName, ReportSys::ContextRef ctx ) const
{
	UNREFERENCED_PARAMETER( rootName );
	UNREFERENCED_PARAMETER( ctx );

	// gather up sizes as a preprocessing step


//$$$	outputBlock( rootName, &m_BlockColl.front(), ctx );
}

#endif // !GP_RETAIL

#if DEBUG_NESTING
ReportSys::Context& GetSaveLoadContext( void )
{
	static ReportSys::Context* gSaveLoadContextPtr = NULL;
	if ( gSaveLoadContextPtr == NULL )
	{
		static ReportSys::Context s_SaveLoadContext( "SaveLoad" );
		gSaveLoadContextPtr = &s_SaveLoadContext;

		s_SaveLoadContext.AddSink( new ReportSys::LogFileSink <> ( "SaveLoad.log" ), true );
		s_SaveLoadContext.SetType( "Log" );
	}
	return ( *gSaveLoadContextPtr );
}
#define gSaveLoadContext GetSaveLoadContext()
#endif // DEBUG_NESTING

bool TreeBinaryReader :: EnterBlock( const char* name )
{
	// get our block iterator and current block
	BlockIter& iter = m_BlockStack.back();
	Block* block = iter.m_Block;
	bool found = false;

	// does this block have children?
	if ( block->m_BlockCount != 0 )
	{
		const int LSEARCH_CUTOFF = 20;

		// get a valid local lowercase name
		LOCAL_COPY_LOWER( name, len );

		// try cache first
		if ( !block->m_BlocksOwned )
		{
			// let's bet it will be the current iter
			const char* blockStr = (const char*)m_Data.mem + iter.m_NextBlock->m_DataOffset;

			// match?
			if ( same_with_case( blockStr, name ) )
			{
				// found!
				found = true;
			}
			else if ( (block->m_BlockCount > 1) && (block->m_BlockCount <= LSEARCH_CUTOFF) )
			{
				// not that many, just do an lsearch, start at the top and find it
				const BlockChild* i, * ibegin = block->m_BlockInfos, * iend = ibegin + block->m_BlockCount;
				for ( i = ibegin ; i != iend ; ++i )
				{
					blockStr = (const char*)m_Data.mem + i->m_DataOffset;
					if ( same_with_case( blockStr, name ) )
					{
						// found it!
						iter.m_NextBlock = i;
						found = true;
						break;
					}
				}
			}

			// if found, use it
			if ( found )
			{
				// look up the block
				Block* block = &m_BlockColl[ iter.m_NextBlock->m_BlockIndex ];

				// advance the pointer
				iter.AdvanceBlockIter();

				// push it on the stack (this will invalidate the iter btw)
				m_BlockStack.push_back( block );
			}
		}

		// not found? must bsearch (only bother if more than one entry)
		if ( !found && (block->m_BlockCount > LSEARCH_CUTOFF) )
		{
			// build sorted array if necessary
			if ( !block->m_BlocksOwned )
			{
				// alloc space and copy
				BlockChild* newBlockInfos = new BlockChild[ block->m_BlockCount ];
				::memcpy( newBlockInfos, block->m_BlockInfos, sizeof( BlockChild ) * block->m_BlockCount );

				// sort
				BlockChild* begin = newBlockInfos, * end = newBlockInfos + block->m_BlockCount;
				std::sort( begin, end, LessByIndirectName( m_Data ) );

				// now we own
				block->m_BlockInfos = newBlockInfos;
				block->m_BlocksOwned = true;
			}

			// now bsearch it
			const BlockChild* begin = block->m_BlockInfos, * end = block->m_BlockInfos + block->m_BlockCount;
			const BlockChild* child = stdx::binary_search( begin, end, name, LessByIndirectName( m_Data ) );
			if ( child != end )
			{
				// found!
				found = true;

				// look up the block
				Block* block = &m_BlockColl[ child->m_BlockIndex ];

				// push it on the stack (this will invalidate the iter btw)
				m_BlockStack.push_back( block );
			}
		}
	}

#	if DEBUG_NESTING
	if ( found )
	{
		gSaveLoadContext.OutputF( ">%s\n", name ? name : "(null)" );
		gSaveLoadContext.Indent();
	}
	else
	{
		gSaveLoadContext.OutputF( "!%s\n", name ? name : "(null)" );
	}
#	endif // DEBUG_NESTING

	// done
	return ( found );
}

bool TreeBinaryReader :: LeaveBlock( void )
{
#	if DEBUG_NESTING
	gSaveLoadContext.Outdent();
	gSaveLoadContext.Output( "<\n" );
#	endif // DEBUG_NESTING

	// gee this is a hard function...
	m_BlockStack.pop_back();
	gpassert( m_BlockStack.size() >= 1 );

	return ( true );
}

bool TreeBinaryReader :: ReadString( eXfer /*xfer*/, const char* key, gpstring& value )
{
	return ( readBinary( key, NULL, &value, NULL ) );
}

bool TreeBinaryReader :: ReadString( eXfer /*xfer*/, const char* key, gpwstring& value )
{
	return ( readBinary( key, NULL, NULL, &value ) );
}

bool TreeBinaryReader :: ReadBinary( const char* key, mem_ptr ptr )
{
	return ( readBinary( key, &ptr, NULL, NULL ) );
}

/*$$$ COMMENTED OUT UNTIL I GET TIME TO MAKE IT WORK RIGHT
void TreeBinaryReader :: outputBlock( const char* blockName, const Block* block, ReportSys::ContextRef ctx ) const
{
	ctx->OutputF( "[%s]\n{\n", blockName );
	ctx->Indent();

	for ( DWORD ivalue = 0 ; ivalue != block->m_ValueCount ; ++ivalue )
	{
		const char* str = (const char*)m_Data.mem + block->m_ValueDataOffsets[ ivalue ];
		const char* end = (const char*)m_Data.mem + block->m_ValueDataOffsets[ ivalue + 1 ];

		ctx->OutputF( "%s = ", str );
		str += ::strlen( str ) + 1;

		bool validAnsi = true;
		for ( const char* istr = str ; istr != end ; ++istr )
		{
			ctx->OutputF( (istr != str) ? " %02x" : "%02x", *istr );
			if ( *istr & 0x80 )
			{
				validAnsi = false;
			}
		}

		if ( validAnsi )
		{
			ctx->OutputF( " // %.*s", end - str, str );
		}

		ctx->Output( "\n" );
	}

	if ( block->m_ValueCount != 0 )
	{
		ctx->Output( "\n" );
	}

	for ( DWORD iblock = 0 ; iblock != block->m_BlockCount ; ++iblock )
	{
		const BlockChild& child = block->m_BlockInfos[ iblock ];

		const char* name = (const char*)m_Data.mem + child.m_DataOffset;
		const Block* block = &m_BlockColl[ child.m_BlockIndex ];

		outputBlock( name, block, ctx );
	}

	ctx->Outdent();
	ctx->Output( "}\n" );
}
*/

bool TreeBinaryReader :: readBinary( const char* key, mem_ptr* ptr, gpstring* str, gpwstring* wstr )
{
	// get our block iterator
	BlockIter& iter = m_BlockStack.back();
	bool found = false;

	// does this block have children?
	if ( iter.m_Block->m_ValueCount != 0 )
	{
		// get a valid local lowercase name
		LOCAL_COPY_LOWER( key, len );

		// let's bet it will be the current iter
		const char* valueStr = (const char*)m_Data.mem + *iter.m_NextValue;
		int valueStrLen = ::strlen( valueStr );

		// match?
		if ( same_with_case_fast( valueStr, valueStrLen, key, len - 1 ) )
		{
			found = true;
		}
		else
		{
			// $ note that the bsearch thing that i'm doing above for block
			//   entry is generally unnecessary here. values are 99% of the time
			//   read back in the proper order, and plus they are almost always
			//   very small sets (<20 items), so lsearching is faster anyway. 

			// ok start at the top and find it
			const DWORD* i, * ibegin = iter.m_Block->m_ValueDataOffsets, * iend = ibegin + iter.m_Block->m_ValueCount;
			for ( i = ibegin ; i != iend ; ++i )
			{
				valueStr = (const char*)m_Data.mem + *i;
				valueStrLen = ::strlen( valueStr );
				if ( same_with_case_fast( valueStr, valueStrLen, key, len - 1 ) )
				{
					// found it!
					iter.m_NextValue = i;
					found = true;
					break;
				}
			}
		}

		// if found, extract data from it
		if ( found )
		{
			// data is immediately after value
			const void* data = (const BYTE*)valueStr + valueStrLen + 1;

			// data sensitive handling
			if ( ptr != NULL )
			{
				gpassert( str == NULL );
				gpassert( wstr == NULL );

				// memcpy data out
				::memcpy( ptr->mem, data, ptr->size );
			}
			else if ( str != NULL )
			{
				gpassert( wstr == NULL );
				
				// assign string directly
				*str = (const char*)data;
			}
			else
			{
				gpassert( wstr != NULL );

				// assign string directly
				*wstr = (const wchar_t*)data;
			}

			// advance the pointer
			iter.AdvanceValueIter();
		}
	}

	// done
	return ( found );
}

//////////////////////////////////////////////////////////////////////////////
// class TreeBinaryWriter implementation

TreeBinaryWriter :: TreeBinaryWriter( FileSys::StreamWriter& dataWriter )
	: m_DataWriter( dataWriter )
{
	// init vars
	m_DataOffset = 0;

	// create an empty root block and push its index on the stack
	m_BlockBuffer.push_back( m_StringBuffer );
	m_BlockStack.push_back( 0 );

	// write out header
	TreeBinaryRaw::DataHeader dataHeader;
	dataHeader.Init();
	writeData( dataHeader );
}

TreeBinaryWriter :: ~TreeBinaryWriter( void )
{
	// simple test
	gpassert( m_BlockStack.size() == 1 );
}

bool TreeBinaryWriter :: WriteIndex( FileSys::StreamWriter& indexWriter )
{
	// write out header
	TreeBinaryRaw::IndexHeader indexHeader;
	indexHeader.Init();
	if ( !indexWriter.WriteStruct( indexHeader ) )  return ( false );

	// re-sort the blocks so they are in the same order that the reader is
	// expecting them. we don't need them sorted for fast bsearch any more.
	BlockBuffer::iterator i, ibegin = m_BlockBuffer.begin(), iend = m_BlockBuffer.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->m_Blocks.resort( LessByIndex() );
	}

	// write out num blocks
	if ( !indexWriter.WriteStruct( (DWORD)m_BlockBuffer.size() ) )  return ( false );

	// $$ opt: is some buffering necessary here to speed this loop up? -sb

	// now write out the blocks in the order they were created. this is an
	// optimization for the reader, which will be assuming that they are stored
	// in the same order that the xfers are requesting.
	for ( i = ibegin ; i != iend ; ++i )
	{
		const Block& block = *i;

		// write out num values
		if ( !indexWriter.WriteStruct( (DWORD)block.m_Values.size() ) )  return ( false );

		// write out value entries
		ValueColl::const_iterator j, jbegin = block.m_Values.begin(), jend = block.m_Values.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			const ValueChild& valueChild = *j;

			// write out index of key/value in data
			if ( !indexWriter.WriteStruct( valueChild.m_DataOffset ) )  return ( false );
		}

		// write out num blocks
		if ( !indexWriter.WriteStruct( (DWORD)block.m_Blocks.size() ) )  return ( false );

		// write out block entries
		BlockColl::const_iterator k, kbegin = block.m_Blocks.begin(), kend = block.m_Blocks.end();
		for ( k = kbegin ; k != kend ; ++k )
		{
			const BlockChild& blockChild = *k;

			// write out index of name in data and index of block within array
			if ( !indexWriter.WriteStruct( blockChild.m_DataOffset ) )  return ( false );
			if ( !indexWriter.WriteStruct( blockChild.m_BlockIndex ) )  return ( false );
		}
	}

	// done!
	return ( true );
}

bool TreeBinaryWriter :: EnterBlock( const char* name )
{
	LOCAL_COPY_LOWER( name, len );

	// find a spot to stick it
	Block& top = m_BlockBuffer[ m_BlockStack.back() ];
	std::pair <BlockColl::iterator, bool> found = top.m_Blocks.find_insert( name );

	// new entry?
	if ( found.second )
	{
		// ok insert a blank
		found.first = top.m_Blocks.unsorted_insert( found.first );
		BlockChild& blockChild = *found.first;

		// add the name to allocator and set it
		StringBuffer::iterator stringIter = m_StringBuffer.insert( m_StringBuffer.end(), name, name + len );
		blockChild.m_NameIndex = stringIter - m_StringBuffer.begin();

		// write out the name and set it
		blockChild.m_DataOffset = writeData( name, len );

		// create a new block and point at it
		BlockBuffer::iterator blockIter = m_BlockBuffer.push_back( m_StringBuffer );
		int blockIndex = blockIter - m_BlockBuffer.begin();
		blockChild.m_BlockIndex = blockIndex;

		// put it on top of the stack
		m_BlockStack.push_back( blockIndex );
	}
	else
	{
		// put it on top of the stack
		m_BlockStack.push_back( found.first->m_BlockIndex );
	}

	// done!
	return ( true );
}

bool TreeBinaryWriter :: LeaveBlock( void )
{
	// gee this is a hard function...
	m_BlockStack.pop_back();
	gpassert( m_BlockStack.size() >= 1 );

	return ( true );
}

bool TreeBinaryWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpstring& value, eVarType /*type*/ )
{
	// simply write the data - reader will detect length
	return ( WriteBinary( key, const_mem_ptr( value.c_str(), value.length() + 1 ) ) );
}

bool TreeBinaryWriter :: WriteString( eXfer /*xfer*/, const char* key, const gpwstring& value )
{
	// simply write the data - reader will detect length
	return ( WriteBinary( key, const_mem_ptr( value.c_str(), (value.length() + 1) * 2 ) ) );
}

bool TreeBinaryWriter :: WriteBinary( const char* key, const_mem_ptr ptr )
{
	LOCAL_COPY_LOWER( key, len );

	// get the current block
	Block& top = m_BlockBuffer[ m_BlockStack.back() ];

	// assert that it's not already there! we are not allowed to rewrite keys
	gpassertf( std::find_if(
			top.m_Values.begin(),
			top.m_Values.end(),
			SameByIndirectName( m_StringBuffer, key ) )
		== top.m_Values.end(),
		( "Illegal to rewrite key '%s'! Your Xfer() has a duplicate!", key ));

	// ok insert a blank
	ValueChild& valueChild = *top.m_Values.push_back();

	// add the name to allocator and set it
	StringBuffer::iterator stringIter = m_StringBuffer.insert( m_StringBuffer.end(), key, key + len );
	valueChild.m_NameIndex = stringIter - m_StringBuffer.begin();

	// write out the name and set it
	valueChild.m_DataOffset = writeData( key, len );

	// finally write out the data
	writeData( ptr.mem, ptr.size );

	// done!
	return ( true );
}

DWORD TreeBinaryWriter :: writeData( const void* data, int size )
{
	DWORD old = m_DataOffset;
	m_DataWriter.Write( data, size );
	m_DataOffset += size;
	return ( old );
}

//////////////////////////////////////////////////////////////////////////////

#undef LOCAL_COPY_LOWER

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
