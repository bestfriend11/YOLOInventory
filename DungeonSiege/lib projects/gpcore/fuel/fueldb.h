#pragma once
/*========================================================================

  FuelDB, part of the "Fuel" system

  prupose:	This is the class responsible for doing all the file I/O in the Fuel
			system.  It also keep a registry of all the currently constructed
			FuelBlocks.

  author:	Bartosz Kijanka

  date:		9/1/1999

  (c)opyright 1999, Gas Powered Games

  conventions:

  todo:

  TextFuelDb:

  -		there is a vague sort of overlap in functionality and usage between LoadAll(..) 
		and LoadToAddress(..)  It would be nice to resolve this and make things clearer.

------------------------------------------------------------------------*/
#ifndef __FUELDB_H
#define __FUELDB_H


#include <set>
#include "fubidefs.h"
#include "Fuel_Types.h"
#include "kerneltool.h"
#include "FileSysDefs.h"
#include "vector_3.h"
#include "BucketVector.h"
#include "FubiTraitsImpl.h"
#include "kerneltool.h"






/*===========================================================================

	Class		: FuelSys

	Author(s)	: Bartosz Kijanka

	Purpose		: This class will keep track of multiple Fuel DB's

---------------------------------------------------------------------------*/
class FuelSys : public Singleton< FuelSys >
{
	typedef std::map < gpdumbstring, DWORD, string_less > StringDb;
	typedef std::map < gpstring, my FuelDB *, istring_less > FuelDBMap;


public:

	typedef std::set< gpstring, string_less > StringSet;

	FuelSys();
	~FuelSys();

	void Update();

	TextFuelDB   * AddTextDb( gpstring const & sName );

	BinaryFuelDB * AddBinaryDb( gpstring const & sName );
	BinaryFuelDB * GetBinaryDb( DWORD id )	{ return m_BinaryDbLookup[ id ]; }

	bool HasDb( gpstring const & sName );
	void Remove( FuelDB * pDB );

	FuelDB	 	 * GetFuelDbFromAddress( const char * sName, const char ** sNewAddress = NULL );
	TextFuelDB	 * GetTextDbFromAddress( const char * sName, const char ** sNewAddress = NULL );
	BinaryFuelDB * GetBinaryDbFromAddress( const char * sName, const char ** sNewAddress = NULL );

	TextFuelDB	 * GetDefaultTextDb()	{ return( m_pDefaultTextDb ); }
	BinaryFuelDB * GetDefaultBinaryDb()	{ return( m_pDefaultBinaryDb ); }

	// add entry to string collector
	const gpdumbstring & AddString( const gpdumbstring & str );
	const gpdumbstring & AddString( const char * str );
	StringDb & GetStringCollector() { return( m_StringCollector ); }

	// will unload all DBs
FEX	void Unload();
FEX void SetUncachingEnabled( bool flag );

FEX	void DebugSetShowHandleAccess( bool flag )			{ m_DebugShowHandleAccess = flag; }
FEX bool DebugShowHandleAccess()						{ return m_DebugShowHandleAccess; }

	static DWORD CalcChecksum( StringSet const & ss );

#	if !GP_RETAIL
FEX	void Dump();
	void Dump( ReportSys::ContextRef ctx, bool verbose = true );
#	endif // !GP_RETAIL


private:

	TextFuelDB * m_pDefaultTextDb;
	BinaryFuelDB * m_pDefaultBinaryDb;
	std::vector< BinaryFuelDB * > m_BinaryDbLookup;

	bool		m_DebugShowHandleAccess;
	StringDb	m_StringCollector;
	FuelDBMap	m_FuelDBMap;

	FUBI_SINGLETON_CLASS( FuelSys, "Fuel system master class." );
	SET_NO_COPYING( FuelSys );
};

#define gFuelSys FuelSys::GetSingleton()




/*===========================================================================

	Class		: 	FuelDB

	Author(s)	: 	Bartosz Kijanka

	Purpose		:

---------------------------------------------------------------------------*/
class FuelDB
{

public:

	enum eOptions
	{
		OPTION_NONE					=	  	  0,
		OPTION_READ					=	1 <<  0,
		OPTION_WRITE				=   1 <<  1,
		OPTION_JIT_READ				=	1 <<  2,
		OPTION_SAVE_EMPTY_BLOCKS	=   1 <<  3,
		OPTION_TEXT_MODE			=   1 <<  4,
		OPTION_BINARY_MODE			=	1 <<  5,
		OPTION_NO_READ_DEV_BLOCKS	=   1 <<  6,
		OPTION_NO_READ_LIQUID		=	1 <<  7,
		OPTION_NO_WRITE_LIQUID		=	1 <<  8,
		OPTION_NO_WRITE_TYPES		=	1 <<  9,
		OPTION_MEM_ONLY				=   1 << 10,
		OPTION_SIMPLE_ABORT_ON_FAIL	=   1 << 11,
	};

	FuelDB();

	virtual ~FuelDB(){};

	virtual void Update() = 0;

	virtual bool Init( 	eOptions options,
						gpstring const & readHome, 
						gpstring const & writeHome = gpstring::EMPTY ) = 0;

	virtual bool Reinit() = 0;

	virtual void Unload() = 0;

	virtual void Shutdown();

	gpstring const & GetName()									{ return m_Name; }
	void SetName( gpstring const & name )						{ gpassertm( m_Name.empty(), "can't re-set name" ); m_Name = name; }

	////////////////////////////////////////
	//	options

	void SetOptions   ( eOptions options, bool set = true )		{ m_Options = (eOptions)(set ? (m_Options | options) : (m_Options & ~options));  }
	void ClearOptions ( eOptions options )						{ SetOptions( options, false );  }
	void ToggleOptions( eOptions options )						{ m_Options = (eOptions)(m_Options ^ options);  }
	bool TestOptions  ( eOptions options ) const				{ return ( (m_Options & options) != 0 );  }
	bool TestOptionsEq( eOptions options ) const				{ return ( (m_Options & options) == options );  }

	virtual gpstring const & GetHomeReadPath() const	= 0;
	virtual gpstring const & GetHomeWritePath() const	= 0;

	bool GetInUserBlockDeleteRequestScope()						{ return( m_InUserBlockDeleteRequestScope ); }
	void SetInUserBlockDeleteRequestScope( bool x )				{ m_InUserBlockDeleteRequestScope = x; }

	// stats
	unsigned int GetNumFilesLoaded()							{ return( m_FilesLoaded 			); }
	unsigned int GetNumBlocksLoaded()							{ return( m_GasBlocksLoaded 		); }
	unsigned int GetLogicalLinesLoaded()						{ return( m_LogicalLinesLoaded 	); }

	////////////////////////////////////////
	//	debug

#	if !GP_RETAIL
	virtual void Dump( ReportSys::ContextRef ctx = NULL, bool verbose = true ) = 0;
#	endif // !GP_RETAIL

protected:

	gpstring m_Name;
	eOptions m_Options;

	unsigned int m_FilesLoaded;
	unsigned int m_GasBlocksLoaded;
	unsigned int m_LogicalLinesLoaded;

	bool m_InUserBlockDeleteRequestScope;
};

MAKE_ENUM_BIT_OPERATORS( FuelDB::eOptions );



/*===========================================================================

	Class		: TextFuelDB

	Author(s)	: Bartosz Kijanka

---------------------------------------------------------------------------*/
class TextFuelDB : public FuelDB
{

public:

	TextFuelDB();
	~TextFuelDB();

	friend class FuelBlock;
	friend class FuelHandle;

	virtual void Update(){};

	virtual bool Init( eOptions options, gpstring const & readHome, gpstring const & writeHome = gpstring::EMPTY );

	virtual bool Reinit();

	virtual void Unload();

	virtual void Shutdown();

	bool CriticalErrorEncountered();

	gpstring const & GetHomeReadPath() const					{ return( m_HomeReadPath ); }
	gpstring const & GetHomeWritePath() const					{ return( m_HomeWritePath ); }

	////////////////////////////////////////
	//	read

	bool ListOriginChangedBlocks( gpstring & output );
	bool ListOriginChangedFiles( gpstring & output );
	bool ListDirtyBlocks( gpstring & output );
	bool ListDirtyFiles( gpstring & output );

	////////////////////////////////////////
	//	write

	bool Save( bool save_all=true );
	bool SaveAll()												{ return( Save(true) ); }
	bool SaveChanges()											{ return( Save(false) ); }

	////////////////////////////////////////
	//	other

	gpstring const & AddOriginPath( gpstring const & path );
	void RemoveOriginPath( gpstring const & path );

	void SetDevBlocksEncountered( bool flag );
	bool DevBlocksEncountered()									{ return( m_DevBlockEncountered ); }

	////////////////////////////////////////
	//	debug

#	if !GP_RETAIL
	void Dump( ReportSys::ContextRef ctx = NULL, bool verbose = true );
#	endif // !GP_RETAIL

	FuelBlock * GetRootBlock();
	FuelHandle	GetRootHandle();


private:

	struct FileEntry
	{
		gpstring m_Name;
		FILETIME m_LastWriteTime;

		FileEntry( void )
			{  }
		FileEntry( const gpstring& name, const FILETIME& lwtime )
			: m_Name( name ), m_LastWriteTime( lwtime )  {  }
	};
	typedef std::vector< FileEntry > FileEntryColl;
	typedef std::vector< gpstring > StringColl;

	void SetHomeReadPath( gpstring const & sPath );

	bool SetHomeWritePath( gpstring const & sPath );

	void FindChildFilesAndDirs( gpstring const & rootPath, FileEntryColl & files, StringColl & dirs );

	
	//===============================================
	// existence
	//-----------------------------------------------

	FuelBlock * CreateBlock(	FuelBlock * parent,
								gpstring const & sBlockName,
								gpstring const & sRelOriginPath,
								bool bDirectory );

	void AddFileToDeleteList( gpstring const & sAbsPath );

	void RemoveFileFromDeleteList( gpstring const & sAbsPath );

	void ProcessFileDeleteList();
	
	// FuelBlock helpers
	FuelBlock * CreateChildDirBlock( FuelBlock * parent, gpstring const & sName );

	FuelBlock * CreateChildBlock( FuelBlock * parent, gpstring const & sName, gpstring const & sFileName );

	bool UnloadBlock( FuelBlock * pBlock );

	
	//===============================================
	// read
	//-----------------------------------------------

	// create blocks from .gas files and directories
	bool LoadAll( FuelBlock * dirBlock );

	// create blocks from a .gas file
	bool LoadBlocksFromFile(	FuelBlock * parent,
								gpstring const & filename, 
								gpstring const & parentRelativeTargetAddress, 
								FuelBlock *& target );

	// load minimum directories and .gas files necessary to resolve said Fuel address
	FuelBlock * LoadBlock( gpstring const & address );

	//----- helpers

	FuelBlock * CreateChildDirectoryBlock(	FuelBlock * parent, gpstring const & dirName );

	bool ReadBlocks(	FuelBlock * parent,
						gpstring const & relativeFindTarget,
						FuelBlock const * & findTarget,
						const char * buffer,
						const unsigned int buffer_size,
						gpstring const & sRelOriginPath,
						FILETIME const * OriginFileTime,
						unsigned int & position
						);

	bool ReadBlocksWrapped(	FuelBlock * parent,
							gpstring const & relativeFindTarget,
							FuelBlock const * & findTarget,
							const char * buffer,
							const unsigned int buffer_size,
							gpstring const & sRelOriginPath,
							FILETIME const * OriginFileTime,
							unsigned int & position
							);

	// this will look for the block in the current set of loaded blocks AND it can potentially look to disk .gas files for it
	FuelBlock * FindBlockGlobal( gpstring const & block_address );

	// this will look for the block in the current set of loaded fuel blocks only
	FuelBlock * FindBlockLocal( gpstring const & block_address );

	// has the file from which said block was loaded changed since the load time?
	bool HasOriginFileChanged( const FuelBlock * pBlock ) const;

	// reload block
	//	warning: this will also reload any blocks which came that same file
	FuelBlock * Reload( FuelBlock * pBlock );

	bool ReadKeyValuePair( gpstring & sKey, gpstring & sValue, eFuelValueProperty & valueProperties, const char * buffer, unsigned int & pos );

	FuelBlock * FindCreateLeafBlockFromAddress( FuelBlock * pParent, const char * buffer, unsigned int & pos );

	
	//===============================================
	// write
	//-----------------------------------------------

	bool WriteDirectoryBlock( FuelBlock * block, bool save_all );

	// this will write the block, and any children of the same origin path to the buffer
	bool WriteBlock(	FileSys::StreamWriter & writer,
						FuelBlock * block,
						unsigned int file_block_depth );

	//===============================================
	// data
	//-----------------------------------------------
	
	typedef std::map< gpstring, DWORD, istring_less > StringToIntMap;

	std::set< gpstring, istring_less > m_FileDeleteList;

	bool m_bCriticalError;

	StringToIntMap m_OriginPaths;

	FuelBlock * m_Root;

	gpstring m_HomeReadPath;

	gpstring m_HomeWritePath;

	bool m_DevBlockEncountered;
};




/*===========================================================================

	Class		:	BinaryFuelDB

	Author(s)	:	Bartosz Kijanka

	Todo:
	-----

	-	add checksum to lqd files
	-	count blocks with a null name, type, or no child blocks...
	-	count total number of blocks
	-	report total number bytes used for headers, and childinfos

  ---------------------------------------------------------------------------*/

class BinaryFuelDB : public FuelDB
{

#pragma pack ( push, 1 )

	struct BlockCacheEntry
	{
		enum eProperty
		{
			BP_NONE					= 0,
			BP_DIRECTORY			= 1 << 0,
			BP_STUB					= 1 << 1,
			BP_MARKED_FOR_DELETION	= 1 << 2,
		};

		inline BlockCacheEntry(){};

		inline void Init()
		{
			ZeroObject( *this );
		}

#if GP_DEBUG
		bool IsValid()
		{
			bool valid = true;
			valid = valid && ( m_pDb && ( m_pDb->GetCacheSlot(m_CacheId)->m_CacheId == m_CacheId ) );
			valid = valid && ( m_RefCount >= 0 );
			valid = valid && ( m_CacheId );

			if( m_pBlockHeader )
			{
				valid = valid && ( m_CacheId == m_pBlockHeader->m_CacheId );
				valid = valid && ( m_pFuelAddress == NULL );
				valid = valid && ( (IsDirectory() && m_pBlockHeader->IsDirectory()) || (!IsDirectory() && !m_pBlockHeader->IsDirectory()) );
				valid = valid && ( m_VisitCount > 0 );
			}
			else if( m_pFuelAddress )
			{
				valid = valid && ( !m_pBlockHeader );
			}
			return valid;
		}
# endif
		inline bool IsDirectory()										{ return ( m_Properties & BP_DIRECTORY ) != 0; }
		inline bool IsMarkedForDeletion()								{ return ( m_Properties & BP_MARKED_FOR_DELETION ) != 0; }

		inline void MarkForDeletion()									{ m_Properties |= BP_MARKED_FOR_DELETION; }

		inline BinFuel::Header *	GetBlockHeader()
		{
			gpassert( IsValid() );
			return m_pBlockHeader;
		}

		inline void					SetBlockHeader( BinFuel::Header * block )
		{
			m_pBlockHeader = block;
		}

#if GP_DEBUG
		DWORD						m_CacheId;
		BinaryFuelDB *				m_pDb;
#endif
		int							m_RefCount;
		gpstring *					m_pFuelAddress;
		DWORD						m_VisitCount;
		BYTE						m_Properties;

	private:

		BinFuel::Header *			m_pBlockHeader;
	};

#pragma pack ( pop )

	typedef UnsafeBucketVector< BlockCacheEntry* > SlotColl;


public:

	BinaryFuelDB();
	~BinaryFuelDB();

	virtual void Update();
	virtual bool Init( eOptions options, gpstring const & readHome, gpstring const & writeHome = gpstring::EMPTY );
	virtual bool Reinit();
	virtual void Unload()										{ PrivateUnload(); }
	virtual void Shutdown();

	gpstring const & GetHomeReadPath() const					{ return( m_HomeReadPath ); }
	gpstring const & GetHomeWritePath() const					{ return( m_HomeWritePath ); }

	DWORD GetId()												{ return( m_Id ); }

	////////////////////////////////////////
	//	read

	inline BinFuel::Header * GetRootHeader()					{ gpassert( m_RootHeader ); return( m_RootHeader ); }

	BinFuel::Header * GetBlockBySoftAddress( const char * addr );
	BinFuel::Header * GetBlockByHardAddress( const char * addr );

	const char * GetBinFileExtension()							{ return( m_Extension ); }
	
	BinFuel::Header * LoadDirBlock( const char * path, BinFuel::Header * parent = NULL );

	////////////////////////////////////////
	//	write

	bool RecompileAll( bool deleteExistingLqd = true );

	void DeleteAllLqdFiles();

	bool BuildDictionary( DWORD sizeLimit );

	bool CompileDir( FuelBlock * textBlock, FileSys::StreamWriter & writer );

	////////////////////////////////////////
	//	util

	gpstring GetDirBlockPath( BinFuel::Header const * block ) const;

	gpstring GetBlockSoftAddress( BinFuel::Header const * block ) const;
	void GetBlockHardAddress( BinFuel::Header const * block, gpstring & out ) const;

	void ResolveStubbedChild( BinFuel::Header * parent, DWORD childNum, BinFuel::Header *& newChildPtr );

	void RUnloadChildren( BinFuel::Header * block );

	BinFuel::DirHeader * ConstructStubbedDirBlock( BinFuel::Header * parent, const char * dirName );

	////////////////////////////////////////
	//	caching

	void SetUncachingEnabled( bool flag ) { m_UncachingEnabled = flag; }

	void Register( BinFuel::Header * block );
	void Unregister( DWORD cacheBlockId );

	void CacheIn( FastFuelHandle & fh, bool resolveStub );
	void CacheOut( DWORD cacheBlockId );

	void OnBlockDelete( BinFuel::Header * block, BinFuel::Header * replacedWithStub = NULL );

	BinFuel::Header * GetCachedBlockPtr( FastFuelHandle & fh, bool resolveStub );

	void MarkBlockVisited( BinFuel::Header * block );

	inline kerneltool::RwCritical& GetCacheSlotCritical()
	{
		return( m_BlockCache.GetCritical() );
	}

	inline BlockCacheEntry * GetCacheSlot( UINT cacheBlockId )
	{
		return( m_BlockCache[ cacheBlockId ] );
	}

	inline bool SlotAboutToExpire( DWORD id )
	{
		return ( (( m_CurrentVisitCount - GetCacheSlot( id )->m_VisitCount ) > ( m_UnloadVisitCountThreashold - 10 ) )	
				|| ( GetCacheSlot( id )->m_Properties & ( scast<DWORD>(BlockCacheEntry::BP_STUB) | scast<DWORD>(BlockCacheEntry::BP_MARKED_FOR_DELETION) ) ) );
	}

	inline bool HasCacheSlot( DWORD id )
	{
		return m_BlockCache.IsValid( id );
	}

	inline DWORD GetCurrentVisitCount()							{ return m_CurrentVisitCount; }

	kerneltool::Critical & GetCritical()						{ return m_Critical; }

	////////////////////////////////////////
	//	debug

	DWORD GetBytesFuelCurrentlyLoaded()							{ return( m_BytesFuelCurrentlyLoaded ); }
	double GetCondenseTimeTotal()								{ return( m_CondenseTimeReading + m_CondenseTimeCondensing + m_CondenseTimeWriting ); }
	DWORD GetNumDirtyDirectoriesFound()							{ return( m_NumDirtyDirectoriesFound ); }

	void IncrementInefficientIntReadCount()						{ ++m_InefficientIntReadCount;		}
	void IncrementInefficientFloatReadCount()					{ ++m_InefficientFloatReadCount;	}
	void IncrementInefficientBoolReadCount()					{ ++m_InefficientBoolReadCount;		}
	void IncrementInefficientSiegePosReadCount()				{ ++m_InefficientSiegePosReadCount;	}
	void IncrementInefficientQuatReadCount()					{ ++m_InefficientQuatReadCount;		}
	void IncrementInefficientVector3ReadCount()					{ ++m_InefficientVector3ReadCount;	}

#	if !GP_RETAIL
	void Dump( ReportSys::ContextRef ctx = NULL, bool verbose = true );
#	endif // !GP_RETAIL


private:

	void PrivateUnload();

	////////////////////////////////////////
	//	read

	BinFuel::Header * LoadDirBinFile( const char * path );

	void RFixupParentPointers( BinFuel::Header * dirBlock, BinFuel::Header * parent );

	bool IsDirDirty( const char * path, gpstring& lqdFilePath, FileSys::AutoFileHandle& lqdFile, FileSys::AutoMemHandle& mem );

	DWORD GetNumChildDirs( FuelBlock * textBlock );

	void DestroyStubbedDirBlock( BinFuel::Header * block );

	void GetKnownChildren( BinFuel::Header * block, bool includeDirs, bool includeNonDirs, HeaderColl & out );

	////////////////////////////////////////
	//	write

	bool RCompileAll( const char * path );

	void DeleteAllBinFiles( const char * path );

	bool CompileDir( const char * path );
	bool CompileDir( const char * path, FuelByteBuffer & buffer );
	bool CompileDir( FuelBlock * textBlock, FuelByteBuffer & buffer );

	bool CompileDirBlock( FuelBlock * textBlock, FuelByteBuffer & buffer );

	void CompileDictionary();

	bool WriteDir( const char * path, FuelByteBuffer const & buffer );


	////////////////////////////////////////////////////////////////////////////////
	//	data

	kerneltool::Critical			m_Critical;

	////////////////////////////////////////
	//	cache

	SlotColl						m_BlockCache;
	double							m_LastVisitIncrementTime;
	DWORD							m_CurrentVisitCount;
	DWORD							m_UnloadVisitCountThreashold;
	DWORD							m_CurrentUnloadSweepIndex;
	DWORD							m_MaxUnloadSweepIndex;

	////////////////////////////////////////
	//	db

	static DWORD					m_InstanceCount;
	DWORD							m_Id;

	gpstring 						m_HomeReadPath;
	gpstring 						m_HomeWritePath;

	gpstring						m_Extension;

	TextFuelDB *					m_TextDb;
	FastFuelHandle *				m_Root;
	BinFuel::Header *				m_RootHeader;

	////////////////////////////////////////
	//	stats 

	DWORD  m_NumDirtyDirectoriesFound;       

	DWORD  m_CondenseBytesCompiled;          
	DWORD  m_CondenseFilesCompiled;          

	DWORD  m_CondenseBytesWritten;           
	DWORD  m_CondenseFilesWritten;           

	double m_CondenseTimeReading;            
	double m_CondenseTimeCondensing;         
	double m_CondenseTimeWriting;            

	DWORD  m_InefficientIntReadCount;        
	DWORD  m_InefficientFloatReadCount;      
	DWORD  m_InefficientBoolReadCount;       
	DWORD  m_InefficientSiegePosReadCount;   
	DWORD  m_InefficientQuatReadCount;
	DWORD  m_InefficientVector3ReadCount;

	DWORD  m_CondenseNoNameBlockCount;       
	DWORD  m_CondenseNoTypeBlockCount;       
	DWORD  m_CondenseNoNameAndTypeBlockCount;

	DWORD  m_CondenseBoolValueCount;         
	DWORD  m_CondenseFloatValueCount;        
	DWORD  m_CondenseIntValueCount;          
	DWORD  m_CondenseStringValueCount;       

	DWORD  m_CondenseKeyValuePairsCount;     
	DWORD  m_CondenseBlockCount;             

	DWORD  m_BytesFuelCurrentlyLoaded;       

	DWORD  m_CacheHits;
	DWORD  m_CacheMisses;
	bool   m_UncachingEnabled;
};

// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 
// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 
// $$$ hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack hack 

// EVIL data struct mirror of SiegePos to add support to binary fuel -bk

struct SiegeGuidData
{
	inline UINT32 GetValue( void ) const
	{
		return( (m_Guid[0]<<24) + (m_Guid[1]<<16) + (m_Guid[2]<<8) + m_Guid[3] );
	}

	inline void SetValue( UINT32 GUID )
	{
		m_Guid[0] = (UINT8)(( GUID & 0xff000000 ) >> 24);
		m_Guid[1] = (UINT8)(( GUID & 0x00ff0000 ) >> 16);
		m_Guid[2] = (UINT8)(( GUID & 0x0000ff00 ) >> 8 );
		m_Guid[3] = (UINT8)(( GUID & 0x000000ff )      );
	}

	gpstring ToString( void ) const;
	bool     FromString( const char* str );

	UINT8 m_Guid[4];
};


struct SiegePosData
{
	vector_3		pos;
	SiegeGuidData	node;
};


FUBI_DECLARE_POD_TRAITS( SiegePosData )
{
	static void ToString  ( gpstring& out, const Type& obj, FuBi::eXfer xfer = FuBi::XFER_NORMAL );
	static bool FromString( const char* str, Type& obj );
};

namespace stringtool
{
	inline bool Get( const char * in, SiegePosData& out )
		{  return ( FuBi::Traits <SiegePosData>::FromString( in, out ) );  }
}

#endif
