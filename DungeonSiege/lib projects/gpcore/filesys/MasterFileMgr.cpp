//////////////////////////////////////////////////////////////////////////////
//
// File     :  MasterFileMgr.cpp (GPCore:FileSys)
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "MasterFileMgr.h"

#include "FileSysUtils.h"
#include "GpStatsDefs.h"
#include "PathFileMgr.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "TankFileMgr.h"

#include <algorithm>

namespace FileSys  {  // begin of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
// future notes

/*
	* Implement relative path caching of some kind. Want to try to avoid the
      repeated path walking that will be expensive.

	* Add dumping stuff.

	* Switch to the MemReader code instead to collect functionality.
*/

//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

ReportSys::Context& GetFileLogContext( void )
{
	static ReportSys::Context* gFileLogContextPtr = NULL;
	if ( gFileLogContextPtr == NULL )
	{
		static ReportSys::LocalContext s_FileLogContext( "FileLog" );
		gFileLogContextPtr = &s_FileLogContext;

		s_FileLogContext.AddSink( &gDebuggerSink, false );
		s_FileLogContext.AddSink( new ReportSys::LogFileSink <> ( "filelog.log" ), true );
		s_FileLogContext.SetType( "Log" );
	}
	return ( *gFileLogContextPtr );
}

#define gpfilelog( msg ) DEV_MACRO_REPORT( &GetFileLogContext(), msg )
#define gpfilelogf( msg ) DEV_MACRO_REPORTF( &GetFileLogContext(), msg )

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr implementation

MasterFileMgr :: MasterFileMgr( eOptions options )
{
	// force construction
	GPDEV_ONLY( GetFileLogContext() );

	// i'm the master!
	if ( !HasMasterFileMgr() )
	{
		SetMasterFileMgr( this );
	}

	// i'm also number one! (zero) - but don't enter self into the order coll
	m_FileMgrColl.push_back( FileMgrEntry( this, false, -1, NULL ) );

	// init other stuff
	m_Options             = options;
	m_NormalDisabledCount = 0;
	m_DriverDisabledCount = 0;
	m_IndexRoot           . Clear();
	m_IndexDirty          = false;
}

MasterFileMgr :: ~MasterFileMgr( void )
{
	// nothing should still be open
	gpassert( !HasUsedHandles() );

	// delete anything that we own
	FileMgrColl::iterator i, begin = m_FileMgrColl.begin() + 1, end = m_FileMgrColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( (i->m_FileMgr != NULL) && i->m_Owned )
		{
			Delete( i->m_FileMgr );
		}
	}

	// kill the index
	m_IndexRoot.Delete();

	// the master has died!
	if ( HasMasterFileMgr() && (GetMasterFileMgr() == this) )
	{
		ReleaseMasterFileMgr( this );
	}
}

void MasterFileMgr :: AddFileMgr( IFileMgr* fileMgr, bool owned, int priority, const char* driverName )
{
	// parameter validation
	gpassert( (fileMgr != NULL) && (fileMgr != this) );							// be nice to me
	gpassert( (driverName != NULL) || !m_FindHandleMgr.HasUsedHandles() );		// don't add new normal filemgrs while find handles are outstanding
	gpassert( stdx::has_none( m_FileMgrColl, fileMgr ) );						// don't allow duplicates
	gpassert( (driverName == NULL) || IsValidFileName( driverName, false ) );	// must have a valid name if one is provided
	gpassert( (driverName == NULL) || !HasDriver( driverName ) );				// no duplicate drivers

// First add it to the master collection.

	// find an empty slot to stick it in (skip "this" which is in slot 0)
	FileMgrColl::iterator fi = std::find( m_FileMgrColl.begin() + 1, m_FileMgrColl.end(), scast <IFileMgr*> ( 0 ) );

	// put it in there
	FileMgrEntry newEntry( fileMgr, owned, priority, driverName );
	if ( fi == m_FileMgrColl.end() )
	{
		fi = m_FileMgrColl.insert( fi, newEntry );
	}
	else
	{
		*fi = newEntry;
	}

	// remember index
	UINT index = fi - m_FileMgrColl.begin();

// Then add it to the ordered collection by priority level.

	// find the right collection
	OrderColl& orderColl = (driverName != NULL) ? m_DriverOrderColl : m_NormalOrderColl;

	// where does it fit in?
	OrderColl::iterator oi, obegin = orderColl.begin(), oend = orderColl.end();
	for ( oi = obegin ; oi != oend ; ++oi )
	{
		if ( priority < m_FileMgrColl[ *oi ].m_Priority )
		{
			break;
		}
	}

	// insert either where it fits or at the end
	orderColl.insert( oi, index );

// Indexes will need updating.

	// only bother if we did not install a driver
	if ( (driverName == NULL) && fileMgr->IsEnabled() )
	{
		SetIndexDirty();
	}
}

bool MasterFileMgr :: RemoveFileMgr( IFileMgr* fileMgr )
{
	// parameter validation
	gpassert( (fileMgr != NULL) && (fileMgr != this) );

// Find it.

	// if it's not in there, just ignore
	FileMgrColl::iterator ffound = std::find( m_FileMgrColl.begin() + 1, m_FileMgrColl.end(), fileMgr );
	if ( ffound == m_FileMgrColl.end() )
	{
		// $ early bailout
		return ( false );
	}

	// figure out where it is in the order
	OrderColl& orderColl = ffound->IsDriver() ? m_DriverOrderColl : m_NormalOrderColl;
	OrderColl::iterator ifound = stdx::find( orderColl, scast <UINT> ( ffound - m_FileMgrColl.begin() ) );
	gpassert( ifound != orderColl.end() );

// Remove it.

	// only bother if not a driver
	if ( !ffound->IsDriver() )
	{
		gpassert( !m_FindHandleMgr.HasUsedHandles() );
		if ( ffound->m_FileMgr->IsEnabled() )
		{
			SetIndexDirty();
		}
	}

	// ok remove it
	gpassert( ffound->m_RefCount == 0 );
	ffound->Reset();
	orderColl.erase( ifound );
	return ( true );
}

bool MasterFileMgr :: EnableFileMgr( IFileMgr* fileMgr, bool enable )
{
	// if it's not in there, just ignore
	FileMgrColl::iterator ffound = std::find( m_FileMgrColl.begin() + 1, m_FileMgrColl.end(), fileMgr );
	if ( ffound == m_FileMgrColl.end() )
	{
		return ( false );
	}

	// ignore if unchanged
	if ( ffound->m_FileMgr->IsEnabled() == enable )
	{
		return ( true );
	}

	// need to dirty index? also update count
	if ( ffound->IsDriver() )
	{
		m_DriverDisabledCount += enable ? -1 : 1;
	}
	else
	{
		m_NormalDisabledCount += enable ? -1 : 1;
		SetIndexDirty();
	}

	// return success code from enable
	return ( ffound->m_FileMgr->Enable( enable ) );
}

void MasterFileMgr :: RebuildMasterIndex( void )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

// Simple checks.

	// can't rebuild index if somebody is iterating over it
	gpassert( !m_FindHandleMgr.HasUsedHandles() );

	// no longer dirty
	m_IndexDirty = false;

	// no need for indexing?
	if ( !IsIndexRequired() )
	{
		return; // $ early bailout
	}

	// notify whoever cares
	CallRebuildCb();

// Rebuild the index.

	// clear out old index
	m_IndexRoot.Delete();

	// passed params: parentFind is FindHandle for parent dir
	//                currentDir is IndexDirEntry* for current dir

	// iterate over normal filemgrs in priority order
	OrderColl::const_iterator oi, obegin = m_NormalOrderColl.begin(), oend = m_NormalOrderColl.end();
	for ( oi = obegin ; oi != oend ; ++oi )
	{
		const FileMgrEntry& fileMgrEntry = m_FileMgrColl[ *oi ];
		if ( fileMgrEntry.m_FileMgr->IsEnabled() )
		{
			AddToIndex( fileMgrEntry.m_FileMgr, *oi, FindHandle::Null, &m_IndexRoot );
		}
	}
}

bool MasterFileMgr :: HasDriver( const char* driver ) const
{
	gpassert( driver && *driver );

	FileMgrColl::const_iterator i, ibegin = m_FileMgrColl.begin() + 1, iend = m_FileMgrColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->IsDriver() && i->m_DriverName.same_no_case( driver ) )
		{
			return ( true );
		}
	}

	return ( false );
}

FileHandle MasterFileMgr :: Open( const char* name, eUsage usage )
{
	// parameter validation
	gpassert( (name != NULL) && (*name != '\0') );

	// reporting
#	if ( !GP_RETAIL )
	{
		if ( TestOptions( OPTION_LOG_OPENS ) )
		{
			gpfilelogf(( "Open: %s\n", name ));
		}
	}
#	endif // !GP_RETAIL

// Check drivers.

	// look for URL-style path
	const char* driverPrefix = ::strstr( name, "://" );
	if ( (driverPrefix != NULL) && (*driverPrefix != '\0') )
	{
		// find it
		OrderColl::const_iterator oi, obegin = m_DriverOrderColl.begin(), oend = m_DriverOrderColl.end();
		for ( oi = obegin ; oi != oend ; ++oi )
		{
			const FileMgrEntry& fileMgrEntry = m_FileMgrColl[ *oi ];

			// paranoia check
			gpassert( fileMgrEntry.m_FileMgr != NULL );
			gpassert( !fileMgrEntry.m_DriverName.empty() );

			// only if enabled
			if ( fileMgrEntry.m_FileMgr->IsEnabled() )
			{
				// check for driver name
				if ( fileMgrEntry.m_DriverName.same_no_case( name, driverPrefix - name ) )
				{
					// definitely use this driver - $ early bailout
					return ( OpenDirect( *oi, driverPrefix + 3, IndexPtr::Null, usage ) );
				}
			}
		}
	}

// Special: if no index required then just use our first real filemgr.

	UINT singleIndex = GetSingleFileMgrIndex();
	if ( singleIndex != 0 )
	{
		// try to open it from there - $ early bailout
		return ( OpenDirect( singleIndex, name, IndexPtr::Null, usage ) );
	}

// Check main index now.

	// walk the path
	const char* lastComponent;
	IndexEntryPtr entryPtr = FindIndex( NULL, name, lastComponent );
	if ( entryPtr.IsNull() )  return ( FileHandle::Null );

	// get the name as lowercase
	FakeIndexEntryPtr search;
	search.m_Name.assign( lastComponent );
	search.m_Name.to_lower();
	IndexEntryPtr filePtr;

	// check for aliases
	int offset = 0;
	const StringColl* aliases = GetFileTypeAliases( search.m_Name, &offset );
	if ( aliases != NULL )
	{
		// now loop over aliases
		StringColl::const_iterator i, ibegin = aliases->begin(), iend = aliases->end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// swap alias
			search.m_Name.replace( offset, *i );

			// attempt open
			filePtr = entryPtr.GetValidDir()->Find( search );
			if ( !filePtr.IsNull() )
			{
				break;
			}
		}

		// not found? go back to original
		if ( filePtr.IsNull() )
		{
			search.m_Name.assign( lastComponent );
			search.m_Name.to_lower();
		}
	}

	// find the component as a file
	if ( filePtr.IsNull() )
	{
		filePtr = entryPtr.GetValidDir()->Find( search );
	}

	// handle error
	if ( filePtr.IsNull() )
	{
		::SetLastError( ERROR_FILE_NOT_FOUND );
		return ( FileHandle::Null );
	}

	// finally open the file
	return ( OpenHelp( filePtr, usage ) );
}

FileHandle MasterFileMgr :: Open( IndexPtr where, eUsage usage )
{
	// $$$ IMPLEMENT LOGGING HERE TOO!!!

// Special: pass along to sole filemgr.

	UINT singleIndex = GetSingleFileMgrIndex();
	if ( singleIndex != 0 )
	{
		// try to open it from there - $ early bailout
		return ( OpenDirect( singleIndex, NULL, where, usage ) );
	}

// It's ours.

	// dereference it
	IndexEntryPtr entryPtr = GetIndexEntryPtr( where );
	if ( entryPtr.IsNull() )  return ( FileHandle::Null );

	return ( OpenHelp( entryPtr, usage ) );
}

bool MasterFileMgr :: Close( FileHandle file )
{
	bool success = false;

	FileMgrEntry* entry = file ? GetFileMgrEntry( file ) : NULL;
	if ( (entry != NULL) && (entry->m_FileMgr != NULL) )
	{
		success = entry->m_FileMgr->Close( file );
		if ( success || (::GetLastError() != ERROR_INVALID_HANDLE) )	// $ generally handles will get closed if they aren't invalid (regardless of errors)
		{
			RemoveRef( *entry );
		}
	}

	return ( success );
}

bool MasterFileMgr :: Read( FileHandle file, void* out, int size, int* bytesRead )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? false : mgr->Read( file, out, size, bytesRead ) );
}

bool MasterFileMgr :: ReadLine( FileHandle file, gpstring& out )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? false : mgr->ReadLine( file, out ) );
}

bool MasterFileMgr :: SetReadWrite( FileHandle file, bool set )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? false : mgr->SetReadWrite( file, set ) );
}

bool MasterFileMgr :: Seek( FileHandle file, int offset, eSeekFrom origin )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? false : mgr->Seek( file, offset, origin ) );
}

int MasterFileMgr :: GetPos( FileHandle file )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? INVALID_POS_VALUE : mgr->GetPos( file ) );
}

int MasterFileMgr :: GetSize( FileHandle file )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? INVALID_SIZE_VALUE : mgr->GetSize( file ) );
}

eLocation MasterFileMgr :: GetLocation( FileHandle file )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? LOCATION_ERROR : mgr->GetLocation( file ) );
}

bool MasterFileMgr :: GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
{
	IFileMgr* mgr = GetFileMgr( file );
	return ( ( mgr == NULL ) ? false : mgr->GetFileTimes( file, creation, lastAccess, lastWrite ) );
}

MemHandle MasterFileMgr :: Map( FileHandle file, int offset, int size )
{
	MemHandle mem;

	FileMgrEntry* entry = file ? GetFileMgrEntry( file ) : NULL;
	if ( (entry != NULL) && (entry->m_FileMgr != NULL) )
	{
		mem = entry->m_FileMgr->Map( file, offset, size );
		if ( mem != MemHandle::Null )
		{
			mem.SetOwnerIndex( file.GetOwnerIndex() );
			AddRef( *entry );
		}
	}

	return ( mem );
}

bool MasterFileMgr :: Close( MemHandle mem )
{
	bool success = false;

	FileMgrEntry* entry = mem ? GetFileMgrEntry( mem ) : NULL;
	if ( (entry != NULL) && (entry->m_FileMgr != NULL) )
	{
		success = entry->m_FileMgr->Close( mem );
		if ( success || (::GetLastError() != ERROR_INVALID_HANDLE) )	// $ generally handles will get closed if they aren't invalid (regardless of errors)
		{
			RemoveRef( *entry );
		}
	}

	return ( success );
}

int MasterFileMgr :: GetSize( MemHandle mem )
{
	IFileMgr* mgr = GetFileMgr( mem );
	return ( ( mgr == NULL ) ? INVALID_SIZE_VALUE : mgr->GetSize( mem ) );
}

bool MasterFileMgr :: Touch( MemHandle mem, int offset, int size )
{
	IFileMgr* mgr = GetFileMgr( mem );
	return ( ( mgr == NULL ) ? false : mgr->Touch( mem, offset, size ) );
}

const void* MasterFileMgr :: GetData( MemHandle mem, int offset )
{
	IFileMgr* mgr = GetFileMgr( mem );
	return ( ( mgr == NULL ) ? NULL : mgr->GetData( mem, offset ) );
}

FindHandle MasterFileMgr :: Find( const char* pattern, eFindFilter filter )
{

// Initialize.

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}

	// if it's just a path then tack on a wildcard
	gpstring localPattern;
	char trail = pattern[ ::strlen( pattern ) - 1 ];
	if ( (trail == '/') || (trail == '\\') || (trail == ':') )
	{
		// redirect
		localPattern = pattern;
		localPattern += '*';
		pattern = localPattern;
	}

// Check drivers.

	// look for URL-style path
	const char* driverPrefix = ::strstr( pattern, "://" );
	if ( (driverPrefix != NULL) && (*driverPrefix != '\0') )
	{
		// find it
		OrderColl::const_iterator oi, obegin = m_DriverOrderColl.begin(), oend = m_DriverOrderColl.end();
		for ( oi = obegin ; oi != oend ; ++oi )
		{
			const FileMgrEntry& fileMgrEntry = m_FileMgrColl[ *oi ];

			// paranoia check
			gpassert( fileMgrEntry.m_FileMgr != NULL );
			gpassert( !fileMgrEntry.m_DriverName.empty() );

			// only if enabled
			if ( fileMgrEntry.m_FileMgr->IsEnabled() )
			{
				// check for driver name
				if ( fileMgrEntry.m_DriverName.same_no_case( pattern, driverPrefix - pattern ) )
				{
					// definitely use this driver - $ early bailout
					return ( FindDirect( *oi, driverPrefix + 3, filter ) );
				}
			}
		}
	}

// Special: if no index required then just use our first real filemgr.

	UINT singleIndex = GetSingleFileMgrIndex();
	if ( singleIndex != 0 )
	{
		// try to open it from there - $ early bailout
		return ( FindDirect( singleIndex, pattern, filter ) );
	}

// Check main index now.

	FindHandle find;

	const char* lastComponent;
	IndexEntryPtr entryPtr = FindIndex( NULL, pattern, lastComponent );
	if ( !entryPtr.IsNull() )
	{
		find = FindHelp( entryPtr, lastComponent, filter );
	}

	return ( find );
}

FindHandle MasterFileMgr :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	// a null base means root $ early bailout
	if ( !base.IsFinding() )
	{
		return ( Find( pattern, filter ) );
	}

// Find the file.

	FindHandle find;

	FileMgrEntry* entry = GetFileMgrEntryMasterOkay( base );
	if ( entry != NULL )
	{
		if ( entry->m_FileMgr == this )
		{
			IndexDirEntry* currentDir = NULL;

			// look up dir from that handle
			{
				RwCritical::ReadLock readLock( m_RwCritical );
				FindEntry* findEntry = GetFindEntry( base );
				if ( findEntry == NULL )
				{
					return ( find );
				}
				currentDir = findEntry->m_CurrentDir;
			}

			// re-search from new base
			const char* lastComponent;
			IndexEntryPtr entryPtr = FindIndex( currentDir, pattern, lastComponent );
			if ( !entryPtr.IsNull() )
			{
				find = FindHelp( entryPtr, lastComponent, filter );
			}
		}
		else if ( entry->m_FileMgr != NULL )
		{
			find = entry->m_FileMgr->Find( base, pattern, filter );
			if ( find != FindHandle::Null )
			{
				find.SetOwnerIndex( base.GetOwnerIndex() );
				AddRef( *entry );
			}
		}
	}

	return ( find );
}

bool MasterFileMgr :: GetNext( FindHandle find, gpstring& out, bool prependPath )
{
	bool success = false;

	IFileMgr* mgr = GetFileMgrMasterOkay( find );
	if ( mgr == this )
	{
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
		if ( findEntry != NULL)
		{
			// get next
			success = findEntry->Next( &out, prependPath );
		}
	}
	else if ( mgr != NULL )
	{
		success = mgr->GetNext( find, out, prependPath );
	}

	return ( success );
}

bool MasterFileMgr :: GetNext( FindHandle find, FindData& out, bool prependPath )
{
	bool success = false;

	IFileMgr* mgr = GetFileMgrMasterOkay( find );
	if ( mgr == this )
	{
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
		if ( findEntry == NULL)  return ( false );

		// find next
		if ( findEntry->Next( &out.m_Name, prependPath ) )
		{
			success = QueryDirect( *findEntry->m_CurrentEntry, out, prependPath );
		}
	}
	else if ( mgr != NULL )
	{
		// it's a direct handle - pass along to owner
		success = mgr->GetNext( find, out, prependPath );
	}

	return ( success );
}

bool MasterFileMgr :: HasNext( FindHandle find )
{
	bool success = false;

	IFileMgr* mgr = GetFileMgrMasterOkay( find );
	if ( mgr == this )
	{
		RwCritical::ReadLock readLock( m_RwCritical );

		// get handle data
		FindEntry* findEntry = m_FindHandleMgr.Dereference( find );
		if ( findEntry == NULL)  return ( false );

		// find next
		if ( findEntry->Next( NULL, false ) )
		{
			success = true;
		}
	}
	else if ( mgr != NULL )
	{
		// it's a direct handle - pass along to owner
		success = mgr->HasNext( find );
	}

	return ( success );
}

bool MasterFileMgr :: Close( FindHandle find )
{
	bool success = false;

	// get handle data
	FileMgrEntry* entry = find ? GetFileMgrEntryMasterOkay( find ) : NULL;
	if ( entry != NULL )
	{
		if ( entry->m_FileMgr == this )
		{
			RwCritical::WriteLock writeLock( m_RwCritical );

			// this is one of ours - release the handle
			m_FindHandleMgr.Release( find );
			RemoveRef( *entry );
		}
		else if (entry->m_FileMgr != NULL)
		{
			// pass it along to owned filemgr and dec ref count
			success = entry->m_FileMgr->Close( find );
			if ( success || (::GetLastError() != ERROR_INVALID_HANDLE) )	// $ generally handles will get closed if they aren't invalid (regardless of errors)
			{
				RemoveRef( *entry );
			}
		}
	}

	return ( success );
}

bool MasterFileMgr :: QueryFileInfo( IndexPtr where, FindData& out )
{
	// find the index entry
	IndexEntryPtr queryPtr;
	where.GetValidPointer( queryPtr );

	// do the query
	return ( QueryDirect( queryPtr, out, false ) );
}

FileHandle MasterFileMgr :: OpenHelp( IndexEntryPtr ptr, eUsage usage )
{
	FileHandle file;

	// make sure it's a file
	if ( ptr.IsFile() )
	{
		IndexEntry* queryEntry = ptr.GetEntry();
		if ( queryEntry->m_IndexPtr.IsNull() )
		{
			gpstring filename;
			queryEntry->BuildName( filename, true );
			file = OpenDirect( queryEntry->m_OwnerIndex, filename.c_str(), IndexPtr::Null, usage );
		}
		else
		{
			file = OpenDirect( queryEntry->m_OwnerIndex, NULL, queryEntry->m_IndexPtr, usage );
		}
	}
	else
	{
		// trying to open a directory? are you crazy?
		gperrorf(( "MasterFileMgr::OpenHelp() - attempted to open a directory as a file '%s'\n",
					  ptr.GetName().c_str() ));
		::SetLastError( ERROR_FILE_NOT_FOUND );
	}

	return ( file );
}

FileHandle MasterFileMgr :: OpenDirect( UINT fileMgrIndex, const char* name, IndexPtr ptr, eUsage usage )
{
	// find entry
	FileMgrEntry* entry = GetFileMgrEntry( Handle( fileMgrIndex, 0 ) );
	if ( entry == NULL )  return ( FileHandle::Null );

	// try to open file from that filemgr
	gpassert( entry->m_FileMgr != NULL );
	FileHandle file = (name == NULL) ? entry->m_FileMgr->Open( ptr, usage )
									 : entry->m_FileMgr->Open( name, usage );

	// fix it up if found
	if ( file != FileHandle::Null )
	{
		file.SetOwnerIndex( fileMgrIndex );
		AddRef( *entry );
	}

	// all done
	return ( file );
}

MasterFileMgr::IndexEntryPtr MasterFileMgr :: FindIndex( IndexDirEntry* base, const char* name, const char*& lastComponent )
{
	// $ note: this should have been called by now in a multithreaded system.
	//         building the index is not thread-locked with Find() and Open()
	//         functions for performance reasons.
	CheckIndexDirty();

	{	// $ scope

		// build search spec
		IndexEntryPtr entryPtr;
		entryPtr.SetDir( (base == NULL) ? &m_IndexRoot : base );
		FakeIndexEntryPtr search;

		// for each path component in the name
		const char* nameBegin = name, * nameIter = nameBegin;
		const char* nameEnd = stringtool::NextPathComponent( nameIter );
		while ( *nameEnd != '\0' )
		{
			// find the component as a child
			search.m_Name.assign( nameBegin, nameEnd );
			search.m_Name.to_lower();
			entryPtr = entryPtr.GetValidDir()->Find( search );
			if ( entryPtr.IsNull() )
			{
				goto error;
			}

			// advance
			nameBegin = nameIter;
			nameEnd   = stringtool::NextPathComponent( nameIter );

			// if it's a file and we're expecting a dir then bail right here
			if ( entryPtr.IsFile() && (nameEnd != '\0') )
			{
				goto error;
			}
		}

		// return results
		lastComponent = nameBegin;
		return ( entryPtr );
	}

// Error case.

error:
	::SetLastError( ERROR_FILE_NOT_FOUND );
	return ( IndexEntryPtr() );
}

FindHandle MasterFileMgr :: FindHelp( IndexEntryPtr dirPtr, const char* pattern, eFindFilter filter )
{
	RwCritical::WriteLock writeLock( m_RwCritical );

// Prep some vars.

	IndexDirEntry* dir = dirPtr.GetValidDir();

	// acquire an empty handle
	FindHandle findHandle;
	FindEntry* findEntry = m_FindHandleMgr.Acquire( findHandle );

	// copy basic spec in
	findEntry->m_CurrentDir   = dir;
	findEntry->m_CurrentEntry = dir->m_Entries.begin();
	findEntry->m_Filter       = filter;

// Analyze and build pattern.

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}
	else
	{
		// pattern can have no path info in it
		gpassert( pattern[ strcspn( pattern, "\\/" ) ] == '\0' );
	}

	// $ note below - there's no point checking for a "no wildcards" case for
	//   a simple string compare match. the normal wildcard case will handle it
	//   fine, plus it's going to be very rare that someone does a find with no
	//   wildcards. of course this can be optimized if needed.

	// test for simplest case first
	if ( same_with_case( pattern, "*" ) || same_with_case( pattern, "*.*" ) )
	{
		// pattern clear for an all match
		findEntry->m_Pattern.erase();
	}
	else
	{
		// copy pattern in and make it lower case
		findEntry->m_Pattern = pattern;
		findEntry->m_Pattern.to_lower();
	}

// Start the find going.

	findEntry->First();
	AddRef( m_FileMgrColl[0] );

	// all done
	return ( findHandle );
}

FindHandle MasterFileMgr :: FindDirect( UINT fileMgrIndex, const char* pattern, eFindFilter filter )
{
	// find entry (slot 0 illegal as it's nonlocal)
	gpassert( (fileMgrIndex > 0) && (fileMgrIndex < m_FileMgrColl.size()) );
	FileMgrEntry& entry = m_FileMgrColl[ fileMgrIndex ];

	// try to find file from that filemgr
	gpassert( entry.m_FileMgr != NULL );
	FindHandle find = entry.m_FileMgr->Find( pattern, filter );

	// fix it up if found
	if ( find != FindHandle::Null )
	{
		find.SetOwnerIndex( fileMgrIndex );
		AddRef( entry );
	}

	// all done
	return ( find );
}

bool MasterFileMgr :: QueryDirect( IndexEntryPtr queryPtr, FindData& out, bool prependPath )
{
	bool success = false;

	// get a valid entry pointer
	IndexEntry* entry = queryPtr.GetValidEntry();

	// find the owning filemgr
	IFileMgr* mgr = GetFileMgr( Handle( entry->m_OwnerIndex, 0 ) );
	if ( mgr != NULL )
	{
		// preserve name in case it's overwritten
		gpstring oldName( out.m_Name );

		// do the query
		if ( entry->m_IndexPtr.IsNull() )
		{
			// null - must use the full name (may have already built it)
			if ( prependPath )
			{
				// already got it
				IndexPtr queryPtr;
				queryPtr.SetPointer( out.m_Name.c_str() );
				success = mgr->QueryFileInfo( queryPtr, out );
			}
			else
			{
				// gotta build it
				gpstring name;
				entry->BuildName( name, true );
				IndexPtr queryPtr;
				queryPtr.SetPointer( name.c_str() );
				success = mgr->QueryFileInfo( queryPtr, out );
			}
		}
		else
		{
			// direct - cool!
			success = mgr->QueryFileInfo( entry->m_IndexPtr, out );
		}

		// update the indexptr for future fast access
		out.m_IndexPtr.SetPointer( queryPtr );

		// update the filename in case it was overwritten
		out.m_Name = oldName;
	}

	return ( success );
}

void MasterFileMgr :: AddToIndex( IFileMgr* mgr, UINT mgrIndex, FindHandle parentFind, IndexDirEntry* currentDir )
{
	gpassert( (mgr != NULL) && (currentDir != NULL) );

	const char* pattern = NULL;
	if ( currentDir->m_Name.empty() )
	{
		pattern = "*";
	}
	else
	{
		int len = currentDir->m_Name.length();
		pattern = (char*)_alloca( len + 3 );
		::memcpy( (char*)pattern, currentDir->m_Name.c_str(), len );
		::memcpy( (char*)(pattern + len), "\\*", 3 );
	}

	FindHandle currentFind = mgr->Find( parentFind, pattern );
	if ( currentFind )
	{
		FindData childData;
		while ( mgr->GetNext( currentFind, childData ) )
		{
			// skip metafiles/metadirs
			if ( childData.m_Name[ 0 ] == '$' )
			{
				continue;
			}

			// create child and own it
			AutoIndexEntryPtr ptr;
			bool recurse = childData.IsDirectory();
			if ( recurse )
			{
				ptr.SetDir( new IndexDirEntry );
			}
			else
			{
				ptr.SetFile( new IndexFileEntry);
			}

			// set up for searching
			IndexEntry* newEntry = ptr.GetValidEntry();
			newEntry->m_Name = childData.m_Name;
			gpassert( newEntry->m_Name.is_lower() );

			// attempt insertion
			IndexInsertRC rc = currentDir->m_Entries.insert( ptr );
			if ( rc.second )
			{
				// it happened - release the ptr and setup entry
				ptr.Release();
				newEntry->m_Parent     = currentDir;
				newEntry->m_IndexPtr   = childData.m_IndexPtr;
				newEntry->m_OwnerIndex = mgrIndex;
			}
			else
			{
				// entry already existed - check to make sure it's not a file
				recurse = rc.first->IsDir();
				if ( recurse )
				{
					// only bother if there is a valid index
					newEntry = rc.first->GetValidEntry();
					if ( !newEntry->m_IndexPtr.IsNull() )
					{
						// find mgr
						FileMgrEntry* fileMgr = GetFileMgrEntry( Handle( newEntry->m_OwnerIndex, 0 ) );
						gpassert( (fileMgr != NULL) && (fileMgr->m_FileMgr != NULL) );

						// get the file time of the existing one
						FindData oldData;
						fileMgr->m_FileMgr->QueryFileInfo( newEntry->m_IndexPtr, oldData );

						// if the new dir is newer then use it instead
						if ( ::CompareFileTime( &oldData.m_LastWriteTime, &childData.m_LastWriteTime ) < 0 )
						{
							newEntry->m_IndexPtr = childData.m_IndexPtr;
							newEntry->m_OwnerIndex = mgrIndex;
						}
					}
				}
			}

			// now recurse
			if ( recurse )
			{
				AddToIndex( mgr, mgrIndex, currentFind, rc.first->GetValidDir() );
			}
		}

		mgr->Close( currentFind );
	}
	else
	{
		gperrorf(( "MasterFileMgr indexing failed on IFileMgr #%d pattern '%s' (%s)\n",
				   mgrIndex, pattern, stringtool::GetLastErrorText().c_str() ));
	}
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::IndexEntry implementation

void MasterFileMgr::IndexEntry :: BuildName( gpstring& name, bool prependPath ) const
{
	int totalInsert = 0;

	// figure out how long path prefix is going to be (don't include root)
	if ( prependPath )
	{
		for ( IndexEntry* i = m_Parent ; (i != NULL) && (i->m_Parent != NULL) ; i = i->m_Parent )
		{
			totalInsert += i->m_Name.length() + 1;	// plus slash
		}
	}

	// create string and copy in the filename
	name.resize( totalInsert + m_Name.length() );
	::memcpy( name.begin_split() + totalInsert, m_Name.c_str(), m_Name.length() );

	// copy in a dir at a time
	if ( prependPath )
	{
		for ( IndexDirEntry* i = m_Parent ; (i != NULL) && (i->m_Parent != NULL) ; i = i->m_Parent )
		{
			name.at_split( --totalInsert ) = '\\';

			gpdumbstring& thisDir = i->m_Name;
			int thisLen = thisDir.length();

			totalInsert -= thisLen;
			::memcpy( name.begin_split() + totalInsert, thisDir.c_str(), thisLen );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::IndexDirEntry implementation

void MasterFileMgr::IndexDirEntry :: Delete( void )
{
	// $ should be able to do this but compiler sees an access problem:
	//
	// stdx::for_each( m_Entries, std::mem_fun( IndexEntryPtr::Delete ) );

	IndexIter i, begin = m_Entries.begin(), end = m_Entries.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->Delete();
	}

	m_Entries.clear();
}

//////////////////////////////////////////////////////////////////////////////
// class MasterFileMgr::FindEntry implementation

void MasterFileMgr::FindEntry :: First( void )
{
	// $ this function cannot fail - the directory must exist otherwise we
	//   never would have made it this far. files not being found is not
	//   an error condition.

	gpassert( m_CurrentDir != NULL );
	if ( m_CurrentEntry != m_CurrentDir->m_Entries.end() )
	{
		if ( IsMatch() )
		{
			// data already available next time around
			m_Mode = MODE_FIRST;
		}
		else
		{
			// if it's not a match then need to keep looking - Next() will set to EMPTY when it runs out
			m_Mode = MODE_NORMAL;
			if ( Next( NULL, false ) )
			{
				m_Mode = MODE_FIRST;
			}
		}
	}
	else
	{
		::SetLastError( ERROR_NO_MORE_FILES );		// someone may want to check this
		m_Mode = MODE_EMPTY;
	}
}

bool MasterFileMgr::FindEntry :: Next( gpstring* filename, bool prependPath )
{
	bool success = false;

// Find the file.

	if ( m_Mode == MODE_FIRST )
	{
		// data is already there - use it (may be empty though)
		m_Mode = MODE_NORMAL;
		success = true;
	}
	else if ( m_Mode == MODE_EMPTY )
	{
		// nothing there
		::SetLastError( ERROR_NO_MORE_FILES );
	}
	else
	{
		// keep looking until we find a match or run out of entries
		for ( ; ; )
		{
			++m_CurrentEntry;
			if ( m_CurrentEntry == m_CurrentDir->m_Entries.end() )
			{
				::SetLastError( ERROR_NO_MORE_FILES );
				m_Mode = MODE_EMPTY;
				break;
			}

			if ( IsMatch() )
			{
				success = true;
				break;
			}
		}
	}

// Tell the caller about it.

	// copy over filename
	if ( success && (filename != NULL) )
	{
		m_CurrentEntry->GetEntry()->BuildName( *filename, prependPath );
	}

	return ( success );
}

bool MasterFileMgr::FindEntry :: IsMatch( void ) const
{
	bool match = m_CurrentEntry->IsDir() ? !!(m_Filter & FINDFILTER_DIRECTORIES)
										 : !!(m_Filter & FINDFILTER_FILES);

	// match only if necessary
	if ( match && !m_Pattern.empty() )
	{
		// paranoia check to see that we're all in lowercase here
		gpassert( m_Pattern.is_lower() );
		gpassert( m_CurrentEntry->GetName().is_lower() );

		// check match
		match = stringtool::IsDosWildcardMatch( m_Pattern, m_CurrentEntry->GetName(), true );
	}

	return ( match );
}

//////////////////////////////////////////////////////////////////////////////
// class MultiFileMgr implementation

#if !GP_RETAIL

// cheap macros 'cause i'm lazy, and all these functions do the same things
#define MULTI_IMPL( TYPE, NAME, HANDLE, PARAMS, BAD )				\
	Critical::Lock critical( m_Critical );							\
	TYPE##Entry* entry = m_##TYPE##HandleMgr.Dereference( HANDLE );	\
	if ( entry == NULL)  return ( BAD );							\
	return ( entry->m_FileMgr->NAME PARAMS );
#define MULTI_FILE_IMPL( NAME, PARAMS, BAD ) MULTI_IMPL( File, NAME, file, PARAMS, BAD )
#define MULTI_MEM_IMPL( NAME, PARAMS, BAD ) MULTI_IMPL( Mem, NAME, mem, PARAMS, BAD )
#define MULTI_HANDLE entry->m_Handle

MultiFileMgr :: MultiFileMgr( void )
{
	// this space intentionally left blank...
}

MultiFileMgr :: ~MultiFileMgr( void )
{
	// nothing should still be open
	gpassert( !HasUsedHandles() );

	// delete anything that we own
	FileMgrColl::iterator i, begin = m_FileMgrColl.begin(), end = m_FileMgrColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->second.m_Owned )
		{
			Delete( i->second.m_FileMgr );
		}
	}
}

void MultiFileMgr :: AddFileMgr( IFileMgr* fileMgr, bool owned, int priority )
{
	// parameter validation
	gpassert( (fileMgr != NULL) && (fileMgr != this) );		// be nice to me
	gpassert( !HasUsedHandles() );							// can't have anything open while we do this!

	// don't allow duplicates
#	if GP_DEBUG
	FileMgrColl::iterator i, ibegin = m_FileMgrColl.begin(), iend = m_FileMgrColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpassert( i->second.m_FileMgr != fileMgr );
	}
#	endif // GP_DEBUG

	// add to end
	m_FileMgrColl.insert( std::make_pair( priority, FileMgrEntry( fileMgr, owned ) ) );
}

bool MultiFileMgr :: RemoveFileMgr( IFileMgr* fileMgr )
{
	// parameter validation
	gpassert( (fileMgr != NULL) && (fileMgr != this) );		// don't try this at home
	gpassert( !HasUsedHandles() );							// can't have anything open while we do this!

	// erase if found, otherwise ignore
	FileMgrColl::iterator i, ibegin = m_FileMgrColl.begin(), iend = m_FileMgrColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->second.m_FileMgr == fileMgr )
		{
			m_FileMgrColl.erase( i );
			return ( true );
		}
	}
	return ( false );
}

bool MultiFileMgr :: EnableFileMgr( IFileMgr* fileMgr, bool enable )
{
	// $ this doesn't need to do anything fancy currently, because there is no
	//   static index to worry about keeping up to date.

	gpassert( fileMgr != NULL );
	if ( fileMgr->IsEnabled() != enable )
	{
		return ( fileMgr->Enable( enable ) );
	}
	return ( true );
}

IFileMgr* MultiFileMgr :: AddTankFileMgr( const wchar_t* fileName )
{
	TankFileMgr* fileMgr = new TankFileMgr;
	if ( fileMgr->OpenTank( ::ToAnsi( fileName ) ) )
	{
		AddFileMgr( fileMgr, true, fileMgr->GetTankPriorityForMaster() );
	}
	else
	{
		Delete( fileMgr );
	}

	return ( fileMgr );
}

bool MultiFileMgr :: AddTankPaths( const wchar_t* paths, bool parseSemicolons, AddResults* results )
{
	std::list <gpwstring> foundFiles;
	FindFilesFromSpec( paths, foundFiles, parseSemicolons, L"*.ds*", results ? &results->m_BadPaths : NULL );

	bool foundAny = false;

	std::list <gpwstring>::const_iterator i, ibegin = foundFiles.begin(), iend = foundFiles.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		IFileMgr* fileMgr = AddTankFileMgr( *i );
		if ( fileMgr != NULL )
		{
			foundAny = true;
			if ( results != NULL )
			{
				results->m_Added.push_back( *i );
				results->m_FileMgrs.push_back( fileMgr );
			}
		}
		else if ( results != NULL )
		{
			results->m_NotAdded.push_back( NotAdded( *i, ::GetLastError() ) );
		}
	}

	return ( foundAny );
}

IFileMgr* MultiFileMgr :: AddBitsFileMgr( const wchar_t* path )
{
	PathFileMgr* fileMgr = new PathFileMgr;
	if ( fileMgr->SetRoot( ::ToAnsi( path ) ) )
	{
		AddFileMgr( fileMgr, true );
	}
	else
	{
		Delete( fileMgr );
	}

	return ( fileMgr );
}

static bool AddBitsPaths( MultiFileMgr* mgr, const wchar_t* path, MultiFileMgr::AddResults* results )
{
	IFileMgr* fileMgr = mgr->AddBitsFileMgr( path );
	if ( results != NULL )
	{
		if ( fileMgr != NULL )
		{
			results->m_Added.push_back( path );
			results->m_FileMgrs.push_back( fileMgr );
		}
		else
		{
			results->m_BadPaths.push_back( path );
			results->m_NotAdded.push_back( MultiFileMgr::NotAdded( path, ::GetLastError() ) );
		}
	}

	return ( fileMgr != NULL );
}

bool MultiFileMgr :: AddBitsPaths( const wchar_t* paths, bool parseSemicolons, AddResults* results )
{
	bool foundAny = false;

	if ( parseSemicolons )
	{
		std::list <gpwstring> foundPaths;
		FindPathsFromSpec( paths, foundPaths );

		std::list <gpwstring>::const_iterator i, ibegin = foundPaths.begin(), iend = foundPaths.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			foundAny |= FileSys::AddBitsPaths( this, *i, results );
		}
	}
	else
	{
		foundAny |= FileSys::AddBitsPaths( this, paths, results );
	}

	return ( foundAny );
}

bool MultiFileMgr :: HasUsedHandles( void ) const
{
	return (   !m_FileHandleMgr.empty()
			|| !m_MemHandleMgr .empty()
			|| !m_FindHandleMgr.empty() );
}

FileHandle MultiFileMgr :: Open( const char* name, eUsage usage )
{
	Critical::Lock critical( m_Critical );

	FileHandle file;

	// find someone to open this with
	FileMgrColl::iterator i, ibegin = m_FileMgrColl.begin(), iend = m_FileMgrColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		IFileMgr* fileMgr = i->second.m_FileMgr;
		if ( fileMgr->IsEnabled() )
		{
			// give each a try
			file = fileMgr->Open( name, usage );
			if ( file )
			{
				// allocate handle
				FileHandle multiFile;
				FileEntry* fileEntry = m_FileHandleMgr.Acquire( multiFile );

				// configure it
				fileEntry->m_FileMgr = fileMgr;
				fileEntry->m_Handle  = file;

				// redirect
				file = multiFile;
				break;
			}
		}
	}

	// done
	return ( file );
}

FileHandle MultiFileMgr :: Open( IndexPtr /*where*/, eUsage /*usage*/ )
{
	Critical::Lock critical( m_Critical );

	// $ IndexPtr's are not available (nor necessary) with the MultiFileMgr so
	//   calling this function is a coding error on the client side. Call the
	//   other Open() instead.

	gpassertm( 0, "Not supported!" );

	::SetLastError( ERROR_NOT_SUPPORTED );
	return ( FileHandle::Null );
}

bool MultiFileMgr :: Close( FileHandle file )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* entry = m_FileHandleMgr.Dereference( file );
	if ( entry == NULL)  return ( false );

	// close it
	bool rc = entry->m_FileMgr->Close( entry->m_Handle );

	// release handle and finish
	m_FileHandleMgr.Release( file );
	return ( rc );
}

bool MultiFileMgr :: Read( FileHandle file, void* out, int size, int* bytesRead )
	{  MULTI_FILE_IMPL( Read, ( MULTI_HANDLE, out, size, bytesRead ), false );  }
bool MultiFileMgr :: ReadLine( FileHandle file, gpstring& out )
	{  MULTI_FILE_IMPL( ReadLine, ( MULTI_HANDLE, out ), false );  }
bool MultiFileMgr :: SetReadWrite( FileHandle file, bool set )
	{  MULTI_FILE_IMPL( SetReadWrite, ( MULTI_HANDLE, set ), false );  }
bool MultiFileMgr :: Seek( FileHandle file, int offset, eSeekFrom origin )
	{  MULTI_FILE_IMPL( Seek, ( MULTI_HANDLE, offset, origin ), false );  }
int MultiFileMgr :: GetPos( FileHandle file )
	{  MULTI_FILE_IMPL( GetPos, ( MULTI_HANDLE ), INVALID_POS_VALUE );  }
int MultiFileMgr :: GetSize( FileHandle file )
	{  MULTI_FILE_IMPL( GetSize, ( MULTI_HANDLE ), INVALID_SIZE_VALUE );  }
eLocation MultiFileMgr :: GetLocation( FileHandle file )
	{  MULTI_FILE_IMPL( GetLocation, ( MULTI_HANDLE ), LOCATION_ERROR );  }
bool MultiFileMgr :: GetFileTimes( FileHandle file, FILETIME* creation, FILETIME* lastAccess, FILETIME* lastWrite )
	{  MULTI_FILE_IMPL( GetFileTimes, ( MULTI_HANDLE, creation, lastAccess, lastWrite ), false );  }

MemHandle MultiFileMgr :: Map( FileHandle file, int offset, int size )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FileEntry* entry = m_FileHandleMgr.Dereference( file );
	if ( entry == NULL)  return ( false );

	// map it
	MemHandle mem = entry->m_FileMgr->Map( entry->m_Handle, offset, size );
	if ( !mem )  return ( false );

	// allocate handle
	MemHandle multiMem;
	MemEntry* memEntry = m_MemHandleMgr.Acquire( multiMem );

	// configure it
	memEntry->m_FileMgr    = entry->m_FileMgr;
	memEntry->m_FileHandle = file;
	memEntry->m_Handle     = mem;

	// done
	return ( multiMem );
}

bool MultiFileMgr :: Close( MemHandle mem )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	MemEntry* entry = m_MemHandleMgr.Dereference( mem );
	if ( entry == NULL)  return ( false );

	// close it
	bool rc = entry->m_FileMgr->Close( entry->m_Handle );

	// release handle and finish
	m_MemHandleMgr.Release( mem );
	return ( rc );
}

int MultiFileMgr :: GetSize( MemHandle mem )
	{  MULTI_MEM_IMPL( GetSize, ( MULTI_HANDLE ), INVALID_SIZE_VALUE );  }
bool MultiFileMgr :: Touch( MemHandle mem, int offset, int size )
	{  MULTI_MEM_IMPL( Touch, ( MULTI_HANDLE, offset, size ), false );  }
const void* MultiFileMgr :: GetData( MemHandle mem, int offset )
	{  MULTI_MEM_IMPL( GetData, ( MULTI_HANDLE, offset ), NULL );  }

FindHandle MultiFileMgr :: Find( const char* pattern, eFindFilter filter )
{
	return ( Find( FindHandle(), pattern, filter ) );
}

FindHandle MultiFileMgr :: Find( FindHandle base, const char* pattern, eFindFilter filter )
{
	Critical::Lock critical( m_Critical );

// Initialize.

	// default to all files
	if ( (pattern == NULL) || (*pattern == '\0') )
	{
		pattern = "*";
	}

	// if we have a base to work from, use that as the starting path
	gpstring localPattern;
	if ( base )
	{
		// look it up first
		FindEntry* entry = m_FindHandleMgr.Dereference( base );
		if ( entry == NULL )  return ( FindHandle::Null );

		// prefix with pattern
		localPattern = entry->m_Path;
		localPattern += pattern;
		pattern = localPattern;
	}

	// if it's just a path then tack on a wildcard
	char trail = pattern[ ::strlen( pattern ) - 1 ];
	if ( (trail == '/') || (trail == '\\') || (trail == ':') )
	{
		// redirect
		localPattern = pattern;
		localPattern += '*';
		pattern = localPattern;
	}

	// output goes here
	FindHandle find;
	typedef std::pair <IFileMgr*, FindHandle> FindPair;
	typedef stdx::fast_vector <FindPair> FindColl;
	FindColl findColl;

	// open a find handle for each mgr
	{
		FileMgrColl::iterator i, ibegin = m_FileMgrColl.begin(), iend = m_FileMgrColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			IFileMgr* fileMgr = i->second.m_FileMgr;
			if ( fileMgr->IsEnabled() )
			{
				FindHandle localFind = fileMgr->Find( pattern, filter );
				if ( localFind )
				{
					findColl.push_back( std::make_pair( fileMgr, localFind ) );
				}
			}
		}
	}

	// got any?
	if ( !findColl.empty() )
	{
		// allocate handle
		FindEntry* findEntry = m_FindHandleMgr.Acquire( find );

		// configure it
		findEntry->m_Path.assign( pattern, GetPathEnd( pattern ) );

		// special optimization: if only one succeeded, we can direct access
		if ( findColl.size() == 1 )
		{
			findEntry->m_FileMgr = findColl.front().first;
			findEntry->m_Handle  = findColl.front().second;
		}
		else
		{
			FindData findData;

			// this dir needs resolving, have to fill it manually
			FindColl::iterator i, ibegin = findColl.begin(), iend = findColl.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				IFileMgr* fileMgr = i->first;
				FindHandle localFind = i->second;

				while ( fileMgr->GetNext( localFind, findData ) )
				{
					// we're sorted in priority order, so any existing
					// entries take precedence.
					findEntry->m_DirDb.insert( std::make_pair( findData.m_Name, findData ) );
				}

				// we have all the data, no need for the find handle
				fileMgr->Close( localFind );
			}

			// start iter at top
			findEntry->m_DirIter = findEntry->m_DirDb.begin();
		}
	}

	// done
	return ( find );
}

bool MultiFileMgr :: GetNext( FindHandle find, gpstring& out, bool prependPath )
{
	FindData findData;
	if ( !GetNext( find, findData, prependPath ) )
	{
		return ( false );
	}

	out = findData.m_Name;
	return ( true );
}

bool MultiFileMgr :: GetNext( FindHandle find, FindData& out, bool prependPath )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* entry = m_FindHandleMgr.Dereference( find );
	if ( entry == NULL)  return ( false );

	// special handling for direct access
	if ( entry->m_FileMgr != NULL )
	{
		return ( entry->m_FileMgr->GetNext( entry->m_Handle, out, prependPath ) );
	}

	// check for end
	if ( entry->m_DirIter == entry->m_DirDb.end() )
	{
		return ( false );
	}

	// copy straight over and advance
	out = entry->m_DirIter->second;
	++entry->m_DirIter;

	// prefix path?
	if ( prependPath )
	{
		out.m_Name.insert( 0, entry->m_Path );
	}

	// done
	return ( true );
}

bool MultiFileMgr :: HasNext( FindHandle find )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* entry = m_FindHandleMgr.Dereference( find );
	if ( entry == NULL)  return ( false );

	// special handling for direct access
	if ( entry->m_FileMgr != NULL )
	{
		return ( entry->m_FileMgr->HasNext( entry->m_Handle ) );
	}

	// just see if we're at the end
	return ( entry->m_DirIter != entry->m_DirDb.end() );
}

bool MultiFileMgr :: Close( FindHandle find )
{
	Critical::Lock critical( m_Critical );

	// get handle data
	FindEntry* entry = m_FindHandleMgr.Dereference( find );
	if ( entry == NULL)  return ( false );

	// close if open
	if ( entry->m_FileMgr != NULL )
	{
		entry->m_FileMgr->Close( entry->m_Handle );
	}

	// cheap reset of data
	*entry = FindEntry();

	// release handle and finish
	m_FindHandleMgr.Release( find );
	return ( true );
}

bool MultiFileMgr :: QueryFileInfo( IndexPtr where, FindData& out )
{
	Critical::Lock critical( m_Critical );
	bool success = false;

	// $ note that GetFileAttributesEx() would probably be more efficient here
	//   but it's not supported on Win95 (98/NT only).

	// get the filename
	const char* queryName;
	where.GetValidPointer( queryName );

	// find it
	FindHandle find = Find( queryName );
	if ( find )
	{
		success = GetNext( find, out );
		Close( find );
	}

	return ( success );
}

#undef MULTI_IMPL
#undef MULTI_FILE_IMPL
#undef MULTI_MEM_IMPL
#undef MULTI_HANDLE

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FileSys

//////////////////////////////////////////////////////////////////////////////
