#include "precomp_gpcore.h"

#include "config.h"
#include "fueldb.h"
#include "fuel.h"
#include "gpstatsdefs.h"
#include "gpmem.h"
#include "filesys.h"
#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "stringtool.h"
#include "ReportSys.h"
#include "quat.h"
#include "vector_3.h"
#include "FubiTraitsImpl.h"
#include "gpprofiler.h"

#include <set>
#include <stdio.h>

using namespace BinFuel;
using namespace FileSys;




////////////////////////////////////////////////////////////////////////////////
//	

// parser state-machine states
enum
{
	DONE=0,
	FIND_NEXT_CONTEXT,

	SEEK_NEXT_PHYSICAL_LINE,
	SEEK_NEXT_LOGICAL_LINE,

	CONTEXT_HEADER_BEGIN,
	CONTEXT_HEADER_VERBOSE_NAME,
	CONTEXT_HEADER_VERBOSE_TYPE,
	CONTEXT_HEADER_TERSE_NAME,
	CONTEXT_HEADER_END,
	CONTEXT_BRIEF_HEADER,
	CONTEXT_KEY_VALUE,
	CONTEXT_C_COMMENT,
	CONTEXT_CPP_COMMENT,
	CONTEXT_BLOCK_FINISHED,
	CONTEXT_SKIP_BLOCK
};


inline bool giswhite( const unsigned int c )
{
	return( !!::isspace( c ) );
}


inline bool giseol( const char * c )
{
	return( (*c=='\r') || (*(c+1)== '\n') );
}


inline bool gisalnum( const unsigned int c )
{
	return( isalnum(c) || c=='_' );
}


inline bool gisnamechar( const unsigned int c )
{
	return( isalnum(c) || c=='-' || c=='_' || c=='*' || c=='$' || c=='.' );
}


gpstring GetModeName( int mode )
{
		gpstring action_name;
		switch( mode )
		{
		case FIND_NEXT_CONTEXT:				{action_name="determining context";							break;	}
																										
		case SEEK_NEXT_PHYSICAL_LINE:		{action_name="seeking next physical line";					break;	}
		case SEEK_NEXT_LOGICAL_LINE:		{action_name="seeking next logical line";					break;	}			
																										
		case CONTEXT_HEADER_BEGIN:			{action_name="reading block header start";					break;	}
		case CONTEXT_HEADER_VERBOSE_NAME:	{action_name="reading block explicit name";					break;	}
		case CONTEXT_HEADER_VERBOSE_TYPE:	{action_name="reading block explicit type";					break;	}
		case CONTEXT_HEADER_TERSE_NAME:		{action_name="reading block name";							break;	}
		case CONTEXT_HEADER_END:			{action_name="reading end of block header";					break;	}
		case CONTEXT_KEY_VALUE:				{action_name="reading key and value pair";					break;	}
		case CONTEXT_C_COMMENT:				{action_name="reading c-style comment";						break;	}
		case CONTEXT_CPP_COMMENT:			{action_name="reading cpp-style comment";					break;	}
		case CONTEXT_BLOCK_FINISHED:		{action_name="finishing processing block";					break;	}
																										
		default:							{action_name="unknown action";								break;	}
		};
		return( action_name );
}


inline unsigned int GetPhysicalLineCountToBufferPosition( const char * buffer, unsigned int position )
{
	unsigned int temp_position=0;
	unsigned int line_count=1;
	while( temp_position < position )
	{
		if( buffer[temp_position]=='\n' )
		{
			++line_count;
		}
		++temp_position;
	}
	return( line_count );
}

////////////////////////////////////////////////////////////////////////////////
//	BinFuel BinFuel BinFuel BinFuel BinFuel BinFuel BinFuel BinFuel BinFuel 

const DWORD BINARY_FUEL_VERSION	= 0x00010006;
const DWORD MAX_NUM_BINARY_DBS	= 8;

// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 
// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 
// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 

// EVIL data struct mirror of SiegePos to add support to binary fuel -bk

gpstring SiegeGuidData :: ToString( void ) const
{
	return ( gpstringf( "0x%08x", GetValue() ) );
}

bool SiegeGuidData :: FromString( const char* str )
{
	bool success = false;

	if ( str[ 0 ] == '0' && str[ 1 ] == 'x' )
	{
		int err = 0;
		DWORD value = stringtool::strtoul( str, NULL, 0, &err );
		if ( !err )
		{
			success = true;
			SetValue( value );
		}
	}
	else
	{
		int value[ 4 ];
		if ( (success = (sscanf( str, "%x %x %x %x", &value[ 0 ], &value[ 1 ], &value[ 2 ], &value[ 3 ] ) == 4)) == true )
		{
			m_Guid[ 0 ] = (UINT8)value[ 0 ];
			m_Guid[ 1 ] = (UINT8)value[ 1 ];
			m_Guid[ 2 ] = (UINT8)value[ 2 ];
			m_Guid[ 3 ] = (UINT8)value[ 3 ];
		}
		else
		{
			gpassertm(0, "sccanf() failed");
			success = false;
		}
	}

	if ( !success )
	{
		*(UINT32*)m_Guid = 0;
	}

	return ( true );
}


template <> void FuBi::Traits <SiegePosData> :: ToString( gpstring& out, const Type& obj, FuBi::eXfer /*xfer*/ )
{
	out.appendf( "%g,%g,%g,%s", obj.pos.x, obj.pos.y, obj.pos.z, obj.node.ToString().c_str() );
}

template <> bool FuBi::Traits <SiegePosData> :: FromString( const char* str, Type& obj )
{
	char buffer[ 50 ];
	if ( ::sscanf( str, "%f , %f , %f , %50s[abcdefABCDEF0123456789 ]",
					   &obj.pos.x, &obj.pos.y, &obj.pos.z, buffer ) != 4 )
	{
		return ( false );
	}

	return ( obj.node.FromString( buffer ) );
}


namespace BinFuel
{

	static BinFuel::DictionaryHeader * s_DictHeader;
	static gpstring s_DictPath;

	BinFuel::DictionaryHeader * GetDictionary()
	{
		if( !s_DictHeader )
		{
			//GPSTATS_SYSTEM( SP_FUEL );

			gpstring dictPath( s_DictPath );

			FileSys::AutoFileHandle dictFile;
			FileSys::AutoMemHandle mem;

			if( !dictFile.Open( dictPath ) )
			{
				gperrorf(( "Critical resource file %s missing at path %s. Make sure the game's data is installed correctly.", FileSys::GetFileName( dictPath ), dictPath.c_str() ));
				return NULL;
			}

			if( ( !mem.Map( dictFile ) ) || ( mem.GetSize() == 0 ) )
			{
				gperrorf(( "Couldn't read critical resource file %s at path %s. Make sure the game's data is installed correctly.", FileSys::GetFileName( dictPath ), dictPath.c_str() ));
				return NULL;
			}

			DictionaryHeader * fileHeader = (DictionaryHeader*)mem.GetData();
			const int fileSize = mem.GetSize();

			BYTE * block = new BYTE[ fileSize ];
			memcpy( block, (BYTE*)fileHeader, fileSize );

			s_DictHeader = (DictionaryHeader *)block;
			return s_DictHeader;
		}
		else
		{
			return s_DictHeader;
		}
	}

	void SetDictionaryPath( const char * path )
	{
		// once we get a good dictionary path, hang on to it...

		gpstring tempPath = path;
		stringtool::AppendTrailingBackslash( tempPath );
		tempPath += "lqd_dictionary.ldc6";

		FileSys::AutoFileHandle dictFile;
		FileSys::AutoMemHandle mem;

		if( dictFile.Open( tempPath ) )
		{
			s_DictPath = tempPath;
		}
	}

	void FreeDictionary()
	{
		Delete( s_DictHeader );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	FuelSys

FuelSys :: FuelSys()
{
	m_pDefaultTextDb	= NULL;
	m_pDefaultBinaryDb	= NULL;
	m_DebugShowHandleAccess = false;

	for( DWORD i = 0; i != MAX_NUM_BINARY_DBS; ++i )
	{
		m_BinaryDbLookup.push_back( 0 );
	}
}


FuelSys :: ~FuelSys()
{
	FuelDBMap::iterator i;

	for( i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		delete (*i).second;
	}

	BinFuel::FreeDictionary();
}


void FuelSys :: Update()
{
	GPSTATS_SYSTEM( SP_FUEL );

	for( FuelDBMap::iterator i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		(*i).second->Update();
	}
}


TextFuelDB * FuelSys :: AddTextDb( gpstring const & sName )
{
	GPSTATS_SYSTEM( SP_FUEL );

	// validate params
	if( HasDb( sName ) )
	{
		gperrorf(( "A FuelDB named '%s' already exists.  You must use a different name.", sName.c_str() ));
		return NULL;
	}

	// add a DB
	TextFuelDB * pDB = new TextFuelDB;
	pDB->SetName( sName );
	m_FuelDBMap.insert( FuelDBMap::value_type( sName, pDB ) );
	if( sName.same_no_case( "default" ) )
	{
		m_pDefaultTextDb = pDB;
	}
	return pDB;
}


DWORD FuelSys :: CalcChecksum( StringSet const & ss )
{
	DWORD sum = 0;
	for( StringSet::const_iterator i = ss.begin(); i != ss.end(); ++i )
	{
		sum = GetCRC32( sum, (void*)(*i).c_str(), (*i).size() );
	}
	return sum;
}


DWORD BinaryFuelDB::m_InstanceCount = 0;

BinaryFuelDB * FuelSys :: AddBinaryDb( gpstring const & sName )
{
	GPSTATS_SYSTEM( SP_FUEL );

	// validate params
	if( HasDb( sName ) )
	{
		gperrorf(( "A FuelDB named '%s' already exists.  You must use a different name.", sName.c_str() ));
		return NULL;
	}

	// add a DB
	BinaryFuelDB * pDB = new BinaryFuelDB;
	m_FuelDBMap.insert( FuelDBMap::value_type( sName, (FuelDB*)pDB ) );
	pDB->SetName( sName );
	if( sName.same_no_case( "default_binary" ) )
	{
		m_pDefaultBinaryDb = pDB;
	}

	m_BinaryDbLookup[ pDB->GetId() ] = pDB;

	return (BinaryFuelDB *)pDB;
}


bool FuelSys :: HasDb( gpstring const & name )
{
	FuelDBMap::iterator i = m_FuelDBMap.find( name );
	return( i != m_FuelDBMap.end() );
}


void FuelSys :: Remove( FuelDB * pDB )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelDBMap::iterator i;

	for( i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		if( (*i).second == pDB )
		{
			if( m_pDefaultTextDb == pDB )
			{
				m_pDefaultTextDb = NULL;
			}
			else if( m_pDefaultBinaryDb == pDB )
			{
				m_pDefaultBinaryDb = NULL;
			}

			delete (*i).second;

			m_FuelDBMap.erase( i );

			return;
		}
	}
}


FuelDB * FuelSys :: GetFuelDbFromAddress( const char * sName, const char ** sNewAddress )
{
	if( sName[0] == ':' && sName[1] == ':' )
	{
		unsigned int i = 2;
		while( sName[i] != ':' && sName[i] != 0 )
		{
			++i;
		}

		gpstring name( &sName[2], i-2 );

		if ( sNewAddress != NULL )
		{
			*sNewAddress = sName + i + 1;
		}

		FuelDBMap::iterator j = m_FuelDBMap.find( name );
		if( j != m_FuelDBMap.end() )
		{
			return (*j).second;
		}
		else
		{
			gpassertm( 0, "Could not find FuelDb from address." );
			return NULL;
		}
	}
	else
	{
		if( sNewAddress != NULL )
		{
			*sNewAddress = sName;
		}

		return NULL;
	}
}


TextFuelDB * FuelSys :: GetTextDbFromAddress( const char * sName, const char ** sNewAddress )
{
	FuelDB* db = GetFuelDbFromAddress( sName, sNewAddress );
	if ( db != NULL )
	{
		// make sure it really is a text db
		if ( !db->TestOptions( FuelDB::OPTION_TEXT_MODE ) )
		{
			db = NULL;
		}
	}

	return ( (TextFuelDB*)db );
}


BinaryFuelDB * FuelSys :: GetBinaryDbFromAddress( const char * sName, const char ** sNewAddress )
{
	FuelDB* db = GetFuelDbFromAddress( sName, sNewAddress );
	if ( db != NULL )
	{
		// make sure it really is a binary db
		if ( !db->TestOptions( FuelDB::OPTION_BINARY_MODE ) )
		{
			db = NULL;
		}
	}

	return ( (BinaryFuelDB*)db );
}


const gpdumbstring & FuelSys :: AddString( const gpdumbstring & str )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	StringDb::iterator found = m_StringCollector.find( str );
	if ( found == m_StringCollector.end() )
	{
		found = m_StringCollector.insert( StringDb::value_type( str, 0 ) ).first;
	}
	++found->second;
	return ( found->first );
}


const gpdumbstring & FuelSys :: AddString( const char * str )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	StringDb::iterator found = m_StringCollector.find( str );
	if ( found == m_StringCollector.end() )
	{
		found = m_StringCollector.insert( StringDb::value_type( str, 0 ) ).first;
	}
	++found->second;
	return ( found->first );
}


void FuelSys :: Unload()
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelDBMap::iterator i;

	for( i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		(*i).second->Unload();
	}

	m_StringCollector.clear();
}


void FuelSys :: SetUncachingEnabled( bool flag )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelDBMap::iterator i;

	for( i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		if( (*i).second->TestOptions( FuelDB::OPTION_BINARY_MODE ) )
		{
			((BinaryFuelDB*)(*i).second)->SetUncachingEnabled( flag );
		}
	}
}


#if !GP_RETAIL

void FuelSys :: Dump()
{
	Dump( NULL );
}


void FuelSys :: Dump( ReportSys::ContextRef ctx = NULL, bool verbose )
{
	ReportSys::AutoReport autoReport( ctx );

	DWORD size = 0;
	for( StringDb::iterator j = m_StringCollector.begin(); j != m_StringCollector.end(); ++j )
	{
		size += (*j).first.size();
	}

	if( verbose )
	{
		ctx->Output(  "============================================================\n" );
		ctx->Output(  "===================     FuelSys Report       ===============\n" );
		ctx->Output(  "============================================================\n" );

		ctx->OutputF( "String collector size (strings) = %d\n", m_StringCollector.size() );
		ctx->OutputF( "String collector size (bytes)   = %d\n", size );
		ctx->OutputF( "Average string size (bytes)     = %f\n\n", float(size)/float(m_StringCollector.size() ) );
	}
	else
	{
		//ctx->OutputF( "
	}

	for( FuelDBMap::iterator i = m_FuelDBMap.begin(); i != m_FuelDBMap.end(); ++i )
	{
		if( verbose )
		{
			ctx->Output(  "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n" );
			ctx->OutputF( "FuelDb name			= %s\n", (*i).first.c_str() );
		}
		// get stats from each DB
		(*i).second->Dump( ctx, verbose );
	}
}

#endif // !GP_RETAIL


//////////////////////////////////////////////////////////////////////////////////
//	class FuelDB

FuelDB::FuelDB()
{
	m_FilesLoaded					= 0;
	m_GasBlocksLoaded				= 0;
	m_LogicalLinesLoaded			= 0;

	m_Options						= OPTION_NONE;
	m_InUserBlockDeleteRequestScope	= false;

	m_Name							= "";
}


void FuelDB::Shutdown()
{
	m_FilesLoaded			= 0;
	m_GasBlocksLoaded		= 0;
	m_LogicalLinesLoaded	= 0;
}




//////////////////////////////////////////////////////////////////////////////////
//	class TextFuelDB




TextFuelDB :: TextFuelDB()
	: FuelDB()
{
	m_Root					= 0;
	m_DevBlockEncountered	= false;
	m_bCriticalError		= false;
}


bool TextFuelDB :: Init( eOptions options, gpstring const & readHome, gpstring const & writeHome )
{
	GPSTATS_SYSTEM( SP_FUEL );

	m_bCriticalError		= false;
	m_DevBlockEncountered	= false;

#	if GP_RETAIL
	options |= OPTION_NO_READ_DEV_BLOCKS;
#	endif // GP_RETAIL

	// special option for loc people
	if ( Config::DoesSingletonExist() && gConfig.GetBool( "force_retail_content", false ) )
	{
		options |= OPTION_NO_READ_DEV_BLOCKS;
	}

	// i am text mode
	m_Options = options | OPTION_TEXT_MODE;

	// jit-read means read too
	if ( m_Options & OPTION_JIT_READ )
	{
		m_Options |= OPTION_READ;
	}

	if( options == OPTION_NONE )
	{
		gperror( "You must specify options for FuelDB" );
		return false;
	}

	if( TestOptions( OPTION_WRITE) && !TestOptions( OPTION_MEM_ONLY ) )
	{
		if( writeHome.empty() )
		{
			gperror( "You must specify a write home path if you will be saving this FuelDB." );
			return false;
		}
		else
		{
			SetHomeWritePath( writeHome );
		}
	}

	gpstring rootName;

	bool writeOnly = TestOptions( OPTION_MEM_ONLY ) || !TestOptions( OPTION_READ );

	if( !writeOnly )
	{
		gpstring directory = readHome;
		stringtool::AppendTrailingBackslash( directory );
		SetHomeReadPath( directory );

		stringtool::RemoveTrailingBackslash( directory );
		stringtool::GetBackDelimitedString( directory, '\\', rootName );
	}

	////////////////////////////////////////
	//	create root block

	m_Root = new FuelBlock;
	gpassert( m_Root );

	// init root node
	m_Root->SetDB( this );
	m_Root->SetOrigin( m_HomeReadPath );
	m_Root->mName        = rootName;   
	m_Root->mIsDirectory = true; 
	m_Root->mIsValid     = true; 

	m_Root->mIsLoaded = writeOnly;

	if( !writeOnly )
	{
		m_Root->Load();
	}

	gpassert( m_Root );

	return true;
}


bool TextFuelDB :: Reinit()
{
	eOptions oldOptions		= m_Options;
	gpstring oldReadHome	= m_HomeReadPath;
	gpstring oldWriteHome	= m_HomeWritePath;

	Shutdown();
	return Init( oldOptions, oldReadHome, oldWriteHome );
}


void TextFuelDB :: Shutdown()
{
	GPSTATS_SYSTEM( SP_FUEL );

	UnloadBlock( m_Root );
	delete m_Root;
	m_Root = NULL;

	m_HomeReadPath		= "";
	m_HomeWritePath		= "";

	m_FileDeleteList.clear();
	m_OriginPaths.clear();

	FuelDB::Shutdown();
}


bool TextFuelDB :: CriticalErrorEncountered()
{
	return( m_bCriticalError );
}


void TextFuelDB :: Unload()
{
	GPSTATS_SYSTEM( SP_FUEL );
	GetRootHandle().Unload();
}


TextFuelDB :: ~TextFuelDB()
{
	if( m_Root )
	{
		delete m_Root;
		m_Root = NULL;
	}
}


FuelHandle TextFuelDB :: GetRootHandle()
{
	return FuelHandle( m_Root );
}


void TextFuelDB :: SetHomeReadPath( gpstring const & home_directory )
{	
	GPSTATS_SYSTEM( SP_FUEL );

	//----- validate params

	if( m_Root )
	{
		gperror( "This FuelDB has already been initialized.  You can't change the home read path." );
		return;
	}

	m_HomeReadPath = home_directory;

	if( !home_directory.empty() )
	{
		if( !stringtool::IsAbsolutePath( home_directory ) )
		{
			// $$$ have this warning come up for all customers, less bin fuel
			//gpwarning( "If you're setting the READ path home, it must be an absolute path." );
			return;
		}
	}
}


FuelBlock * TextFuelDB :: GetRootBlock()
{
	gpassert( m_Root );
	return( m_Root );
}


bool TextFuelDB :: SetHomeWritePath( gpstring const & home_directory )
{
	GPSTATS_SYSTEM( SP_FUEL );

	//----- validate params

	if( !m_HomeWritePath.empty() )
	{
		gperror( "This FuelDB has already been initialized.  You can't change the home write path." );
		return false;
	}

	if( !stringtool::IsAbsolutePath( home_directory ) )
	{
		if ( !TestOptions( OPTION_SIMPLE_ABORT_ON_FAIL ) )
		{
			gperror( "If you're setting the WRITE path home, it must be an absolute path." );
			gpfatal( "Please check your 'home' command line setting." );
		}
		return false;
	}

	//----- do the work

	gpstring directory( home_directory );
	//directory.to_lower();
	stringtool::AppendTrailingBackslash( directory );

	m_HomeWritePath = directory;

	return true;
}


FuelBlock * TextFuelDB :: FindBlockGlobal( gpstring const & block_address )
{
	GPSTATS_SYSTEM( SP_FUEL );

#if !GP_RETAIL

	gpstring::size_type n = block_address.find_first_of( "*" );

	if( n != gpstring::npos )
	{
		gpstring error = "Address contains wildcard, so it's non-unique.  You can't construct a handle to a non-uniquely addressed block->";
		error += "\nSaid block address = " + block_address;
		gpassertm( 0, error.c_str() );
		gpwarning( error.c_str() );
		return( 0 );
	}
#endif

	FuelBlock * found = FindBlockLocal( block_address );

	// block found and has been loaded in
	if( found && found->IsLoaded() )
	{
		return found;
	}
	// not found
	else
	{
		if( TestOptions( OPTION_JIT_READ ) && !TestOptions( OPTION_MEM_ONLY ) )
		{
			// Ok, since block wasn't found, let's go to disk and try to read all unread blocks up to that point
			//return LoadBlocksToAddress( m_Root, block_address );
			return LoadBlock( block_address );
		}
		else
		{
			return( NULL );
		}
	}
}


FuelBlock * TextFuelDB :: FindBlockLocal( gpstring const & block_address )
{
	GPSTATS_SYSTEM( SP_FUEL );

#if !GP_RETAIL

	gpstring::size_type n = block_address.find_first_of( "*" );

	if( n != gpstring::npos )
	{
		gpstring error = "Address contains wildcard, so it's non-unique.  You can't construct a handle to a non-uniquely addressed block->";
		error += "\nSaid block address = " + block_address;
		gpassertm( 0, error.c_str() );
		gpwarning( error.c_str() );
		return( NULL );
	}

#endif

	FuelBlock * currentBlock = m_Root;
	gpstring blockName;
	gpstring blockAddress( block_address );

	while( stringtool::GetFrontDelimitedString( blockAddress, ':', blockName ) )
	{
		stringtool::RemoveFrontDelimitedString( blockAddress, ':', blockAddress );

		fuel_block_list::iterator i;
		for( i = currentBlock->mChildren.begin(); i != currentBlock->mChildren.end(); ++i )
		{
			if( blockName.same_no_case( (*i)->GetName() ) )
			{
				currentBlock = (*i);
				break;
			}
		}
	}

	if( currentBlock && block_address.same_no_case( currentBlock->GetAddress() ) )
	{
		return currentBlock;
	}
	else
	{
		return NULL;
	}
}




FuelBlock * TextFuelDB :: CreateBlock(	FuelBlock * parent,
										gpstring const & sBlockName, 
										gpstring const & originPath, 
										bool bDirectory )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( !sBlockName.empty() );

	if( parent->IsDirectory() )
	{
		if(	(!bDirectory && parent->HasChildDirBlockDumb( sBlockName ) ) || 
			( bDirectory && parent->HasChildBlockDumb( sBlockName ) ) )
		{
			gpfatalf(( "Block name collision.\nParent block '%s' has a dir and %s child block named '%s'",
						parent->GetAddress().c_str(),
						bDirectory ? "a second dir" : "non-dir", sBlockName.c_str() ));
		}
	}

	FuelBlock * pNewBlock = new FuelBlock;
	
	gpassert( pNewBlock );
	
	pNewBlock->SetDB( this );

	gpstring sTempBlockName( sBlockName );
	sTempBlockName.to_lower();
	pNewBlock->mName		= sTempBlockName;

	//gpstring sTempRelOriginPath( originPath );
	//sTempRelOriginPath.to_lower();
	pNewBlock->SetOrigin( originPath );

	RemoveFileFromDeleteList( originPath );

	pNewBlock->mIsDirectory	= bDirectory;
	pNewBlock->mIsValid		= true;
	pNewBlock->SetIsLoaded( false );

	bool success = parent->AddChild( pNewBlock );
	//parent->ClientModifiedChildBlocks();

	gpassert( success );
	return( pNewBlock );
}


// this recursive function should only be entered with a dir block
bool TextFuelDB :: UnloadBlock( FuelBlock * block )
{
	GPSTATS_SYSTEM( SP_FUEL );

	if( block->IsDirty() )
	{
		gpwarningf(( "You're unloading a dirty block = %s, changes not saved to .gas file\n", block->GetAddress().c_str() ));
	}
	gpassert( block->IsDirectory() );

	if( !block->GetChildBlocks().empty() )
	{
		for( fuel_block_list::iterator i = block->GetChildBlocks().begin(); i != block->GetChildBlocks().end(); ++i )
		{
			delete *i;
		}
		block->GetChildBlocks().clear();
	}

	block->SetIsLoaded( false );
	return true;
}


void TextFuelDB :: FindChildFilesAndDirs( gpstring const & rootPath, FileEntryColl & files, StringColl & dirs )
{
	GPSTATS_SYSTEM( SP_FUEL );

	//---- prep find root
	FileSys::AutoFindHandle hFindRoot( rootPath );
	if_not_gpassertf( !hFindRoot.IsNull(), ("Can't FindChildFilesAndDirs for [%s]", rootPath.c_str()) )
	{
		// $ early bailout if dir invalid
		return;
	}

	FileSys::FindData TempFindData;
	FileSys::AutoFindHandle hFound;

	//----- find files
	hFound.Find( hFindRoot, "*.gas", FileSys::FINDFILTER_FILES );

	if( hFound )
	{
		FileSys::FindData findData;

		while( hFound.GetNext( findData, false ) )
		{
			if( findData.m_Size != 0 )
			{
				files.push_back( FileEntry( findData.m_Name, findData.m_LastWriteTime ) );
			}
		}
	}

	//----- find dirs
	hFound.Find( hFindRoot, "*", FileSys::FINDFILTER_DIRECTORIES );

	if( hFound )
	{
		gpstring name;

		while( hFound.GetNext( name, false ) )
		{
			name.append( "\\" );
			dirs.push_back( name );
		}
	}
}


bool TextFuelDB :: LoadAll( FuelBlock * dirBlock )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( dirBlock->IsDirectory() );

	if( !dirBlock->IsLoaded() )
	{
		FuelSys::StringSet paths;
		StringColl childDirs;
		FileEntryColl childFiles;
		FindChildFilesAndDirs( dirBlock->GetOriginPath(), childFiles, childDirs );

		//----- now create blocks from immediate child directory names and the immediate .gas files, then recurse
		// if this block wasn't "loaded" yet, we must load it's contents: to create FuelBlocks
		// from the immediate .gas files and child directories...

		// create children from the .gas files
		{for( FileEntryColl::iterator i = childFiles.begin(); i != childFiles.end(); ++i )
		{
			paths.insert( gpstring( i->m_Name ).appendf( ".%x", i->m_LastWriteTime ) );
			FuelBlock * found = NULL;
			if( !LoadBlocksFromFile( dirBlock, i->m_Name, gpstring::EMPTY, found ) )
			{
				return false;
			}
		}}

		// create children from immediate subdirectory names
		{for( StringColl::iterator i = childDirs.begin(); i != childDirs.end(); ++i )
		{
			paths.insert( *i );
			FuelBlock * child_dir = CreateChildDirectoryBlock( dirBlock, (*i) );
			gpassert( child_dir );
		}}
		// now we mark this block as loaded
		dirBlock->SetIsLoaded( true );
		DWORD sum = FuelSys::CalcChecksum( paths );
		dirBlock->SetChildPathChecksum( sum );
	}

	// if we're in relaxed mode, don't bother loading any other blocks...
	if( TestOptions( OPTION_JIT_READ ) )
	{
		return( true );
	}
	else
	{
		// boy, strict mode... load everything from here down...
		for( fuel_block_list::iterator i = dirBlock->mChildren.begin(); i != dirBlock->mChildren.end(); ++i )
		{
			if( (*i)->IsDirectory() )
			{
				if( !LoadAll( (*i) ) )
				{
					return false;
				}
			}
		}
		return( true );
	}
}


FuelBlock * TextFuelDB :: LoadBlock( gpstring const & _address )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelBlock * parent = m_Root;
	gpassert( parent );
	gpassert( parent->IsLoaded() );

	gpstring childName, nextChildName;
	gpstring address( _address );
	StringColl childDirs;
	FileEntryColl childFiles;

	while( stringtool::GetFrontDelimitedString( address, ':', childName ) )
	{
		FuelBlock * child = NULL;

		for( fuel_block_list::iterator i = parent->GetChildBlocks().begin(); i != parent->GetChildBlocks().end(); ++i )
		{
			if( childName.same_no_case( (*i)->GetName() ) )
			{
				child= *i;
				break;
			}
		}
		if( !child )
		{
			return NULL;
		}

		if( child->IsDirectory() && !child->IsLoaded() )
		{
			childDirs.clear();
			childFiles.clear();

			FindChildFilesAndDirs( child->GetOriginPath(), childFiles, childDirs );

			// create children from immediate subdirectory names
			{for( StringColl::iterator i = childDirs.begin(); i != childDirs.end(); ++i )
			{
				FuelBlock * child_dir = CreateChildDirectoryBlock( child, (*i) );
				gpassert( child_dir );
			}}

			// look-ahead to see if the next child is also a dir
			bool fullyLoadChild = true;
/*
			if( (stringtool::GetNumDelimitedStrings( address, ':' ) > 1) && stringtool::GetDelimitedValue( address, ':', 1, nextChildName ) )
			{
				for( fuel_block_list::iterator i = parent->GetChildBlocks().begin(); i != parent->GetChildBlocks().end(); ++i )
				{
					if( (*i)->IsDirectory() && nextChildName.same_no_case( (*i)->GetName() ) )
					{
						fullyLoadChild = false;
						break;
					}
				}
			}
*/
			// create children from the .gas files
			if( fullyLoadChild )
			{
				{for( FileEntryColl::iterator i = childFiles.begin(); i != childFiles.end(); ++i )
				{
					FuelBlock * dummy = NULL;
					if( !LoadBlocksFromFile( child, i->m_Name, "", dummy ) )
					{
						return NULL;
					}
				}}

				// now we mark this block as loaded
				child->SetIsLoaded( true );
			}
		}
		stringtool::RemoveFrontDelimitedString( address, ':', address );
		parent = child;
	}

	gpassert( parent->IsLoaded() );
	return parent;
}


FuelBlock * TextFuelDB :: CreateChildDirBlock( FuelBlock * parent, gpstring const & sName )
{
	GPSTATS_SYSTEM( SP_FUEL );

	// validate parameters:
	//	if a directory block is being created, it MUST be
	//	a child of another directory block->  Why?  If it isn't what we're saying
	//  is that a GAS block in a .txt file can be the parent of a directory?!
	//  There is no way to express this in conventional file systems.  How can a -file-
	//	be a parent to a directory?  It can't.
	
	if( !parent->IsDirectory() )
	{
		gpstring sError = "FuelDB: you asked to create a child directory block to a non-directory parent.  This is illegal.\n";
		gpwarning( sError.c_str() );
		gpassertm( 0, sError.c_str() );
		return( 0 );
	}

	gpassert( parent->GetOriginPath()[ strlen( parent->GetOriginPath() ) -1 ] == '\\' );

	parent->Load();

	FuelBlock * pNewBlock = CreateBlock(	parent,
											sName,
											parent->GetOriginPath() + sName + "\\",
											true );
	pNewBlock->SetIsLoaded( true );

	return( pNewBlock );
}


FuelBlock * TextFuelDB :: CreateChildDirectoryBlock( FuelBlock * parent, gpstring const & _dirName )
{
	//GPSTATS_SYSTEM( SP_FUEL );

#	ifdef DEBUG_FUEL
	gpgenericf(( "Fuel[%08d] - creating block from dir = %s\n", size(), dirPath.c_str() ));
#	endif

	gpstring dirName( _dirName );
	stringtool::RemoveTrailingBackslash( dirName );

	gpassertm( parent->IsDirectory(), "Dirs must have other dirs as parents." );

	gpstring dirPath( parent->GetOriginPath() );
	dirPath.append( dirName );
	dirPath.append( "\\" );

	FuelBlock * new_block = CreateBlock( parent, dirName, dirPath, true );

	return( new_block );
}


bool TextFuelDB :: LoadBlocksFromFile(	FuelBlock * parent,
										gpstring const & filename, 
										gpstring const & relTargetAddress,
										FuelBlock *& targetBlock )
{
	//GPSTATS_SYSTEM( SP_FUEL );

#	ifdef DEBUG_FUEL
	gpgenericf(( "Fuel[%08d] - creating blocks from file: %s\n", size(), filePath.c_str() ));
#	endif

	unsigned int bytes_read = 0;

	gpstring filePath( parent->GetOriginPath() );
	filePath.append( filename );

	FileSys::AutoFileHandle file;
	FileSys::AutoMemHandle mem;

	if ( !file.Open( filePath.c_str() ) || !mem.Map( file ) )
	{
		return false;
	}

	if ( mem.GetSize() == 0 )
	{
		return true;
	}

	const char *mReadBuffer = (const char*)mem.GetData();
	const int mReadBufferSize = mem.GetSize();
	bytes_read = mReadBufferSize;

	// get time of last file modification
	FILETIME OriginFileTime;
	file.GetFileTimes( NULL, NULL, &OriginFileTime );

	// now parse the file
	unsigned int file_size = bytes_read;

	// parse buffer here
	unsigned int buffer_read_position = 0;

	while( buffer_read_position < file_size )
	{
		if( !ReadBlocks(	parent,
							relTargetAddress,
							targetBlock,
							mReadBuffer,
							file_size,
							filePath,
							& OriginFileTime,
							buffer_read_position ) )
		{
			gpstring error = "Fuel: failed to parse file.  file = ";
			error += filePath + "\n";
			gpwarning( error.c_str() );
			gpassertm( 0, error.c_str() );
			return( false );
		}
	}
	++m_FilesLoaded;
	return( true );
}


bool TextFuelDB :: ReadKeyValuePair( gpstring & sKey, gpstring & sValue, eFuelValueProperty & valueProperties, const char * buffer, unsigned int & pos )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	valueProperties = FVP_NONE;

	//----- read key

	// seek over white-space

	while( giswhite( buffer[pos] ) ) { ++pos; }

	// check to see if the key defines a property for the value
	if( ( buffer[pos+1] == ' ' || buffer[pos+1] == '\t' ) && gisnamechar( buffer[pos+2] ) )
	{
		switch( buffer[pos] )
		{
		case 'i':
			valueProperties = FVP_NORMAL_INT;
			break;
		case 'x':
			valueProperties = FVP_HEX_INT;
			break;
		case 'f':
			valueProperties = FVP_FLOAT;
			break;
		case 'd':
			valueProperties = FVP_DOUBLE;
			break;
		case 'b':
			valueProperties = FVP_BOOL;
			break;
		case 's':
			valueProperties = FVP_STRING;
			break;
		case 'p':
			valueProperties = FVP_SIEGE_POS;	
			break;
		case 'q':
			valueProperties = FVP_QUAT;	
			break;
		case 'v':
			valueProperties = FVP_VECTOR_3;	
			break;

		default:
			gperrorf(( "'%c' is an unsupported value property.", buffer[pos] ));
			return false;
		};
		pos += 2;
	}
	
	if( (valueProperties & FVP_NUMERIC_PROPERTIES) == 0 )
	{
		valueProperties |= FVP_STRING;
	}

	//----- read value

	// seek to end of name
	unsigned int begin = pos;

	while( gisnamechar( buffer[pos] ) )	{ ++pos; }

	sKey.append( &buffer[begin], pos-begin );

	sKey.to_lower();

	while( giswhite( buffer[pos] ) )	{ ++pos; }

	if( buffer[pos] != '=' )
	{
		return false;
	}

	++pos;

	while( giswhite( buffer[pos] ) ) { ++pos; }

	switch( buffer[pos] )
	{
		case '[':
		{
			if( buffer[pos+1] == '[' )
			{
				valueProperties |= FVP_BRACKETED_STRING;

				pos += 2;
				begin = pos;

				while( !(buffer[pos] == ']' && buffer[pos+1] == ']') ) { ++pos; }

				sValue.append( &buffer[begin], pos-begin );

				while( buffer[pos] != ';' )	{ ++pos; }
			}
			else
			{
				begin = pos;
				while( buffer[pos] != ';' )
				{
					if( buffer[pos] == '}' )
					{
						return false;
					}
					else
					{
						++pos;
					}
				}
				sValue.append( &buffer[begin], pos-begin );
				sValue.to_lower();
			}
			break;
		}

		case '"':
		{
			valueProperties |= FVP_QUOTED_STRING;

			++pos;
			begin = pos;

			while( buffer[pos] != '"' )	{ ++pos; }

			sValue.append( &buffer[begin], pos-begin );

			while( buffer[pos] != ';' )	{ ++pos; }

			break;
		}
										
		default:
		{
			begin = pos;
			while( buffer[pos] != ';' )
			{
				if( buffer[pos] == '}' )
				{
					return false;
				}
				else
				{
					++pos;
				}
			}

			sValue.append( &buffer[begin], pos-begin );

			if( valueProperties & FVP_STRING )
			{
				sValue.to_lower();
			}

			break;
		}
	}

	if( buffer[pos] == ';' )
	{
		++pos;
		return true;
	}
	else
	{
		return false;
	}
}


FuelBlock * TextFuelDB :: FindCreateLeafBlockFromAddress( FuelBlock * pParent, const char * buffer, unsigned int & pos )
{
	GPSTATS_SYSTEM( SP_FUEL );

	unsigned int begin, end;
	begin = end = 0;
	FuelBlock * pBlock = NULL;

	while( buffer[pos] != '=' )
	{
		while( giswhite(buffer[pos] ) )		{ ++pos; }
		begin = pos;
		
		while( gisnamechar( buffer[pos] ) )	{ ++pos; }
		end = pos;

		while( giswhite(buffer[pos] ) )		{ ++pos; }

		// new block
		if( buffer[pos] == ':' )
		{
			gpstring sName( &buffer[begin], end-begin );

			fuel_block_list::iterator i;

			pBlock = NULL;

			// $$$ need more speed... change to a map
			for( i = pParent->GetChildBlocks().begin(); i != pParent->GetChildBlocks().end(); ++i )
			{
				if( sName.same_no_case( (*i)->GetName() ) )
				{
					pBlock = *i;
					break;
				}
			}

			if( !pBlock )
			{
				pBlock = CreateBlock(	pParent,
										sName,
										pParent->GetOriginPath(),
										false );
				// set last modification time for file where block comes from
				pBlock->mOriginFileTime = pParent->GetOriginFileTime();
				pBlock->SetIsLoaded( true );
			}

			pParent = pBlock;

			++pos;
		}
	}
	pos = begin;
	return pBlock;
}


bool TextFuelDB :: ReadBlocks(	FuelBlock * parent,
								gpstring const & relTargetAddress,
								FuelBlock const * & targetBlock,
								const char * buffer,
								const unsigned int buffer_size,
								gpstring const & originPath,
								FILETIME const * OriginFileTime,
								unsigned int & position )
{
	__try
	{
		return ReadBlocksWrapped(	parent,
									relTargetAddress,
									targetBlock,
									buffer,
									buffer_size,
									originPath,
									OriginFileTime,
									position  );
	}
	__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH, EXCEPTION_EXECUTE_HANDLER ) )
	{
		m_bCriticalError = true;
#if	!GP_RETAIL
		gperrorf(( "DS encountered a critical error while reading file - file=%s\nPlease repair, replace or remove file.\n",
					originPath.c_str() ));
#endif
	}
	return false;
}


bool TextFuelDB :: ReadBlocksWrapped(	FuelBlock * parent,
										gpstring const & relTargetAddress,
										FuelBlock const * & targetBlock,
										const char * buffer,
										const unsigned int buffer_size,
										gpstring const & originPath,
										FILETIME const * OriginFileTime,
										unsigned int & position )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelBlock * mCurrentBlock = 0;

	unsigned int mKeyBegin, mKeyEnd, mValueBegin, mValueEnd;
	unsigned int mNameBegin, mNameEnd, mTypeBegin, mTypeEnd;
	bool mPreserveValueCase = false;
	bool mDevBlock			= false;
	
	mKeyBegin	=
	mKeyEnd		=
	mValueBegin	=
	mValueEnd	=
	mNameBegin	=
	mNameEnd	=
	mTypeBegin	=
	mTypeEnd	= position;

	unsigned int mode = FIND_NEXT_CONTEXT;
	unsigned int mode_entered_at_position = position;

	while( position < buffer_size && buffer[position] != 0 )
	{
		switch( mode )
		{
			case FIND_NEXT_CONTEXT:
			{
				mPreserveValueCase = false;

				// seek first non-whitespace char
				while( (position < buffer_size) && giswhite( buffer[position] ) ) { ++position; }
				if( position >= buffer_size ) {
					return( true );
				}

				// choose next context based on where we landed
				if( gisnamechar( buffer[position] ) )
				{
					unsigned int lookAhead = position;
					while( gisnamechar( buffer[lookAhead] ) ) { ++lookAhead; }
					// seek past white space...
					while( giswhite( buffer[lookAhead] ) ) { ++lookAhead; }

					if( buffer[lookAhead] == ':' )
					{
						mode = CONTEXT_BRIEF_HEADER;
						mode_entered_at_position = position;
					}
					else
					{
						mode = CONTEXT_KEY_VALUE;
						mode_entered_at_position = position;
					}
				}
				else if( buffer[position] == '[' )
				{
					mode = CONTEXT_HEADER_BEGIN;
					mode_entered_at_position = position;
				}
				else if( buffer[position] == '/' && buffer[position+1] == '*' )
				{
					mode = CONTEXT_C_COMMENT;
					mode_entered_at_position = position;
				}
				else if( buffer[position] == '/' && buffer[position+1] == '/' )
				{
					mode = CONTEXT_CPP_COMMENT;
					mode_entered_at_position = position;
				}
				else if( buffer[position] == '}' )
				{
					mode = CONTEXT_BLOCK_FINISHED;
					mode_entered_at_position = position;
				}
				else if( giseol( &buffer[position] ) )
				{
					// shouldn't really get here
					mode = SEEK_NEXT_PHYSICAL_LINE;
					mode_entered_at_position = position;
				}
				else {
					goto parse_error;
				}
				break;
			}

			case SEEK_NEXT_PHYSICAL_LINE:
			{
				while( !giseol( &buffer[position] ) && ( position < buffer_size )  ) { ++position; }
				position+=2;
				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			case SEEK_NEXT_LOGICAL_LINE:
			{
				while( buffer[position] != ';' && ( position < buffer_size ) ) { ++position; }
				++position;
				++m_LogicalLinesLoaded;
				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			// "[t:", "[n:", or "[name"
			case CONTEXT_HEADER_BEGIN:
			{
				gpassert( buffer[position] == '[' );

				// if we already created a block, then this must be a child block header, recurse to create it
				if( mCurrentBlock )
				{
					gpstring newAddress;
					stringtool::RemoveFrontDelimitedString( relTargetAddress, ':', newAddress );

					ReadBlocksWrapped(	mCurrentBlock,
										newAddress,
										targetBlock,
										buffer, 
										buffer_size, 
										originPath,
										OriginFileTime,
										position );

					mode = FIND_NEXT_CONTEXT;
					mode_entered_at_position = position;
				}
				// otherwise, create the first block of this scope
				else
				{
					gpassert( buffer[position] == '[' );

					if( _memicmp( buffer + position, "[dev,", 5 ) == 0 )
					{
						SetDevBlocksEncountered( true );
						if( TestOptions( OPTION_NO_READ_DEV_BLOCKS ) )
						{
							mode = CONTEXT_SKIP_BLOCK;
							mode_entered_at_position = position;
							break;
						}
						mDevBlock = true;
						position += 5;
					}
					else
					{	
						++position;
					}

					if( (buffer[position] == 't' && buffer[position+1] == ':') )
					{
						mode = CONTEXT_HEADER_VERBOSE_TYPE;
						mode_entered_at_position = position;
					}
					else if( (buffer[position] == 'n' && buffer[position+1] == ':') )
					{
						mode = CONTEXT_HEADER_VERBOSE_NAME;
						mode_entered_at_position = position;
					}
					else if( gisnamechar( buffer[position] ) )
					{
						mode = CONTEXT_HEADER_TERSE_NAME;
						mode_entered_at_position = position;
					}
					else
					{
						goto parse_error;
					}
				}
				break;
			}

			// parse: ..."t:<name>,"
			case CONTEXT_HEADER_VERBOSE_NAME:
			{
				// read name
				if( buffer[position] == 'n' && buffer[position+1] == ':' )
				{
					position+=2;
					gpassert( gisnamechar( buffer[position] ) );
					mNameBegin = position;

					// seek end of type name
					while( gisnamechar( buffer[position] ) ) { ++position; }

					mNameEnd = position;

					if( (buffer[position] == ',') && (buffer[position+1] == 't') && (buffer[position+2] == ':') )
					{
						mode = CONTEXT_HEADER_VERBOSE_TYPE;
						mode_entered_at_position = ++position;
					}
					else if( buffer[position] == ']' )
					{
						mode = CONTEXT_HEADER_END;
						mode_entered_at_position = position;
					}
					else
					{
						goto parse_error;
					}
				}
				else
				{
					goto parse_error;
				}
				break;
			}

			// parse: type of ..."t:<type>,"
			case CONTEXT_HEADER_VERBOSE_TYPE:
			{
				// read type
				if( buffer[position] == 't' && buffer[position+1] == ':' )
				{
					position+=2;
					gpassert( gisnamechar( buffer[position] ) );
					mTypeBegin = position;

					// seek end of type name
					while( gisnamechar( buffer[position] ) ) { ++position; }

					mTypeEnd = position;

					if( buffer[position] == ']' )
					{
						mode = CONTEXT_HEADER_END;
						mode_entered_at_position = position;
					}
					else
					{
						++position;
						mode = CONTEXT_HEADER_VERBOSE_NAME;
						mode_entered_at_position = position;
					}
				}
				else
				{
					goto parse_error;
				}
				break;
			}

			// parse: name of ..."n:<name>,"
			case CONTEXT_HEADER_TERSE_NAME:
			{
				gpassert( gisnamechar( buffer[position] ) );

				mNameBegin = position;

				// seek to end of name
				while( gisnamechar( buffer[position] ) ) { ++position; }

				if( buffer[position] == ']' )
				{
					mNameEnd = position;
					mode = CONTEXT_HEADER_END;
					mode_entered_at_position = position;
				}
				else
				{
					goto parse_error;
				}

				break;
			}

			// parse: ...",s]" or ..."]"
			case CONTEXT_HEADER_END:
			{
				bool mIsSmart = false;

				if( (buffer[position]==',') && (buffer[position+1]=='s') )
				{
					mIsSmart = true;
					position +=2;
				}
				if( buffer[position] == ']' )
				{
					gpassert( mCurrentBlock == 0 );
					gpassert( mNameBegin<mNameEnd );

					// create and set name
					gpstring block_name( &buffer[mNameBegin], mNameEnd-mNameBegin );
					gpassertm( !block_name.empty(), "Block must be named." );

					mCurrentBlock = CreateBlock(	parent,
													block_name,
													originPath,
													false );
					if( mDevBlock )
					{
						mCurrentBlock->SetIsDevOnly();
					}

					if( block_name.same_no_case( relTargetAddress ) )
					{
						gpassertm( targetBlock == NULL, "There can be only one target found." );
						targetBlock = mCurrentBlock;
					}

					// set last modification time for file where block comes from
					if( OriginFileTime )
					{
						mCurrentBlock->mOriginFileTime = *OriginFileTime;
					}

					// set type
					gpstring block_type( &buffer[mTypeBegin], mTypeEnd-mTypeBegin );
					block_type.to_lower();

					mCurrentBlock->mType = block_type;
					mCurrentBlock->SetIsLoaded( true );

//#					ifdef DEBUG_FUEL
//					gpgenericf(( "\n\n[ %s ]\n", block_name.c_str() ));
//#					endif
				}
				else
				{
					goto parse_error;
				}
				// seek past body opening brace
				while( buffer[position] != '{' ) { ++position; }
				gpassert( buffer[position] == '{' );
				++position;
				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			case CONTEXT_SKIP_BLOCK:
			{
				while( buffer[position] != '{' )
				{
					++position;
				}

				++position;
				DWORD braceCount = 1;

				while( braceCount )
				{
					if( buffer[position] == '{' )
					{
						++braceCount;
					}
					else if( buffer[position] == '}' )
					{
						--braceCount;
					}
					++position;
				}

				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			// parse: block_name:key=value;
			case CONTEXT_BRIEF_HEADER:
			{
				// if we already created a block, then this must be a child block header, recurse to create it

				FuelBlock * pBlock = FindCreateLeafBlockFromAddress( mCurrentBlock, buffer, position );
				gpassert( pBlock );

				gpstring sKey, sValue;

				eFuelValueProperty valueProperties = FVP_NONE;

				if( ReadKeyValuePair( sKey, sValue, valueProperties, buffer, position ) )
				{
					pBlock->mBody.insert( BlockDataMap::value_type( sKey, ValueData( sValue, valueProperties ) ) );
				}
				else
				{
					goto parse_error;
				}

				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;

				break;
			}

			case CONTEXT_KEY_VALUE:
			{
				gpassert( gisnamechar( buffer[position] ) );
				gpstring sKey, sValue;

				eFuelValueProperty valueProperties = FVP_NONE;

				if( !ReadKeyValuePair( sKey, sValue, valueProperties, buffer, position ) )
				{
					goto parse_error;
				}

				mCurrentBlock->mBody.insert( BlockDataMap::value_type( sKey, ValueData( sValue, valueProperties ) ) );

				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			case CONTEXT_BLOCK_FINISHED:
			{
				if( buffer[position]=='}' )
				{
					// we're done parsing this block
					++position;
					++m_GasBlocksLoaded;
					return true;
				}
				else
				{
					goto parse_error;
				}
				break;
			}

			case CONTEXT_C_COMMENT:
			{
				while( !((buffer[position]=='*') && (buffer[position+1]=='/')) ) { ++position; }
				position += 2;
				mode = FIND_NEXT_CONTEXT;
				mode_entered_at_position = position;
				break;
			}

			case CONTEXT_CPP_COMMENT:
			{
				mode = SEEK_NEXT_PHYSICAL_LINE;
				mode_entered_at_position = position;
				break;
			}

			default:
			{
				goto parse_error;
				break;
			}
		} // end master switch
	} // end master while
	return( true );


	parse_error:
	{
		gperrorf(( "Fuel: encountered GAS syntax error while '%s' - file=%s, line=%d, position entered=%d, current position=%d\n",
				   GetModeName( mode ).c_str(), originPath.c_str(),
				   GetPhysicalLineCountToBufferPosition( buffer, mode_entered_at_position ),
				   mode_entered_at_position, position ));

		if( mCurrentBlock )
		{
			// $$$ handle error here...
			//DestroyBlock( *mCurrentBlock );
			mCurrentBlock = 0;
		}

		mode = SEEK_NEXT_LOGICAL_LINE;
		mode_entered_at_position = position;

		return( false );
	}
}


bool TextFuelDB :: Save( bool save_all )
{
	GPSTATS_SYSTEM( SP_FUEL );

	if( TestOptions( OPTION_MEM_ONLY ) )
	{
		gperror( "You are big crazy person.  You can't save a OPTION_MEM_ONLY fuel db." );
		return( false );
	}

	ProcessFileDeleteList();
	gpassert( m_Root && m_Root->IsDirectory() );
	return( WriteDirectoryBlock( m_Root, save_all ) );
}


bool TextFuelDB :: WriteDirectoryBlock( FuelBlock * block, bool save_all )
{
	GPSTATS_SYSTEM( SP_FUEL );

	/* 

	procedure:

	1. create the directory for this block if it doesn't exist
	
	2. sort children according to origin file - ignore directories
	write all like-filed blocks to buffer and
	save buffer as file
	
	3. for all children who are directories, recurse 
	
	*/

	gpassert( block->IsDirectory() );
	
	//----- 1.

	if( save_all || (!save_all && block->mIsDirty) )
	{
		gpstring newPath;
		stringtool::ReplaceRootPath( block->GetOriginPath(), GetHomeReadPath(), GetHomeWritePath(), newPath );

		// save current buffer
		if ( !FileSys::DoesPathExist( newPath ) )
		{
			if( !::CreateDirectory( newPath.c_str(), 0 ) )
			{
				// $$ could display here...
				//return( false );
			}
		}
	}
	
	//----- 2.

	typedef std::multimap<gpstring, FuelBlock *, istring_less> file_to_block_map;
	typedef std::set<gpstring, istring_less> dirty_file_names;
	file_to_block_map mMap;
	dirty_file_names mDirtyFileNames;

	{
	std::list<FuelBlock*>::const_iterator j;

	for( j = block->GetChildBlocks().begin(); j != block->GetChildBlocks().end(); ++j )
	{
		if( (*j)->IsDirectory() )
		{
			continue;
		}

		mMap.insert( file_to_block_map::value_type( (*j)->GetOriginPath(), *j ) );

		if( (*j)->mIsDirty )
		{
			mDirtyFileNames.insert( (*j)->GetOriginPath() );
		}
	}}

	// if we're not saving all, save just the dirty blocks... we do this by removing
	// all blocks who are not in a 'dirty' file from the file_to_block_map
	if( !save_all )
	{
		file_to_block_map::iterator i;
		for( i = mMap.begin(); i != mMap.end(); )
		{
			dirty_file_names::iterator d = mDirtyFileNames.find( (*i).first );
			if( d == mDirtyFileNames.end() )
			{
				i = mMap.erase( i );
			}
			else {
				++i;
			}
		}
	}

	// now process all child blocks and write like blocks to same file

	if( !mMap.empty() )
	{
		file_to_block_map::iterator current = mMap.begin();

		gpstring writePath;
		stringtool::ReplaceRootPath( ((*current).second)->GetOriginPath(), GetHomeReadPath(), GetHomeWritePath(), writePath );

		FileSys::File writeFile;
		writeFile.CreateAlways( writePath );
		gpassert( writeFile.IsOpen() );

		//FILE * writeFile = fopen( writePath.c_str(), "wb" );

		for( ; current != mMap.end(); ++current )
		{
			if( writeFile == NULL || !writeFile.IsOpen() )
			{
				gperrorf(( "Fuel failed to write file %s, file likely marked read-only.\n", writePath.c_str() ));
			}
			else
			{
				FileSys::FileWriter fileWriter( writeFile );

				//----- write current block to buffer ( this is a recursive function )
				bool success = WriteBlock(	fileWriter,
											((*current).second),
											0	);
				gpassert( success );
			}

			// peek ahead at next block to be written
			file_to_block_map::iterator next = current;
			++next;

			// see if we need to switch files we're writing to
			if( next==mMap.end() )
			{
				writeFile.Close();
			}
			else if( !(*current).first.same_no_case( (*next).first) )
			{
				((*current).second)->mIsDirty = false;
				writeFile.Close();

				writePath.clear();
				stringtool::ReplaceRootPath( ((*next).second)->GetOriginPath(), GetHomeReadPath(), GetHomeWritePath(), writePath );

				writeFile.CreateAlways( writePath );
				gpassert( writeFile.IsOpen() );
			}
		}
	}

	//----- 3.

	// now recurse into children which are directories
	{std::list<FuelBlock*>::const_iterator j;

	for( j = block->GetChildBlocks().begin(); j!=block->GetChildBlocks().end(); ++j )
	{
		if( (*j)->IsDirectory() )
		{
			if( save_all || ( !save_all && (*j)->IsDirty() ) )
			{
				WriteDirectoryBlock( (*j), save_all );
			}
		}
	}}

	//----- finish
	
	// clear dirty flag for directory as it, and all it's children have been written out
	block->mIsDirty = false;
	return( true );
}


bool TextFuelDB :: WriteBlock(	FileSys::StreamWriter & writer,
								FuelBlock * block,
								unsigned int file_block_depth )
{
	GPSTATS_SYSTEM( SP_FUEL );

	/*

	procedure:

	1. write header
	2. write all keys,values
	3. write all children ( recursively ) - who happen to have the same file_origin as the input block

	*/

	// validate parameters
	if( *block->GetName() == 0 )
	{
		gpassert( 0 );
		return( false );	// all blocks -must- be named
	}

	gpassertm( !block->IsDirectory(), "You can't ask to write a dir block out via this function.\n" );

	bool success = true;

	/*
	check if block can be written in brief form:

	-	must have no type, only name
	-	must have a single key/value pair
	*/

	bool BlockEmpty		= block->mBody.empty() && block->mChildren.empty();

	////////////////////////////////////////////////////////////////////////////////
	//	write normal block

	if( BlockEmpty && !TestOptions( OPTION_SAVE_EMPTY_BLOCKS ) )
	{
		return true;
	}
	// 1. write header

	gpstring header;
	stringtool::PadBackToLength( header, '\t', file_block_depth );

	header += "[";

	gpassert( *block->GetType() || *block->GetName() );

	if( block->IsDevOnly() )
	{
		header += "dev,";
	}

	if( *block->GetType() != 0 )
	{
		header += "t:";
		header.append( block->GetType() );
		header.append( ",n:" );
	}

	header.append( block->GetName() );

	header.append( "]\n" );
	header.append( file_block_depth, '\t' );
	header.append( "{\n" );

	success = writer.WriteText( header ) && success;
	gpassert( success );

	// 2. write all keys,values

	++file_block_depth;
	{BlockDataMap::iterator i;
	for( i = block->mBody.begin(); i != block->mBody.end(); ++i )
	{
		gpstring line;
		stringtool::PadBackToLength( line, '\t', file_block_depth - 1 );

		////////////////////////////////////////
		//	write value property info
		if ( !TestOptions( OPTION_NO_WRITE_TYPES ) )
		{
			if( (*i).second.m_Properties & FVP_HEX_INT )
			{
				line += "  x ";
			}
			else if ( (*i).second.m_Properties & FVP_NORMAL_INT )
			{
				line += "  i ";
			}
			else if ( (*i).second.m_Properties & FVP_FLOAT )
			{
				line += "  f ";
			}
			else if ( (*i).second.m_Properties & FVP_DOUBLE )
			{
				line += "  d ";
			}
			else if ( (*i).second.m_Properties & FVP_BOOL )
			{
				line += "  b ";
			}
			else if ( (*i).second.m_Properties & FVP_SIEGE_POS )
			{
				line += "  p ";
			}
			else if ( (*i).second.m_Properties & FVP_QUAT )
			{
				line += "  q ";
			}
			else if ( (*i).second.m_Properties & FVP_VECTOR_3 )
			{
				line += "  v ";
			}
			else
			{
				line += "\t";
			}
		}
		else
		{
			line += "\t";
		}

		line += (*i).first;
		line += " = ";

		if( (*i).second.m_Properties & FVP_QUOTED_STRING )
		{
			line += "\"";
		}

		line += (*i).second.m_String;

		if( (*i).second.m_Properties & FVP_QUOTED_STRING )
		{
			line += "\"";
		}

		line += ";\n";
		success = writer.WriteText( line ) && success;
		if( !success ) {
			gpassert( 0 );
		}
	}}
	--file_block_depth;

	// 3. write children via recursion

	block->SortChildBlocks();

	{fuel_block_list::iterator i;
	for( i = block->GetChildBlocks().begin(); i != block->GetChildBlocks().end(); ++i )
	{
		if( stricmp( (*i)->GetOriginPath(), block->GetOriginPath() ) == 0 )
		{
			success = WriteBlock( writer, (*i), file_block_depth+1 );
			if( !success ) {
				gpassert( 0 );
			}
		}
	}}

	gpstring close_body;
	stringtool::PadBackToLength( close_body, '\t', file_block_depth );
	close_body += "}\n";
	success = writer.WriteText( close_body ) && success;

	gpassert( success  );

	return( success );
}


FuelBlock * TextFuelDB :: CreateChildBlock( FuelBlock * parent, gpstring const & sName, gpstring const & sFileName )
{
	GPSTATS_SYSTEM( SP_FUEL );

	// validate parameters:

	if( parent->IsDirectory() && sFileName.empty() ) {
		gperror( "FuelDB: if you're creating a new GAS block in a new file, the parent must be a directory.\n" );
		return( 0 );
	}

	if( !parent->IsDirectory() && !sFileName.empty() ) {
		gperror( "FuelDB: you can't specify a new file for a block if the parent isn't a directory.\n" );
		return( 0 );
	}

	gpstring originPath;

	if( parent->IsDirectory() )
	{
		originPath = parent->GetOriginPath();
		if( sFileName.empty() ) {
			// make up a default block name 
			originPath += "no_origin_file_specified.gas";
		}
		else {
			originPath += sFileName;
		}
	}
	else
	{
		originPath = parent->GetOriginPath();
	}

	FuelBlock * pNewBlock = CreateBlock(	parent,
											sName,				// duh
											originPath,			// same origin file as parent
											false );			// not a directory block
	pNewBlock->SetIsLoaded( true );

	return( pNewBlock );
}


bool TextFuelDB :: HasOriginFileChanged( const FuelBlock * pBlock ) const
{
	//GPSTATS_SYSTEM( SP_FUEL );

	gpassert( pBlock && !pBlock->IsDirectory() );
	gpassert( *pBlock->GetOriginPath()!=0 );

	FileSys::AutoFileHandle file( pBlock->GetOriginPath() );
	gpassert( file.IsOpen() );
	FILETIME lastChanged;
	file.GetFileTimes( NULL, NULL, &lastChanged );

	return( ::CompareFileTime( &lastChanged , &pBlock->GetOriginFileTime() ) == 1 );
}


FuelBlock * TextFuelDB :: Reload( FuelBlock * pReloadBlock )
{
	GPSTATS_SYSTEM( SP_FUEL );

	// procedure:
	//
	// 1. - remember reload block address and it's parent
	// 2. - find all blocks which share reload block's file
	// 3. - destroy all said blocks
	// 4. - reload said file thus re-creating all child fuel blocks
	// 5. - find the newly reloaded block which should have the same adress as the original reload block
	// 6. - return ptr to said block

	gpassert( pReloadBlock );

	
	////////////////////////////////////////////////////////////////////////
	//	1. - remember reload block address and it's parent

	gpstring sReloadBlockPath		= pReloadBlock->GetOriginPath();
	gpstring sReloadBlockAddress	= pReloadBlock->GetAddress();
	gpstring sReloadBlockName		= pReloadBlock->GetName();
	bool bReloadingDir				= pReloadBlock->IsDirectory();
	FuelBlock * pAffectedParent		= 0;


	if( bReloadingDir )
	{
		// reloading a directory means that the immediate parent which will be affected is the parent directory
		pAffectedParent = pReloadBlock->mParent;
		gpassert( pAffectedParent->IsDirectory() );
	}
	else
	{
		// reloading a non-directory block means we need to climb up block relationships until we step out of the same file scope in order
		//	to find the parent which will ultimately be affected when the entire .gas file is reloaded
		pAffectedParent = pReloadBlock;

		while( pAffectedParent->GetOriginPath().same_no_case( pReloadBlock->GetOriginPath() ) )
		{
			pAffectedParent = pReloadBlock->mParent;
			gpassert( pAffectedParent );
		}
	}

	
	////////////////////////////////////////////////////////////////////////
	//	2. - find all blocks which share reload block's file

	fuel_block_list & Children = pAffectedParent->GetChildBlocks();

	for( fuel_block_list::iterator i = Children.begin(); i != Children.end() ; )
	{
		if( sReloadBlockPath.same_no_case( (*i)->GetOriginPath() ) )
		{
			// 3. - destroy all said blocks
			delete (*i);
			i = Children.erase( i );
		}
		else
		{
			++i;
		}
	}


	////////////////////////////////////////////////////////////////////////
	//  4. - reload said file thus re-creating all child fuel blocks

	if( bReloadingDir )
	{
		FuelBlock * NewBlock = CreateChildDirectoryBlock( pAffectedParent, sReloadBlockName );
		gpassert( sReloadBlockPath.same_no_case( NewBlock->GetOriginPath() ) );

		if( !LoadAll( NewBlock ) )
		{
			gpstring error = "TextFuelDB :: Reload - failed to reload directory block->  Address = " + sReloadBlockAddress;
			gpwarning( error.c_str() );
			gpassertm( 0, error.c_str() );
			return( 0 );
		}
	}
	else
	{
		FuelBlock * targetBlock = NULL;
		gpstring sReloadBlockFilename = FileSys::GetFileName( pReloadBlock->GetOriginPath() );

		if( !LoadBlocksFromFile( pAffectedParent, sReloadBlockFilename, gpstring::EMPTY, targetBlock ) )
		{
			gpstring error = "TextFuelDB :: Reload - failed to reload block->  Address = " + sReloadBlockAddress;
			gpwarning( error.c_str() );
			gpassertm( 0, error.c_str() );
			return( 0 );
		}
	}

	
	////////////////////////////////////////////////////////////////////////
	//	5. confirm reload block exists again

	FuelBlock * ReloadedBlock = FindBlockLocal( sReloadBlockAddress );

	if( !ReloadedBlock )
	{
		gpstring error = "TextFuelDB :: Reload - failed to reload block->  Address = " + sReloadBlockAddress;
		gpwarning( error.c_str() );
		gpassertm( 0, error.c_str() );
		return( 0 );
	}
	else
	{
		// 6.
		return( ReloadedBlock );
	}
}


bool TextFuelDB :: ListOriginChangedBlocks( gpstring & output )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelSys::StringSet ChangedFileNames;

	fuel_block_list allBlocks;
	m_Root->NLListAllChildren( allBlocks );

	bool bBlocksChanged = false;

	for( fuel_block_list::iterator i = allBlocks.begin(); i != allBlocks.end(); ++i )
	{
		if( !(*i)->IsDirectory() && (*i)->HasOriginFileChanged() )
		{
			output.append( (*i)->GetAddress() );
			output.append( "\n" );
			bBlocksChanged = true;
		}
	}
	return( bBlocksChanged );
}


bool TextFuelDB :: ListOriginChangedFiles( gpstring & output )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelSys::StringSet ChangedFileNames;

	fuel_block_list allBlocks;
	m_Root->NLListAllChildren( allBlocks );

	// find changed blocks and remember their filenames
	bool bFilesChanged = false;

	for( fuel_block_list::iterator i = allBlocks.begin(); i != allBlocks.end(); ++i )
	{
		if( !(*i)->IsDirectory() && (*i)->HasOriginFileChanged() )
		{
			ChangedFileNames.insert( (*i)->GetOriginPath() );
			bFilesChanged = true;
		}
	}

	// output all the file names
	{for( FuelSys::StringSet::iterator i = ChangedFileNames.begin(); i != ChangedFileNames.end(); ++i ) {
		output += (*i) + "\n";
	}}
	return( bFilesChanged );
}


bool TextFuelDB :: ListDirtyBlocks( gpstring & output )
{
	GPSTATS_SYSTEM( SP_FUEL );

	bool bBlocksDirty = false;

	fuel_block_list allBlocks;
	m_Root->NLListAllChildren( allBlocks );

	for( fuel_block_list::iterator i = allBlocks.begin(); i != allBlocks.end(); ++i )
	{
		if( (*i)->IsDirty() )
		{
			output.append( (*i)->GetAddress() );
			output.append( "\n" );
			bBlocksDirty = true;
		}
	}
	return( bBlocksDirty );
}


bool TextFuelDB :: ListDirtyFiles( gpstring & output )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelSys::StringSet ChangedFileNames;

	fuel_block_list allBlocks;
	m_Root->NLListAllChildren( allBlocks );

	// find changed blocks and remember their filenames
	bool bBlocksDirty = false;

	for( fuel_block_list::iterator i = allBlocks.begin(); i != allBlocks.end(); ++i )
	{
		if( !(*i)->IsDirectory() && (*i)->HasOriginFileChanged() )
		{
			ChangedFileNames.insert( (*i)->GetOriginPath() );
			bBlocksDirty = true;
		}
	}

	// output all the file names
	{for( FuelSys::StringSet::iterator i = ChangedFileNames.begin(); i != ChangedFileNames.end(); ++i )
	{
		output += (*i) + "\n";
	}}

	return( bBlocksDirty );
}




void TextFuelDB :: AddFileToDeleteList( gpstring const & sAbsPath )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( !sAbsPath.empty() );
	m_FileDeleteList.insert( sAbsPath );
}




void TextFuelDB :: RemoveFileFromDeleteList( gpstring const & sAbsPath )
{
	GPSTATS_SYSTEM( SP_FUEL );

	if( !m_FileDeleteList.empty() ) {

		std::set< gpstring, istring_less >::iterator i;

		i = m_FileDeleteList.find( sAbsPath );

		if( i != m_FileDeleteList.end() )
		{
			m_FileDeleteList.erase( i );
		}
	}
}


void TextFuelDB :: ProcessFileDeleteList()
{
	GPSTATS_SYSTEM( SP_FUEL );

	std::set< gpstring, istring_less >::iterator i;

	for( i = m_FileDeleteList.begin(); i != m_FileDeleteList.end(); ++i )
	{
		// delete the original file
		gpstring deleteFilename( GetHomeWritePath() + (*i) );
		gpassert( stringtool::IsAbsolutePath( deleteFilename ) );

		bool success = !!::DeleteFile( deleteFilename.c_str() );
		if( !success )
		{
			gpwarningf(( "TextFuelDB :: ProcessFileDeleteList - couldn't delete file %s\n", deleteFilename.c_str() ));
		}

		if( success )
		{
			// now create an empty placeholder
			FileSys::File writeFile;
			writeFile.CreateAlways( deleteFilename.c_str() );
			gpassert( writeFile.IsOpen() );

			gpgenericf(( "TextFuelDB :: ProcessFileDeleteList - emptied file %s\n", deleteFilename.c_str() ));
		}
		else
		{
			gpgenericf(( "TextFuelDB :: ProcessFileDeleteList - failed to empty file %s\n", deleteFilename.c_str() ));
		}
	}
	m_FileDeleteList.clear();
}


gpstring const & TextFuelDB :: AddOriginPath( gpstring const & path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	std::pair<StringToIntMap::iterator, bool> rc = m_OriginPaths.insert( StringToIntMap::value_type( path, 0 ) );
	++rc.first->second;
	return( rc.first->first );
}


void TextFuelDB :: RemoveOriginPath( gpstring const & path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	StringToIntMap::iterator i = m_OriginPaths.find( path );

	if( i != m_OriginPaths.end() )
	{
		gpassert( (*i).second > 0 );
		(*i).second -= 1;

		if( GetInUserBlockDeleteRequestScope() && ( (*i).second == 0 ) )
		{
			AddFileToDeleteList( (*i).first );
			gpgenericf(( "Adding '%s' to deletion list.\n", (*i).first.c_str() ));
			//m_OriginPaths.erase( i );
		}
	}
	else
	{
		gpassertm( 0, "Not found" );
	}
}


void TextFuelDB :: SetDevBlocksEncountered( bool flag )
{
	m_DevBlockEncountered = flag;
}


#if !GP_RETAIL

void TextFuelDB :: Dump( ReportSys::ContextRef ctx, bool verbose )
{
	ReportSys::AutoReport autoReport( ctx );


	DWORD sizeOriginPaths = 0;
	StringToIntMap::iterator i;
	for( i = m_OriginPaths.begin(); i != m_OriginPaths.end(); ++i )
	{
		sizeOriginPaths += (*i).first.size() + 4;
	}

	if( verbose )
	{
		ctx->Output(  "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n" );
		ctx->OutputF( "Home read path		= %s\n", m_HomeReadPath.c_str() );
		ctx->OutputF( "Home write path		= %s\n", m_HomeWritePath.c_str() );
		ctx->OutputF( "Files parsed			= %d\n", m_FilesLoaded );
		ctx->OutputF( "Blocks read			= %d\n", m_GasBlocksLoaded );
		ctx->OutputF( "Logical lines read	= %d\n", m_LogicalLinesLoaded );

		ctx->OutputF(  "\nSize origin paths list (bytes) = %d\n", sizeOriginPaths );

		ctx->OutputF( "File delete list size= %d\n", m_FileDeleteList.size() );

		ctx->Output(  "Files in delete list:\n" );

		for( std::set< gpstring, istring_less >::iterator j = m_FileDeleteList.begin(); j != m_FileDeleteList.end(); ++j )
		{
			ctx->OutputF( "\t%s\n", (*j).c_str() );
		}

		gpstring dirtyFiles;
		ListDirtyFiles( dirtyFiles );
		ctx->OutputF(  "Dirty file list:\n%s\n", dirtyFiles.c_str() );

		gpstring originChangedFiles;
		ListOriginChangedFiles( originChangedFiles );
		ctx->OutputF(  "Dirty origin-changed files list:\n%s\n", originChangedFiles.c_str() );
	}
	else
	{
		ctx->OutputF(	"[textfueldb]   n.filesloaded=%d, n.blocksread=%d, s.originPaths=%d\n",
						m_FilesLoaded,
						m_GasBlocksLoaded,
						sizeOriginPaths );
	}
}

#endif // !GP_RETAIL






////////////////////////////////////////////////////////////////////////////////
//	







////////////////////////////////////////////////////////////////////////////////
//	BinaryFuelDB

BinaryFuelDB :: BinaryFuelDB()
:	m_BlockCache( ( 1 << 12 ) - 1 )
{
	m_Root				= NULL;
	m_TextDb			= NULL;

	m_Extension			= ".lqd20";

	m_Id				= m_InstanceCount;
	++m_InstanceCount;
	gpassert( m_InstanceCount < MAX_NUM_BINARY_DBS );
}


////////////////////////////////////////////////////////////////////////////////
//	
BinaryFuelDB :: ~BinaryFuelDB()
{
	--m_InstanceCount;

	gpassert( m_InstanceCount >= 0 );

	Shutdown();


	if( m_TextDb )
	{
		delete m_TextDb;
		m_TextDb = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: Init( eOptions options, gpstring const & readHome, gpstring const & writeHome )
{
	GPSTATS_SYSTEM( SP_FUEL );

#	if GP_RETAIL
	options |= OPTION_NO_READ_DEV_BLOCKS | OPTION_NO_WRITE_LIQUID;
#	endif // GP_RETAIL

	// special option for loc people
	if ( Config::DoesSingletonExist() && gConfig.GetBool( "force_retail_content", false ) )
	{
		options |= OPTION_NO_READ_DEV_BLOCKS;
	}

	m_HomeReadPath	= readHome;
	m_HomeWritePath	= writeHome;

	stringtool::AppendTrailingBackslash( m_HomeReadPath );
	stringtool::AppendTrailingBackslash( m_HomeWritePath );

	m_Options	= options | OPTION_BINARY_MODE;
	m_TextDb	= new TextFuelDB();

	m_NumDirtyDirectoriesFound		= 0;
	m_CondenseBytesCompiled			= 0;
	m_CondenseFilesCompiled			= 0;
	m_CondenseBytesWritten			= 0;
	m_CondenseFilesWritten			= 0;
	m_CondenseTimeReading			= 0;
	m_CondenseTimeCondensing		= 0;
	m_CondenseTimeWriting			= 0;

	m_CondenseKeyValuePairsCount	= 0;
	m_CondenseBlockCount			= 0;

	m_InefficientIntReadCount		= 0;
	m_InefficientFloatReadCount		= 0;
	m_InefficientBoolReadCount		= 0;
	m_InefficientSiegePosReadCount	= 0;
	m_InefficientQuatReadCount		= 0;
	m_InefficientVector3ReadCount	= 0;

	m_BytesFuelCurrentlyLoaded		= 0;

	m_CacheHits						= 0;
	m_CacheMisses					= 0;
	m_UncachingEnabled				= true;

	m_CurrentVisitCount				= 1;
	m_UnloadVisitCountThreashold	= 60;
	m_LastVisitIncrementTime		= 0;

	m_CurrentUnloadSweepIndex		= 0;
	m_MaxUnloadSweepIndex			= 0;

	gpassert( !m_Root );

	BinFuel::Header * root = LoadDirBlock( m_HomeReadPath );
	RFixupParentPointers( root, NULL );

#	if PROTECTED_BUFFERS
	gpmem::vaProtect( (BYTE*)root, ((BinFuel::DirHeader*)root)->m_Size );
#	endif

	gpassert( !root->IsStub() );

	if( !root )
	{
		gpfatal( "Binary fueldb failed to init." );
	}

	m_RootHeader	= root;
	m_Root			= new FastFuelHandle( root, this );

	m_CurrentUnloadSweepIndex = root->m_CacheId + 1;
	gpassert( !m_RootHeader->IsStub() );

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: Reinit()
{
	eOptions oldOptions		= m_Options;
	gpstring oldReadHome	= m_HomeReadPath;
	gpstring oldWriteHome	= m_HomeWritePath;

	Shutdown();
	return Init( oldOptions, oldReadHome, oldWriteHome );
}


///////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: Shutdown()
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( m_RootHeader );

	RUnloadChildren( m_RootHeader );
	OnBlockDelete( m_RootHeader );

	delete( m_Root );
	m_RootHeader = NULL;

	for( SlotColl::iterator i = m_BlockCache.begin(); i != m_BlockCache.end(); ++i )
	{
		if( (*i)->m_pFuelAddress )
		{
			delete (*i)->m_pFuelAddress;
		}
		delete (*i);
	}
	m_BlockCache.clear();
}



////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: RUnloadChildren( Header * block )
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: RUnloadChildren", SP_FUEL );

	for( DWORD iChild = 0; iChild != block->m_NumChildren; ++iChild )
	{
		ChildInfo * childInfo = GetChildInfo( block, iChild );
		gpassert( childInfo->m_BlockHeaderOffset != 0xffffffff );
		BinFuel::Header * child = GetChild( block, iChild );

		if( child->IsStub() )
		{
			gpassert( child->IsDirectory() );
			OnBlockDelete( child );
			DestroyStubbedDirBlock( child );
		}
		else
		{
			RUnloadChildren( child );
			OnBlockDelete( child );
			if( child->IsDirectory() )
			{
				gpassert( !child->IsStub() );
				gpassert( ((BinFuel::DirHeader*)child)->m_Size );
				m_BytesFuelCurrentlyLoaded -= ((BinFuel::DirHeader*)child)->m_Size;
				gpmem::vaFree( (BYTE*)child, ((BinFuel::DirHeader*)child)->m_Size );
			}
		}
#		if PROTECTED_BUFFERS
		BinFuel::Header * owningDir = block->GetDirectory();
		if( !owningDir->IsStub() )
		{
			gpmem::vaUnprotect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
		}
#		endif
		childInfo->m_BlockHeaderOffset = NULL;
#		if PROTECTED_BUFFERS
		if( !owningDir->IsStub() )
		{
			gpmem::vaProtect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
		}
#		endif
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
DWORD BinaryFuelDB :: GetNumChildDirs( FuelBlock * textBlock )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	DWORD count = 0;
	for( fuel_block_list::iterator i = textBlock->GetChildBlocks().begin(); i != textBlock->GetChildBlocks().end(); ++i )
	{
		if( (*i)->IsDirectory() )
		{
			++count;
		}
	}
	return count;
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: ResolveStubbedChild( BinFuel::Header * parent, DWORD childNum, BinFuel::Header *& newChildPtr )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( parent->IsDirectory() );

	ChildInfo * info = GetChildInfo( parent, childNum );
	gpassert( info );
	gpassert( info->m_Properties & CP_IS_STUB );
	gpassert( info->m_Properties & CP_OFFSET_IS_POINTER );

	BinFuel::Header * stubChild = (Header*)( info->m_BlockHeaderOffset );

	gpassert( stubChild->m_Properties & BP_IS_STUB );
	gpstring childName = BinFuel::GetName( stubChild );

	gpstring dirPath = GetDirBlockPath( parent );
	dirPath += childName;
	dirPath += "\\";

	BinFuel::Header * stub = (BinFuel::Header *)info->m_BlockHeaderOffset;

	BinFuel::Header * loadedBlock = LoadDirBlock( dirPath.c_str() );
	gpassert( loadedBlock );

#	if PROTECTED_BUFFERS
	gpmem::vaProtect( (BYTE*)loadedBlock, ((BinFuel::DirHeader*)loadedBlock)->m_Size );
#	endif

	DestroyStubbedDirBlock( stub );

	info->m_BlockHeaderOffset = (DWORD)loadedBlock;
	info->m_Properties = (BYTE)( info->m_Properties & NOT(CP_IS_STUB) );

	RFixupParentPointers( loadedBlock, parent );

	::InterlockedDecrement( (long*)&((BinFuel::DirHeader*)parent)->m_NumStubChildren );

	newChildPtr = loadedBlock;
}


////////////////////////////////////////////////////////////////////////////////
//	
BinFuel::DirHeader * BinaryFuelDB :: ConstructStubbedDirBlock( BinFuel::Header * parent, const char * dirName )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( parent && dirName );
	gpassert( parent->IsDirectory() );

	::InterlockedIncrement( (long*)&((BinFuel::DirHeader*)parent)->m_NumStubChildren );

	BinFuel::DirHeader * block = new DirHeader();

	block->m_Parent = parent;

	gpstring * name = new gpstring( dirName );
	block->m_NameOffset = (DWORD)name;
	block->m_Properties |= BP_IS_STUB|BP_IS_DIRECTORY;

	m_BytesFuelCurrentlyLoaded += sizeof( BinFuel::DirHeader );
	m_BytesFuelCurrentlyLoaded += ((gpstring*)(block->m_NameOffset ))->size();

	return( block );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: DestroyStubbedDirBlock( BinFuel::Header * block )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( block->IsDirectory() );
	gpassert( block->m_Parent->IsDirectory() );
	gpassert( block->m_Properties & BP_IS_STUB );

	m_BytesFuelCurrentlyLoaded -= sizeof( BinFuel::DirHeader );
	m_BytesFuelCurrentlyLoaded -= ((gpstring*)(block->m_NameOffset ))->size();

	delete (gpstring*)(block->m_NameOffset );
	delete block;
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: GetKnownChildren( BinFuel::Header * block, bool includeDirs, bool includeNonDirs, HeaderColl & out )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	HeaderColl toVisitStack;
	toVisitStack.push_back( block );

	while( !toVisitStack.empty() )
	{
		BinFuel::Header * current = toVisitStack.pop_back_t();

		for( DWORD i = 0; i != current->m_NumChildren; ++i )
		{
			Header * child = BinFuel::GetChild( current, i );	// $$$ add parent/child linkage test in here

			if( ( child->IsDirectory() && includeDirs ) || ( !child->IsDirectory() && includeNonDirs ) )
			{
				toVisitStack.push_back( child );
				out.push_back( child );
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: Register( BinFuel::Header * block )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	gpassert( block );

	bool bElse = true;

	if( !block->m_CacheId )
	{
		kerneltool::Critical::Lock lock( m_Critical );

		if( !block->m_CacheId )
		{
#			if PROTECTED_BUFFERS
			BinFuel::Header * owningDir = block->GetDirectory();
			if( !owningDir->IsStub() )
			{
				gpmem::vaUnprotect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
			}
#			endif

			UINT index;
			index = 0;
			BlockCacheEntry * bc = new BlockCacheEntry;
			index = m_BlockCache.Alloc( bc );
			gpassert( index && bc );
			bc->Init();

#if			GP_DEBUG
			bc->m_CacheId		= index;
			bc->m_pDb			= this;
#			endif

			block->m_CacheId	= index;
			bc->m_RefCount		= 1;
			DWORD properties	= ( block->IsDirectory() ? BlockCacheEntry::BP_DIRECTORY : 0 )
									| ( block->IsStub() ? BlockCacheEntry::BP_STUB : 0 );
			bc->m_Properties	= scast<BYTE>( properties );
			bc->SetBlockHeader( block );

			gpassert( bc->m_pFuelAddress == NULL );

			MarkBlockVisited( block );
#			if PROTECTED_BUFFERS
			if( !owningDir->IsStub() )
			{
				gpmem::vaProtect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
			}
#			endif

			bElse = false;	// $ silly thread safety
		}
	}

	if( bElse )
	{
		InterlockedIncrement( (long*)&GetCacheSlot( block->m_CacheId )->m_RefCount );
		kerneltool::Critical::OptionalLock lock( SlotAboutToExpire( block->m_CacheId ) ? &m_Critical : 0 );

#		if PROTECTED_BUFFERS
		BinFuel::Header * owningDir = block->GetDirectory();
		if( !owningDir->IsStub() )
		{
			gpmem::vaUnprotect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
		}
#		endif

		gpassert( GetCacheSlot( block->m_CacheId )->m_pFuelAddress == NULL );
		gpassert( GetCacheSlot( block->m_CacheId )->GetBlockHeader() );

		MarkBlockVisited( block );

#		if PROTECTED_BUFFERS
		if( !owningDir->IsStub() )
		{
			gpmem::vaProtect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
		}
#		endif
	}

	gpassert( GetCacheSlot( block->m_CacheId )->IsValid() );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: Unregister( DWORD cacheBlockId )
{
	//GPSTATS_SYSTEM( SP_FUEL );

	gpassert( cacheBlockId );

	::InterlockedDecrement( (LPLONG)&GetCacheSlot( cacheBlockId )->m_RefCount );

	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: CacheIn( FastFuelHandle & fh, bool resolveStub )
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: CacheIn", SP_FUEL );

	DWORD cacheBlockId = fh.m_BlockCacheId;
	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );
	BinFuel::Header * block = NULL;

	block = GetCacheSlot( cacheBlockId )->GetBlockHeader();

	// if we just have the fuel address
	if( !block )
	{
		kerneltool::Critical::Lock lock( m_Critical );

		block = GetCacheSlot( cacheBlockId )->GetBlockHeader();

		if( !block )
		{
	#		if !GP_RETAIL		
			if( GetCacheSlot( cacheBlockId )->GetBlockHeader() )
			{
				gperror( "GetCacheSlot( cacheBlockId )->GetBlockHeader()" );
			}
	#		endif

			DWORD deleteFuelAddressCacheId	= fh.m_BlockCacheId;

	#		if !GP_RETAIL
			if( !( GetCacheSlot( cacheBlockId )->m_RefCount > 0 ) )
			{
				gperror( "GetCacheSlot( cacheBlockId )->m_RefCount > 0" );
			}
			if( !( GetCacheSlot( cacheBlockId )->m_pFuelAddress != NULL ) )
			{
				gperror( "GetCacheSlot( cacheBlockId )->m_pFuelAddress != NULL" );
			}
			if( !( !(*GetCacheSlot( cacheBlockId )->m_pFuelAddress).empty() ) )
			{
				gperror( "!(*GetCacheSlot( cacheBlockId )->m_pFuelAddress).empty() )" );
			}
	#		endif

			block = GetBlockByHardAddress( (*GetCacheSlot( cacheBlockId )->m_pFuelAddress).c_str() );

			if( block->m_CacheId )
			{
				--GetCacheSlot( fh.m_BlockCacheId )->m_RefCount;
				++GetCacheSlot( block->m_CacheId )->m_RefCount;
				fh.m_BlockCacheId = block->m_CacheId;
				gpassert( GetCacheSlot( fh.m_BlockCacheId )->IsValid() );

				cacheBlockId = block->m_CacheId;
			}
			else
			{
				gpassert( !block->m_CacheId );

	#			if DEBUG_FUEL_CACHE
				gpstring debugAddress = GetBlockHardAddress( block );
				gpassert( debugAddress.same_no_case( (*GetCacheSlot( cacheBlockId ).m_pFuelAddress) ) );
	#			endif

#				if PROTECTED_BUFFERS
				BinFuel::Header * owningDir = block->GetDirectory();
				gpmem::vaUnprotect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
#				endif

				block->m_CacheId = cacheBlockId;

#				if PROTECTED_BUFFERS
				gpmem::vaProtect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
#				endif
			}

			delete GetCacheSlot( deleteFuelAddressCacheId )->m_pFuelAddress;
			GetCacheSlot( deleteFuelAddressCacheId )->m_pFuelAddress = NULL;

			GetCacheSlot( cacheBlockId )->SetBlockHeader( block );

			++m_CacheMisses;

#			if !GP_RETAIL
			if( gFuelSys.DebugShowHandleAccess() )
			{
				gpgenericf(( "FuelHandle cache in : c.locks 0x%08x : %s\n", FastFuelHandle::ConditionalLock::m_Locks, fh.GetAddress().c_str() ));
			}
#			endif
		}
		else
		{
			++m_CacheHits;
		}
	}
	else
	{
		++m_CacheHits;
	}

	gpassert( block );
	gpassert( block != block->m_Parent );
	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );

	bool resolve = block->IsStub() && resolveStub;

	// $$ thazard
	if( resolve )
	{
		kerneltool::Critical::Lock lock( m_Critical );

		resolve = block->IsStub() && resolveStub;

		if( resolve )
		{
			gpassert( ( block->IsStub() && resolveStub ) );
			gpassert( block->IsDirectory() );
			gpassert( GetCacheSlot( cacheBlockId )->m_RefCount > 0 );
			gpassert( GetCacheSlot( cacheBlockId )->m_pFuelAddress == NULL );

			BinFuel::Header * parent = block->m_Parent;

			bool found = false;
			for( DWORD iChild = 0; iChild != parent->m_NumChildren; ++iChild )
			{
				BinFuel::ChildInfo * childInfo = BinFuel::GetChildInfo( parent, iChild );

				if( childInfo->m_BlockHeaderOffset == (DWORD)block )
				{
					found = true;
					gpassert( childInfo->IsStub() );
					break;
				}
			}

			if( !found )
			{
				gpwatson( "fatal binary fuel error" );
			}

#			if PROTECTED_BUFFERS
			gpmem::vaUnprotect( (BYTE*)parent, ((BinFuel::DirHeader*)parent)->m_Size );
#			endif
			
			ResolveStubbedChild( parent, iChild, block );
			//gpassert( GetCacheSlot( cacheBlockId ).m_RefCount == 1 );
			gpassert( GetCacheSlot( cacheBlockId )->m_RefCount > 0 );
			
			gpassert( !block->m_CacheId );
			
#			if PROTECTED_BUFFERS
			gpmem::vaUnprotect( (BYTE*)block, ((BinFuel::DirHeader*)block)->m_Size );
#			endif

			block->m_CacheId = cacheBlockId;

			MarkBlockVisited( block );
			
#			if PROTECTED_BUFFERS
			gpmem::vaProtect( (BYTE*)block, ((BinFuel::DirHeader*)block)->m_Size );
#			endif

			GetCacheSlot( cacheBlockId )->SetBlockHeader( block );

			DWORD properties = GetCacheSlot( cacheBlockId )->m_Properties;
			properties = properties & ~BlockCacheEntry::BP_STUB;
			GetCacheSlot( cacheBlockId )->m_Properties = scast<BYTE>(properties);

#			if PROTECTED_BUFFERS
			gpmem::vaProtect( (BYTE*)parent, ((BinFuel::DirHeader*)parent)->m_Size );
#			endif

			++m_CacheMisses;

			gpassert( block->IsDirectory() );
			gpassert( !block->IsStub() );
		}
	}

	// $$ thazard
	if( !resolve )
	{
		kerneltool::Critical::OptionalLock lock( SlotAboutToExpire( block->m_CacheId ) ? &m_Critical : 0 );

		MarkBlockVisited( block );
	}

	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader() );
	gpassert( GetCacheSlot( cacheBlockId )->m_pFuelAddress == NULL );
	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: OnBlockDelete( BinFuel::Header * block, BinFuel::Header * replacedWithStub )
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: OnBlockDelete", SP_FUEL );

	if( block->m_CacheId )
	{
		gpassert( GetCacheSlot( block->m_CacheId )->m_CacheId == block->m_CacheId );

		if( replacedWithStub )
		{
			gpassert( !block->IsStub() );
			gpassert( block->IsDirectory() && replacedWithStub->IsDirectory() );

			replacedWithStub->m_CacheId = block->m_CacheId;
			GetCacheSlot( block->m_CacheId )->SetBlockHeader( replacedWithStub );
			gpassert( GetCacheSlot( block->m_CacheId )->IsValid() );
		}
		else
		{
			gpassert( !replacedWithStub );

			if( GetCacheSlot( block->m_CacheId )->m_RefCount == 0 )
			{
				GetCacheSlot( block->m_CacheId )->MarkForDeletion();
				GetCacheSlot( block->m_CacheId )->SetBlockHeader( NULL );
			}
			else if( GetCacheSlot( block->m_CacheId )->GetBlockHeader() )
			{
				GPPROFILERSAMPLE( "BinaryFuelDB :: OnBlockDelete - make hard address", SP_FUEL );
				gpassert( GetCacheSlot( block->m_CacheId )->m_pFuelAddress == NULL);
				gpstring * pAddress = new gpstring;
				GetBlockHardAddress( block, *pAddress );
				GetCacheSlot( block->m_CacheId )->m_pFuelAddress = pAddress;
				GetCacheSlot( block->m_CacheId )->SetBlockHeader( NULL );
			}
			else
			{
				gpassert( GetCacheSlot( block->m_CacheId )->m_pFuelAddress != NULL );
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//	
BinFuel::Header * BinaryFuelDB :: GetCachedBlockPtr( FastFuelHandle & fh, bool resolveStub )
{
	//GPPROFILERSAMPLE( "BinaryFuelDB :: GetCachedBlockPtr", SP_FUEL );

	if( !fh.m_BlockCacheId )
	{
		gpwatson( "Attempting to use a NULL FastFuelHandle.\n" );
	}

	bool stub = GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() && GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader()->IsStub();

	if( stub )
	{
		kerneltool::Critical::Lock lock( m_Critical );

		stub = GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader()->IsStub();

		if( stub )
		{
			gpassert( GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() );
			gpassert( GetCacheSlot(fh.m_BlockCacheId)->IsValid() );
			gpassert( GetCacheSlot(fh.m_BlockCacheId)->IsDirectory() );

			CacheIn( fh, resolveStub  );

			gpassert( GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() );
			gpassert( GetCacheSlot(fh.m_BlockCacheId)->IsValid() );
			gpassert( GetCacheSlot(fh.m_BlockCacheId)->IsDirectory() );

			return( GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() );
		}
	}

	if( !stub )
	{
		FastFuelHandle::ConditionalLock lock( fh.ShouldGrabLock(), &fh );

		CacheIn( fh, false );

		gpassert( GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() );
		gpassert( GetCacheSlot(fh.m_BlockCacheId)->IsValid() );

		return( GetCacheSlot( fh.m_BlockCacheId )->GetBlockHeader() );
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: CacheOut( DWORD cacheBlockId )
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: CacheOut", SP_FUEL );

	kerneltool::Critical::Lock lock( m_Critical );

	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );
	gpassert( !GetCacheSlot( cacheBlockId )->GetBlockHeader()->IsStub() );
	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader()->IsDirectory() );
	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader() );

	BinFuel::Header * block = GetCacheSlot( cacheBlockId )->GetBlockHeader();
	gpassert( block );
	gpassert( block->m_Parent );

	// unload children liquid

	RUnloadChildren( block );

	// sub out self

	BinFuel::Header * parent = block->m_Parent;

	gpstring blockName = BinFuel::GetName( block );

	// find child entry in parent to update

	ChildInfo * info = NULL;

	for( DWORD iChild = 0; iChild != parent->m_NumChildren; ++iChild )
	{
		if( BinFuel::GetChild( parent, iChild ) == block )
		{
			info = GetChildInfo( parent, iChild );
			break;
		}
	}

	if( !info )
	{
		gpwatson( "Block parent doesn't claim child!" );
		return;
	}

#	if PROTECTED_BUFFERS
	BinFuel::Header * owningDir = parent->GetDirectory();
	gpmem::vaUnprotect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
#	endif

	BinFuel::Header * unloaded = (BinFuel::Header *)ConstructStubbedDirBlock( parent, blockName.c_str() );
	info->m_BlockHeaderOffset = (DWORD)unloaded;
	info->m_Properties |= CP_IS_STUB | CP_IS_DIRECTORY | CP_OFFSET_IS_POINTER;

	OnBlockDelete( block, unloaded );

	gpassert( block->IsDirectory() );
	m_BytesFuelCurrentlyLoaded -= ((BinFuel::DirHeader*)(block))->m_Size;

	gpmem::vaFree( (BYTE*)block, ((BinFuel::DirHeader*)block)->m_Size );
	block = NULL;

#	if PROTECTED_BUFFERS
	gpmem::vaProtect( (BYTE*)owningDir, ((BinFuel::DirHeader*)owningDir)->m_Size );
#	endif

	gpassert( GetCacheSlot( cacheBlockId )->IsValid() );
	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader()->IsStub() );
	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader()->IsDirectory() );
	gpassert( GetCacheSlot( cacheBlockId )->GetBlockHeader() );
	gpassert( GetCacheSlot( cacheBlockId )->m_pFuelAddress == NULL );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: MarkBlockVisited( BinFuel::Header * block )
{
	GPSTATS_SYSTEM( SP_FUEL );

//	kerneltool::Critical::OptionalLock lock( SlotAboutToExpire( block->m_CacheId ) ? m_Critical : NULL );
//	FastFuelHandle::ConditionalLock lock( SlotAboutToExpire( block->m_CacheId ), this );

	BinFuel::Header * current = block;

	// mark self
	gpassert( current->m_CacheId );

	// early out
	if( GetCacheSlot( current->m_CacheId )->m_VisitCount == m_CurrentVisitCount )
	{
		return;
	}

	GetCacheSlot( current->m_CacheId )->m_VisitCount = m_CurrentVisitCount;
	current = current->m_Parent;

	// mark all parent dirs
	while( current )
	{
		if( current->IsDirectory() && current->m_CacheId )
		{
			GetCacheSlot( current->m_CacheId )->m_VisitCount = m_CurrentVisitCount;
			gpassert( GetCacheSlot( current->m_CacheId )->IsValid() );
		}
		current = current->m_Parent;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: Update()
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: Update", SP_FUEL );

	gpassert( m_RootHeader );

	////////////////////////////////////////
	//	update global visit count

	double currentTime = ::GetGlobalSeconds();
	if( currentTime - m_LastVisitIncrementTime > 1.0 )
	{
		++m_CurrentVisitCount;
		m_LastVisitIncrementTime = currentTime;
	}

	kerneltool::Critical::Lock lock( m_Critical );

	////////////////////////////////////////
	//	deep debug

#if DEBUG_FUEL_CACHE

	HeaderColl toVisitStack;
	toVisitStack.push_back( m_RootHeader );

	while( !toVisitStack.empty() )
	{
		BinFuel::Header * current = toVisitStack.pop_back_t();
		if( current->m_CacheId )
		{
			gpassert( GetCacheSlot( current->m_CacheId )->IsValid() );
		}

		for( DWORD i = 0; i != current->m_NumChildren; ++i )
		{
			toVisitStack.push_back( BinFuel::GetChild( current, i ) );
		}
	}

	SlotColl::iterator iv;
	for( iv = m_BlockCache.begin(); iv != m_BlockCache.end(); ++iv )
	{
		gpassert( (*iv)->IsValid() );
	}

#endif 

	////////////////////////////////////////////////////////////////////////////////
	//	scan a portion of cache slots, and unload any expired leaf dirs

	const debugFactor = 1;

	DWORD bytesDeleted = 0;
	DWORD numDeletions = 0;
	const DWORD bytesDeletedLimit = 1024 * 64 * debugFactor;

	for( DWORD j = 0; j < 100 * debugFactor; ++j )
	{
		if( (bytesDeleted > bytesDeletedLimit) || (numDeletions > ( 50 * debugFactor ) ) )
		{
			break;
		}

		SlotColl::iterator i = m_BlockCache.GetIteratorAt( m_CurrentUnloadSweepIndex, true );
		if( (i == m_BlockCache.end()) || (i == m_BlockCache.begin()) )
		{
			m_CurrentUnloadSweepIndex = m_RootHeader->m_CacheId + 1;
			return;
		}
		else
		{
			//BlockCacheEntry * cs = *i;
			DWORD index = i.GetIndex();

			gpassert( (*i)->IsValid() );
			gpassert( index == (*i)->m_CacheId );

			if( (*i)->IsMarkedForDeletion() || ( ((*i)->m_RefCount == 0) && !(*i)->GetBlockHeader() ) )
			{
				gpassert( (*i)->IsMarkedForDeletion() || ( ((*i)->m_RefCount == 0) && !(*i)->GetBlockHeader() ) );

				////////////////////////////////////////
				//	delete cache slot due to no references

				if( (*i)->m_pFuelAddress )
				{
					bytesDeleted += (*i)->m_pFuelAddress->size();
					++numDeletions;
					delete (*i)->m_pFuelAddress;
					(*i)->m_pFuelAddress = NULL;
				}

				(*i)->Init();
				bytesDeleted += sizeof( BlockCacheEntry );
				++numDeletions;
				delete (*i);
				m_BlockCache.Free( index );
			}
			else if( (m_CurrentVisitCount - (*i)->m_VisitCount) > m_UnloadVisitCountThreashold )
			{
				gpassert( (m_CurrentVisitCount - (*i)->m_VisitCount) > m_UnloadVisitCountThreashold );

				BinFuel::Header * uncacheCandidate = (*i)->GetBlockHeader();

				if( uncacheCandidate && uncacheCandidate->IsDirectory() && !uncacheCandidate->IsStub() )
				{
					////////////////////////////////////////
					//	found potential dir to unload, check to see if it's a leaf

					bool bCacheOut = true;

					// only unload this if it's a leaf dir
					for( DWORD k = 0; k != uncacheCandidate->m_NumChildren; ++k )
					{
						BinFuel::Header * child = GetChild( uncacheCandidate, k );
						if( child->IsDirectory() && !child->IsStub() )
						{
							bCacheOut = false;
							break;
						}
					}

					// unload dir and all childrem
				
					if( bCacheOut )
					{
						if(( (((BinFuel::DirHeader*)(uncacheCandidate))->m_Size + bytesDeleted ) < bytesDeletedLimit) || (numDeletions==0))
						{
							bytesDeleted += ((BinFuel::DirHeader*)(uncacheCandidate))->m_Size;
							++numDeletions;
							CacheOut( index );
						}
					}
					gpassert( GetCacheSlot( index )->IsValid() );
				}
			}
			m_CurrentUnloadSweepIndex = index + 1;
		}
	}

	m_MaxUnloadSweepIndex = max( m_MaxUnloadSweepIndex, m_CurrentUnloadSweepIndex );

	////////////////////////////////////////
	//	deep debug

#if DEBUG_FUEL_CACHE

	for( iv = m_BlockCache.begin(); iv != m_BlockCache.end(); ++iv )
	{
		gpassert( (*iv)->IsValid() );
	}

#endif
}


////////////////////////////////////////////////////////////////////////////////
//	
gpstring BinaryFuelDB :: GetDirBlockPath( BinFuel::Header const * block ) const
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( block->IsDirectory() );

	gpstring address( GetBlockSoftAddress( block ) );
	stringtool::ReplaceAllChars( address, ':', '\\' );

	gpstring path( m_HomeReadPath );

	path += address;
	if( !address.empty() )
	{
		path += "\\";
	}

	return path;
}


////////////////////////////////////////////////////////////////////////////////
//	
gpstring BinaryFuelDB :: GetBlockSoftAddress( BinFuel::Header const * block ) const
{
	GPSTATS_SYSTEM( SP_FUEL );

	HeaderColl blocks;
	Header * current = ccast<BinFuel::Header*>(block);

	while( current->m_Parent != NULL )
	{
		blocks.push_back( current );
		current = current->m_Parent;
	}

	gpstring address;

	for( HeaderColl::reverse_iterator i = blocks.rbegin(); i != blocks.rend(); ++i )
	{
		if( !address.empty() )
		{
			address += ":";
		}
		address += BinFuel::GetName( (*i) );
	}

	return address;
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: GetBlockHardAddress( BinFuel::Header const * block, gpstring & out ) const
{
	//GPPROFILERSAMPLE( "BinaryFuelDB :: GetBlockHardAddress", SP_FUEL );

	HeaderColl blocks;
	Header * current = ccast<BinFuel::Header*>(block);

	while( current->m_Parent != NULL && ( current != m_RootHeader ) )
	{
		blocks.push_back( current );
		current = current->m_Parent;
	}

	out = "";
	BinFuel::Header * parent = m_RootHeader;

	for( HeaderColl::reverse_iterator i = blocks.rbegin(); i != blocks.rend(); ++i )
	{
		BinFuel::Header * child = *i;
		bool found = false;

		for( DWORD ichild = 0; ichild != parent->m_NumChildren; ++ichild )
		{
			if( child == GetChild( parent, ichild ) )
			{
				//GPPROFILERSAMPLE( "BinaryFuelDB :: GetBlockHardAddress - build string", SP_FUEL );
				if( !out.empty() )
				{
					out.append( ":" );
				}
				out.appendf( "%d", ichild );
				found = true;
				break;
			}
		}
		gpassert( found );
		parent = child;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
Header * BinaryFuelDB :: GetBlockBySoftAddress( const char * addr )
{
	GPSTATS_SYSTEM( SP_FUEL );

	Header * parent = GetRootHeader();
	gpassert( !parent->IsStub() );

	gpstring address( addr );
	gpstring childName, childPath;


	while( stringtool::GetFrontDelimitedString( address, ':', childName ) )
	{
		Header * child = BinFuel::GetChildNamed( parent, childName, TestOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS ) ? BinFuel::BP_IS_DEV_ONLY : 0 );

		if( !child )
		{
			//BinFuel::Dump( parent, true );
			return( NULL );
		}

		DWORD cacheId = 0;
		{
			FastFuelHandle cacheIn( child, this );
			cacheIn.GetHeader();	// this will cache it in
			cacheId = cacheIn.GetHeader()->m_CacheId;
			gpassert( GetCacheSlot( cacheId )->IsValid() );
		}
		gpassert( GetCacheSlot( cacheId )->IsValid() );

		child = BinFuel::GetChildNamed( parent, childName );

		stringtool::RemoveFrontDelimitedString( address, ':', address );

		parent = child;
	}

	return( parent );
}


////////////////////////////////////////////////////////////////////////////////
//	
Header * BinaryFuelDB :: GetBlockByHardAddress( const char * addr )
{
	GPPROFILERSAMPLE( "BinaryFuelDB :: GetBlockByHardAddress", SP_FUEL );

	kerneltool::Critical::Lock lock( m_Critical );

	Header * child = GetRootHeader();
	gpassert( !child->IsStub() );

	gpstring address( addr ), childName;

	while( stringtool::GetFrontDelimitedString( address, ':', childName ) )
	{
		DWORD childNum;
		stringtool::Get( childName, childNum );

		DWORD cacheId = 0;
		{
			FastFuelHandle cacheIn( child, this );
			child = cacheIn.GetHeader();
			cacheId = child->m_CacheId;
			child = GetChild( child, childNum );
			gpassert( child );
			gpassert( GetCacheSlot( cacheId )->IsValid() );
		}
		gpassert( GetCacheSlot( cacheId )->IsValid() );

		stringtool::RemoveFrontDelimitedString( address, ':', address );
	}

	return( child );
}


////////////////////////////////////////////////////////////////////////////////
//	
Header * BinaryFuelDB :: LoadDirBlock( const char * path, Header * reloadBlock )
{
	GPSTATS_SYSTEM( SP_FUEL );

	Header * block = NULL;

	if( !reloadBlock )
	{
		////////////////////////////////////////
		//	load this block

		block = LoadDirBinFile( path );
		if( !block )
		{
			gpassertm( 0, "wtf?" );
			return( NULL );
			
		}
	}
	else
	{
		block = reloadBlock;
	}

	gpassert( (block->m_Properties & BP_IS_DIRECTORY ) != 0 );

	////////////////////////////////////////
	//	if this is not a stand-alone block, recurse into child dirs

	////////////////////////////////////////
	//	load child dir blocks

	//	concurrently iterate over children and subdirs... matching dir to
	//	dir child.  Make sure no dirs or children left at and of iteration

	FileSys::AutoFindHandle hFindRoot( path );
	if_not_gpassertf( !hFindRoot.IsNull(), ("Can't LoadDirBlock for [%s]", path ) )
	{
		return NULL;
	}

	FileSys::AutoFindHandle hDir;
	hDir.Find( hFindRoot, "*", FileSys::FINDFILTER_DIRECTORIES );

	if( hDir )
	{
		// get all the subdir names
		std::set< gpstring, string_less > dirSet;
		gpstring dir;
		while( hDir.GetNext( dir, false ) )
		{
			dirSet.insert( dir );
		}

		// read in subdirs
		std::set< gpstring, string_less >::iterator i = dirSet.begin();
		for( DWORD iChild = 0; iChild != block->m_NumChildren; ++iChild )
		{
			ChildInfo * info = GetChildInfo( block, iChild );

			if( !info->IsDirectory() )
			{
				continue;
			}

			Header * child = (BinFuel::Header *)ConstructStubbedDirBlock( block, (*i).c_str() );
			gpassert( child->m_Parent == block );
			info->m_Properties |= CP_IS_STUB;
			//const char * debugName = BinFuel::GetName( child );

			if( !child )
			{
				gpassertm( 0, "failed to load child" );
				return( NULL );
			}
			else
			{
				// fill in the child info
				info->m_BlockHeaderOffset = (DWORD)child;
				info->m_Properties |= CP_IS_STUB|CP_IS_DIRECTORY|CP_OFFSET_IS_POINTER;
			}
			++i;
		}
		// make sure dirs read matched expectations
		gpassert( (i == dirSet.end()) && (iChild == block->m_NumChildren) );
	}
	else
	{
		return( NULL );
	}

	return( block );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: PrivateUnload()
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( m_RootHeader );

	kerneltool::Critical::Lock lock( m_Critical );

	////////////////////////////////////////////////////////////////////////////////
	//	free all cached blocks

	for( DWORD iChild = 0; iChild != m_RootHeader->m_NumChildren; ++iChild )
	{
		BinFuel::Header * child = GetChild( m_RootHeader, iChild );
		if( child->m_CacheId && child->IsDirectory() && !child->IsStub() ) 
		{
			CacheOut( GetChild( m_RootHeader, iChild )->m_CacheId );
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//	free all cache slots

	m_CurrentUnloadSweepIndex = m_RootHeader->m_CacheId + 1;

	SlotColl::iterator i = m_BlockCache.GetIteratorAt( m_CurrentUnloadSweepIndex, true );
	while( !(i == m_BlockCache.end() || i == m_BlockCache.begin() ) )
	{
		DWORD index = i.GetIndex();

		gpassert( (*i)->IsValid() );
		gpassert( index == (*i)->m_CacheId );

		if( (*i)->IsMarkedForDeletion() || ( ((*i)->m_RefCount == 0) && !(*i)->GetBlockHeader() ) )
		{
			if( (*i)->m_pFuelAddress )
			{
				delete (*i)->m_pFuelAddress;
				(*i)->m_pFuelAddress = NULL;
			}
			delete (*i);
			m_BlockCache.Free( index );
		}

		m_CurrentUnloadSweepIndex = ++index;

		i = m_BlockCache.GetIteratorAt( m_CurrentUnloadSweepIndex, true );
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
Header * BinaryFuelDB :: LoadDirBinFile( const char * path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpstring lqdFilePath;
	FileSys::AutoFileHandle lqdFile;
	FileSys::AutoMemHandle mem;

	if( TestOptions( OPTION_NO_READ_LIQUID ) || IsDirDirty( path, lqdFilePath, lqdFile, mem ) )
	{
		mem.Close();
		lqdFile.Close();

		////////////////////////////////////////
		//	build file if dir dirty

		FuelByteBuffer buffer;
		buffer.reserve( 1024*64 );

		++m_NumDirtyDirectoriesFound;

		bool reportStatus = !TestOptions( OPTION_NO_WRITE_LIQUID ) || !TestOptions( OPTION_NO_READ_LIQUID );
		if ( reportStatus )
		{
			gpgenericf(( "BinaryFuelDB - JIT condensing dir %s.....", path ));
		}

		if( !CompileDir( path, buffer ) )
		{
			gpgeneric( "FAILED to compile dir!\n" );
			return NULL;
		}

		if ( !TestOptions( OPTION_NO_WRITE_LIQUID ) )
		{
			if( !WriteDir( path, buffer ) )
			{
				gpgeneric( "FAILED to write dir!\n" );
			}
			else
			{
				BinaryFileHeader * fileHeader = (BinaryFileHeader*)buffer.begin();
#if				GP_DEBUG
				++fileHeader;
				gpgenericf(( "OK, CRC = 0x%08x\n", ((DirHeader*)fileHeader)->m_StaticChecksum ));
#else
				gpgeneric( "OK\n" );
#endif
			}
		}
		else if ( reportStatus )
		{
			gpgeneric( "OK but LQD file not written (per options)\n" );
		}

		BinaryFileHeader * fileHeader = (BinaryFileHeader*)buffer.begin();
		const int fileSize = buffer.size();
		++fileHeader;

		const DWORD blockSize = fileSize - sizeof( BinaryFileHeader );
		//BYTE * block = new BYTE[ blockSize ];
		BYTE * block = gpmem::vaAlloc( blockSize );
		memcpy( block, (BYTE*)fileHeader, blockSize );

		m_BytesFuelCurrentlyLoaded += blockSize;

		return( (Header*)block );
	}
	else
	{
		////////////////////////////////////////
		//	actual load part

#		if !GP_RETAIL
		if( ( lqdFile.GetLocation() & FileSys::LOCATION_TANK ) == 0 )
		{
			FileSys::AutoFindHandle find( lqdFilePath );
			FileSys::FindData data;
			find.GetNext( data );
			if ( data.m_Attributes & FILE_ATTRIBUTE_READONLY )
			{
				gperrorf(( "You have read-only liquid files!  This will hurt performance and prevents Fuel from updating them.  Please fix.  Hidden File = %s\n", lqdFilePath.c_str() ));
			}
		}
#		endif // !GP_RETAIL

		// version comparison not needed at this point

		BinaryFileHeader * fileHeader = (BinaryFileHeader*)mem.GetData();
		const int fileSize = mem.GetSize();
		++fileHeader;

		const DWORD blockSize = fileSize - sizeof( BinaryFileHeader );
		//BYTE * block = new BYTE[ blockSize ];
		BYTE * block = gpmem::vaAlloc( blockSize );

		memcpy( block, (BYTE*)fileHeader, blockSize );

		m_BytesFuelCurrentlyLoaded += blockSize;

		return( (Header*)block );
	}

	// $$ scan through block... if block has no child dir blocks, mark it as loaded

}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: RFixupParentPointers( Header * block, Header * parent )
{
	//GPSTATS_SYSTEM( SP_FUEL );
#	if PROTECTED_BUFFERS
	if( block->IsDirectory() )
	{
		gpmem::vaUnprotect( (BYTE*)block, ((BinFuel::DirHeader*)block)->m_Size );
	}
#	endif

	// I set my parent
	block->m_Parent = parent;

	// I become parent to my loaded children
	for( DWORD iChild = 0; iChild != block->m_NumChildren; ++iChild )
	{
		ChildInfo * childInfo = GetChildInfo( block, iChild );

		if( !childInfo->IsStub() )
		{
			Header * child = GetChild( block, iChild );
			RFixupParentPointers( child, block );
		}
	}
#	if PROTECTED_BUFFERS
	if( block->IsDirectory() )
	{
		gpmem::vaProtect( (BYTE*)block, ((BinFuel::DirHeader*)block)->m_Size );
	}
#	endif
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: IsDirDirty( const char * path, gpstring& lqdFilePath, FileSys::AutoFileHandle& lqdFile, FileSys::AutoMemHandle& mem )
{
	GPSTATS_SYSTEM( SP_FUEL );

	////////////////////////////////////////
	//	look for a LQD file

	FILETIME lqdFileTime;

	// $$$ todo: if write fails, consider the file written anyway..
	// $$$ disable writes in retail...
	// stringtool :: nextfield in stringtool... in GetBlock()...

	////////////////////////////////////////
	//	get all the paths straight

	lqdFilePath = path;
	lqdFilePath += "dir";
	lqdFilePath += GetBinFileExtension();

	if( !lqdFile.Open( lqdFilePath ) )
	{
		// if no .lqd file found, this dir needs compilation
		return( true );	
	}
	else
	{
		// get the FILETIME
		lqdFile.GetFileTimes( 0, 0, &lqdFileTime );	
	}

	////////////////////////////////////////////////////////////////////////////////
	//	compare .lqd FILETIME against present .gas files and subdirs

	FileSys::AutoFindHandle hFindRoot( path );

	if_not_gpassertf( !hFindRoot.IsNull(), ("Can't IsDirDirty for [%s]", path ) )
	{
		return false;
	}

	FuelSys::StringSet paths;

	////////////////////////////////////////
	//	look for newer .gas files

	FileSys::AutoFindHandle hGasFile;
	hGasFile.Find( hFindRoot, "*.gas", FileSys::FINDFILTER_FILES );

	if( hGasFile )
	{
		FileSys::FindData findData;
		while( hGasFile.GetNext( findData ) )
		{
			// skip zero-length files, as they are left out of tank
			if ( findData.m_Size != 0 )
			{
				// reprocess the name - lowercase and appended with the low part of the time
				findData.m_Name.to_lower();
				findData.m_Name.appendf( ".%x", findData.m_LastWriteTime.dwLowDateTime );
				paths.insert( findData.m_Name );

				if( ::CompareFileTime( &lqdFileTime , &findData.m_LastWriteTime ) == -1 )
				{
					return true;
				}
			}
		}
	}

	////////////////////////////////////////
	//	look for newer dirs

	FileSys::AutoFindHandle hDir;
	hDir.Find( hFindRoot, "*", FileSys::FINDFILTER_DIRECTORIES );
	if( hDir )
	{
		gpstring name;
		FileSys::FindData findData;
		while( hDir.GetNext( findData ) )
		{
			findData.m_Name.to_lower();
			stringtool::AppendTrailingBackslash( findData.m_Name );
			paths.insert( findData.m_Name );

			//if( ::CompareFileTime( &lqdFileTime , &findData.m_LastWriteTime ) == -1 )
			//{
			//	return true;
			//}
		}
	}

	////////////////////////////////////////
	//	most expensive check - see if a subdir or gas file was deleted

	if( !mem.Map( lqdFile ) || ( mem.GetSize() == 0 ) )
	{
		gpassert( 0 );
		return true;
	}

	// compare LQD versions with LQD file header here...

	BinaryFileHeader * fileHeader = (BinaryFileHeader*)mem.GetData();

	if( fileHeader->m_Id != BINARY_FUEL_ID )
	{
		gpassertm( 0, "imposter file found" );
		return( true );
	}

	if( fileHeader->m_Version != BINARY_FUEL_VERSION )
	{
		gpassertm( 0, "incompatible file version found" );
		return( true );
	}

	bool originalDevBlocksMissing = ( fileHeader->m_Properties & BinFuel::BinaryFileHeader::FP_INCLUDES_DEV_BLOCKS_NORMALLY ) && !( fileHeader->m_Properties & BinFuel::BinaryFileHeader::FP_INCLUDES_DEV_BLOCKS_NOW );

	if( !TestOptions( OPTION_NO_READ_DEV_BLOCKS ) && originalDevBlocksMissing )
	{
		return( true );
	}

	DWORD currentChecksum = FuelSys::CalcChecksum( paths );

	if( currentChecksum != fileHeader->m_ChildPathChecksum )
	{
		return true;	// just so I can break on this
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: DeleteAllLqdFiles()
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpgenericf(( "Deleting all binary .gas for '%s'\n", m_HomeWritePath.c_str() ));

	DeleteAllBinFiles( m_HomeWritePath.c_str() );
}


////////////////////////////////////////////////////////////////////////////////
//	
void BinaryFuelDB :: DeleteAllBinFiles( const char * path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FileSys::AutoFindHandle hFindRoot( path );
	if( hFindRoot.IsNull() )
	{
		return;
	}

	FileSys::AutoFindHandle hBinFile;
	hBinFile.Find( hFindRoot, "*.lqd*", FileSys::FINDFILTER_FILES );
	if( hBinFile )
	{
		gpstring name;
		while( hBinFile.GetNext( name, false ) )
		{
			gpstring filePath( path );
			filePath += name;
			::SetFileAttributes( filePath, FILE_ATTRIBUTE_NORMAL );
			::DeleteFile( filePath );
		}
	}

	////////////////////////////////////////
	//	recurse to subdirs

	FileSys::AutoFindHandle hDir;
	hDir.Find( hFindRoot, "*", FileSys::FINDFILTER_DIRECTORIES );
	if( hDir )
	{
		gpstring name;
		while( hDir.GetNext( name, false ) )
		{
			gpstring dirPath( path );
			dirPath += name;
			dirPath += "\\";
			DeleteAllBinFiles( dirPath );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: RecompileAll( bool deleteExistingLqd )
{
	GPSTATS_SYSTEM( SP_FUEL );

	if ( deleteExistingLqd )
	{
		gpgenericf(( "Deleting all binary .gas for '%s'\n", m_HomeWritePath.c_str() ));

		DeleteAllBinFiles( m_HomeWritePath.c_str() );
	}

	gpgenericf(( "Condensing all .gas for '%s'\n", m_HomeWritePath.c_str() ));

	double timeBefore = GetGlobalSeconds();

	bool success = RCompileAll( m_HomeReadPath );

	double timeAfter = GetGlobalSeconds();

	gpgenericf(( "\nTook %f seconds to recompile all .gas\n", timeAfter-timeBefore ));

	return success;
}


typedef std::multimap< DWORD, gpdumbstring > ReferenceMap;


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: BuildDictionary( DWORD sizeLimit )
{
	GPSTATS_SYSTEM( SP_FUEL );

	if( m_Root )
	{
		gperror( "You can't build a dictionary once you've begin to use the Db." );
		return false;
	}

	// collect enough strings to build a decent dictionary

	if( !m_TextDb->Init( OPTION_READ | OPTION_JIT_READ, m_HomeReadPath ) )
	{
		gpassertm( 0, "Something horrible" );
		return false;
	}

	FuelBlock * block = m_TextDb->GetRootBlock();
	FuelHandleList rootKids;
	FuelHandleList tempList;
	block->ListChildBlocks( 1, 0, 0, rootKids );
	for( FuelHandleList::iterator i = rootKids.begin(); i != rootKids.end(); ++i )
	{
		tempList.clear();
		if( stricmp( (*i)->GetName(), "world" ) != 0 )
		{
			gpgenericf(( "sampling address %s\n", (*i)->GetAddress().c_str() ));
			(*i)->ListChildBlocks( -1, 0, 0, tempList );
			(*i).Unload();
		}
		else
		{
			FuelHandle regionDir = (*i)->GetChildBlock( "maps:map_world:regions" );
			if( regionDir )
			{
				FuelHandleList regionList;
				regionDir->ListChildBlocks( 1, 0, 0, regionList );
				for( FuelHandleList::iterator ir = regionList.begin(); ir != regionList.end(); ++ir )
				{
					gpgenericf(( "sampling region %s\n", (*ir)->GetName() ));
					tempList.clear();
					(*ir)->ListChildBlocks( -1, 0, 0, tempList );
					(*ir).Unload();
				}
			}
		}
	}

	ReferenceMap refMap;

	FuelSys :: StringDb::iterator is;

	for( is = gFuelSys.GetStringCollector().begin(); is != gFuelSys.GetStringCollector().end(); ++is )
	{
		// only consider strings that are likely to save us space...
		//if( ( (*is).second > 1 ) && ( ( (*is).first.size() > 4 ) ) )
		{
			refMap.insert( std::make_pair( (*is).second, (*is).first ) );
		}
	}

	// write dictionary to mem buffer

	FuelByteBuffer buffer;
	buffer.reserve( sizeLimit * 2 );

	// count how many strings we're going to write
	DWORD stringCount			= 0;
	DWORD stringSize			= 0;
	DWORD lastStringRefCount	= 0;

	for( ReferenceMap::reverse_iterator iref = refMap.rbegin(); iref != refMap.rend(); ++iref )
	{
		if( (*iref).first < 1 )
		{
			break;
		}
		if( stringCount >= sizeLimit )
		{
			break;
		}

		stringSize += (*iref).second.size() + 1;
		lastStringRefCount = (*iref).first;
		//gpgenericf(( "%08d - %s\n", (*iref).first, (*iref).second.c_str() ));
		++stringCount;
	}

	gpgenericf(( "Storing %d strings into dictionary.  Last string ref count = %d\n", stringCount, lastStringRefCount ));

	// store keys in sorted container


	iref = refMap.rbegin();
	std::set< gpdumbstring, string_less > sortedStrings;
	for( DWORD istring = 0; istring != stringCount; ++istring, ++iref )
	{
		sortedStrings.insert( (*iref).second );
	}

	// store header

	DictionaryHeader header;
	header.m_NumStrings = stringCount;
	BinFuel::Store( header, buffer, 0 );

	// store offset table for strings

	for( istring = 0; istring != stringCount; ++istring )
	{
		BinFuel::Store( 0, buffer, 0 );
	}

	std::set< gpdumbstring, string_less >::iterator isorted = sortedStrings.begin();
	for( istring = 0; istring != stringCount; ++istring, ++isorted )
	{
		// store strings
		DWORD offset = Store( (*isorted).c_str(), (*isorted).size(), buffer, 0 );
		Store( BYTE(0), buffer, 0 );
		// adjust offset for each string
		*(((DWORD*)(buffer.begin() + sizeof( DictionaryHeader )))+istring) = offset;
	}

	gpgenericf(( "Stored %d strings into the dictionary\n", sortedStrings.size() ));

	// write mem buffer to disk

	gpstring writePath = m_HomeWritePath;
	stringtool::AppendTrailingBackslash( writePath );
	gpassert( stringtool::IsAbsolutePath( writePath ) );

	writePath += "\\";
	writePath += "lqd_dictionary.ldc6";

	FileSys::File writeFile;
	writeFile.CreateAlways( writePath );
	gpassert( writeFile.IsOpen() );

	FileSys::FileWriter writer( writeFile );

	if( !writer.Write( buffer.begin(), buffer.size() ) )
	{
		gpassert( 0 );
	}
	else
	{
		m_CondenseBytesWritten += buffer.size();
		++m_CondenseFilesWritten;
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: RCompileAll( const char * path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	////////////////////////////////////////
	//	compilre this dir

	if( ( m_CondenseFilesWritten % 16 ) == 0 )
	{
		gpgeneric( "-" );
	}

	if( !CompileDir( path ) )
	{
		gpgenericf(( "BinaryFuelDB - failed to compile '%s'\n", path ));
		return false;
	}

	++m_NumDirtyDirectoriesFound;

	////////////////////////////////////////
	//	recurse into dirs and compile

	FileSys::AutoFindHandle hFindRoot( path );

	if_not_gpassertf( !hFindRoot.IsNull(), ("Can't RCompileAll for [%s]", path ) )
	{
		return false;
	}

	FileSys::FindData TempFindData;
	FileSys::AutoFindHandle hFound;

	////////////////////////////////////////
	//	find dirs and recurse

	hFound.Find( hFindRoot, "*", FileSys::FINDFILTER_DIRECTORIES );

	if( hFound )
	{
		gpstring name;
		while( hFound.GetNext( name, false ) )
		{			gpstring subDir( path );
			subDir += name;
			subDir += "\\";
			if( !RCompileAll( subDir ) )
			{
				gpassertm( 0, "It's horrible, Sir." );
				return( false );
			}
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: CompileDir( const char * path )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelByteBuffer buffer;
	buffer.reserve( 1025*64 );
	if( CompileDir( path, buffer ) )
	{
		return WriteDir( path, buffer );
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: CompileDir( const char * path, FuelByteBuffer & buffer )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( buffer.empty() );

	double timeBeforeRead = GetSystemSeconds();

	if( !m_TextDb->Init( OPTION_READ | OPTION_JIT_READ, gpstring( path ) ) )
	{
		gpassertm( 0, "Something horrible" );
		m_TextDb->Shutdown();
		return false;
	}

	double timeAfterRead = GetSystemSeconds();
	m_CondenseTimeReading += timeAfterRead - timeBeforeRead;

	if( !CompileDir( m_TextDb->GetRootBlock(), buffer ) )
	{
		gperrorf(( "Failed to condense path '%s'\n", path ));
		m_TextDb->Shutdown();
		return false;
	}

	m_TextDb->Shutdown();
	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: CompileDir( FuelBlock * textBlock, FuelByteBuffer & buffer )
{
	GPSTATS_SYSTEM( SP_FUEL );

	gpassert( textBlock->IsDirectory() );
	gpassert( buffer.empty() );

	////////////////////////////////////////
	//	get all the paths straight

	double timeBeforeCompile = GetSystemSeconds();
	DWORD bufferSizeBefore = buffer.size();

	////////////////////////////////////////
	//	write the binary file

	// store file header - version info etc.
	Store( BinaryFileHeader(), buffer, 0 );
	gpassert( !buffer.empty() );

	// save pointer to dir header
	DWORD sizeBeforeCompile = buffer.size();

	CompileDirBlock( textBlock->GetDB()->GetRootBlock(), buffer );

	// calc static checksum for buffer
	BinFuel::DirHeader * dirHeader	= (BinFuel::DirHeader *)(BYTE*)&buffer[ sizeBeforeCompile ];
	BYTE * beginDataBody			= (BYTE*)&buffer[ sizeBeforeCompile + sizeof( BinFuel::DirHeader ) ];
	BYTE * endDataBody				= (BYTE*)&buffer[ buffer.size()	];
	DWORD dataBodySize				= endDataBody - beginDataBody;
	dirHeader->m_StaticChecksum		= GetCRC32( 0, (void*)beginDataBody, dataBodySize );

	// store info in the header as to how many child dirs and .gas files this dir has
	BinaryFileHeader * fileHeader = (BinaryFileHeader *)buffer.begin();

	// did we encounter any dev blocks?
	fileHeader->m_Properties |= textBlock->GetDB()->DevBlocksEncountered() ? BinFuel::BinaryFileHeader::FP_INCLUDES_DEV_BLOCKS_NORMALLY : 0;
	// did we actually store any?
	fileHeader->m_Properties |= ( TestOptions( OPTION_NO_READ_DEV_BLOCKS ) ? 0 : BinFuel::BinaryFileHeader::FP_INCLUDES_DEV_BLOCKS_NOW );

	fileHeader->m_ChildPathChecksum = textBlock->GetChildPathChecksum();

	m_CondenseTimeCondensing += GetGlobalSeconds() - timeBeforeCompile;

	m_CondenseBytesCompiled += buffer.size() - bufferSizeBefore;
	++m_CondenseFilesCompiled;

	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: CompileDir( FuelBlock * textBlock, FileSys::StreamWriter & writer )
{
	GPSTATS_SYSTEM( SP_FUEL );

	FuelByteBuffer buffer;
	CompileDir( textBlock, buffer );

	if( writer.Write( buffer.begin(), buffer.size() ) )
	{
		return true;
	}
	else
	{
		gperrorf(( "Failed to write text block %s to bin\n", textBlock->GetAddress().c_str() ));
		return false;
	}
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: WriteDir( const char * path, FuelByteBuffer const & buffer )
{
	GPSTATS_SYSTEM( SP_FUEL );

	double timeBeforeWrite = GetSystemSeconds();

	gpstring writePathName, writeFileName;
	stringtool::ReplaceRootPath( path, m_HomeReadPath, m_HomeWritePath, writePathName );
	stringtool::AppendTrailingBackslash( writePathName );

	writeFileName = writePathName;
	writeFileName += "dir";
	writeFileName += GetBinFileExtension();

	FileSys::File writeFile;
	writeFile.CreateAlways( writeFileName, File::ACCESS_READ_WRITE, File::SHARE_NONE, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_HIDDEN );

	bool success = writeFile.IsOpen();

	if( !success )
	{
		if( !FileSys::DoesPathExist( writePathName ) )
		{
			// try to make the dir
			success = FileSys::CreateFullDirectory( writePathName );
			if( success )
			{
				writeFile.CreateAlways( writeFileName, File::ACCESS_READ_WRITE, File::SHARE_NONE, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_HIDDEN );
				success = writeFile.IsOpen();
			}
		}
	}

	if( success )
	{
		FileSys::FileWriter writer( writeFile );
		if( writer.Write( buffer.begin(), buffer.size() ) )
		{
			m_CondenseBytesWritten += buffer.size();
			++m_CondenseFilesWritten;
		}
		else
		{
			success = false;
		}
	}

	double timeAfterWrite = GetSystemSeconds();

	m_CondenseTimeWriting += timeAfterWrite - timeBeforeWrite;
	
	return success;
}


////////////////////////////////////////////////////////////////////////////////
//	
inline Header * GetHeaderPtr( FuelByteBuffer & buffer, DWORD offset )
{
	return (Header*)(buffer.begin() + offset);
}


////////////////////////////////////////////////////////////////////////////////
//	
bool BinaryFuelDB :: CompileDirBlock( FuelBlock * textBlock, FuelByteBuffer & buffer )
{
	GPSTATS_SYSTEM( SP_FUEL );

	/*
		output format:
		--------------
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

	// should children get rolled into the data block?
	// local buffer

	const DWORD numKeys		= textBlock->GetKeyAndValueCount();
	const DWORD numChildren = textBlock->GetChildBlocks().size();
	gpassert( !textBlock->IsDirectory() || ( textBlock->IsDirectory() && ( numKeys == 0 ) ) );

	// store dummy header
	const DWORD headerOffset = buffer.size();
	if( textBlock->IsDirectory() )
	{
		Store( DirHeader(), buffer, headerOffset );
	}
	else
	{
		Store( Header(), buffer, headerOffset );
	}
	Header * pHeader = NULL;

	// store dummy keyvalue infos

	for( DWORD iKey = 0; iKey != numKeys; ++iKey )
	{
		Store( KeyValueInfo(), buffer, headerOffset);
	}

	// store dummy child infos

	for( DWORD iChild = 0; iChild != numChildren; ++iChild )
	{
		Store( ChildInfo(), buffer, headerOffset );
	}

	// write data block

	const DWORD nameOffset	= Store( textBlock->GetName(), buffer, headerOffset );
	const DWORD typeOffset	= Store( textBlock->GetType(), buffer, headerOffset );

	pHeader = (Header *)( buffer.begin() + headerOffset );
	pHeader->m_NameOffset	= nameOffset;
	pHeader->m_TypeOffset	= typeOffset;
	pHeader->m_NumKeys		= numKeys;
	pHeader->m_NumChildren	= numChildren;
	pHeader->m_Properties	|= textBlock->IsDirectory() ? BP_IS_DIRECTORY : 0;
	pHeader = NULL;

	// write
	// write keys
	textBlock->FindFirstKeyAndValue();

	DWORD offset = 0;
	KeyValueInfo * pKeyValInfo = NULL;
	const char* key = NULL, * value = NULL;

	{for( DWORD iKey = 0; iKey != numKeys; ++iKey )
	{
		offset = 0;
		pKeyValInfo = NULL;

		eFuelValueProperty fvp = textBlock->GetNextKeyAndValue( key, value );

		//----- store key

		// refer key to dictionary, or store it

		DWORD dictionaryStringNum = BinFuel::BSearchGetDictionaryString( BinFuel::GetDictionary(), key );
		if( dictionaryStringNum )
		{
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_KeyOffset = dictionaryStringNum;
			pKeyValInfo->m_Properties |= KVP_KEY_IN_DICTIONARY;
		}
		else
		{
			offset = Store( key, buffer, headerOffset );
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_KeyOffset = offset;
		}

		//----- store value

		if( fvp & FVP_INT )
		{
			if( fvp & FVP_NORMAL_INT )
			{
				pKeyValInfo->m_Properties		|= KVP_VALUE_NORMAL_INT;
			}
			else if( fvp & FVP_HEX_INT )
			{
				pKeyValInfo->m_Properties		|= KVP_VALUE_HEX_INT;
			}
			else
			{
				gpassert( 0 );
			}
			int intValue;
			stringtool::Get( value, intValue );
			pKeyValInfo->m_ValueOrOffset	= intValue;
		}
		else if( fvp & FVP_FLOAT )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_FLOAT;

			float floatValue;
			stringtool::Get( value, floatValue );
			*(float*)&pKeyValInfo->m_ValueOrOffset = floatValue;
		}
		else if( fvp & FVP_BOOL )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_BOOL;

			bool boolValue;
			stringtool::Get( value, boolValue );
			pKeyValInfo->m_ValueOrOffset	= boolValue;
		}
		else if( fvp & FVP_DOUBLE )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_DOUBLE;

			double doubleValue;
			stringtool::Get( value, doubleValue );

			offset = Store( doubleValue, buffer, headerOffset );
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_ValueOrOffset = offset;
		}
		else if( fvp & FVP_SIEGE_POS )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_SIEGE_POS;

			SiegePosData posValue;
			::FromString( value, posValue );
			offset = Store( posValue, buffer, headerOffset );
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_ValueOrOffset = offset;
		}
		else if( fvp & FVP_QUAT )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_QUAT;

			Quat quatValue;
			::FromString( value, quatValue );
			offset = Store( quatValue, buffer, headerOffset );
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_ValueOrOffset = offset;
		}
		else if( fvp & FVP_VECTOR_3 )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_VECTOR_3;

			vector_3 vectorValue;
			::FromString( value, vectorValue );
			offset = Store( vectorValue, buffer, headerOffset );
			pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
			pKeyValInfo->m_ValueOrOffset = offset;
		}
		else if( fvp & FVP_STRING_PROPERTIES )
		{
			pKeyValInfo->m_Properties		|= KVP_VALUE_STRING;

			DWORD dictionaryStringNum = BinFuel::BSearchGetDictionaryString( BinFuel::GetDictionary(), value );

			if( dictionaryStringNum )
			{
				pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
				pKeyValInfo->m_ValueOrOffset = dictionaryStringNum;
				pKeyValInfo->m_Properties |= KVP_VALUE_IN_DICTIONARY;
			}
			else
			{
				offset = Store( value, buffer, headerOffset );
				pKeyValInfo	= GetKeyValueInfo( GetHeaderPtr( buffer, headerOffset ), iKey );
				pKeyValInfo->m_ValueOrOffset = offset;
			}
		}
		else
		{
			gpassert( "unsupported type" );
		}
		++m_CondenseKeyValuePairsCount;
	}}

	++m_CondenseBlockCount;

	// resurse into children and add their buffers to ours

	textBlock->SortChildBlocks();
	fuel_block_list::iterator iterChild = textBlock->GetChildBlocks().begin();

	{for( DWORD iChild = 0; iChild != numChildren; ++iChild )
	{
		// store normal block
		if( !(*iterChild)->IsDirectory() )
		{
			DWORD childOffset = buffer.size() - headerOffset;
			CompileDirBlock( *iterChild, buffer );

			pHeader = (Header *)( buffer.begin() + headerOffset );
			ChildInfo * pChildInfo = GetChildInfo( pHeader, iChild );

			pChildInfo->m_BlockHeaderOffset = childOffset;
		}
		// store dir block stub
		else
		{
			pHeader = (Header *)( buffer.begin() + headerOffset );
			ChildInfo * pChildInfo = GetChildInfo( pHeader, iChild );

			pChildInfo->m_BlockHeaderOffset		= 0xffffffff;
			pChildInfo->m_Properties			|= CP_IS_DIRECTORY;
		}

		++iterChild;
	}}

	// revisit header, adjust data

	{
		Header * pHeader = (Header*)( buffer.begin() + headerOffset );
		if( textBlock->IsDevOnly() )
		{
			pHeader->m_Properties |= BP_IS_DEV_ONLY;
		}

		if( textBlock->IsDirectory() )
		{
			((DirHeader *)pHeader)->m_Size = buffer.size() - headerOffset;
		}
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
//	

#if !GP_RETAIL

void BinaryFuelDB :: Dump( ReportSys::ContextRef ctx, bool verbose )
{
//	kerneltool::Critical::Lock lock( m_Critical );

	// $ no need to report if nothing done!
	if( verbose && ( m_NumDirtyDirectoriesFound != 0 ) )
	{
		ReportSys::AutoReport autoReport( ctx );

		ctx->Output(  "\n\nBinary Fuel Report:\n" );
		ctx->Output(  "===================\n" );

		ctx->OutputF( "Dirty dirs found             = %d\n",               	m_NumDirtyDirectoriesFound	);
		ctx->OutputF( "TimeReading                  = %.2f seconds\n",     	m_CondenseTimeReading		);
		ctx->OutputF( "TimeCondensing               = %.2f seconds\n",     	m_CondenseTimeCondensing	);
		ctx->OutputF( "TimeWriting                  = %.2f seconds\n\n",   	m_CondenseTimeWriting		);
		ctx->OutputF( "bytes compiled               = %d bytes\n",         	m_CondenseBytesCompiled		);
		ctx->OutputF( "files compiled               = %d files\n",         	m_CondenseFilesCompiled		);
		ctx->OutputF( "bytes written                = %d bytes\n",         	m_CondenseBytesWritten		);
		ctx->OutputF( "files written                = %d files\n\n",       	m_CondenseFilesWritten		);
		ctx->OutputF( "key value pairs written      = %d\n",			   	m_CondenseKeyValuePairsCount	);
		ctx->OutputF( "total kvp info size          = %d\n",			   	m_CondenseKeyValuePairsCount * sizeof( KeyValueInfo ) );
		ctx->OutputF( "blocks written               = %d\n",			   	m_CondenseBlockCount );
		ctx->OutputF( "total block header size      = %d\n",			   	m_CondenseBlockCount * sizeof( Header ) );
		ctx->OutputF( "average file size            = %d bytes/file \n\n", 	m_CondenseFilesWritten != 0 ? m_CondenseBytesWritten/m_CondenseFilesWritten : 0 );
		ctx->OutputF( "inefficient int reads        = %d\n",			   	m_InefficientIntReadCount	);
		ctx->OutputF( "inefficient float reads      = %d\n",			   	m_InefficientFloatReadCount	);
		ctx->OutputF( "inefficient bool reads       = %d\n",			   	m_InefficientBoolReadCount	);
		ctx->OutputF( "inefficient SiegePos reads   = %d\n",			   	m_InefficientSiegePosReadCount	);
		ctx->OutputF( "inefficient Quat reads       = %d\n",			   	m_InefficientQuatReadCount	);
		ctx->OutputF( "inefficient vector_3 reads   = %d\n",			   	m_InefficientVector3ReadCount	);
	}

	// cache related stats... ... .. .

	DWORD cachedInSlots			= 0;
	DWORD cachedOutSlots		= 0;
	DWORD zeroReferenceSlots	= 0;
	DWORD fuelAddressStringSize = 0;
/*
	DWORD zeroReferenceSlotsExpired = 0;
	DWORD zeroReferenceSlotsGood = 0;

	SlotColl::iterator i;
	for( i = m_BlockCache.begin(); i != m_BlockCache.end(); ++i )
	{
		if( (*i).m_RefCount == 0 )
		{
			++zeroReferenceSlots;
			if( m_CurrentVisitCount - (*i).m_VisitCount > m_UnloadVisitCountThreashold )
			{
				++zeroReferenceSlotsExpired;
			}
			else
			{
				++zeroReferenceSlotsGood;
			}
		}

		if( (*i).GetBlockHeader() )
		{
			++cachedInSlots;
		}
		else
		{
			++cachedOutSlots;
		}

		if( (*i).m_pFuelAddress )
		{
			fuelAddressStringSize += (*i).m_pFuelAddress->size();
		}
	}
*/
	if( verbose )
	{
		ctx->Output( "\n\n" );
		ctx->OutputF( "cache block slots               = %d\n",		m_BlockCache.size()				);
		ctx->OutputF( "bytes of cache block slots      = %d\n",		m_BlockCache.size() * sizeof( BlockCacheEntry ) );
		ctx->OutputF( "fuel address strings size       = %d\n",		fuelAddressStringSize			);
		ctx->OutputF( "zero reference slots            = %d\n",		zeroReferenceSlots				);
		ctx->OutputF( "cached in slots                 = %d\n",		cachedInSlots					);
		ctx->OutputF( "cached out slots                = %d\n",		cachedOutSlots					);
	}
	else
	{
		ctx->OutputF( "[binfuel]      s.lqd=%d, n.slt=%d, s.slt=%d, n.sco=%d, n.sci=%d\n",
						m_BytesFuelCurrentlyLoaded,
						m_BlockCache.size(),
						m_BlockCache.size() * sizeof( BlockCacheEntry ),
						cachedOutSlots,
						cachedInSlots );
/*
		ctx->OutputF( "               n.zref=%d, n.zref exp=%d, n.zref good=%d\n",
						zeroReferenceSlots,
						zeroReferenceSlotsExpired,
						zeroReferenceSlotsGood );
*/
		ctx->OutputF( "               uncache sweep = %d%%, c.lock = %d / %d (%2.0f%%), c.hit=0x%08x, c.miss=0x%08x\n", 
						int( (float( m_CurrentUnloadSweepIndex )/float( m_MaxUnloadSweepIndex )) * 100.0f ),
						FastFuelHandle::ConditionalLock::m_Locks, FastFuelHandle::ConditionalLock::m_Constructions,
						100.0f * ( float( FastFuelHandle::ConditionalLock::m_Locks ) / float( FastFuelHandle::ConditionalLock::m_Constructions ) ),
						m_CacheHits,
						m_CacheMisses );
	}
}


#endif // !GP_RETAIL