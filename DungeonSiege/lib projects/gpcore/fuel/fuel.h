#pragma once
/*========================================================================

  FuelBlock, part of the "Fuel" system

  Create this class to create an interface to a GAS block.  You can then
  add or remove contents from this block.  See method comments for more.

  author: Bartosz Kijanka

  date: 9/1/1999

  (c)opyright 1999, Gas Powered Games

  -----

  todo(FuelBlock and FuelDB):

  - add a reference counted lock/unlock mechanism to FuelHandles
  - fix potential bug: when SetName() is called on a block with children, all children have to have their FuelAddress recalculated and re-registered
  - have one dirty block call instead of ClientModified* Params and Children
  - dynamic buffer growth for the read and write buffers
  - support nested comments

------------------------------------------------------------------------*/
#ifndef __FUEL_H
#define __FUEL_H




#include <list>
#include <vector>

#include "Fuel_Types.h"
#include "FuelDb.h"




/*============================================================================

	FuelHandle

	This handle basically acts as a pointer to a FuelBlock.  When you
	dereference this handle, you will actually be using the interface for
	FuelBlock.  see below.

	note: If you construct a handle to a FuelBlock which does not exist, it
		  will simply point to a standard 'invalid' FuelBlock.  You can
		  test for this condition by: handle->IsValid();

----------------------------------------------------------------------------*/
class FuelHandle
{
public:

	// existence
	explicit FuelHandle( const char * block_address );

	FuelHandle( FuelBlock * block );

	FuelHandle()						{ SetBlock( NULL ); }

	~FuelHandle()						{ }

	// the assumption here is that you can't get a handle to a block unless it's valid
	bool IsValid() const				{ return( mBlock != NULL ); }

	// this will cause the pointed to block, and all blocks with which is shares a file to be reloaded
	// if the file has changes since the block was created.
	// warning: calling this will invalidate all FuelHandles which point to blocks that share a file with the
	//			block being reloaded.
	bool Reload();	// returns true if the block was indeed realoaded

	// do the same as above, but only if the block origin file has changed since the block was loaded
	bool ReloadIfDated();

	// deletes the fuel block and all it's children - note: it will be deleted from the source .gas files as well
	void Delete();

	// unloads the fuel block and all it's children
	bool Unload() const;	// a bit of a lie here, but it's not significantly changed... just cached out

	FuelBlock * operator->() const		{ gpassertm( mBlock, "Dereferencing invalid FuelHandle" );  return( mBlock ); }
	FuelBlock * GetPointer() const		{ return( mBlock ); }

	operator bool () const							{ return( IsValid() ); }
	inline bool operator < ( const FuelHandle& r )	{ return ( mBlock < r.mBlock );  }

	void SetBlock( FuelBlock * block )	{ mBlock = block; }

private:

	FuelBlock	* mBlock;
};


// this function will convert a path\dir\file.ext style file system address to
// a path:dir:file style gas address. returns empty string on failure.
gpstring FilePathToFuelAddress( const char* filePath );

// returns a fuel handle from a file path (calls FilePathToFuelAddress). returns
// invalid handle on failure.
FuelHandle     FilePathToFuelHandle    ( const char* filePath );
FastFuelHandle FilePathToFastFuelHandle( const char* filePath );

// this function will convert a path:dir style gas address to a
// path\dir\ style file system address. returns empty string on failure.
gpstring FuelAddressToFilePath( const char* fuelAddress );

// pop one level in gas path i.e. a:b:c becomes a:b
gpstring GetParentFuelAddress( gpstring const & path );

// check to see if this is fuel or path address
bool IsFuelAddress( const char* address );

typedef std::vector<FuelHandle> FuelHandleList;


//////////////////////////////////////////////////////////////////////////////
//	class FuelString declaration - lifted from Scott's GoString

#if !GP_RETAIL

	class FuelString
	{
	public:
		FuelString( void )										{  m_String = &FuelString::EMPTY_STRING;  }
		FuelString( const FuelString& str )						{  m_String = str.m_String;  }
		FuelString( const gpstring& str )						{  m_String = &FuelString::EMPTY_STRING;  Assign( str );  }
		FuelString( const gpdumbstring& str )					{  m_String = &FuelString::EMPTY_STRING;  Assign( str );  }
		FuelString( const char* str )							{  m_String = &FuelString::EMPTY_STRING;  Assign( str );  }
															
		FuelString& Assign( const char* str );				
		FuelString& Assign( const FuelString& str )				{  m_String = str.m_String;  return ( *this );  }
		FuelString& Assign( const gpstring& str );			
		FuelString& Assign( const gpdumbstring& str );		
															
		const gpdumbstring& GetString( void ) const				{  return ( *m_String );  }
		const char* c_str( void ) const							{  return ( *m_String );  }

		inline bool same_no_case( const gpstring & str ) const	{  return ( (*m_String).same_no_case( str ) ); }

		FuelString& operator = ( const FuelString& str )		{  return ( Assign( str ) );  }
		FuelString& operator = ( const gpstring& str )			{  return ( Assign( str ) );  }
		FuelString& operator = ( const gpdumbstring& str )		{  return ( Assign( str ) );  }
		FuelString& operator = ( const char* str )				{  return ( Assign( str ) );  }
																
		operator const gpdumbstring& ( void ) const				{  return ( GetString() );  }

		inline operator const char* ( void ) const;

	private:

		const gpdumbstring * m_String;
		static gpdumbstring EMPTY_STRING;
	};

	inline FuelString :: operator const char* ( void ) const
	{
		return ( m_String->c_str() );
	}
#else

#define FuelString gpstring

#endif

	
/////////////////////////////////////////////////////////////////////////////
//	enum eFuelValueProperty

enum eFuelValueProperty
{
	FVP_NONE						=	   0,
	FVP_NORMAL_INT					= 1 << 1,		// value string represents a decimal integer
	FVP_HEX_INT						= 1 << 2,		// value string represents a hexadecimal integer
	FVP_FLOAT						= 1 << 3,		// value string represents a floating-point number
	FVP_DOUBLE						= 1 << 4,		// value string represents a floating-point number
	FVP_BOOL						= 1 << 5,
	FVP_STRING						= 1 << 6,		// normal value - leading and trailing white-space is removed
	FVP_WIDE_STRING					= 1 << 7,
	FVP_QUOTED_STRING				= 1 << 8,		// quoted or bracketed value
	FVP_BRACKETED_STRING			= 1 << 9,		// quoted or bracketed value
	FVP_SIEGE_POS					= 1 << 10,		// value string represents a floating-point number
	FVP_QUAT						= 1 << 11,
	FVP_VECTOR_3					= 1 << 12,
									
	FVP_INT							= FVP_NORMAL_INT | FVP_HEX_INT,
	FVP_STRING_PROPERTIES			= FVP_STRING | FVP_QUOTED_STRING | FVP_BRACKETED_STRING,
	FVP_NUMERIC_PROPERTIES			= FVP_INT | FVP_FLOAT | FVP_DOUBLE | FVP_BOOL | FVP_SIEGE_POS | FVP_QUAT | FVP_VECTOR_3,

	FVP_READ						= 1 << 30,		// value has been read by client
	FVP_DIRTY						= 1 << 31,		// value has changed since being loaded
								
	FVP_ALL							= 0xffffffff,
};

MAKE_ENUM_BIT_OPERATORS( eFuelValueProperty );

eFuelValueProperty GetFVP( const char * );
eFuelValueProperty GetFVP( const gpstring & );
eFuelValueProperty GetFVP( const gpwstring & );
eFuelValueProperty GetFVP( int );
eFuelValueProperty GetFVP( unsigned int );
eFuelValueProperty GetFVP( unsigned long );
eFuelValueProperty GetFVP( float );
eFuelValueProperty GetFVP( double );
eFuelValueProperty GetFVP( bool );
eFuelValueProperty GetFVP( const SiegePos & );
eFuelValueProperty GetFVP( const SiegePosData & );
eFuelValueProperty GetFVP( const Quat & );


/////////////////////////////////////////////////////////////////////////////
//	struct ValueData declaration

struct ValueData
{
	inline ValueData( gpdumbstring const & s, eFuelValueProperty p )
		: m_String( s )
	{
		m_Properties	= p;
	};

	inline ValueData( const char * s, eFuelValueProperty p )
		: m_String( s )
	{
		m_Properties	= p;
	};

	FuelString			m_String;
	eFuelValueProperty	m_Properties;
};

typedef std::multimap< FuelString, ValueData, istring_less > BlockDataMap;


/*============================================================================

	FuelBlock

	This is the actual interface you are using when you're using the FuelHandle.

----------------------------------------------------------------------------*/
class FuelBlock
{

public:

	//================================================
	// read
	//------------------------------------------------

	const char * GetType() const					{ return( mType ); }
	bool HasType() const							{ return( *mType != '\0' ); }
	const char * GetName() const					{ return( mName ); }
	gpstring GetAddress( bool fullAddress = false ) const;

	inline bool IsLoaded() const					{ return( mIsLoaded ); }
	inline bool IsDirectory() const					{ return( mIsDirectory ); }
	bool IsDirty() const							{ return( mIsDirty ); }
	bool IsDevOnly() const							{ return( mIsDevOnly ); }

	gpstring const & GetOriginPath() const;
	const FILETIME & GetOriginFileTime() const		{ return( mOriginFileTime ); }
	bool HasOriginFileChanged() const;

	void SetChildPathChecksum( DWORD sum )			{ mChildPathChecksum = sum; }
	DWORD GetChildPathChecksum()					{ return mChildPathChecksum; }

	// read parameter value
	template <typename T> eFuelValueProperty Get( char const * key, T & out, bool optional )
	{
		const char* value;
		eFuelValueProperty result = Get( key, value, optional );
		if ( result )
		{
			stringtool::Get( value, out );
		}
		return ( result );
	}

	// default: optional is true (this separate function is to work around a compiler default param bug)
	template <typename T> eFuelValueProperty Get( char const * key, T & out )
	{  
		return ( Get( key, out, true ) );
	}

	// specializations
	eFuelValueProperty Get( char const * key, const char*& out, bool optional );
	eFuelValueProperty Get( char const * key, gpstring & out, bool optional );
	eFuelValueProperty Get( char const * key, gpwstring & out, bool optional );

	// read parameter value with default value (implies optional)
	gpstring  GetString ( const char * key, const gpstring&  defValue = gpstring::EMPTY );
	gpwstring GetWString( const char * key, const gpwstring& defValue = gpwstring::EMPTY );
	int       GetInt    ( const char * key, int              defValue = 0 );
	float     GetFloat  ( const char * key, float            defValue = 0 );
	double    GetDouble ( const char * key, double           defValue = 0 );
	bool      GetBool   ( const char * key, bool             defValue = true );

	// for getting wild-card parameters
	bool FindFirst( const char * wildcard );
	bool FindFirst( const eFuelValueProperty fvp );

	template <typename T> eFuelValueProperty GetNext( T & out )
	{
		const char* value;
		eFuelValueProperty result = GetNext( value );
		if ( result )
		{
			return ( stringtool::Get( value, out ) );
		}
		return ( result );
	}

	eFuelValueProperty GetNext( const char*& out );
	eFuelValueProperty GetNext( gpstring & out );
	eFuelValueProperty GetNext( gpwstring & out );

	// this is for stepping through all the key, value FuelBlock pairs without knowing what they are beforehand
	bool FindFirstKeyAndValue();
	eFuelValueProperty GetNextKeyAndValue( const char*& key, const char*& value );
	eFuelValueProperty GetNextKeyAndValue( gpstring & key, const char*& value );
	eFuelValueProperty GetNextKeyAndValue( gpstring & key, gpstring & value );
	eFuelValueProperty GetNextKeyAndValue( gpstring & key, gpwstring & value );
	eFuelValueProperty SkipNextKeyAndValue();
	int  GetKeyAndValueCount() const				{ return ( scast <int> ( mBody.size() ) ); }

	// got it?
	bool HasKey( char const* key ) const;

	//-----  find blocks
	FuelHandle GetParentBlock()						{ return mParent; }
	FuelBlock * GetParent()							{ return mParent; }

	// this till return the -first- child block of said name
	FuelHandle GetChildBlock( const char * name, bool create_if_not_found=false );
	bool GetChildBlock( const char * name, FuelHandle & hChild );
	bool HasChildBlock( const char * name );

	bool HasChildBlockDumb( const char * name );		// only for fueldb use
	bool HasChildDirBlockDumb( const char * name );		// only for fueldb use

	int  GetChildCount() const						{ return ( scast <int> ( mChildren.size() ) ); }

	// this will return the -first- child block of said type
	bool GetChildBlockOfType( const char * block_type, FuelHandle & hChildBlock );

	// parameters:
	//
	// recursion_depth_limit - a recursion depth limit of -1 means full recursion.  By default the limit is 1,
	// which means you will only list the immediate children of the block youre querying
	// type - include only blocks of said type in list... empty string means include all block types

	FuelHandleList ListChildBlocks(		int recursion_depth_limit=1,
										const char * type = 0,
										const char * name = 0,
										bool sort_results = false );

	// description: same as above, except it will append the output to the output list
	// result: true/false for child blocks were found
	bool ListChildBlocks(	const int recursion_depth_limit,
							const char * type,
							const char * name,
							FuelHandleList & OutputList );

	void NLListAllChildren( fuel_block_list & out );


	////////////////////////////////////////////////////////////////////////////////
	//	Write

	bool SetType( const char * type );

	// Change the block name, which will in turn change it's address - use with extreme caution.
	// Should only really be used by content creation tools.
	bool SetName( gpstring const & name );

	////////////////////////////////////////////////////////////////////////////////
	//	SET value

	// Sets a value for a key,value pair of matching key, if multiple matching keys exist, you don't know which key,value pair you're
	// actually changing.  Key,value pair will be created if it doesn't already exist.
	template <typename T> bool Set( const char * _key, T const & value, const eFuelValueProperty _fvp )
	{
		eFuelValueProperty fvp = GetFVP( value ) | _fvp;
		gpassert( fvp );

		gpstring value_string;
		stringtool::Set( value, value_string, (fvp&FVP_HEX_INT) != 0 );
		return ( Set( _key, value_string, fvp ) );
	}
	// default: fvp is FVP_NONE (this separate function is to work around a compiler default param bug)
	template <typename T> bool Set( const char * _key, T const & value )
	{
		return ( Set( _key, value, GetFVP( value ) ) );
	}
	bool Set( const char* _key, gpstring const & value, const eFuelValueProperty fvp );
	bool Set( const char* _key, gpwstring const & value, const eFuelValueProperty fvp );

	////////////////////////////////////////////////////////////////////////////////
	//	ADD value

	// Simply adds a key,value pair.  Allows for multiple identical keys.
	template <typename T> bool Add( const char * _key, T const & value, const eFuelValueProperty _fvp )
	{
		eFuelValueProperty fvp = GetFVP( value ) | _fvp;
		gpassert( fvp );
		
		gpstring value_string;
		stringtool::Set( value, value_string, ( fvp&FVP_HEX_INT ) != 0 );
		return( Add( _key, value_string, fvp ) );
	}

	// default: fvp is FVP_NONE (this separate function is to work around a compiler default param bug)
	template <typename T> bool Add( char const * _key, T const & value )
	{
		return ( Add( _key, value, GetFVP( value ) ) );
	}

	bool Add( const char * _key, gpstring const & value, const eFuelValueProperty fvp );
	bool Add( const char * _key, gpwstring const & value, const eFuelValueProperty fvp );

	// removes ALL key,value parameter pairs with matching key from block body
	bool Remove( const char * key );

	// create a block within current block
	// notes: is parent block is a DirBlock, you have the option of creating this block in
	//		  it's own file in that directory.
	FuelHandle CreateChildBlock( const char * name, const char * file_name = "" );

	// create a block as a directory
	//	notes:	only directory blocks can have other directory blocks as children
	FuelHandle CreateChildDirBlock( const char * dir_name );

	// this will destroy the child and all it's children recursively
	bool DestroyAllChildBlocks( bool deleteOriginFile = true );
	bool DestroyChildBlocks( const char * name, bool deleteOriginFile = true );	// destroy all children of said name
	bool DestroyChildBlock( FuelHandle & child, bool deleteOriginFile = true  );		// destroy only child pointed to by handle, handle becomes invalid after deletion

	void ClearKeys();
	void ClearKeysAndChildren();

	// $$$ change into a map
	inline fuel_block_list & GetChildBlocks() { return( mChildren ); }

	// $$ sneaky IO
	bool AddChildren( const char * buffer, DWORD size );
	bool Write( FileSys::StreamWriter & writer );

	TextFuelDB * GetDB()			{ return m_Db; }
	TextFuelDB * GetDB() const		{ return m_Db; }

	void SortChildBlocks();

	// special preloading support
	static FUBI_EXPORT bool PreloadFuelBlock( const char* address, bool recurse );


private:

	// Set a new origin path for a fuel block.
	bool SetOrigin( gpstring const & path );

	//----- existence

	friend class TextFuelDB;
	friend class FuelHandle;


	inline FuelBlock()
	{
		m_Db							= NULL;
		mParent							= NULL;

		mIsValid						= false;
		mIsDirectory					= false;
		mChildBlocksSorted				= false;
		mIsDirty						= false;
		mIsDevOnly						= false;
		mIsLoaded						= false;

		mOriginFileTime.dwLowDateTime	= 0;
		mOriginFileTime.dwHighDateTime	= 0;
		mChildPathChecksum				= 0;

		mCurrentFVP						= FVP_NONE;
		mCurrentKeyWildcardIterator		= mBody.end();
	}

	~FuelBlock();


	//----- methods ------

	void SetDB( TextFuelDB * pDB )	{ m_Db = pDB; }


	void SetIsDirty();
	void SetIsDevOnly()				{ mIsDevOnly = true; }
	void SetIsLoaded( bool flag );

	bool Load();

	void GetChildBlocksHelper(	int recursion_depth_limit,
								const char * type,
								const char * name,
								int current_recursion_level,
								FuelHandleList & output	);

	bool AddChild( FuelBlock * child );
	bool RemoveChild( FuelBlock * child );

	//bool AddKeyAndValue( const char * key, const char * value );

	void BeginChildBlocksRead();
	void BeginParametersRead();
	void ClientModifiedBlock();
	void ClientModifiedChildBlocks();

	
	//----- data ------

	// identification
	bool mIsValid			: 1;
	bool mIsDirectory		: 1;
	bool mIsLoaded			: 1;
	bool mIsDirty			: 1;
	bool mIsDevOnly			: 1;
	bool mChildBlocksSorted	: 1;

	FuelBlock	* mParent;
	TextFuelDB	* m_Db;

	// $$$ change into a map
	fuel_block_list mChildren;

	// type, name, address, originpath are determined at object creation and shouldn't be changed later
	FuelString mType;
	FuelString mName;

	DWORD mChildPathChecksum;

	gpstring const * mOriginPath;
	FILETIME mOriginFileTime;

	// $$$ OPTIMIZE - move searches out to handle
	gpstring mKeyWildcard;
	BlockDataMap::iterator mCurrentKeyWildcardIterator;
	eFuelValueProperty mCurrentFVP;
	// $$$ end OPTIMIZE

	///////////////////////////////////////
	//	the actual key-value pairs
	BlockDataMap mBody;

	SET_NO_COPYING( FuelBlock );
};




////////////////////////////////////////////////////////////////////////////////
//	binary fuel
////////////////////////////////////////////////////////////////////////////////








/*===========================================================================

	Class		: 	FastFuelHandle

	Author(s)	: 	Bartosz Kijanka

---------------------------------------------------------------------------*/
typedef std::vector< FastFuelHandle > FastFuelHandleColl;

void Translate( FastFuelHandleColl const & in, FuelHandleList & out );



class FastFuelHandle
{

public:

	BinaryFuelDB * GetDb()			{ return m_pDb; }
	BinaryFuelDB * GetDb() const	{ return m_pDb; }

	////////////////////////////////////////////////////////////////////////////////
	//	ConditionalLock
	//
	//	Grab a lock if the data hasn't been accessed long enough to a point of there
	//	being a likelyhood that the main thread may want to unload it
	//

	class ConditionalLock
	{
	public:
			inline ConditionalLock( bool conditional, FastFuelHandle const * ph )
			   : m_Conditional( conditional )
			   , m_pDb( ph->GetDb() )
		   {
				if( m_Conditional )
				{
#					if !GP_RETAIL
					++m_Locks;
					m_pFastFuelHandle = ph;
#					endif

					m_pDb->GetCritical().Enter();

#					if !GP_RETAIL
/*
					gpgenericf((	"locked cid 0x%08x, ref 0x%04x, vis 0x%08x, curvis 0x%08x, %s\n",
									m_pFastFuelHandle->m_BlockCacheId,
									m_pDb->GetCacheSlot( m_pFastFuelHandle->m_BlockCacheId )->m_RefCount,
									m_pDb->GetCacheSlot( m_pFastFuelHandle->m_BlockCacheId )->m_VisitCount,
									m_pDb->GetCurrentVisitCount(),
									m_pDb->GetCacheSlot( m_pFastFuelHandle->m_BlockCacheId )->GetBlockHeader() ? m_pDb->GetBlockSoftAddress( m_pDb->GetCacheSlot( m_pFastFuelHandle->m_BlockCacheId ).GetBlockHeader() ).c_str() : m_pDb->GetCacheSlot( m_pFastFuelHandle->m_BlockCacheId ).m_pFuelAddress->c_str()  ));
*/
#					endif
				}
#				if !GP_RETAIL
				++m_Constructions;
#				endif
		   }

		   inline ~ConditionalLock()
		   {
			   if( m_Conditional )
			   {
					m_pDb->GetCritical().Leave();
			   }
		   }

	#if !GP_RETAIL
		static DWORD m_Constructions;
		static DWORD m_Locks;
		FastFuelHandle const * m_pFastFuelHandle;
	#endif

	private:
		BinaryFuelDB * m_pDb;
		bool m_Conditional;
	};

	////////////////////////////////////////////////////////////////////////////////
	//	
	FastFuelHandle()
	{
		m_BlockCacheId	= 0;
		m_pDb			= NULL;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	explicit FastFuelHandle( const char * addr );

	////////////////////////////////////////////////////////////////////////////////
	//	
	FastFuelHandle( BinFuel::Header * block, BinaryFuelDB * db )
	{
		m_pDb = NULL;
		m_BlockCacheId = 0;

		if( !block )
		{
			return;
		}

		m_pDb = db;

		gpassert( block );
		gpassert( db );

		ConditionalLock lock( ( block->IsStub() || ( block->IsDirectory() && ((BinFuel::DirHeader*)block)->m_NumStubChildren ) ), this );

		m_pDb->Register( block );
		m_BlockCacheId = block->m_CacheId;

		gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
		gpassert( m_BlockCacheId );
		gpassert( m_pDb );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	FastFuelHandle( FastFuelHandle const & cloneSource )
	{
		gpassert( &cloneSource != this );

		m_BlockCacheId	= cloneSource.m_BlockCacheId;
		m_pDb			= cloneSource.m_pDb;
		if( m_BlockCacheId )
		{
			gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
			::InterlockedIncrement( (LPLONG)&m_pDb->GetCacheSlot( m_BlockCacheId )->m_RefCount );
		}
	}

	FastFuelHandle & operator = ( const FastFuelHandle & fh )
	{
		gpassert( &fh != this );

		if( m_BlockCacheId )
		{
			ConditionalLock lock( ShouldGrabLock(), this );
			gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
			if( m_BlockCacheId )
			{
				// unregister handle reference with existing db
				m_pDb->Unregister( m_BlockCacheId );
			}
		}

		// take on new data and register handle reference with db
		m_BlockCacheId	= fh.m_BlockCacheId;
		m_pDb			= fh.m_pDb;

		if( m_BlockCacheId )
		{
			ConditionalLock lock( ShouldGrabLock(), this );
			gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
			::InterlockedIncrement( (LPLONG)&m_pDb->GetCacheSlot( m_BlockCacheId )->m_RefCount );
		}

		return ( *this );
	}

	~FastFuelHandle()
	{
		if( FuelSys::DoesSingletonExist() )
		{
			if( m_BlockCacheId )
			{
				//gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
				GetDb()->Unregister( m_BlockCacheId );
			}
		}
	};

	inline BinFuel::Header * GetHeader( bool resolveStub = true )				{ return GetDb()->GetCachedBlockPtr( *this, resolveStub ); }
	inline BinFuel::Header const * GetHeader( bool resolveStub = true ) const	{ return GetDb()->GetCachedBlockPtr( ccast<FastFuelHandle&>(*this), resolveStub ); }

	////////////////////////////////////////////////////////////////////////////////
	//	block identification

	inline bool 	IsValid() const			{ return m_BlockCacheId != 0; };
//	inline bool 	IsLoaded() const 		{ return !GetHeader( false )->IsStub(); }

	bool 			Unload();
	const char *	GetName() const;
	const char *	GetType() const;
	bool			HasType();

	gpstring 		GetAddress( bool fullAddress = false ) const;
	bool 			IsDirectory()         	const;

	DWORD			GetStaticChecksum() const;
	DWORD			GetRecursiveStaticChecksum();

	inline DWORD	GetNumKeys() const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		gpassert( IsValid() );
		gpassertm( !GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );
		return GetHeader()->m_NumKeys;
	}

	inline DWORD	GetNumChildren() const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		gpassert( IsValid() );
		return GetHeader()->m_NumChildren;
	}

	inline bool IsEmpty() 
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		gpassert( IsValid() );
		return GetHeader()->IsEmpty();
	}

	inline bool HasKey( const char * key ) const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		gpassert( IsValid() );
		gpassertm( !GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );
		BinFuel::KeyValueInfo * info = BinFuel::GetKeyValueInfoForKey( GetHeader(), key );
		return info != NULL;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	key,value pairs

	template <typename T> eFuelValueProperty Get( char const * key, T & out, bool optional ) const
	{
		FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

		gpassertm( !GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		// this handles only numeric OUT types
		DWORD location = 0;
		BinFuel::KeyValueInfo * info = BinFuel::GetKeyValueInfoForKey( GetHeader(), key, &location );

		if( info )
		{
			return( Get( location, out, optional ) );
		}
		else
		{
			if( !optional )
			{
				gperrorf(( "Failed to read '%s' from fuel block '%s'\n", key, GetAddress().c_str() ));
			}
		}
		return( FVP_NONE );
	}

	template <typename T> eFuelValueProperty Get( char const * key, T & out ) const
	{
		return Get( key, out, true );
	}

	eFuelValueProperty Get( char const * key, gpstring & out, bool optional = true ) const;

	
	gpstring	GetString(	const char * key, const gpstring & defValue = gpstring::EMPTY )	const;
	int			GetInt(		const char * key, int defValue = 0 ) const;
	float		GetFloat(	const char * key, float defValue = 0 ) const;
	double		GetDouble(	const char * key, double defValue = 0 ) const;
	bool		GetBool(	const char * key, bool defValue = true ) const;

	////////////////////////////////////////////////////////////////////////////////
	//	parent

	inline FastFuelHandle GetParent() const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		gpassert( IsValid() );
		return FastFuelHandle( GetHeader(false)->m_Parent, m_pDb );
	}

	inline FastFuelHandle GetDirectory() const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		for ( FastFuelHandle i = *this ; i && !i.IsDirectory() ; i = i.GetParent() )
		{
			// just iterate
		}
		return ( i );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	child blocks

	////////////////////////////////////////
	//	
	inline FastFuelHandle GetChildNamed( const char * name ) const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		const char* subPath = ::strchr( name, ':' );

		if ( subPath != NULL )
		{
			FastFuelHandle subChild = GetChildNamed( gpstring().assign( name, subPath ) );
			if( subChild )
			{
				return ( subChild.GetChildNamed( subPath + 1 ) );
			}
		}
		else
		{
			return FastFuelHandle( BinFuel::GetChildNamed( GetHeader(), name, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 ), m_pDb );
		}

		return FastFuelHandle();
	}

	////////////////////////////////////////
	//	
	inline FastFuelHandle GetChildTyped( const char * type ) const
	{
		ConditionalLock lock( ShouldGrabLock(), this );

		return FastFuelHandle( BinFuel::GetChildTyped( GetHeader(), type, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 ), m_pDb );
	}

	////////////////////////////////////////
	//	
	inline bool ListChildren( FastFuelHandleColl & out, const char * name, const char * type, const int recursionLimit = 1 )
	{
		return ListChildren( out, *this, name, type, recursionLimit, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 );
	}

	////////////////////////////////////////
	//	
	inline bool ListChildrenNamed( FastFuelHandleColl & out, const char * name, const int recursionLimit = 1 )
	{
		return ListChildren( out, *this, name, NULL, recursionLimit, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 );
	}

	////////////////////////////////////////
	//	
	inline bool ListChildrenTyped( FastFuelHandleColl & out, const char * type, const int recursionLimit = 1 )
	{
		return ListChildren( out, *this, NULL, type, recursionLimit, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 );
	}

	////////////////////////////////////////
	//	
	inline bool ListChildren( FastFuelHandleColl & out, const int recursionLimit = 1 )
	{
		return ListChildren( out, *this, NULL, NULL, recursionLimit, m_pDb->TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 );
	}

	////////////////////////////////////////
	//	util

	template <typename T> eFuelValueProperty Get( DWORD keyNum, T & out, bool optional ) const
	{

		FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

		gpassertm( !GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		gpassert( keyNum < GetHeader()->m_NumKeys );
		eFuelValueProperty outputProperties = FVP_NONE;
		BinFuel::KeyValueInfo * info = BinFuel::GetKeyValueInfo( GetHeader(), keyNum );

		if( info )
		{
			BYTE * valueData = BinFuel::GetValueDataPtr( GetHeader(), info );
			gpassert( valueData );

			outputProperties = GetFVP( out );

			if( outputProperties & FVP_NUMERIC_PROPERTIES )
			{
				if( info->m_Properties & BinFuel::KVP_VALUE_NUMERIC )
				{
					// $$$ add more translation
					out = *((T*)valueData );
				}
				else if( info->m_Properties & BinFuel::KVP_VALUE_STRING )
				{
					stringtool::Get( (const char*)valueData, out );
					// $$$ tick inefficient read
				}
				else
				{
					gpassert( 0 );
				}
			}
			else if( outputProperties & FVP_STRING_PROPERTIES )
			{
				gpassertm( 0, "This variant handled by different method." );
			}
			else
			{
				gpassert( 0 );
			}
		}
		else
		{
			if( !optional )
			{
				gperrorf(( "Failed to find key number %d in fuel block '%s'\n", keyNum, GetAddress().c_str() ));
			}
		}
		return( outputProperties );
	}

	template <typename T> eFuelValueProperty Get( DWORD keyNum, T & out ) const
	{
		return Get( keyNum, out, true );
	}

	eFuelValueProperty Get( DWORD keyNum, gpstring & out, bool optional = true ) const;
	eFuelValueProperty Get( DWORD keyNum, gpwstring & out, bool optional = true ) const;

	inline operator bool () const	{ return( IsValid() ); }

	////////////////////////////////////////////////////////////////////////////////
	//	cache

	inline bool ShouldGrabLock() const
	{
		if( m_pDb )
		{
			kerneltool::RwCritical::ReadLock lock( m_pDb->GetCacheSlotCritical() );

			BinFuel::Header * header = m_pDb->GetCacheSlot( m_BlockCacheId )->GetBlockHeader();
			return(	( header
						&& ( header->IsStub() || ( header->IsDirectory() && ((BinFuel::DirHeader*)header)->m_NumStubChildren ) ) )
					|| m_pDb->SlotAboutToExpire( m_BlockCacheId ) );
		}
		return false;
	}
/*
	inline bool ShouldGrabLock( DWORD cacheId ) const
	{
		if( m_pDb )
		{
			BinFuel::Header * header = m_pDb->HasCacheSlot( cacheId ) ? m_pDb->GetCacheSlot( cacheId ).GetBlockHeader() : 0;
			return(	( header
						&& ( header->IsStub() || ( header->IsDirectory() && ((BinFuel::DirHeader*)header)->m_NumStubChildren ) ) )
					|| m_pDb->SlotAboutToExpire( cacheId ) );
		}
		return false;
	}
*/
	////////////////////////////////////////////////////////////////////////////////
	//	debug

	void Dump( ReportSys::ContextRef ctx = NULL );


	DWORD			m_BlockCacheId;
	BinaryFuelDB *	m_pDb;

private:

	bool ListChildren( FastFuelHandleColl & out, FastFuelHandle & block, const char * name, const char * type, const int recursionLimit, DWORD ignoreProperties = 0 );

};


inline bool operator < ( const FastFuelHandle & l, const FastFuelHandle & r )
{
	if( l.m_pDb == r.m_pDb )
	{
		return l.m_BlockCacheId < r.m_BlockCacheId;
	}
	else 
	{
		return l.m_pDb < r.m_pDb;
	}
}




/*===========================================================================

	Class		: 	FastFuelFindHandle

	Author(s)	: 	Bartosz Kijanka

---------------------------------------------------------------------------*/
class FastFuelFindHandle
{

public:

	FastFuelFindHandle( FastFuelHandle & handle )
	{
		gpassertm( handle.IsValid(), "Attempted to construct a FastFuelFindHandle using an invalid FastFuelHandle." );

		m_Handle = &handle;
		m_CurrentKeyIndex = 0;
	}

	~FastFuelFindHandle(){};

	bool IsValid() { return( m_Handle && m_Handle->IsValid() ); }

	////////////////////////////////////////////////////////////////////////////////
	//	
	bool FindFirstValue( const char * key );

	////////////////////////////////////////////////////////////////////////////////
	//	
	template <typename T> eFuelValueProperty GetNextValue( T & out, bool optional )
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );

		if( (m_CurrentKeyIndex >= m_Handle->GetHeader()->m_NumKeys ) || 
			!m_FindKey.same_no_case( BinFuel::GetKeyName( m_Handle->GetHeader(), m_CurrentKeyIndex ) ) )
		{
			if( !optional )
			{
				gperrorf(( "FastFuelFindHandle for block %s failed to read non-optional next value.", m_Handle->GetAddress().c_str() ));
			}
			return FVP_NONE;
		}
		
		eFuelValueProperty result = m_Handle->Get( m_CurrentKeyIndex, out );
		++m_CurrentKeyIndex;
		return( result );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	template <typename T> eFuelValueProperty GetNextValue( T & out )
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );

		return GetNextValue( out, true );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline eFuelValueProperty GetNextValue( gpstring & out, bool optional = true )
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );
		gpassertm( !m_Handle->GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		if( (m_CurrentKeyIndex >= m_Handle->GetHeader()->m_NumKeys ) ||
			!m_FindKey.same_no_case( BinFuel::GetKeyName( m_Handle->GetHeader(), m_CurrentKeyIndex ) ) )
		{
			return FVP_NONE;
		}

		eFuelValueProperty result = m_Handle->Get( m_CurrentKeyIndex, out, optional );
		++m_CurrentKeyIndex;
		return( result );
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline int GetKeyAndValueCount()
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );
		gpassertm( !m_Handle->GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		return m_Handle->GetHeader()->m_NumKeys;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline eFuelValueProperty SkipNextKeyAndValue()
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );
		gpassertm( !m_Handle->GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		if( m_CurrentKeyIndex >= m_Handle->GetHeader()->m_NumKeys )
		{
			return FVP_NONE;
		}
		++m_CurrentKeyIndex;
		return FVP_NONE;
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	inline bool FindFirstKeyAndValue()
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );
		gpassertm( !m_Handle->GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		m_CurrentKeyIndex = 0;
		return( m_Handle->GetHeader()->m_NumKeys > 0 );
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	template <typename T> eFuelValueProperty GetNextKeyAndValue( gpstring & key, T & value )
	{
		gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );
		gpassertm( !m_Handle->GetHeader()->IsDirectory(), "Usage warning - Dir blocks don't have keys." );

		if( m_CurrentKeyIndex >= m_Handle->GetHeader()->m_NumKeys )
		{
			return FVP_NONE;
		}

		key = BinFuel::GetKeyName( m_Handle->GetHeader(), m_CurrentKeyIndex );
		eFuelValueProperty result = m_Handle->Get( m_CurrentKeyIndex, value );
		++m_CurrentKeyIndex;
		return( result );
	}


private:
	
	FastFuelHandle * m_Handle;

	DWORD		m_CurrentKeyIndex;
	gpstring	m_FindKey;

};



#endif
