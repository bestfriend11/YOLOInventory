#include "precomp_nema.h"
#include "nema_keymgr.h"

#include "filesys.h"
#include "fubipersist.h"
#include "fubitraits.h"
#include "nema_persist.h"

using namespace nema;

//************************************************************
PRSKeysStorage::PRSKeysStorage(void) 
	: m_PRSKeysManager(10000) 
{
}
  
//************************************************************
PRSKeysStorage::~PRSKeysStorage(void) {

	m_PRSKeysManager.FreeAllHandlesAndCompact();

}

FUBI_DECLARE_XFER_TRAITS( PRSKeysStorage::PRSKeysEntry )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		return ( persist.XferList( "m_Inst", obj.m_Inst ) );
	}
};

// ***************************************************
int PRSKeysStorage::AddRefPRSKeys ( HPRSKeys hKeys )
{
	kerneltool::Critical::Lock locker( m_CreateLock );
	return m_PRSKeysManager.AddRef(hKeys);
}

// ***************************************************
int PRSKeysStorage::ReleasePRSKeys( HPRSKeys hkeys) 
{

	kerneltool::Critical::Lock locker( m_CreateLock );

	gpassertf(hkeys.IsValid(),("Attempting to call ReleasePRSKeys() on an invalid handle"));

	int ret = NULL;

	if (hkeys.IsValid()) {

		int rc = m_PRSKeysManager.GetRefCount( hkeys );

		if ( rc == 1 )
		{
			// list entry
			IndexEntry& indEntry = *m_PRSKeysManager.GetExtra( hkeys );
			PRSKeysEntry& prsEntry = indEntry.m_NameEntry->second;
			prsEntry.m_Inst.erase( indEntry.m_ListEntry );

			// if it's the last one, remove name entry
			if ( prsEntry.m_Inst.empty() )
			{
				m_PRSKeysByNameIndex.erase( indEntry.m_NameEntry );

			}
		}

		// release the handle
		ret =  m_PRSKeysManager.Release( hkeys );
	}

	return ret;

}


//************************************************************
HPRSKeys PRSKeysStorage::LoadPRSKeys(char const * const fname, char const * const dbgname) 
{

	m_CreateLock.Enter();

	HPRSKeys hkeys = HPRSKeys();

	std::pair <StringToPRSKeysMap::iterator, bool>
			rc = m_PRSKeysByNameIndex.insert( std::make_pair( fname, PRSKeysEntry() ) );

	if ( rc.second )
	{
		gpassert( (fname != NULL) && (*fname != '\0') );

		PRSKeys* keys = NULL;

		//gpgenericf(("\nCreating PRS [%s]\n",dbgname));

		FileSys::FileHandle file;
		FileSys::MemHandle mem;
		if ( file.Open( fname ) )
		{
			if ( mem.Map( file ) )
			{
				__try
				{
					keys = new PRSKeys(const_mem_ptr( mem.GetData(), mem.GetSize() ) );
				}
				__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH, EXCEPTION_EXECUTE_HANDLER ) )
				{
					gperrorf(( "Something has corrupted [%s] If you choose to continue, expect to see a lot of warnings and graphical glitches", fname ));
					Delete(keys);
				}
				mem.Close();
			}
			file.Close();
		}

		if (!keys) {
			//gpwarningf(("Unable to find/load PRS key file %s",fname));
			m_PRSKeysByNameIndex.erase(rc.first);
		}
		else
		{
			keys->SetDebugName(dbgname);

			// create handle
			hkeys = m_PRSKeysManager.Create( keys );
			// gpgenericf(("Assigned to handle %08x\n",hkeys));

			// add to index
			PRSKeysEntry& prsEntry = rc.first->second;
			prsEntry.m_Impl = keys->m_pImpl;
			prsEntry.m_Inst.push_front(hkeys);

			// give handle extra data reverse reference to index
			IndexEntry& indEntry = *m_PRSKeysManager.GetExtra( hkeys );
			indEntry.m_NameEntry = rc.first;
			indEntry.m_ListEntry = prsEntry.m_Inst.begin();
		}

	} else {

		hkeys = rc.first->second.m_Inst.back();

		gpassertf(hkeys.IsValid(),("The managed hkey for %s has been invalidated",fname));
		if (hkeys.IsValid())
		{
			//gpgenericf(("\nSharing PRS [%s]\n",dbgname));
			// Can't just ADD ref, always have to clone it

			// find index
			IndexEntry& indexEntry = *m_PRSKeysManager.GetExtra( hkeys );
			PRSKeysEntry& prsEntry = indexEntry.m_NameEntry->second;

			// Create a new set of keys, sharing the pimple
			PRSKeys* keys = new PRSKeys(hkeys->m_pImpl);

			if (!keys) return HPRSKeys();

			// create handle
			hkeys = m_PRSKeysManager.Create( keys );
			hkeys->SetDebugName(dbgname);

			// push it onto the instance list
			prsEntry.m_Inst.push_front(hkeys);

			// add to index
			IndexEntry& cloneIndexEntry = *m_PRSKeysManager.GetExtra( hkeys );
			cloneIndexEntry.m_NameEntry = indexEntry.m_NameEntry;
			cloneIndexEntry.m_ListEntry = prsEntry.m_Inst.begin();
		}

	}

	m_CreateLock.Leave();

	return ( hkeys );
	// return it (may be null on failure)

}


