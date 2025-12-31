#include "precomp_gpcore.h"

#include "fuel.h"
#include "fueldb.h"
#include "ReportSys.h"
#include "stringtool.h"
#include <algorithm>
#include "vector_3.h"
#include "quat.h"
#include "FubiTraitsImpl.h"


using namespace BinFuel;


eFuelValueProperty GetFVP( const char * )			{ return( FVP_STRING ); }
eFuelValueProperty GetFVP( const gpstring & )		{ return( FVP_STRING ); }
eFuelValueProperty GetFVP( const gpwstring & )		{ return( FVP_WIDE_STRING ); }
eFuelValueProperty GetFVP( int  )					{ return( FVP_NORMAL_INT ); }
eFuelValueProperty GetFVP( unsigned int  )			{ return( FVP_NORMAL_INT ); }
eFuelValueProperty GetFVP( unsigned long )			{ return( FVP_NORMAL_INT ); }
eFuelValueProperty GetFVP( bool )					{ return( FVP_BOOL ); }
eFuelValueProperty GetFVP( float )					{ return( FVP_FLOAT ); }
eFuelValueProperty GetFVP( double )					{ return( FVP_DOUBLE ); }
eFuelValueProperty GetFVP( const SiegePos & )		{ return( FVP_SIEGE_POS ); }
eFuelValueProperty GetFVP( const SiegePosData & )	{ return( FVP_SIEGE_POS ); }
eFuelValueProperty GetFVP( const Quat & )			{ return( FVP_QUAT ); }
eFuelValueProperty GetFVP( const vector_3 & )		{ return( FVP_VECTOR_3 ); }


#if !GP_RETAIL
DWORD FastFuelHandle::ConditionalLock::m_Constructions = 0;
DWORD FastFuelHandle::ConditionalLock::m_Locks = 0;
#endif

// for sorting the child block list
struct FuelBlockPointerLess
{
	bool operator() ( const FuelBlock* f1, const FuelBlock* f2 ) const
	{
		return( ::stricmp( f1->GetName(), f2->GetName() ) < 0 );
	}
};

struct FuelHandleLess
{
	bool operator() ( const FuelHandle & f1, const FuelHandle & f2 ) const
	{
		return( ::stricmp( f1->GetName(), f2->GetName() ) < 0 );
	}
};




/*============================================================================
	FuelHandle
----------------------------------------------------------------------------*/
FuelHandle :: FuelHandle( const char * block_address )
{
	mBlock				= NULL;
	TextFuelDB * pDB	= NULL;

	if( *block_address == 0 || ( strlen( block_address ) == 0 ) )
	{
		return;
	}

	if( FuelSys::DoesSingletonExist() )
	{
		const char * relBlockAddress = NULL;

		pDB = gFuelSys.GetTextDbFromAddress( block_address, &relBlockAddress );

		if( !pDB )
		{
			pDB = gFuelSys.GetDefaultTextDb();
		}

		if( !pDB )
		{
			gperrorf(( "Failed to find fuel db for address = %s", block_address ));
			return;
		}

		if( stricmp( relBlockAddress, "root" ) == 0 )
		{
			SetBlock( pDB->GetRootBlock() );
		}
		else
		{
			gpstring lc_block_address( relBlockAddress );

			lc_block_address.to_lower();

			mBlock = pDB->FindBlockGlobal( lc_block_address );
		}
	}
}


FuelHandle :: FuelHandle( FuelBlock * block )
{
	mBlock = block;
	if( !mBlock )
	{
		gpwarning( "FuelHandle: initialized with NULL pointer\n" );
		gpassert( 0 );
		SetBlock( NULL );
	}
}



bool FuelHandle :: Reload()
{
	FuelBlock * pReloadedBlock = mBlock->GetDB()->Reload( mBlock );

	if( pReloadedBlock )
	{
		mBlock = pReloadedBlock;
	}
	else
	{
		mBlock = NULL;
	}

	return mBlock != NULL;
}




bool FuelHandle :: ReloadIfDated()
{
	if( mBlock->GetDB()->HasOriginFileChanged( mBlock ) )
	{
		FuelBlock * pReloadedBlock = mBlock->GetDB()->Reload( mBlock );

		if( pReloadedBlock )
		{
			mBlock = pReloadedBlock;
		}
		else
		{
			mBlock = NULL;
		}
	}

	return mBlock != NULL;
}


void FuelHandle :: Delete()
{
	TextFuelDB * db = mBlock->GetDB();
	db->SetInUserBlockDeleteRequestScope( true );

	if( mBlock->mParent )
	{
		mBlock->mParent->RemoveChild( mBlock );
	}

	delete mBlock;
	mBlock = NULL;

	db->SetInUserBlockDeleteRequestScope( false );
}


bool FuelHandle :: Unload() const
{
	return( mBlock->GetDB()->UnloadBlock( mBlock ) );
}


gpstring FilePathToFuelAddress( const char* filePath )
{
	gpstring fuel;

	if( *filePath == 0 )
	{
		return gpstring("root");
	}

	// copy the string over, converting \ and / to :, eliminating slash runs,
	// and remembering the last dot...
	bool slash = false;
	const char* dot = NULL;
	for ( const char* i = filePath ; *i != '\0' ; ++i )
	{
		if ( (*i == '/') || (*i == '\\') )
		{
			if ( !slash )
			{
				slash = true;
				fuel += ':';
			}
		}
		else
		{
			if ( *i == '.' )
			{
				dot = i;
			}
			slash = false;
			fuel += *i;
		}
	}

	// remove the extension if there was one
	if ( dot != NULL )
	{
		fuel.erase( dot - filePath );
	}

	// done
	return ( fuel );
}


FuelHandle FilePathToFuelHandle( const char* filePath )
{
	gpstring fuelAddress = FilePathToFuelAddress( filePath );
	return ( fuelAddress.empty() ? FuelHandle() : FuelHandle( fuelAddress ) );
}


FastFuelHandle FilePathToFastFuelHandle( const char* filePath )
{
	gpstring fuelAddress = FilePathToFuelAddress( filePath );
	return ( fuelAddress.empty() ? FastFuelHandle() : FastFuelHandle( fuelAddress ) );
}


gpstring FuelAddressToFilePath( const char* fuelAddress )
{
	gpstring path( fuelAddress );
	stringtool::ReplaceAllChars( path, ':', '\\' );
	stringtool::AppendTrailingBackslash( path );
	return ( path );
}


gpstring GetParentFuelAddress( gpstring const & path )
{
	gpstring::size_type last = path.find_last_of( ":" );

	gpstring result( path, 0, last );

	return( result );
}


bool IsFuelAddress( const char* address )
{
	// check to see if it is a fuel address
	bool isFuelAddress = ::strchr( address, ':' ) != NULL;
	if ( isFuelAddress )
	{
		// extra check: it could be unc paths or c:\path style
		if (   (::strchr( address, '/'  ) != NULL)
			|| (::strchr( address, '\\' ) != NULL) )
		{
			isFuelAddress = false;
		}
	}

	return ( isFuelAddress );
}


#if !GP_RETAIL

	//////////////////////////////////////////////////////////////////////////////
	// class FuelString implementation

	gpdumbstring FuelString::EMPTY_STRING;


	FuelString& FuelString :: Assign( const char* str )
	{
		if ( (m_String == NULL) || (m_String->c_str() != str) )
		{
			if ( str == NULL )
			{
				m_String = &EMPTY_STRING;
			}
			else
			{
				m_String = &gFuelSys.AddString( str );
			}
		}
		return ( *this );
	}


	FuelString& FuelString :: Assign( const gpstring& str )
	{
		m_String = &gFuelSys.AddString( str );
		return ( *this );
	}


	FuelString& FuelString :: Assign( const gpdumbstring& str )
	{
		m_String = &gFuelSys.AddString( str );
		return ( *this );
	}

#endif

//////////////////////////////////////////////////////////////////////////////
//	class FuelBlock implementation

gpstring FuelBlock :: GetAddress( bool fullAddress ) const
{
	gpstring address;

	if ( fullAddress && m_Db )
	{
		address += "::";
		address += m_Db->GetName();
		address += ":";
	}

	fuel_block_list blocks;

	FuelBlock * block = const_cast<FuelBlock*>(this);

	while( block && ( block != m_Db->GetRootBlock() ) )
	{
		blocks.push_back( block );
		block = block->GetParent();
	};

	fuel_block_list::reverse_iterator i;

	for( i = blocks.rbegin(); i != blocks.rend(); )
	{
		address.append( (*i)->GetName() );
		++i;
		if( i != blocks.rend() )
		{
			address.append( ":" );
		}
	}

	return address;
}


gpstring const & FuelBlock :: GetOriginPath() const
{
#if !GP_DEBUG
	if( !GetDB()->GetHomeReadPath().empty() )
	{
		gpassertm( mOriginPath->empty(), "Invalid origin path!" );
	}
#endif

	return( *mOriginPath );
}


bool FuelBlock :: HasOriginFileChanged() const
{
	return( m_Db->HasOriginFileChanged( this ) );
}


eFuelValueProperty FuelBlock :: Get( char const * key, const char*& out, bool optional )
{
	if( !IsLoaded() )
	{
		return FVP_NONE;
	}

	BlockDataMap::iterator f = mBody.find( key );

	if( f != mBody.end() )
	{
		out = f->second.m_String;
		(*f).second.m_Properties |= FVP_READ;
		return (*f).second.m_Properties;
	}
	else
	{
		if( !optional )
		{
			gperrorf(( "FuelBlock: failed to get non-optional parameter '%s' from '%s'\n", key, GetAddress().c_str() ));
		}
		return( FVP_NONE );
	}
}


eFuelValueProperty FuelBlock :: Get( char const * key, gpstring & out, bool optional )
{
	const char* value;
	eFuelValueProperty result = Get( key, value, optional );
	if ( result )
	{
		out = value;
	}

	return ( result );
}


eFuelValueProperty FuelBlock :: Get( char const * key, gpwstring & out, bool optional )
{
	const char* value;
	eFuelValueProperty result = Get( key, value, optional );
	if ( result )
	{
		out = Utf8ToUnicode( value );
	}
	return ( result );
}


gpstring FuelBlock :: GetString( const char * key, const gpstring& defValue )
{
	gpstring value;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}

gpwstring FuelBlock :: GetWString( const char * key, const gpwstring& defValue )
{
	gpwstring value;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}

int FuelBlock :: GetInt( const char * key, int defValue )
{
	int value = 0;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}

float FuelBlock :: GetFloat( const char * key, float defValue )
{
	float value = 0;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}

double FuelBlock :: GetDouble( const char * key, double defValue )
{
	double value = 0;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}

bool FuelBlock :: GetBool( const char * key, bool defValue )
{
	bool value = false;
	if ( !Get( key, value ) )
	{
		value = defValue;
	}
	return ( value );
}


bool FuelBlock :: FindFirst( const char * wildcard )
{
	if( IsLoaded() )
	{
		BeginParametersRead();
		BlockDataMap::iterator f = mBody.find( wildcard );
		if( f != mBody.end() )
		{
			mCurrentKeyWildcardIterator = f;
			mKeyWildcard	= wildcard;
			mCurrentFVP		= FVP_NONE;
			return( true );
		}
		else
		{
			mCurrentFVP = FVP_NONE;
			mKeyWildcard.clear();
			mCurrentKeyWildcardIterator = mBody.end();
			return( false );
		}
	}
	else
	{
		gpassertm( 0, "Invalid flow." );
		return( false );
	}
}


bool FuelBlock :: FindFirst( const eFuelValueProperty fvp )
{
	if( IsLoaded() )
	{
		BeginParametersRead();

		for( BlockDataMap::iterator i = mBody.begin(); i != mBody.end(); ++i )
		{
			if( ((*i).second.m_Properties & fvp) != 0 )
			{
				mCurrentKeyWildcardIterator = i;
				mCurrentFVP = fvp;
				return( true );
			}
		}
		mCurrentKeyWildcardIterator = mBody.end();
		mCurrentFVP = FVP_NONE;
		return( false );
	}
	else
	{
		gpassertm( 0, "Invalid flow." );
		return( false );
	}
}


eFuelValueProperty FuelBlock :: GetNext( const char*& out )
{
	if( !IsLoaded() )
	{
		return FVP_NONE;
	}

	if( mCurrentKeyWildcardIterator != mBody.end() )
	{
		if( mCurrentFVP == FVP_NONE )
		{
			// searching by name
			if( ((*mCurrentKeyWildcardIterator).first.same_no_case( mKeyWildcard ) ) )
			{
				out = mCurrentKeyWildcardIterator->second.m_String;
				(*mCurrentKeyWildcardIterator).second.m_Properties |= FVP_READ;
				eFuelValueProperty result = (*mCurrentKeyWildcardIterator).second.m_Properties;
				++mCurrentKeyWildcardIterator;
				return( result );
			}
		}
		else
		{
			while( mCurrentKeyWildcardIterator != mBody.end() )
			{
				if( ((*mCurrentKeyWildcardIterator).second.m_Properties & mCurrentFVP) != 0 )
				{
					out = mCurrentKeyWildcardIterator->second.m_String;
					(*mCurrentKeyWildcardIterator).second.m_Properties |= FVP_READ;
					eFuelValueProperty result = (*mCurrentKeyWildcardIterator).second.m_Properties;
					++mCurrentKeyWildcardIterator;
					return( result );
				}
				else
				{
					++mCurrentKeyWildcardIterator;
				}
			}
		}
	}

	mCurrentFVP = FVP_NONE;
	return( FVP_NONE );
}


eFuelValueProperty FuelBlock :: GetNext( gpstring & out )
{
	const char* value;
	eFuelValueProperty result = GetNext( value );
	if ( result )
	{
		out = value;
	}
	return ( result );
}


eFuelValueProperty FuelBlock :: GetNext( gpwstring & out )
{
	const char* value;
	eFuelValueProperty result = GetNext( value );
	if ( result )
	{
		out = Utf8ToUnicode( value );
	}
	return ( result );
}


bool FuelBlock :: FindFirstKeyAndValue()
{
	if( IsLoaded() )
	{
		BeginParametersRead();

		mKeyWildcard.clear();		// invalidate the other search
		mCurrentFVP	= FVP_NONE;

		mCurrentKeyWildcardIterator = mBody.begin();

		if( mCurrentKeyWildcardIterator != mBody.end() )
		{
			return( true );
		}
		else {
			return( false );
		}
	}
	else
	{
		gpassert( 0 );
		return( false );
	}
}


eFuelValueProperty FuelBlock :: GetNextKeyAndValue( const char*& key, const char*& value )
{
	if( IsLoaded() )
	{
		if( mCurrentKeyWildcardIterator != mBody.end() )
		{
			key		= mCurrentKeyWildcardIterator->first;
			value	= mCurrentKeyWildcardIterator->second.m_String;
			eFuelValueProperty result = mCurrentKeyWildcardIterator->second.m_Properties;
			++mCurrentKeyWildcardIterator;
			return( result );
		}
		else
		{
			return( FVP_NONE );
		}
	}
	else {
		gpassert( 0 );
		return( FVP_NONE );
	}
}


eFuelValueProperty FuelBlock :: GetNextKeyAndValue( gpstring & key, const char*& value )
{
	const char* ckey;
	eFuelValueProperty result = GetNextKeyAndValue( ckey, value );
	if ( result )
	{
		key = ckey;
	}
	return ( result );
}


eFuelValueProperty FuelBlock :: GetNextKeyAndValue( gpstring& key, gpstring & out )
{
	const char* value;
	eFuelValueProperty result = GetNextKeyAndValue( key, value );
	if ( result )
	{
		out = value;
	}
	return ( result );
}


eFuelValueProperty FuelBlock :: GetNextKeyAndValue( gpstring& key, gpwstring & out )
{
	const char* value;
	eFuelValueProperty result = GetNextKeyAndValue( key, value );
	if ( result )
	{
		out = Utf8ToUnicode( value );
	}
	return ( result );
}


eFuelValueProperty FuelBlock :: SkipNextKeyAndValue()
{
	if( IsLoaded() )
	{
		if( mCurrentKeyWildcardIterator != mBody.end() )
		{
			++mCurrentKeyWildcardIterator;
			return( FVP_NONE );
		}
		else
		{
			return( FVP_NONE );
		}
	}
	else {
		gpassert( 0 );
		return( FVP_NONE );
	}
}


bool FuelBlock :: HasKey( char const* key ) const
{
	return ( mBody.find( key ) != mBody.end() );
}


FuelHandle FuelBlock :: GetChildBlock( const char * block_name, bool create_if_not_found )
{
	FuelHandle found;
	if( !GetChildBlock( block_name, found ) && create_if_not_found ) {
		found = CreateChildBlock( block_name );
	}
	return ( found );
}


bool FuelBlock :: GetChildBlock( const char * block_name, FuelHandle & hChild )
{
	BeginChildBlocksRead();

	// sub-path?
	const char* subPath = ::strchr( block_name, ':' );

	if ( subPath != NULL )
	{
		FuelHandle subChild;
		if ( GetChildBlock( gpstring().assign( block_name, subPath ), subChild ) )
		{
			return ( subChild->GetChildBlock( subPath + 1, hChild ) );
		}
	}
	else
	{
		// $$$ todo: should find a better container for this so lookups can be done quickly
		fuel_block_list::iterator i;
		for( i=mChildren.begin(); i!=mChildren.end(); ++i )
		{
			if( stricmp( (*i)->GetName(), block_name ) == 0 )
			{
				hChild = *i;
				return true;
			}
		}
	}

	return false;
}


bool FuelBlock :: HasChildBlock( const char * name )
{
	BeginChildBlocksRead();

	fuel_block_list::iterator i;

	for( i=mChildren.begin(); i!=mChildren.end(); ++i )
	{
		if( stricmp( (*i)->GetName(), name ) == 0 )
		{
			return true;
		}
	}
	return false;
}


bool FuelBlock :: HasChildBlockDumb( const char * name )
{
	fuel_block_list::iterator i;

	for( i=mChildren.begin(); i!=mChildren.end(); ++i )
	{
		if( stricmp( (*i)->GetName(), name ) == 0 )
		{
			return true;
		}
	}
	return false;
}


bool FuelBlock :: HasChildDirBlockDumb( const char * name )
{
	fuel_block_list::iterator i;

	for( i=mChildren.begin(); i!=mChildren.end(); ++i )
	{
		if( (*i)->IsDirectory() && ( stricmp( (*i)->GetName(), name ) == 0 ) )
		{
			return true;
		}
	}
	return false;
}


bool FuelBlock :: GetChildBlockOfType( const char * block_type, FuelHandle & hChildBlock )
{
	BeginChildBlocksRead();

	// $$$ todo: should find a better container for this so lookups can be done quickly
	fuel_block_list::iterator i;
	for( i=mChildren.begin(); i!=mChildren.end(); ++i ) {
		if( stricmp( (*i)->GetType(), block_type ) == 0 )
		{
			hChildBlock = FuelHandle( *i );
			return( true );
		}
	}
	return( false );
}


// public
FuelHandleList FuelBlock :: ListChildBlocks( int recursion_depth_limit, const char * type, const char * name, bool sort_results )
{
	BeginChildBlocksRead();

	FuelHandleList output;

	GetChildBlocksHelper(	recursion_depth_limit,
							type,
							name,
							0,
							output );
	if( sort_results )
	{
		std::sort( output.begin(), output.end(), FuelHandleLess() );
	}
	return( output );
}


bool FuelBlock :: ListChildBlocks(	const int recursion_depth_limit,
									const char * type,
									const char * name,
									FuelHandleList & OutputList )
{
	BeginChildBlocksRead();

	unsigned int SizeBefore = OutputList.size();

	GetChildBlocksHelper(	recursion_depth_limit,
							type,
							name,
							0,
							OutputList );

	return( SizeBefore < OutputList.size() );
}


void FuelBlock :: NLListAllChildren( fuel_block_list & out )
{
	BeginChildBlocksRead();

	// $$ optimization: use explicit stack -bk

	fuel_block_list::iterator i;

	for( i = mChildren.begin(); i != mChildren.end(); ++i )
	{
		out.push_back( *i );
	}

	for( i = mChildren.begin(); i != mChildren.end(); ++i )
	{
		(*i)->NLListAllChildren( out );
	}
}


bool FuelBlock :: SetType( const char * _type )
{
	gpstring type( _type );
	type.to_lower();

	mType = type;

	ClientModifiedBlock();
	return( true );
}


bool FuelBlock :: SetName( gpstring const & name )
{
	gpstring lcName( name );
	lcName.to_lower();

	mName = lcName;

	ClientModifiedBlock();
	return( true );
}


// this stores a value of any type as a string...
bool FuelBlock :: Set( const char * _key, gpstring const & value, const eFuelValueProperty _fvp )
{
	gpassert( _fvp != FVP_NONE );

	eFuelValueProperty fvp = _fvp;

	if( ((fvp & FVP_STRING_PROPERTIES) != 0 ) && ((fvp & FVP_NUMERIC_PROPERTIES ) != 0 ) )
	{
		fvp = fvp & NOT( FVP_STRING_PROPERTIES );
	}

	gpstring key(_key);
	gpassertm( !key.empty(), "Setting a value with empty key name not a good idea." );

	if( (fvp & (FVP_QUOTED_STRING|FVP_BRACKETED_STRING)) == 0 )
	{
		key.to_lower();
	}

	/////////////////////////////////////////
	//	wildcards just get added
	if( !key.empty() && key[ key.size()-1 ]=='*' )
	{
		BlockDataMap::value_type new_entry( key, ValueData( value, (fvp|FVP_DIRTY) ) );
		mBody.insert( new_entry );
	}
	/////////////////////////////////////////
	//	non-wildcard - can't have duplicates
	else
	{
		BlockDataMap::iterator i = mBody.find( key );

		// key already exists
		if( i!=mBody.end() )
		{
			(*i).second.m_String		= value;
			(*i).second.m_Properties	|= ( fvp|FVP_DIRTY );
		}
		// key didn't exist
		else
		{
			mBody.insert( BlockDataMap::value_type( key, ValueData( value, fvp|FVP_DIRTY ) ) );
		}
	}

	ClientModifiedBlock();
	return( true );
}


bool FuelBlock :: Set( const char* _key, const gpwstring& value, const eFuelValueProperty fvp )
{
	// $ utf-8 must be quoted

	gpstring local( ::UnicodeToUtf8( value ) );

	return ( Set( _key, local, fvp|FVP_QUOTED_STRING ) );
}


bool FuelBlock :: Add( const char * _key, gpstring const & value, const eFuelValueProperty _fvp )
{
	gpassert( _fvp != FVP_NONE );

	eFuelValueProperty fvp = _fvp;

	if( ((fvp & FVP_STRING_PROPERTIES) != 0 ) && ((fvp & FVP_NUMERIC_PROPERTIES ) != 0 ) )
	{
		fvp = fvp & NOT( FVP_STRING_PROPERTIES );
	}

	int len = ::strlen( _key );
	char* key = (char*)_alloca( len + 1 );
	for ( char* i = key ; *_key ; ++_key, ++i )
	{
		*i = (char)::tolower( *_key );
	}
	*i = '\0';

	mBody.insert( BlockDataMap::value_type( key, ValueData( value, fvp ) ) );

	ClientModifiedBlock();
	return( true );
}


bool FuelBlock :: Add( const char * _key, gpwstring const & value, const eFuelValueProperty fvp )
{
	// $ utf-8 must be quoted

	gpstring local( ::UnicodeToUtf8( value ) );

	return ( Add( _key, local, fvp|FVP_QUOTED_STRING ) );
}


bool FuelBlock :: Remove( const char * key )
{
	mBody.erase( key );
	ClientModifiedBlock();
	return( true );
}


FuelHandle FuelBlock :: CreateChildBlock( const char * block_name, const char * file_name )
{
	if( ::strchr( block_name, ':' ) != NULL )
	{
		gpwarning( "Fuel: child block names may not be absolute Fuel addresses.  Relative only.\n" );
		gpassert( 0 );
		return( FuelHandle() );
	}
	else
	{
		FuelBlock * new_block = m_Db->CreateChildBlock( this, block_name, file_name );
		gpassert( new_block );
		new_block->mIsDirty = true;
		ClientModifiedChildBlocks();
		return( FuelHandle( new_block ) );
	}
}


FuelHandle FuelBlock :: CreateChildDirBlock( const char * dir_name )
{
	if( ::strchr( dir_name, ':' ) != NULL ) {
		gpwarning( "Fuel: child block names may not be absolute Fuel addresses.  Relative only.\n" );
		gpassert( 0 );
		return( FuelHandle() );
	}
	else
	{
		FuelBlock * new_block = m_Db->CreateChildDirBlock( this, dir_name );
		gpassert( new_block );
		new_block->mIsDirty = true;
		ClientModifiedChildBlocks();
		return( FuelHandle( new_block ) );
	}
}


bool FuelBlock :: DestroyAllChildBlocks( bool deleteOriginFile )
{
	m_Db->SetInUserBlockDeleteRequestScope( deleteOriginFile );

	fuel_block_list::iterator i;

	for( i = mChildren.begin(); i != mChildren.end(); ++i )
	{
		delete (*i);
	}

	mChildren.clear();

	ClientModifiedChildBlocks();

	m_Db->SetInUserBlockDeleteRequestScope( false );

	return( true );
}


/* $$$ i noticed this function was unimplemented... -sb
bool FuelBlock :: DestroyChildBlocks( const char * block_name, bool deleteOriginFile )
{
	
}
*/


bool FuelBlock :: DestroyChildBlock( FuelHandle & child, bool deleteOriginFile )
{
	m_Db->SetInUserBlockDeleteRequestScope( deleteOriginFile );

	gpassertm( child->GetParent() == this, "Parent/child link error." );

	fuel_block_list::iterator i;
	for( i = mChildren.begin(); i != mChildren.end(); ++i )
	{
		if( (*i) == child.GetPointer() )
		{
			delete (*i);
			mChildren.erase( i );
			break;
		}
	}

	ClientModifiedChildBlocks();

	m_Db->SetInUserBlockDeleteRequestScope( false );

	return( true );
}


void FuelBlock :: ClearKeys()
{
	mBody.clear();
	ClientModifiedBlock();
}


void FuelBlock :: ClearKeysAndChildren()
{
	mBody.clear();
	DestroyAllChildBlocks( false );
	ClientModifiedBlock();
}


bool FuelBlock :: AddChildren( const char * buffer, DWORD size )
{
	fuel_block_list kids;
/*
	NLListAllChildren( kids );
	gpgenericf(( "children of '%s' before adding:\n", GetAddress().c_str() ));
	for( fuel_block_list::iterator i = kids.begin(); i != kids.end(); ++i )
	{
		gpgenericf(( "addr 0x%08x name= %s\n", (*i), (*i)->GetAddress().c_str() ));
	}
*/
	FuelBlock * target = NULL;
	unsigned int offset = 0;

	bool success = m_Db->ReadBlocks(	this,
										"",
										target,
										buffer,
										size,
										*mOriginPath,
										0,
										offset );
/*
	kids.clear();
	NLListAllChildren( kids );
	gpgenericf(( "children of '%s' after adding:\n", GetAddress().c_str() ));
	for( i = kids.begin(); i != kids.end(); ++i )
	{
		gpgenericf(( "addr 0x%08x name= %s\n", (*i), (*i)->GetAddress().c_str() ));
	}
*/
	return success;
}


bool FuelBlock :: Write( FileSys::StreamWriter & writer )
{
	return m_Db->WriteBlock( writer, this, 0 );
}


bool FuelBlock :: PreloadFuelBlock( const char* address, bool recurse )
{
	FuelHandle fuel( address );

	if ( recurse && fuel )
	{
		// just force it to pull in all the data
		FuelHandleList junk = fuel->ListChildBlocks( -1, "never_get_this_type!" );
	}

	return ( fuel );
}


// private
bool FuelBlock :: SetOrigin( gpstring const & path )
{
#	ifdef GP_DEBUG
	if( this != m_Db->GetRootBlock() )
	{
		gpassertm( !path.empty(), "Invalid origin path" );
	}
#	endif

	mOriginPath = &m_Db->AddOriginPath( path );

	return( true );
}


// private
FuelBlock :: ~FuelBlock()
{
	if( !mChildren.empty() )
	{
		fuel_block_list::iterator iChild;

		// all my children are destroyed with me

		for( iChild = mChildren.begin(); iChild != mChildren.end(); ++iChild )
		{
			delete *iChild;
		}
	}

	m_Db->RemoveOriginPath( *mOriginPath );
}


// private
void FuelBlock :: SetIsDirty()
{
	mIsDirty = true;

	// parent is dirty (and its parent, etc.)
	if ( mParent )
	{
		mParent->SetIsDirty();
	}
}

void FuelBlock :: SetIsLoaded( bool flag )
{
#if !GP_RETAIL
	if( mIsDirectory && mParent )
	{
		if( !mParent->IsLoaded() )
		{
			gperrorf(( "Fuel: setting loaded state for block with unloaded parent.  block = %s\n", GetAddress().c_str() ));
		}
	}
#endif
	mIsLoaded = flag;
}

// private
bool FuelBlock :: Load()
{
	if( IsLoaded() )
	{
		return ( true );
	}
	else
	{
		if( IsDirectory() )
		{
			// directory blocks may or may not be loaded
			return ( m_Db->LoadAll( this ) );
		}
		else
		{
			gpassertm( 0, "Can't ask a non-dir block to load." );
			return ( false );
		}
	}
}


// private
void FuelBlock :: GetChildBlocksHelper(	int recursion_depth_limit,
										const char * type,
										const char * name,
										int current_recursion_level,
										FuelHandleList & output )
{
	// either recurse at -1 or >= 1, never 0
	gpassert( (recursion_depth_limit < 0) || (recursion_depth_limit >= 1) );

	BeginChildBlocksRead();

	// children are added breadth-first

	fuel_block_list::iterator i, begin = mChildren.begin(), end = mChildren.end();
	for ( i = begin; i != end ; ++i )
	{
		if (   ((type == NULL) || (stricmp( type, (*i)->GetType() ) == 0))
			&& ((name == NULL) || (stricmp( name, (*i)->GetName() ) == 0)) )
		{
			output.push_back( *i );
		}
	}

	// advance
	++current_recursion_level;

	// now recurse into children if not past recursion limit
	if( (recursion_depth_limit < 0) || (current_recursion_level < recursion_depth_limit) )
	{
		// get children too
		for ( i = begin; i != end ; ++i )
		{
			(*i)->GetChildBlocksHelper( recursion_depth_limit, type, name, current_recursion_level, output );
		}
	}
}


// private
bool FuelBlock :: AddChild( FuelBlock * child )
{
	gpassert( child );
	gpassert( child->mParent == 0 );

	mChildren.push_back( child );
	child->mParent = this;

	//ClientModifiedChildBlocks();

	return( true );
}


// private
bool FuelBlock :: RemoveChild( FuelBlock * child )
{
	fuel_block_list::iterator i;
	for( i = mChildren.begin(); i != mChildren.end(); ++i )
	{
		if( (*i) == child )
		{
			mChildren.erase( i );
			ClientModifiedChildBlocks();
			return true;
		}
	}
	return false;
}


// private
void FuelBlock :: BeginChildBlocksRead()
{
	Load();
	SortChildBlocks();
}


// private
void FuelBlock :: BeginParametersRead()
{
	// don't need to do anything special yet
}


// private
void FuelBlock :: ClientModifiedBlock()
{
	mCurrentKeyWildcardIterator = mBody.end();
	mKeyWildcard.clear();

	SetIsDirty();
}


//private
void FuelBlock :: ClientModifiedChildBlocks()
{
	mChildBlocksSorted = false;

	SetIsDirty();
}


// private
void FuelBlock :: SortChildBlocks()
{
	if ( !mChildBlocksSorted )
	{
		if( mChildren.size() > 1 )
		{
			mChildren.sort( FuelBlockPointerLess() );
		}
		mChildBlocksSorted = true;
	}
}




//////////////////////////////////////////////////////////////////////////////




void Translate( FastFuelHandleColl const & in, FuelHandleList & out )
{
	for( FastFuelHandleColl::const_iterator i = in.begin(); i != in.end(); ++i )
	{
		out.push_back( FuelHandle( (*i).GetAddress() ) );
	}
}


namespace BinFuel
{
	////////////////////////////////////////////////////////////////////////////////
	//	
	void Dump( Header const * block, bool verbose, ReportSys::ContextRef ctx )
	{
		gpassert( block != block->m_Parent );

		ReportSys::AutoReport autoReport( ctx );

		ctx->OutputF( "\nblock name = '%s', cacheId = 0x%08x\n", BinFuel::GetName( block ), block->m_CacheId );
	
		if( verbose )
		{
			ctx->OutputF( "block type = %s\n", BinFuel::GetType( block ) );
			//ctx->OutputF( "m_Debug  = 0x%08x \n", block->m_Debug );
			ctx->OutputF( "m_NameOffset = 0x%08x\n", block->m_NameOffset );
			ctx->OutputF( "m_TypeOffset = 0x%08x\n", block->m_TypeOffset );
		}

		gpstring prop;
		if( block->m_Properties & BP_IS_STUB )
		{
			prop += "| BP_IS_STUB";
		}
		if( block->m_Properties & BP_IS_DIRECTORY )
		{
			prop += "| BP_IS_DIRECTORY";
		}
		if( block->m_Properties & BP_NAME_IN_DICTIONARY )
		{
			prop += "| BP_NAME_IN_DICTIONARY";
		}
		if( block->m_Properties & BP_TYPE_IN_DICTIONARY )
		{
			prop += "| BP_TYPE_IN_DICTIONARY";
		}
		ctx->OutputF( "m_Properties = %s\n", prop.c_str() );

		
		ctx->OutputF( "m_NumChildren = %d\n", block->m_NumChildren );
		ctx->OutputF( "m_NumKeys = %d\n", block->m_NumKeys );

		ctx->Indent();

		for( DWORD i = 0; i != block->m_NumKeys; ++i )
		{
			ctx->OutputF( "%s = ", BinFuel::GetKeyName( block, i ) );

			KeyValueInfo * info = BinFuel::GetKeyValueInfo( block, i );
			gpassert( info );

			if( info->m_Properties & KVP_VALUE_FLOAT )
			{
				ctx->OutputF( "%g\n", *(float*)(&info->m_ValueOrOffset) );
			}
			if( info->m_Properties & KVP_VALUE_DOUBLE )
			{
				ctx->OutputF( "%2.30f\n", *((double*)( ((BYTE*)block)+info->m_ValueOrOffset )) );
			}
			if( info->m_Properties & KVP_VALUE_NORMAL_INT )
			{
				ctx->OutputF( "%d\n", (int)info->m_ValueOrOffset );
			}
			if( info->m_Properties & KVP_VALUE_HEX_INT )
			{
				ctx->OutputF( "0x%08x\n", (DWORD)info->m_ValueOrOffset );
			}
			if( info->m_Properties & KVP_VALUE_BOOL )
			{
				ctx->OutputF( "0x%s\n", bool(info->m_ValueOrOffset==0) ? "false" : "true" );
			}
			if( info->m_Properties & KVP_VALUE_STRING )
			{
				if( info->m_Properties & KVP_VALUE_IN_DICTIONARY )
				{
					ctx->OutputF( "%s\n", BinFuel::GetDictionaryString( BinFuel::GetDictionary(), info->m_ValueOrOffset ) );
				}
				else
				{
					ctx->OutputF( "%s\n", ((char*)(block))+info->m_ValueOrOffset );
				}
			}
			if( info->m_Properties & KVP_VALUE_SIEGE_POS )
			{
				ctx->OutputF( "%s\n", ::ToString( *((SiegePosData*)( ((BYTE*)block)+info->m_ValueOrOffset ))).c_str() );
			}
			if( info->m_Properties & KVP_VALUE_QUAT )
			{
				ctx->OutputF( "%s\n", ::ToString( *((Quat*)( ((BYTE*)block)+info->m_ValueOrOffset ))).c_str() );
			}
			if( info->m_Properties & KVP_VALUE_VECTOR_3 )
			{
				ctx->OutputF( "%s\n", ::ToString( *((vector_3*)( ((BYTE*)block)+info->m_ValueOrOffset ))).c_str() );
			}
		}

		
		for( DWORD iChild = 0; iChild != block->m_NumChildren; ++iChild )
		{
			ctx->OutputF( "child #%d:\n", iChild );
			ctx->Indent();

			ChildInfo * childInfo = GetChildInfo( block, iChild );
			if( childInfo->m_BlockHeaderOffset != 0xffffffff )
			{
				Header * child = GetChild( block, iChild );
				Dump( child, verbose, ctx );
			}
			else
			{
				ctx->OutputF( "NOT LOADED\n", iChild );
			}
			ctx->Outdent();
		}



		ctx->Outdent();
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	const char * GetKeyName( Header const * header, DWORD n )
	{
		gpassert( header != header->m_Parent );
		gpassert( n <= header->m_NumKeys );

		if( GetKeyValueInfo( header, n )->m_Properties & KVP_KEY_IN_DICTIONARY )
		{
			return BinFuel::GetDictionaryString( BinFuel::GetDictionary(), GetKeyValueInfo( header, n )->m_KeyOffset );
		}
		else
		{
			return (const char*)(((BYTE*)header) + GetKeyValueInfo( header, n )->m_KeyOffset);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	
	BYTE * GetValueDataPtr( Header const * block, KeyValueInfo * info )
	{
		gpassert( block != block->m_Parent );
		gpassert( info );

		if( info->m_Properties & KVP_VALUE_NUMERIC )
		{
			if( ( info->m_Properties & KVP_VALUE_COMPLEX_NUMERIC ) == 0 )
			{
				return( (BYTE*)(&info->m_ValueOrOffset) );
			}
			else
			{
				return( ((BYTE*)(block)) + info->m_ValueOrOffset );
			}
		}
		else if( info->m_Properties & KVP_VALUE_STRING )
		{
			if( info->m_Properties & KVP_VALUE_IN_DICTIONARY )
			{
				return (BYTE*)BinFuel::GetDictionaryString( BinFuel::GetDictionary(), info->m_ValueOrOffset );
			}
			else
			{
				return( ((BYTE*)(block)) + info->m_ValueOrOffset );
			}
		}
		else
		{
			gpassert( 0 );
			Dump( block );
			return NULL;
		}
	}
/*
	////////////////////////////////////////////////////////////////////////////////
	//	
	BinaryFuelDB * Header::GetDb()
	{
		gpassert( this != this->m_Parent );
		return( gFuelSys.GetBinaryDb( ( this->m_Properties ) >> 5 ) );
	}
*/
	////////////////////////////////////////////////////////////////////////////////
	//	
	Header * GetChild( Header const * parent, DWORD n )
	{
		gpassert( parent != parent->m_Parent );
		gpassert( n <= parent->m_NumChildren );

		ChildInfo * childInfo = GetChildInfo( parent, n );
		gpassert( !childInfo->IsDirectory() || (childInfo->IsDirectory() && parent->IsDirectory() && !parent->IsStub()) );

		if( (childInfo->m_Properties & CP_OFFSET_IS_POINTER) == 0 )
		{
			return (Header*)( ((BYTE*)parent) + childInfo->m_BlockHeaderOffset );
		}
		else
		{
			return (Header*)childInfo->m_BlockHeaderOffset;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
FastFuelHandle :: FastFuelHandle( const char * addr )
{
	m_BlockCacheId = 0;
	m_pDb = NULL;

	////////////////////////////////////////
	//	validate params

	if( (addr == NULL) || (*addr == 0) )
	{
		return;
	}

	////////////////////////////////////////
	//	find db

	if( FuelSys::DoesSingletonExist() )
	{
		const char * relBlockAddress = NULL;
		m_pDb = gFuelSys.GetBinaryDbFromAddress( addr, &relBlockAddress );

		if( !m_pDb )
		{
			m_pDb = gFuelSys.GetDefaultBinaryDb();
			gpassert( m_pDb );

			if( !m_pDb )
			{
				gperrorf(( "Failed to find fuel db for address = %s", addr ));
				return;
			}
		}

		kerneltool::Critical::Lock lock( m_pDb->GetCritical() );

		BinFuel::Header * block = NULL;

		if( stricmp( relBlockAddress, "root" ) == 0 )
		{
			block = m_pDb->GetRootHeader();
		}
		else
		{
			char* lc_block_address = (char*)::_alloca( ::strlen( relBlockAddress ) + 1 );
			::strlwr_copy( lc_block_address, relBlockAddress );

			block = m_pDb->GetBlockBySoftAddress( lc_block_address );
		}

		if( block )
		{
			m_pDb->Register( block );
			m_BlockCacheId = block->m_CacheId;

			gpassert( m_BlockCacheId );
			gpassert( m_pDb );
			gpassert( m_pDb->GetCacheSlot( m_BlockCacheId )->IsValid() );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelHandle :: Unload()
{
	if( !GetHeader( false )->IsStub() )
	{
		GetDb()->CacheOut( m_BlockCacheId );
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	
const char * FastFuelHandle :: GetName() const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpassert( IsValid() );
	return BinFuel::GetName( GetHeader(false) );
}


////////////////////////////////////////////////////////////////////////////////
//	
const char * FastFuelHandle :: GetType() const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpassert( IsValid() );
	return BinFuel::GetType( GetHeader() );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelHandle :: HasType()
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpassert( IsValid() );
	return *GetType() != 0;
}


////////////////////////////////////////////////////////////////////////////////
//	
gpstring FastFuelHandle :: GetAddress( bool full ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpassert( m_BlockCacheId );

	gpstring address;

	if ( full )
	{
		address += "::";
		address += GetDb()->GetName();
		address += ":";
	}

	address += GetDb()->GetBlockSoftAddress( GetHeader(false) );

	return( address );
}


////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelHandle :: IsDirectory() const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpassert( m_BlockCacheId );
	return( GetHeader(false)->IsDirectory() );
}


////////////////////////////////////////////////////////////////////////////////
//	
DWORD FastFuelHandle :: GetStaticChecksum() const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	if( IsDirectory() )
	{
		return 0;
	}
	else
	{
		return ((BinFuel::DirHeader*)GetHeader())->m_StaticChecksum;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
DWORD FastFuelHandle :: GetRecursiveStaticChecksum()
{
	typedef stdx::fast_vector< DWORD > intArray;
	typedef stdx::fast_vector< FastFuelHandle > childArray;

	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	if( !IsDirectory() )
	{
		gpassertm( 0, "Not a valid query on non-dir block." );
		return 0;
	}
	else
	{
		intArray checksums;

		// prime stack
		FastFuelHandleColl toVisit;
		toVisit.push_back( *this );

		while( !toVisit.empty() )
		{
			FastFuelHandle visiting = toVisit.back();
			toVisit.pop_back();
			checksums.push_back( visiting.GetStaticChecksum() );

			FastFuelHandleColl children;

			visiting.ListChildren( children, 1 );

			for( FastFuelHandleColl::iterator i = children.begin(); i != children.end(); ++i )
			{
				if( (*i).IsDirectory() )
				{
					toVisit.push_back( *i );
				}
			}
		}

		// calc checksum of checksums
		DWORD sum = 0;
		for( intArray::iterator is = checksums.begin(); is != checksums.end(); ++is )
		{
			sum = GetCRC32( sum, (void*)&(*is), sizeof( DWORD ) );
		}

		return sum;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
eFuelValueProperty FastFuelHandle :: Get( DWORD keyNum, gpstring & out, bool optional ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	eFuelValueProperty result = FVP_NONE;
	KeyValueInfo * info = GetKeyValueInfo( GetHeader(), keyNum );

	if( info )
	{
		BYTE * valueData = GetValueDataPtr( GetHeader(), info );

		if( valueData )
		{
			if( info->m_Properties & KVP_VALUE_NORMAL_INT )
			{
				stringtool::Set( *((int*)(valueData)), out );
				GetDb()->IncrementInefficientIntReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_HEX_INT )
			{
				stringtool::Setx( *((unsigned int*)(valueData)), out );
				GetDb()->IncrementInefficientIntReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_FLOAT )
			{
				stringtool::Set( *((float*)(valueData)), out );
				GetDb()->IncrementInefficientFloatReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_BOOL )
			{
				stringtool::Set( *((bool*)(valueData)), out );
				GetDb()->IncrementInefficientBoolReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_DOUBLE )
			{
				stringtool::Set( *((double*)(valueData)), out );
				//GetDb()->IncrementInefficientDoubleReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_SIEGE_POS )
			{
				out = ToString( *((SiegePosData*)(valueData)) );
				GetDb()->IncrementInefficientSiegePosReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_QUAT )
			{
				out = ToString( *((Quat*)(valueData)) );
				GetDb()->IncrementInefficientQuatReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_VECTOR_3 )
			{
				out = ToString( *((vector_3*)(valueData)) );
				GetDb()->IncrementInefficientVector3ReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_STRING )
			{
				out = (const char*)valueData;
			}
			else
			{
				gpassert( 0 );
			}
			result = FVP_STRING;
		}
	}
	else
	{
		if( !optional )
		{
			gperrorf(( "Failed to read key num %d from fuel block '%s'\n", keyNum, GetAddress().c_str() ));
		}
	}

	return( result );
}


////////////////////////////////////////////////////////////////////////////////
//	
eFuelValueProperty FastFuelHandle :: Get( DWORD keyNum, gpwstring & out, bool optional ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	eFuelValueProperty result = FVP_NONE;
	KeyValueInfo * info = GetKeyValueInfo( GetHeader(), keyNum );

	if( info )
	{
		BYTE * valueData = GetValueDataPtr( GetHeader(), info );

		if( valueData )
		{
			if( info->m_Properties & KVP_VALUE_NORMAL_INT )
			{
				gpstring _out;
				stringtool::Set( *((int*)(valueData)), _out );
				out = ToUnicode( _out );
				GetDb()->IncrementInefficientIntReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_HEX_INT )
			{
				gpstring _out;
				stringtool::Setx( *((unsigned int*)(valueData)), _out );
				out = ToUnicode( _out );
				GetDb()->IncrementInefficientIntReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_FLOAT )
			{
				gpstring _out;
				stringtool::Set( *((float*)(valueData)), _out );
				out = ToUnicode( _out );
				GetDb()->IncrementInefficientFloatReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_BOOL )
			{
				gpstring _out;
				stringtool::Set( *((bool*)(valueData)), _out );
				out = ToUnicode( _out );
				GetDb()->IncrementInefficientBoolReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_DOUBLE )
			{
				gpstring _out;
				stringtool::Set( *((double*)valueData), _out );
				out = ToUnicode( _out );
				//$$$GetDb()->IncrementInefficientDoubleReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_SIEGE_POS )
			{
				gpstring _out;
				_out = ToString( *((SiegePosData*)(valueData)) );
				out = ToUnicode( _out );
				//$$$GetDb()->IncrementInefficientBoolReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_QUAT )
			{
				gpstring _out;
				_out = ToString( *((Quat*)(valueData)) );
				out = ToUnicode( _out );
				//$$$GetDb()->IncrementInefficientBoolReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_VECTOR_3 )
			{
				gpstring _out;
				_out = ToString( *((vector_3*)(valueData)) );
				out = ToUnicode( _out );
				//$$$GetDb()->IncrementInefficientBoolReadCount();
			}
			else if( info->m_Properties & KVP_VALUE_STRING )
			{
				out = ToUnicode( (const char*)valueData );
			}
			else
			{
				gpassert( 0 );
			}
			result = FVP_WIDE_STRING;
		}
	}
	else
	{
		if( !optional )
		{
			gperrorf(( "Failed to read key num %d from fuel block '%s'\n", keyNum, GetAddress().c_str() ));
		}
	}

	return( result );
}


////////////////////////////////////////////////////////////////////////////////
//	
eFuelValueProperty FastFuelHandle :: Get( char const * key, gpstring & out, bool optional ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	DWORD location = 0;
	KeyValueInfo * info = GetKeyValueInfoForKey( GetHeader(), key, &location );

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

	return FVP_NONE;
}


////////////////////////////////////////////////////////////////////////////////
//	
int FastFuelHandle :: GetInt( const char * key, int defValue ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	int result = defValue;
	Get( key, result );
	return result;
}


////////////////////////////////////////////////////////////////////////////////
//	
gpstring FastFuelHandle :: GetString( const char * key, const gpstring & defValue ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	gpstring result;
	if( Get( key, result ) )
	{
		return( result );
	}
	else
	{
		return defValue;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
float FastFuelHandle :: GetFloat( const char * key, float defValue ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	float result = defValue;
	Get( key, result );
	return result;
}


////////////////////////////////////////////////////////////////////////////////
//	
double FastFuelHandle :: GetDouble( const char * key, double defValue ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	double result = defValue;
	Get( key, result );
	return result;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelHandle :: GetBool( const char * key, bool defValue ) const
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	bool result = defValue;
	Get( key, result );
	return result;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelHandle :: ListChildren( FastFuelHandleColl & out, FastFuelHandle & parent, const char * name, const char * type, const int recursionLimit, DWORD ignoreProperties )
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	DWORD sizeBefore = out.size();

	if( parent.GetHeader()->IsDirectory() && parent.GetHeader()->IsStub() )
	{
		GetDb()->LoadDirBlock( GetDb()->GetDirBlockPath( parent.GetHeader() ).c_str(), ccast<BinFuel::Header*>( parent.GetHeader() ) );
		gpassert( !parent.GetHeader()->IsStub() );
	}

	for( DWORD i = 0; i != parent.GetHeader()->m_NumChildren; ++i )
	{
		bool store = true;

		Header * child = BinFuel::GetChild( parent.GetHeader(), i );

#if		!GP_RETAIL
		gpassert( child->m_Parent == parent.GetHeader() );
#endif

		if( name )
		{
			const char * childName = BinFuel::GetName( child );
			if( stricmp( name, childName ) != 0 )
			{
				store = false;
			}
		}

		if( store && type )
		{
			const char * childType = BinFuel::GetType( child );
			if( stricmp( type, childType ) != 0 )
			{
				store = false;
			}
		}


		FastFuelHandle load( ccast<BinFuel::Header*>(child), parent.GetDb() );

		bool ignore = ignoreProperties && ( (load.GetHeader( false )->m_Properties & ignoreProperties ) != 0 );

		if( store && !ignore )
		{
			out.push_back( load );
		}

		int newRecursionLimit = recursionLimit;
		
		if( recursionLimit != -1 )
		{
			--newRecursionLimit;
		}

		if( !ignore && ( newRecursionLimit > 0 || ( recursionLimit == -1 ) ) )
		{
			ListChildren( out, load, name, type, newRecursionLimit, ignoreProperties );
		}
	}

	return out.size() > sizeBefore;
}


////////////////////////////////////////////////////////////////////////////////
//	
void FastFuelHandle :: Dump( ReportSys::ContextRef ctx )
{
	FastFuelHandle::ConditionalLock lock( ShouldGrabLock(), this );

	if( GetHeader(false) )
	{
		BinFuel::Dump( GetHeader(false), true, ctx );
	}
}



////////////////////////////////////////////////////////////////////////////////
//	
bool FastFuelFindHandle::FindFirstValue( const char * key )
{
	gpassertm( IsValid(), "Attempted to use an invalid FastFuelFindHandle." );

	FastFuelHandle::ConditionalLock lock( m_Handle->ShouldGrabLock(), m_Handle );

	BinFuel::Header * block	= m_Handle->GetHeader();
	if( block->m_NumKeys == 0 )
	{
		return false;
	}

	m_FindKey = "";
	m_CurrentKeyIndex = 0;
	DWORD firstOccurance = 0;
	BinFuel::KeyValueInfo * found = BinFuel::BSearchGetKeyValueInfoForKey( block, key, 0, block->m_NumKeys-1, &firstOccurance );

	if( found )
	{
		while( ( firstOccurance > 0 ) && ( stricmp( BinFuel::GetKeyName( block, firstOccurance-1 ), key ) == 0 ) )
		{
			--firstOccurance;
		}
		m_CurrentKeyIndex = firstOccurance;
		m_FindKey = key;
	}
	return( found != NULL );
}
