#include "precomp_nema.h"

#include "Choreographer.h"		// Provides access to game animation API
#include "FileSys.h"
#include "Filter_1.h"
#include "FuBi.h"
#include "FuBiTraitsImpl.h"
#include "GoSupport.h"
#include "GpProfiler.h"
#include "NamingKey.h"
#include "Nema_Blender.h"
#include "Nema_IoStructs.h"
#include "Nema_Persist.h"
#include "RapiPrimitive.h"
#include "RatioStack.h"
#include "Services.h"
#include "Server.h"
#include "Siege_Light_Structs.h"
#include "Siege_Loader.h"
#include "SkritObject.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "GoAspect.h"		// for error reporting

#include <algorithm>


using namespace nema;

FUBI_EXPORT_ENUM( eBoneFreeze, BFREEZE_BEGIN, BFREEZE_COUNT );
FUBI_REPLACE_NAME("nemaeBoneFreeze",eBoneFreeze);

	// Spoof the namespace
FUBI_REPLACE_NAME("nemaAspect",Aspect);


#define GP_ANIMVIEWER 1

#define NEMA_DEBUGGING 0
bool NEMAFLAG_ColorizedWeights = false;


#define NEMA_MEMBLOCK_CHECKING 0

#if NEMA_MEMBLOCK_CHECKING

// NEMA_MEMBLOCK_CHECKING is true
#define PADDING_BUFFER_SIZE 16										
	
const int CONST_PADDING_BUFFER[PADDING_BUFFER_SIZE/4] = 
{
	0xEEEEEEEE,
	0xEEEEEEEE,
	0xEEEEEEEE,
	0xEEEEEEEE
};

#define PADDING_BUFFER_SETUP(a,sz)											\
if (a->m_PurgeLevel<2)														\
{																			\
	memset(a->m_memblock,0,sz+PADDING_BUFFER_SIZE);							\
	memcpy(a->m_memblock,CONST_PADDING_BUFFER,PADDING_BUFFER_SIZE);			\
};		

#define PADDING_BUFFER_CHECK(a)												\
if ((a->m_PurgeLevel<2) && (memcmp(CONST_PADDING_BUFFER,a->m_memblock,PADDING_BUFFER_SIZE) != 0))	\
{																			\
	gperror("THE PADDING BUFFER IS CORRUPT");								\
};													

#else // NEMA_MEMBLOCK_CHECKING is 0 

#define PADDING_BUFFER_SIZE 0
#define PADDING_BUFFER_SETUP(a,sz)
#define PADDING_BUFFER_CHECK(a)

#endif

static const char* s_AttachMethodStrings[] =
{
	"unattached",
	"rigid_linked",
	"reverse_rigid_linked",
	"deformable_overlay",
};


COMPILER_ASSERT( ELEMENT_COUNT( s_AttachMethodStrings ) == ATTACHMETHOD_COUNT );

static stringtool::EnumStringConverter s_AttachMethodConverter( s_AttachMethodStrings, ATTACHMETHOD_BEGIN, ATTACHMETHOD_END, UNATTACHED );

const char* nema::ToString( eAttachMethods val )
	{  return ( s_AttachMethodConverter.ToString( val ) );  }
bool nema::FromString( const char* str, eAttachMethods& val )
	{  return ( s_AttachMethodConverter.FromString( str, rcast <int&> ( val ) ) );  }

FUBI_DECLARE_ENUM_TRAITS( eAttachMethods, UNATTACHED );

static const char* s_BoneFreezeStrings[] =
{
	"bfreeze_none",
	"bfreeze_position",
	"bfreeze_rotation",
	"bfreeze_both",
	"bfreeze_rigid",
	"bfreeze_position_rigid",
	"bfreeze_rotation_rigid",
	"bfreeze_both_rigid",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_BoneFreezeStrings ) == BFREEZE_COUNT );

static stringtool::EnumStringConverter s_BoneFreezeConverter( s_BoneFreezeStrings, BFREEZE_BEGIN, BFREEZE_END, BFREEZE_NONE );

const char* nema::ToString( eBoneFreeze val )
	{  return ( s_BoneFreezeConverter.ToString( val ) );  }
bool nema::FromString( const char* str, eBoneFreeze& val )
	{  return ( s_BoneFreezeConverter.FromString( str, rcast <int&> ( val ) ) );  }

FUBI_DECLARE_ENUM_TRAITS( eBoneFreeze, BFREEZE_NONE );

FUBI_DECLARE_SELF_TRAITS( sBoneLink );

// persistence support

FUBI_DECLARE_SELF_TRAITS( AspectStorage::AspectEntry );

struct NemaShouldPersistHelper
{
	bool operator () ( HAspect haspect ) const
	{
		return ( haspect->ShouldPersist() );
	}

	bool operator () ( const AspectStorage::AspectEntry& entry ) const
	{
		return ( entry.ShouldPersist() );
	}
};

struct NemaShouldPersistFirst
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( NemaShouldPersistHelper()( obj.first ) );
	}
};

struct NemaShouldPersistSecond
{
	template <typename T>
	bool operator () ( const T& obj ) const
	{
		return ( NemaShouldPersistHelper()( obj.second ) );
	}
};

bool AspectStorage::AspectEntry :: Xfer( FuBi::PersistContext& persist )
{
	return ( persist.XferIfList( "m_Inst", m_Inst, NemaShouldPersistHelper() ) );
}

bool AspectStorage::AspectEntry :: ShouldPersist( void ) const
{
	// must be at least one persistent go in there
	HAspectList::const_iterator i, ibegin = m_Inst.begin(), iend = m_Inst.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( (*i)->ShouldPersist() )
		{
			return ( true );
		}
	}

	// none found
	return ( false );
}

FUBI_DECLARE_XFER_TRAITS( Blender* )
{
	static Aspect* ms_Owner;

	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		if ( persist.IsRestoring() )
		{
			obj = new Blender(ms_Owner);
			return ( obj->Xfer( persist ) );
		}
		else
		{
			if (obj != NULL)
			{
				return ( obj->Xfer( persist ) );
			}
			return false;
		}
	}
};

Aspect* FuBi::Traits <Blender*>::ms_Owner = NULL;

//************************************************************
#if !GP_RETAIL
void BitchAboutPurgeLevel(const Aspect* asp, bool errsev, const char* badcall)
{
	// $$$$ TEMPORARY HACK $$$$
	static bool s_Hack = true;
	if ( s_Hack )
	{
		return;
	}

	// SE does not care about performance
	if ( Services::IsEditor() )
	{
		return;
	}

	// whatever it is, if we're doing a show-all, it will spew errors so ignore 'em
	if ( gWorldOptions.GetShowAll() || gWorldOptions.GetShowGizmos() )
	{
		return;
	}

	char* errinfo;

	Go* own = GetGo(asp->GetGoid());

	const char* ownername = own ? own->GetTemplateName() : asp->GetDebugName();

	if (!own)
	{
		errinfo = "NO-GOASPECT";
	}
	else if (!own->IsInAnyWorldFrustum())
	{
		errinfo = "NOT-IN-WORLD";
	}
	else if (own->HasAspect() && !own->GetAspect()->GetIsVisible())
	{
		errinfo = "INVISIBLE";
	}
	else
	{
		errinfo = "OTHER";
	}

	if (errsev)
	{
		gperrorf(("PURGE ERROR: [%s] Tried to \"%s\" on unloaded [%s]",errinfo,badcall, ownername ));
	}
	else
	{
		gpwarningf(("PURGE WARNING: [%s] Tried to \"%s\" on unloaded [%s]",errinfo,badcall, ownername ));
	}
}

#define BITCHSLAP(b) BitchAboutPurgeLevel(this,true,b)
#define BITCHWARN(b) BitchAboutPurgeLevel(this,false,b)

#else

#define BITCHSLAP(b)
#define BITCHWARN(b)

#endif

//************************************************************
bool sBoneLink::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_childBoneIndex", m_childBoneIndex );
	persist.Xfer( "m_parentBoneIndex", m_parentBoneIndex );
	return ( true );
}

//************************************************************
bool sBone::XferPost( FuBi::PersistContext& persist )
{
	persist.Xfer( "m_frozen",   m_frozen         );
	persist.Xfer( "m_position", m_position       );
	persist.Xfer( "m_rotation", m_rotation       );
	return ( true );
}

//************************************************************
AspectStorage::AspectStorage(void)
	: m_AspectManager(10000)
{
	m_BlendVerts.resize(2000,vector_3::ZERO);
	m_BlendQuats.resize(2000,Quat::IDENTITY);
}

//************************************************************
AspectStorage::~AspectStorage(void)
{
	CommitRequests();
	m_AspectManager.FreeAllHandlesAndCompact();
}

// ***************************************************
bool AspectStorage::Xfer( FuBi::PersistContext& persist )
{
	RatioSample ratioSample( "xfer_aspect_storage", 0, 0 );

	// setup
	if ( persist.IsRestoring() )
	{
		// can't have anything allocated
		gpassert( m_AspectManager.empty() && m_AspectsByNameIndex.empty() );
	}

	// xfer index
	persist.XferIfMap( "m_AspectsByNameIndex", m_AspectsByNameIndex, NemaShouldPersistSecond() );

	// xfer object handle magics
	UINT magic = HAspect::GetCurrentMagic();
	persist.Xfer( "m_AspectManagerMagic", magic );
	HAspect::SetCurrentMagic( magic );

	// rebuild object map
	persist.EnterBlock( "m_AspectManager" );
	{
		StringToAspectMap::iterator i, ibegin = m_AspectsByNameIndex.begin(), iend = m_AspectsByNameIndex.end();

		if ( persist.IsSaving() )
		{
			RatioSample ratioSample( "xfer_aspects", m_AspectManager.size(), 0.95 );

			// simple - just save all persistent aspects out
			AspectMgr::iterator i, begin = m_AspectManager.begin(), end = m_AspectManager.end();
			for ( i = begin ; i != end ; ++i )
			{
				AspectMgr::Entry& data = i.GetRawData();
				HAspect haspect( i.GetIndex(), data.m_Magic );
				Aspect* aspect = rcast <Aspect*> ( data.m_Object );
				if ( aspect->ShouldPersist() )
				{
					persist.EnterBlock( ::ToString( haspect ) );
					persist.Xfer( "_refs", data.m_RefCount );
					aspect->Xfer( persist );
					persist.LeaveBlock();
				}

				ratioSample.Advance( GPDEV_ONLY( GetAspectName( haspect ) ) );
			}
		}
		else
		{
			// have to rebuild aspect coll from index
			{
				RatioSample ratioSample( "xfer_aspects", m_AspectsByNameIndex.size(), 0.13 );

				for ( i = ibegin ; i != iend ; ++i )
				{
					// build basic entry
					IndexEntry indexEntry;
					indexEntry.m_NameEntry = i;

					// iter through all instances
					HAspectList::iterator j, jbegin = i->second.m_Inst.begin(), jend = i->second.m_Inst.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						// get the handle
						HAspect handle = *j;

						// new or shared?
						Aspect* aspect = NULL;
						if ( i->second.m_Impl == NULL )
						{
							// open file
							FileSys::AutoFileHandle file;
							FileSys::AutoMemHandle mem;
							if ( file.Open( i->first ) && mem.Map( file ) )
							{
								// create new
								aspect = new Aspect( const_mem_ptr( mem.GetData(), mem.GetSize() ) );
								i->second.m_Impl = aspect->m_shared;
							}
							else
							{
								// $$$ IMPLEMENT THE FAILURE CASE - switch to default aspect and don't xfer it
								gpassert( 0 );
							}
						}
						else
						{
							// addref on shared
							aspect = new Aspect( i->second.m_Impl );
						}

						// xfer it
						WORD refCount = 0;
						persist.EnterBlock( ::ToString( handle ) );
						persist.Xfer( "_refs", refCount );
						persist.LeaveBlock();

						// add to db
						m_AspectManager.CreateSpecific( handle.m_Index, handle.m_Magic, aspect );
						indexEntry.m_ListEntry = j;
						*m_AspectManager.GetExtra( handle ) = indexEntry;
						m_AspectManager.GetRawData( handle ).m_RefCount = refCount;
					}

					ratioSample.Advance( i->first );
				}
			}

			// now xfer all the aspects
			{
				RatioSample ratioSample( "post_xfer_aspects", m_AspectsByNameIndex.size(), 0.78 );
			
				for ( i = ibegin ; i != iend ; ++i )
				{
					// iter through all instances
					HAspectList::iterator j, jbegin = i->second.m_Inst.begin(), jend = i->second.m_Inst.end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						persist.EnterBlock( ::ToString( *j ) );
						(*j)->Xfer( persist );
						persist.LeaveBlock();
					}

					ratioSample.Advance( i->first );
				}
			}
		}

		// post-xfer all the aspects
		{
			RatioSample ratioSample( "post_xfer_aspects" );

			for ( i = ibegin ; i != iend ; ++i )
			{
				HAspectList::iterator j, jbegin = i->second.m_Inst.begin(), jend = i->second.m_Inst.end();

				if ( persist.IsRestoring() )
				{
					// When Aspects are initially created (by a call to LoadAspect(), they are
					// in the 'purged' state. While no purged data is ever preserved the purged
					// state itself is saved.
					//
					// We need to iter through all instances, reconstituting when the
					// current purge state indicates that the data WAS in memory when the save
					// was made.

					// We only need to call reconstitute on the root of each hierarchy. All
					// attached children will be called recursively

					for ( j = jbegin ; j != jend ; ++j )
					{
						// If the aspect was not COMPLETELY purged when it was saved out then force
						// a restore by sending in the level as a negative value
						if ( !(*j)->IsPurged(2) && ((*j)->GetAttachType() == UNATTACHED))
						{
							(*j)->PrivateReconstitute(~(*j)->m_PurgeLevel,false);
						}
					}
				}

				// iter through all instances, persisting any custom bone positions
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( persist.IsRestoring() || (*j)->ShouldPersist() )
					{
						persist.EnterBlock( ::ToString( *j ) );
						(*j)->XferPost( persist );
						persist.LeaveBlock();
					}
				}
			}
		}

		// finally, force all aspects to startup positions
		if ( persist.IsRestoring() )
		{
			for ( i = ibegin ; i != iend ; ++i )
			{
				// iter through all instances
				HAspectList::iterator j, jbegin = i->second.m_Inst.begin(), jend = i->second.m_Inst.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					// Do we need to bend the bones?
					// Don't handle bending bones with blenders yet
					// We need to load the GoBody before we know what the chore dictionary is
					// so the blender isn't quite ready yet.
					// Aspects with blenders get deformed in the blender->XferPost which happens
					// later in the restoration

					if (!(*j)->IsPurged(2) && !(*j)->HasBlender())
					{
						bool oldEnabled = (*j)->IsUpdateEnabled();

						(*j)->ForceDeformation();
						(*j)->EnableUpdate();

						if (((*j)->GetAttachType() == UNATTACHED))
						{
							// Assume we have to morph the skin (safe to call on rigid meshes)
							(*j)->Deform( 0, 0 );

						}

						if (!oldEnabled )
						{
							(*j)->DisableUpdate();
						}
					}
				}
			}
		}
	}
	persist.LeaveBlock();

	return ( true );
}

// ***************************************************
bool AspectStorage::ValidateParentLinks( bool fix_errors )
{
	StringToAspectMap::iterator i, ibegin = m_AspectsByNameIndex.begin(), iend = m_AspectsByNameIndex.end();

	for ( i = ibegin ; i != iend ; ++i )
	{
		// iter through all instances
		HAspectList::iterator j, jbegin = i->second.m_Inst.begin(), jend = i->second.m_Inst.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			(*j)->ValidateParentLinks( fix_errors );
		}
	}

	return ( true );
}

// ***************************************************
int AspectStorage::AddRefAspect ( HAspect haspect )
{
	kerneltool::Critical::Lock locker( m_CreateLock );
	return m_AspectManager.AddRef(haspect);
}

// ***************************************************
int AspectStorage::ReleaseAspect( HAspect haspect, bool clearOwner )
{

	kerneltool::Critical::Lock locker( m_CreateLock );

	gpassertf(haspect.IsValid(),("Attempting to call ReleaseAspect() on an invalid handle"));

	if ( !haspect.IsValid() )
	{
		return 0;
	}

	if ( clearOwner )
	{
		haspect->ClearGoid();
	}

	// All releases are now queued so that they can be done on the the primary thread
	m_QueuedForRelease.push_back( haspect );

	return m_AspectManager.GetRefCount( haspect );
}

// ***************************************************
int AspectStorage::RefCountAspect( HAspect haspect )
{
	kerneltool::Critical::Lock locker( m_CreateLock );
	return m_AspectManager.GetRefCount(haspect);
}


//************************************************************
gpstring AspectStorage::GetAspectName( HAspect haspect ) const
{
	kerneltool::Critical::Lock locker( m_CreateLock );
	const IndexEntry* entry = m_AspectManager.GetExtra( haspect );

	gpassertf(entry != NULL,("AspectStorage::GetAspectName() the aspect handle is not in the AspectStorage"));
	if (entry != NULL)
	{
		return ( entry->m_NameEntry->first );
	}

	return "<aspect not found>";

}

#if !GP_RETAIL
gpstring FileNameOfLastLoadAspect;
#endif

//************************************************************
HAspect AspectStorage::LoadAspect(char const * const fname, char const * const dbgname) {

	gpassert( (fname != NULL) && (*fname != '\0') );

	// serialize
	m_CreateLock.Enter();

	// get some vars
	HAspect haspect;
	Aspect* aspect = NULL;

	// see if already there - don't use the plain insert() so we can avoid the
	// temporary construction of a gpstring() on instanced data
	StringToAspectMap::iterator mapEntry = m_AspectsByNameIndex.find( fname );
	if ( mapEntry == m_AspectsByNameIndex.end() )
	{
		// insert into map
		mapEntry = m_AspectsByNameIndex.insert( std::make_pair( fname, AspectEntry() ) ).first;
		mapEntry->second.m_UseCount++;
		m_CreateLock.Leave();

		// attempt to open and read aspect
		FileSys::FileHandle file;
		FileSys::MemHandle mem;

#if !GP_RETAIL
		FileNameOfLastLoadAspect = fname;
#endif

		if ( file.Open( fname ) )
		{
			if ( mem.Map( file ) )
			{

				__try
				{
					// construct and parse
					aspect = new Aspect( const_mem_ptr( mem.GetData(), mem.GetSize() ) );
				}
				__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH, EXCEPTION_EXECUTE_HANDLER ) )
				{
					gperrorf(( "Something has corrupted [%s] If you choose to continue, expect to see a lot of warnings and graphical glitches", fname ));
					Delete(aspect);
				}

				mem.Close();

			}
			file.Close();
		}

#if !GP_RETAIL
		FileNameOfLastLoadAspect = "";
#endif

		AspectEntry& aspEntry = mapEntry->second;
		if ( aspect && aspect->m_shared != NULL )
		{
			aspEntry.m_Impl = aspect->m_shared;
		}
		else
		{
			// failed, delete owner
			Delete( aspect );
			aspEntry.m_Impl = (nema::AspectImpl*)0xFFFFFFFF;
		}
	}
	else if( mapEntry->second.m_Impl == (nema::AspectImpl*)0xFFFFFFFF )
	{
		// failed to load
		m_CreateLock.Leave();
		return haspect;
	}
	else if( mapEntry->second.m_Impl == NULL )
	{
		// middle of loading, must spin
		mapEntry->second.m_UseCount++;
		m_CreateLock.Leave();

		// spin and wait
		while( mapEntry->second.m_Impl == NULL )
		{
			Sleep( 1 );
		}

		// check for failure
		if( mapEntry->second.m_Impl == (nema::AspectImpl*)0xFFFFFFFF )
		{
			m_CreateLock.Enter();
			mapEntry->second.m_UseCount--;
			m_CreateLock.Leave();
			return haspect;
		}
		else
		{
			// for the moment I am always going to make a copy with a shared pimpl --biddle
			aspect = new Aspect( mapEntry->second.m_Impl );
		}
	}
	else
	{
		mapEntry->second.m_UseCount++;
		m_CreateLock.Leave();

		// for the moment I am always going to make a copy with a shared pimpl --biddle
		aspect = new Aspect( mapEntry->second.m_Impl );
	}

	m_CreateLock.Enter();

	// if have a valid aspect to insert
	if ( aspect != NULL )
	{
		// create handle for it
		haspect = m_AspectManager.Create( aspect );

		// init the new aspect with our new info
		aspect->SetDebugName( dbgname );
		aspect->SetGoid( GOID_INVALID );
		aspect->SetHandle( haspect );

		// give handle extra data reverse reference to index
		AspectEntry& aspEntry = mapEntry->second;
		IndexEntry& indEntry = *m_AspectManager.GetExtra( haspect );
		indEntry.m_NameEntry = mapEntry;
		indEntry.m_ListEntry = aspEntry.m_Inst.insert( aspEntry.m_Inst.begin(), haspect );
	}
	mapEntry->second.m_UseCount--;

	m_CreateLock.Leave();

	// return it (may be null on failure)
	return ( haspect );
}

// ***************************************************
void AspectStorage::CommitRequests( void )
{

	// Process the release queue

	if (m_QueuedForRelease.empty())
	{
		return;
	}
	
	HAspectList toRelease;
	HAspectList::iterator it;

	kerneltool::Critical::Lock locker( m_CreateLock );

	for (it = m_QueuedForRelease.begin(); it != m_QueuedForRelease.end(); ++it)
	{
		if ( m_AspectManager.GetRefCount( *it ) == 1 )
		{

			// it's going to be deleted - remove index entries

			IndexEntry& indEntry = *m_AspectManager.GetExtra( *it );
			AspectEntry& aspEntry = indEntry.m_NameEntry->second;
			aspEntry.m_Inst.erase(indEntry.m_ListEntry);

			// if it's the last one, remove name entry
			if ( aspEntry.m_Inst.empty() && !aspEntry.m_UseCount )
			{
				m_AspectsByNameIndex.erase( indEntry.m_NameEntry );
			}
		}

		// release the handle
		m_AspectManager.Release( *it );
	}

	m_QueuedForRelease.clear();

}

//************************************************************
AspectImpl::AspectImpl(void)
	: m_RefCount(0)
	, m_nbones(0)
	, m_restbones(0)
	, m_nmeshes(0)
	, m_maxverts(0)
	, m_nverts(0)
	, m_ncorners(0)
	, m_ntriangles(0)
	, m_verts(0)
	, m_triangles(0)
	, m_vertmappings(0)
	, m_stringtable(0)
	, m_ntextures(0)
	, m_submeshmatcount(0)
	, m_submeshmatlookup(0)
	, m_submeshfacecount(0)
	, m_submeshcorncount(0)
	, m_submeshstartindex(0)
	, m_primarybone(0)
{
}

//************************************************************
AspectImpl::~AspectImpl(void) {

	{for (DWORD m = 0; m < m_nmeshes; m++ ) {
		delete [] m_submeshmatlookup[m];	m_submeshmatlookup[m]	= 0;
		delete [] m_submeshfacecount[m];	m_submeshfacecount[m]	= 0;
		delete [] m_submeshcorncount[m];	m_submeshcorncount[m]	= 0;
		delete [] m_submeshstartindex[m];	m_submeshstartindex[m]	= 0;
		delete [] m_verts[m];				m_verts[m]				= 0;
		delete [] m_triangles[m];			m_triangles[m]			= 0;
		delete [] m_vertmappings[m];		m_vertmappings[m]		= 0;
		delete [] m_wcorners[m];			m_wcorners[m]			= 0;
		{for (DWORD s = 0; s < m_nstitchsets[m]; s++ ) {
			delete [] m_stitchset[m][s].m_verts; m_stitchset[m][s].m_verts = 0;
		}}
		delete [] m_stitchset[m] ; m_stitchset[m] = 0;
	}}

	delete [] m_stringtable;		m_stringtable		= 0;

	delete [] m_submeshfacecount;	m_submeshfacecount  = 0;
	delete [] m_submeshcorncount;	m_submeshcorncount  = 0;
	delete [] m_submeshstartindex;	m_submeshstartindex  = 0;
	delete [] m_submeshmatcount;	m_submeshmatcount	= 0;
	delete [] m_submeshmatlookup;	m_submeshmatlookup	= 0;

	delete [] m_stitchset;		m_stitchset = 0;
	delete [] m_nstitchsets;	m_nstitchsets = 0;

	delete [] m_vertmappings;	m_vertmappings		= 0;
	delete [] m_verts;			m_verts				= 0;
	delete [] m_triangles;		m_triangles			= 0;

	delete [] m_nverts;			m_nverts			= 0;
	delete [] m_ncorners;		m_ncorners			= 0;
	delete [] m_ntriangles;		m_ntriangles		= 0;

	delete [] m_wcorners;		m_wcorners			= 0;

	{for (DWORD b = 0; b < m_nbones; b++ ) {

		{for (DWORD m = 0; m < m_nmeshes; m++ ) {
			delete m_restbones[b].m_vertWeights[m];	m_restbones[b].m_vertWeights[m] = 0;
		}}

		delete m_restbones[b].m_vertWeights;		m_restbones[b].m_vertWeights = 0;
		delete m_restbones[b].m_numberOfPairs;		m_restbones[b].m_numberOfPairs = 0;

	}}

	delete [] m_restbones;				m_restbones	= 0;
}


//************************************************************
Aspect::~Aspect(void) {


	//$$$$$$$$$ Something isn't right, added this detach from parent Nov 2 --biddle
	if (m_parent)
	{
		m_parent->DetachChild(m_handle);
	}
	//$$$$$$$$$$$$$$$$$$

	DetachAllChildren();

	for (DWORD t = 0; t < m_shared->m_ntextures; t++ )
	{
		PrivateDestroyTexture(t);
	}

	delete [] m_currenttextures; m_currenttextures = 0;
	delete [] m_textureloadids; m_textureloadids = 0;

	if (!IsPurged(2))
	{

		PADDING_BUFFER_CHECK(this)

#if GP_DEBUG
//		gpwarningf(("Purging %8d\n", m_memsize));
#endif
		gpassert(m_memblock		&& !IsMemoryBadFood(*(DWORD*)m_memblock)	);
		gpassert(m_corners		&& !IsMemoryBadFood(*(DWORD*)m_corners)		);
		gpassert(m_cornercolors && !IsMemoryBadFood(*(DWORD*)m_cornercolors));
		gpassert(m_normals		&& !IsMemoryBadFood(*(DWORD*)m_normals)		);
		gpassert(m_bones		&& !IsMemoryBadFood(*(DWORD*)m_bones)		);
		
		delete [] m_memblock; m_memblock = 0;

		m_corners		= 0;
		m_cornercolors	= 0;
		m_normals		= 0;
		m_bones			= 0;

		m_PurgeLevel = 2;
	}

	delete m_Blender;			m_Blender = 0;

	if (m_shared->Release() == 0) {
		delete m_shared;
		m_shared = 0;
	}

}

//************************************************************
Aspect::Aspect(const_mem_ptr data) {

	// Make sure we decent data loaded first
	// then we can create the aspect

	const char* runner = (const char *)data.mem;

	if (!runner)
	{
		gperror( "NEMA_ERROR:File is corrupt\n" );
		m_shared = NULL;
		return ;
	}

	// Parse the mesh
	const sNeMaMesh_Chunk *header = (const sNeMaMesh_Chunk *)runner;
	runner += sizeof(sNeMaMesh_Chunk);

	if (header->ChunkName != IDMeshHeader) {
		gperrorf(( "NEMA_ERROR:Not an Aspect file [%s]\n", FileNameOfLastLoadAspect.c_str() ));
		return ;
	}

	if (header->MajorVersion < 1)
	{
		gpwarningf(("NEMA_WARNING:Unable to load [%s] version %d.%d is too old\n",FileNameOfLastLoadAspect.c_str(),header->MajorVersion,header->MinorVersion));
		return ;
	}

	m_shared = new AspectImpl;

	if (!m_shared)
	{
		gperror("Failed to allocated an aspect implementation");
		return;
	}

	m_shared->m_MajorVersion = header->MajorVersion;
	m_shared->m_MinorVersion = header->MinorVersion;

	if ((m_shared->m_MajorVersion < 1) || (m_shared->m_MajorVersion == 1 && m_shared->m_MinorVersion < 3))
	{
		gpwarningf(("Out-of-date ASP detected [%s], please have this file updated",FileNameOfLastLoadAspect.c_str()));
	}

	// Make sure that the offsets are identity
	m_linkOffsetRotation = Quat::IDENTITY;			// $$ could initialize to the NEMAtoSEIGE fixup rotation
	m_linkOffsetPosition = vector_3::ZERO;

	m_attachtype = UNATTACHED;
	m_linkBone = 0;
	m_parent = HAspect();


	// Here we assume that the mesh is deformable (hence is has no primary bone)
	// TODO this primary bone stuff is a bit of a hack, should re-evaluate
	// how I am differentiating between the various mesh types
	m_shared->m_primarybone = -1;

	m_instflags = NULL;

	m_shared->m_flags = header->MeshAttrFlags;
	m_shared->m_nmeshes = header->NumberOfSubMeshes;
	m_shared->m_maxverts = header->MaximumVerts;

	m_shared->m_stringtable = new char[header->StringTableSize];

	// Only aspects with weights need blended verts
	// We allocate space (if needed) after parsing the weights
	m_shared->m_nbones= header->NumberOfBones;
	m_shared->m_restbones = new sBoneImpl[header->NumberOfBones];

	m_memblock = 0;

	m_bones = 0;

	{for (DWORD i=0; i < m_shared->m_nbones;i++) {
		m_shared->m_restbones[i].m_vertWeights = new sBoneVertWeightPair*[header->NumberOfSubMeshes];
		m_shared->m_restbones[i].m_numberOfPairs = new DWORD[header->NumberOfSubMeshes];
	}}

	m_shared->m_nverts = new DWORD[m_shared->m_nmeshes];
	m_shared->m_verts = new sVert*[m_shared->m_nmeshes];

	m_shared->m_ncorners = new DWORD[m_shared->m_nmeshes];

	m_corners = 0;
	m_cornercolors = 0;
	m_normals = 0;

	m_shared->m_wcorners = new sWeightedCorner*[m_shared->m_nmeshes];

	m_shared->m_ntriangles = new DWORD[m_shared->m_nmeshes];
	m_shared->m_triangles  = new sTri*[m_shared->m_nmeshes];

	m_shared->m_vertmappings = new sVertMap*[m_shared->m_nmeshes];

	m_shared->m_ntextures = header->MaximumMaterials;

	m_currenttextures = new DWORD[header->MaximumMaterials];
	::ZeroMemory( m_currenttextures, sizeof( DWORD ) * header->MaximumMaterials );
	m_textureloadids = new siege::LoadOrderId[header->MaximumMaterials];
	::ZeroMemory( m_textureloadids, sizeof( siege::LoadOrderId ) * header->MaximumMaterials );

	m_shared->m_submeshmatcount		= new DWORD[header->NumberOfSubMeshes];
	m_shared->m_submeshmatlookup	= new DWORD*[header->NumberOfSubMeshes];
	m_shared->m_submeshfacecount	= new DWORD*[header->NumberOfSubMeshes];
	m_shared->m_submeshcorncount	= new DWORD*[header->NumberOfSubMeshes];
	m_shared->m_submeshstartindex	= new DWORD*[header->NumberOfSubMeshes];

	m_shared->m_nstitchsets	= new DWORD[header->NumberOfSubMeshes];
	m_shared->m_stitchset	= new sStitchSet*[header->NumberOfSubMeshes];

	// **************** Read in the STRING data

	memcpy(m_shared->m_stringtable,runner,header->StringTableSize);
	runner += header->StringTableSize;

	// **************** Read in the BONE hierarchy info
	const sBoneHeader_Chunk *tmpBoneHeader = (const sBoneHeader_Chunk*)runner;
	runner += sizeof(sBoneHeader_Chunk);

#if !GP_RETAIL
	if( tmpBoneHeader->ChunkName != IDBoneHeader )
	{
		gperror( "tmpBoneHeader->ChunkName != IDBoneHeader" );
	}
#endif

	// Assume we don't won't find a head bone
	m_shared->m_HeadBoneIndex = 0;

	{for (DWORD i=0; i < header->NumberOfBones;i++) {

		const sBoneInfo_Chunk *tmpBoneInfo = (const sBoneInfo_Chunk*)runner;
		runner += sizeof(sBoneInfo_Chunk);

		if ((i == 0) || (int)tmpBoneInfo->BoneIndex < 1) {

			m_shared->m_restbones[i].m_boneIndex =	0;
			m_shared->m_restbones[i].m_parentBoneIndex =	0;

		}
		else
		{
#if !GP_RETAIL
			if( (int)tmpBoneInfo->BoneIndex <= 0 )
			{
				gperrorf(("zero/negative bone index at position %d!",i ));
			}

			if( (int)tmpBoneInfo->ParentBoneIndex < 0 )
			{
				gperrorf(("zero/negative parent bone index at position %d!",i ));
			}
#endif
			m_shared->m_restbones[i].m_boneIndex		=	tmpBoneInfo->BoneIndex;
			m_shared->m_restbones[i].m_parentBoneIndex	=	tmpBoneInfo->ParentBoneIndex;

		}

		m_shared->m_restbones[i].m_name =	&(m_shared->m_stringtable[tmpBoneInfo->NameOffset]);

		if (! stricmp(m_shared->m_restbones[i].m_name,"Bip01_Head")) {
			m_shared->m_HeadBoneIndex = i;
		}


	}}

	//*************************************************
	//********** Begin SUB MESH loop ******************
	//*************************************************


	DWORD current_submesh = 0;

	for (current_submesh = 0 ; current_submesh < m_shared->m_nmeshes; current_submesh++) {

		// **************** Read in the sub texture sets (if there are any)


		const sNeMaSubMesh_Chunk *tmpSubMeshChunk = (const sNeMaSubMesh_Chunk*)runner;
		runner += sizeof(sNeMaSubMesh_Chunk);

#if	!GP_RETAIL
		if( tmpSubMeshChunk->ChunkName != IDSubMeshHeader )
		{
			gperror( "tmpSubMeshChunk->ChunkName != IDSubMeshHeader" );
		}
#endif
		const sSubMeshMaterial_Chunk *tmpSubMeshMatChunk = (const sSubMeshMaterial_Chunk*)runner;
		runner += sizeof(sSubMeshMaterial_Chunk);

#if !GP_RETAIL
		if( tmpSubMeshMatChunk->ChunkName != IDSubMeshMaterial )
		{
			gperror( "tmpSubMeshMatChunk->ChunkName != IDSubMeshMaterial" );
		}
		if( m_shared->m_ntextures < tmpSubMeshMatChunk->NumberOfMaterials )
		{
			gperror( "m_shared->m_ntextures >= tmpSubMeshMatChunk->NumberOfMaterials" );
		}
#endif
		m_shared->m_submeshmatcount[current_submesh] = tmpSubMeshMatChunk->NumberOfMaterials;
		m_shared->m_submeshmatlookup[current_submesh] = new DWORD[tmpSubMeshMatChunk->NumberOfMaterials];
		m_shared->m_submeshfacecount[current_submesh] = new DWORD[tmpSubMeshMatChunk->NumberOfMaterials];
		m_shared->m_submeshcorncount[current_submesh] = new DWORD[tmpSubMeshMatChunk->NumberOfMaterials];
		m_shared->m_submeshstartindex[current_submesh] = new DWORD[tmpSubMeshMatChunk->NumberOfMaterials];

		m_shared->m_ntriangles[current_submesh] = 0;

		for (DWORD i=0; i < tmpSubMeshMatChunk->NumberOfMaterials; i++) {

			const sSubMeshMaterialData *tmpSubMeshMaterialData = (const sSubMeshMaterialData*)runner;
			runner += sizeof(sSubMeshMaterialData);

			m_shared->m_submeshmatlookup[current_submesh][i] = tmpSubMeshMaterialData->MaterialID;
			m_shared->m_submeshfacecount[current_submesh][i] = tmpSubMeshMaterialData->NumberOfFaces;
			// Assume all mesh indices start at 0, may be revised when triangle info is loaded -- biddle
			m_shared->m_submeshstartindex[current_submesh][i] = 0;
			m_shared->m_submeshcorncount[current_submesh][i] = 0;

			m_shared->m_ntriangles[current_submesh] += tmpSubMeshMaterialData->NumberOfFaces;

		}

		// **************** Read in the VERTEX data

		const sVertList_Chunk *tmpVertList = (const sVertList_Chunk*)runner;
		runner += sizeof(sVertList_Chunk);

#if	!GP_RETAIL
		if( tmpVertList->ChunkName != IDVertList )
		{
			gperror( "tmpVertList->ChunkName != IDVertList" );
		}

		if( tmpVertList->NumberOfVerts > header->MaximumVerts )
		{
			gperror( "tmpVertList->NumberOfVerts > header->MaximumVerts" );
		}
#endif

		m_shared->m_nverts[current_submesh] = tmpVertList->NumberOfVerts;
		m_shared->m_verts[current_submesh] = new sVert[tmpVertList->NumberOfVerts];

		DWORD size = sizeof(sVert) * tmpVertList->NumberOfVerts;
		memcpy(&m_shared->m_verts[current_submesh][0],runner,size);
		runner += size;

		if ((m_shared->m_MajorVersion < 1) || (m_shared->m_MajorVersion == 1 && m_shared->m_MinorVersion < 3)) 
		{
			// **************** Read in the obsolete CORNER data
			const sCornerList_Chunk *tmpCornList = (const sCornerList_Chunk*)runner;
			runner += sizeof(sCornerList_Chunk);

#if !GP_RETAIL
			if( tmpCornList->ChunkName != IDCornerList )
			{
				gperror( "tmpCornList->ChunkName != IDCornerList" );
			}
#endif
			m_shared->m_ncorners[current_submesh] = tmpCornList->NumberOfCorners;
			m_shared->m_wcorners[current_submesh] = new sWeightedCorner[tmpCornList->NumberOfCorners];

			for (DWORD i=0; i < tmpCornList->NumberOfCorners;i++)
			{
				const sCorner_Chunk *tmpCorn = (const sCorner_Chunk*)runner;
				runner += sizeof(sCorner_Chunk);

				// This is an old ASP file written WITHOUT weighted corners
				// we need to copy the data from the plain corner instead
				if ((m_shared->m_MajorVersion < 1) || (m_shared->m_MajorVersion == 1 && m_shared->m_MinorVersion < 3)) {

					m_shared->m_wcorners[current_submesh][i].v = m_shared->m_verts[current_submesh][tmpCorn->i];

					// Must rotate the normals for YisUP
					m_shared->m_wcorners[current_submesh][i].n = (vector_3(tmpCorn->nx,tmpCorn->ny,tmpCorn->nz));

					m_shared->m_wcorners[current_submesh][i].color	= tmpCorn->color;
					m_shared->m_wcorners[current_submesh][i].uv.u	= tmpCorn->uv.u;
					m_shared->m_wcorners[current_submesh][i].uv.v	= tmpCorn->uv.v;
					if ((m_shared->m_wcorners[current_submesh][i].uv.u < 0.0f) || (m_shared->m_wcorners[current_submesh][i].uv.u > 1.0f)) {
						m_shared->m_flags |= TEXTURE_UWRAP;
					}
					if ((m_shared->m_wcorners[current_submesh][i].uv.v < 0.0f) || (m_shared->m_wcorners[current_submesh][i].uv.v > 1.0f)) {
						m_shared->m_flags |= TEXTURE_VWRAP;
					}
				}
			}
		}	
		else
		{
			if (m_shared->m_MajorVersion < 4)
			{
				// Skip the corner data altogether, everything is in the WCRN data
				// Version 4.x and higher does not contain any BCRN data at all

				const sCornerList_Chunk *tmpCornList = (const sCornerList_Chunk*)runner;
				runner += sizeof(sCornerList_Chunk);

				runner += sizeof(sCorner_Chunk) * tmpCornList->NumberOfCorners;

			}

			// **************** Read in the WEIGHTED CORNER data
			const sWeightedCornerList_Chunk *tmpWCornList = (const sWeightedCornerList_Chunk*)runner;

#if !GP_RETAIL
			if( tmpWCornList->ChunkName != IDWCornerList )
			{
				gperror( "tmpWCornList->ChunkName != IDWCornerList" );
			}
#endif
			runner += sizeof(sWeightedCornerList_Chunk);

#if !GP_RETAIL
			if( !tmpWCornList->NumberOfCorners )
			{
				gpwarning( "No weighted corners found while parsing mesh data" );
			}
#endif

			m_shared->m_ncorners[current_submesh] = tmpWCornList->NumberOfCorners;
			m_shared->m_wcorners[current_submesh] = new sWeightedCorner[tmpWCornList->NumberOfCorners];

			// Clear the vertex alpha flag
			m_shared->m_flags &= ~VERTEX_ALPHA;

			{for (DWORD i=0; i < tmpWCornList->NumberOfCorners;i++) {

				const sWeightedCorner_Chunk *tmpWCorn = (const sWeightedCorner_Chunk*)runner;
				runner += sizeof(sWeightedCorner_Chunk);

				m_shared->m_wcorners[current_submesh][i].v			= tmpWCorn->v;

				// Get the bone weights
				m_shared->m_wcorners[current_submesh][i].b0			= tmpWCorn->b0;
				m_shared->m_wcorners[current_submesh][i].b1			= tmpWCorn->b1;
				m_shared->m_wcorners[current_submesh][i].b2			= tmpWCorn->b2;
				m_shared->m_wcorners[current_submesh][i].b3			= tmpWCorn->b3;

				// Bone indices in packed format
				
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				// $$$$  
				// $$$$ WARNING: ALL ASP VERSIONS PRIOR TO 4.0
				// $$$$  
				// $$$$ I incorrectly added 1 to each Bone index
				// $$$$ 
				// $$$$ As of DS1.1, we are not using these values so
				// $$$$ I am leaving them uncorrected!
				// $$$$  
				m_shared->m_wcorners[current_submesh][i].indices	= tmpWCorn->indices;
				// $$$$
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
				// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

				// Must rotate the normals for YisUP
				//m_shared->m_wcorners[current_submesh][i].n = tmpWCorn->n;
				m_shared->m_wcorners[current_submesh][i].n = (tmpWCorn->n);

				// Colour and UVees
				m_shared->m_wcorners[current_submesh][i].color		= tmpWCorn->color;

				if ((tmpWCorn->color & 0xff000000) != 0xff000000) {
					m_shared->m_flags |= VERTEX_ALPHA;
				}

				m_shared->m_wcorners[current_submesh][i].uv.u	= tmpWCorn->uv.u;
				m_shared->m_wcorners[current_submesh][i].uv.v	= tmpWCorn->uv.v;

				if ((m_shared->m_wcorners[current_submesh][i].uv.u < 0.0f) || (m_shared->m_wcorners[current_submesh][i].uv.u > 1.0f)) {
					m_shared->m_flags |= TEXTURE_UWRAP;
				}
				if ((m_shared->m_wcorners[current_submesh][i].uv.v < 0.0f) || (m_shared->m_wcorners[current_submesh][i].uv.v > 1.0f)) {
					m_shared->m_flags |= TEXTURE_VWRAP;
				}

			}}

		}

		// **************** Read in the VERTEX MAPPING data

		const sVertMapData_Chunk *tmpVertMapData = (const sVertMapData_Chunk*)runner;
		runner += sizeof(sVertMapData_Chunk);

#if !GP_RETAIL
		if( tmpVertMapData->ChunkName != IDVertMap )
		{
			gperror( "tmpVertMapData->ChunkName != IDVertMap" );
		}
#endif
		m_shared->m_vertmappings[current_submesh] = new DWORD[m_shared->m_nverts[current_submesh]+m_shared->m_ncorners[current_submesh]];

		sVertMap *map_runner = m_shared->m_vertmappings[current_submesh];
		{for (DWORD i=0; i < m_shared->m_nverts[current_submesh]; i++) {

			const sVertMappingSize_Chunk *tmpSize = (const sVertMappingSize_Chunk*)runner;
			runner += sizeof(sVertMappingSize_Chunk);

			*map_runner++ = (sVertMap)(*tmpSize);

			for (DWORD j=0; j < *tmpSize; j++) {

				const sVertMappingIndex_Chunk *tmpIndex = (const sVertMappingIndex_Chunk*)runner;
				runner += sizeof(sVertMappingIndex_Chunk);

				*map_runner++ = (sVertMap)(*tmpIndex);
			}

		}}

		// **************** Read in the TRIANGLE data

		const sTriangleList_Chunk *tmpTriList = (const sTriangleList_Chunk*)runner;
		runner += sizeof(sTriangleList_Chunk);

#if !GP_RETAIL
		if( tmpTriList->ChunkName != IDTriangleList )
		{
			gperror( "tmpTriList->ChunkName != IDTriangleList" );
		}
#endif

		m_shared->m_triangles[current_submesh] = new sTri[tmpTriList->NumberOfTriangles];

		if ((m_shared->m_MajorVersion > 2) || (m_shared->m_MajorVersion == 2 && m_shared->m_MinorVersion >= 2))
		{
			// Read in the starting indices
			for (DWORD i = 0; i < m_shared->m_submeshmatcount[current_submesh]; ++i)
			{
				m_shared->m_submeshstartindex[current_submesh][i] = *(DWORD*)runner;
				runner += sizeof(DWORD);
				if ((m_shared->m_MajorVersion > 2) || (m_shared->m_MajorVersion == 2 && m_shared->m_MinorVersion >= 3))
				{
					m_shared->m_submeshcorncount[current_submesh][i] = *(DWORD*)runner;
					runner += sizeof(DWORD);
				}
			}
		}

		// $$$ Export the corner count too
		DWORD cc = 0;
		DWORD fc = 0;
		DWORD mat = 0;

		{for (DWORD i=0; i < tmpTriList->NumberOfTriangles;i++) {

			const sTriangle_Chunk *tmpTri = (const sTriangle_Chunk*)runner;
			runner += sizeof(sTriangle_Chunk);

			m_shared->m_triangles[current_submesh][i].CornerAIndex	= (WORD)tmpTri->CornerAIndex;
			m_shared->m_triangles[current_submesh][i].CornerBIndex	= (WORD)tmpTri->CornerBIndex;
			m_shared->m_triangles[current_submesh][i].CornerCIndex	= (WORD)tmpTri->CornerCIndex;

			if ((m_shared->m_MajorVersion < 2) || (m_shared->m_MajorVersion == 2 && m_shared->m_MinorVersion < 3))
			{
				while ( fc == m_shared->m_submeshfacecount[current_submesh][mat] )
				{
					m_shared->m_submeshcorncount[current_submesh][mat] = cc+1;
					mat++;
					cc = 0;
					fc = 0;
				}
				fc++;

				cc = max_t(cc,tmpTri->CornerAIndex);
				cc = max_t(cc,tmpTri->CornerBIndex);
				cc = max_t(cc,tmpTri->CornerCIndex);

			}
		}}

		if ((m_shared->m_MajorVersion < 2) || (m_shared->m_MajorVersion == 2 && m_shared->m_MinorVersion < 3))
		{
			m_shared->m_submeshcorncount[current_submesh][mat] = cc+1;
		}

		// **************** Read in the BONE vertex weight data

		const sBoneVertData_Chunk *tmpVertBoneData = (const sBoneVertData_Chunk*)runner;
		runner += sizeof(sBoneVertData_Chunk);

#if !GP_RETAIL
		if( tmpVertBoneData->ChunkName != IDBoneVertWeightList )
		{
			gperror( "tmpVertBoneData->ChunkName != IDBoneVertWeightList" );
		}
#endif

		{for (DWORD i=0; i < header->NumberOfBones;i++) {

			const sBoneVertWeightList_Chunk *tmpBWVL = (const sBoneVertWeightList_Chunk*)runner;
			runner += sizeof(sBoneVertWeightList_Chunk);

			m_shared->m_restbones[i].m_vertWeights[current_submesh] = NULL;
			m_shared->m_restbones[i].m_numberOfPairs[current_submesh] = 0;

			if (tmpBWVL->NumberOfPairs == -1) {

				// A -1 value indicates that this is a rigid mesh with this bone
				// to be used as the primary
				m_shared->m_primarybone = i;


			} else {

				m_shared->m_restbones[i].m_numberOfPairs[current_submesh] = tmpBWVL->NumberOfPairs;

				if( tmpBWVL->NumberOfPairs )
				{
					m_shared->m_restbones[i].m_vertWeights[current_submesh] = new sBoneVertWeightPair[tmpBWVL->NumberOfPairs];

					for (int j=0; j < tmpBWVL->NumberOfPairs; j++) {

						const sBoneVertWeightPair *tmpPair = (const sBoneVertWeightPair*)runner;
						runner += sizeof(sBoneVertWeightPair);
#if !GP_RETAIL
						if( !(tmpPair->VertIndex>=0 && tmpPair->VertIndex<m_shared->m_nverts[current_submesh]) )
						{
							gperror( "!(tmpPair->VertIndex>=0 && tmpPair->VertIndex<m_shared->m_nverts[current_submesh])" );
						}
#endif
						m_shared->m_restbones[i].m_vertWeights[current_submesh][j].VertIndex = tmpPair->VertIndex;
						m_shared->m_restbones[i].m_vertWeights[current_submesh][j].Weight = tmpPair->Weight;
					}
				}
				else
				{
					m_shared->m_restbones[i].m_vertWeights[current_submesh] = 0;
				}
			}


		}}

		// **************** Read any stitch sets

		if ((m_shared->m_MajorVersion < 1) || (m_shared->m_MajorVersion == 1 && m_shared->m_MinorVersion < 4)) {

			m_shared->m_nstitchsets[current_submesh] = 0;
			m_shared->m_stitchset[current_submesh] = 0;

		} else {

			const sStitchSetHeader_Chunk *tmpStitchHeader = (const sStitchSetHeader_Chunk*)runner;
			runner += sizeof(sStitchSetHeader_Chunk);

#if !GP_RETAIL
			if( tmpStitchHeader->ChunkName != IDStitchSet )
			{
				gperror( "tmpStitchHeader->ChunkName != IDStitchSet" );
			}
#endif
			// Assume we don't won't find a head bone
			m_shared->m_nstitchsets[current_submesh] = tmpStitchHeader->NumberOfSets;

			if (m_shared->m_nstitchsets[current_submesh] == 0) {

				m_shared->m_stitchset[current_submesh] = 0;

			} else {

				m_shared->m_stitchset[current_submesh] = new sStitchSet[m_shared->m_nstitchsets[current_submesh]];

				for (DWORD s = 0; s<m_shared->m_nstitchsets[current_submesh]; s++) {

					const sStitchSet_Chunk *tmpStitchSet = (const sStitchSet_Chunk*)runner;
					runner += sizeof(sStitchSet_Chunk);

					m_shared->m_stitchset[current_submesh][s].m_tag = tmpStitchSet->m_tag;
					m_shared->m_stitchset[current_submesh][s].m_nverts = tmpStitchSet->m_nverts;
					m_shared->m_stitchset[current_submesh][s].m_verts = new DWORD[tmpStitchSet->m_nverts];
					memcpy(m_shared->m_stitchset[current_submesh][s].m_verts,runner,sizeof(DWORD)*tmpStitchSet->m_nverts);

					runner += sizeof(DWORD)*tmpStitchSet->m_nverts;

				}

			}
		}

	}
	//*************************************************
	//********** End of SUB MESH loop *****************
	//*************************************************

	// **************** Read in the REST POSITION DATA

	const sRestPoseList_Chunk *tmpRestPoseBones = (const sRestPoseList_Chunk*)runner;
	runner += sizeof(sRestPoseList_Chunk);

	gpassert(tmpRestPoseBones->ChunkName == IDRestPose);
	gpassert(tmpRestPoseBones->NumberOfBones == header->NumberOfBones);

	{for (DWORD b = 0; b < m_shared->m_nbones; b++) {

		const sRestPoseBoneData *tmpInvBPD = (const sRestPoseBoneData*)runner;
		runner += sizeof(sRestPoseBoneData);

		if ((m_shared->m_MajorVersion<2) && b==0) { //|| b == m_bones[b].m+parentBone->m_sharedbone->m_boneIndex) {

			// This is a root
			const vector_3& p = tmpInvBPD->Position;

			PreRotateNEMAtoSEIGE.RotateVector(m_shared->m_restbones[b].m_relRestPos, p); // vector_3(-p.x,p.z,p.y); --was this biddle
			m_shared->m_restbones[b].m_relRestRot = PreRotateNEMAtoSEIGE * tmpInvBPD->Rotation;

		} else {

			// This is a relative bone
			vector_3 ip = tmpInvBPD->InvPosition;

			m_shared->m_restbones[b].m_relRestPos = tmpInvBPD->Position;
			m_shared->m_restbones[b].m_relRestRot = tmpInvBPD->Rotation;

		}

		m_shared->m_restbones[b].m_invRestPos = tmpInvBPD->InvPosition;
		m_shared->m_restbones[b].m_invRestRot = tmpInvBPD->InvRotation;

	}}

	// **************** Read in any static bounding boxes

	if ((m_shared->m_MajorVersion < 2) || (m_shared->m_MajorVersion == 2 && m_shared->m_MinorVersion < 1)) {

		// This is an old mesh without Static Bounding Boxes

	} else {

		const sBBoxList_Chunk *tmpBBoxes = (const sBBoxList_Chunk*)runner;
		runner += sizeof(sBBoxList_Chunk);

		gpassert(tmpBBoxes->ChunkName == IDBoundingBoxes);

		{for (DWORD b = 0; b < tmpBBoxes->NumberOfBoxes; b++) {

			const sBBoxData *tmpBBox = (const sBBoxData*)runner;
			runner += sizeof(sBBoxData);

			BoundingBox b;
			b.offset = vector_3(tmpBBox->OffsetX,tmpBBox->OffsetY,tmpBBox->OffsetZ);
			b.halfdiag = vector_3(tmpBBox->HalfDiagX,tmpBBox->HalfDiagY,tmpBBox->HalfDiagZ);
			b.orientation = Quat(tmpBBox->OrientX,tmpBBox->OrientY,tmpBBox->OrientZ,tmpBBox->OrientW);

			m_shared->m_boundingboxes[tmpBBox->Tag] = b;

		}}

	}
	// **************** Read in the End-Of-Mesh

	const DWORD *tmpEndMark = (const DWORD*)runner;
	runner += sizeof(DWORD);

	gpassert(*tmpEndMark == IDMeshEndMarker);

	// **************** Post Load Setup

	// Now that we have all the data, copy the verts
	// into the corners to finish the setup

	float max_x,max_y,max_z;
	float min_x,min_y,min_z;

	max_x = max_y = max_z = RealMinimum;
	min_x = min_y = min_z = RealMaximum;

	{for (DWORD csm = 0; csm < m_shared->m_nmeshes; csm++) {

		const sVert * const source_vert = m_shared->m_verts[csm];

		sVertMap* vmap_runner = m_shared->m_vertmappings[csm];

		{for (DWORD i=0; i < m_shared->m_nverts[csm] ;i++) {

			vector_3 temp(source_vert[i]);

			if ((m_shared->m_MajorVersion<2) && m_shared->m_primarybone < 0) {

				// Fix the deformed aspect's initial pose
				// Need to do this because the model is loaded and drawn without
				// any controller driving it (so it is not deformable at load)
				// TODO:: fix game code so that a 'rest pose' set of controller are attached
				// immediatly upon load - biddle

				PreRotateNEMAtoSEIGE.RotateVectorInPlace(temp);
			}

			DWORD count = *vmap_runner++;

			for (DWORD j=1; j <= count; j++, vmap_runner++ )
			{
				// Construct the initial bounding box
				if      (temp.x > max_x)	max_x = temp.x;
				else if (temp.x < min_x)	min_x = temp.x;
				if      (temp.y > max_y)	max_y = temp.y;
				else if (temp.y < min_y)	min_y = temp.y;
				if      (temp.z > max_z)	max_z = temp.z;
				else if (temp.z < min_z)	min_z = temp.z;

				m_shared->m_wcorners[csm][*vmap_runner].v = temp;

				// This is cheap approximation of the COG, just the average of the verts...
				m_shared->m_CenterOfGravity += temp;

			}

		}}

		if (NEMAFLAG_ColorizedWeights && m_shared->m_wcorners) {

			{for (DWORD c=0; c < m_shared->m_ncorners[csm] ;c++) {

				DWORD cval = 0;

				if (m_shared->m_primarybone == -1) {
					if (!IsZero(m_shared->m_wcorners[csm][c].b0)) cval++;
					if (!IsZero(m_shared->m_wcorners[csm][c].b1)) cval++;
					if (!IsZero(m_shared->m_wcorners[csm][c].b2)) cval++;
					if (!IsZero(m_shared->m_wcorners[csm][c].b3)) cval++;
				}

				switch (cval) {

				case 0:	{
						m_corners[csm][c].color				= 0xffffffff;
						m_shared->m_wcorners[csm][c].color	= 0xffffffff;
						break;
					}
				case 1:	{
						m_corners[csm][c].color				= 0xff0000aa;
						m_shared->m_wcorners[csm][c].color	= 0xff0000aa;
						break;
					}
				case 2: {
						m_corners[csm][c].color				= 0xff00aa00;
						m_shared->m_wcorners[csm][c].color	= 0xff00aa00;
						break;
					}
				case 3:  {
						m_corners[csm][c].color				= 0xffaaaa00;
						m_shared->m_wcorners[csm][c].color	= 0xffaaaa00;
						break;
					}
				default: {
						m_corners[csm][c].color				= 0xffaa0000;
						m_shared->m_wcorners[csm][c].color	= 0xffaa0000;
					}

				}
			}}

		}

	}}

	// *************** Compute the Mesh Bounding box

	m_BBoxCenter    = vector_3((max_x+min_x)*0.5f,(max_y+min_y)*0.5f,(max_z+min_z)*0.5f);
	m_BBoxHalfDiag  = vector_3((max_x-min_x)*0.5f,(max_y-min_y)*0.5f,(max_z-min_z)*0.5f);
	m_BSphereRadius = Length(m_BBoxHalfDiag);

	// *************** Compute the Bone Bounding boxes

	#define BONE_BBOX_CUTOFF 0.0f

	m_shared->m_restbones[0].m_bboxExists = false;

	{for (DWORD b = 0; b < m_shared->m_nbones; b++) {

		float min_x,min_y,min_z;

		Quat 		invrestrot = m_shared->m_restbones[b].m_invRestRot;
		vector_3	invrestpos = m_shared->m_restbones[b].m_invRestPos;

		max_x = max_y = max_z = 0;
		min_x = min_y = min_z = 0;

		m_shared->m_restbones[b].m_bboxExists = false;

		{for (DWORD csm = 0; csm < m_shared->m_nmeshes; csm++) {

			for (DWORD p=0; p < m_shared->m_restbones[b].m_numberOfPairs[csm]; p++) {

				if (m_shared->m_restbones[b].m_vertWeights[csm][p].Weight > BONE_BBOX_CUTOFF) {

					const int vi = m_shared->m_restbones[b].m_vertWeights[csm][p].VertIndex;

					// Put the vert into the bone's space
					vector_3 v( DoNotInitialize );
					invrestrot.RotateVector(v, m_shared->m_verts[csm][vi]) ;
					v += invrestpos;

					if      (v.x > max_x)	max_x = v.x;
					else if (v.x < min_x)	min_x = v.x;
					if      (v.y > max_y)	max_y = v.y;
					else if (v.y < min_y)	min_y = v.y;
					if      (v.z > max_z)	max_z = v.z;
					else if (v.z < min_z)	min_z = v.z;

					m_shared->m_restbones[b].m_bboxExists = true;

				}

			}
		}}


		// Include the offsets of all the bones that are children of this one

		{for (DWORD k = b+1; k < m_shared->m_nbones; k++) {

			// Look for children of this bone

			if (m_shared->m_restbones[k].m_parentBoneIndex == b) {

				// Include this child bone's origin, so we get 'end' of the bone

				const vector_3 v = m_shared->m_restbones[k].m_relRestPos;

				if      (v.x > max_x)	max_x = v.x;
				else if (v.x < min_x)	min_x = v.x;
				if      (v.y > max_y)	max_y = v.y;
				else if (v.y < min_y)	min_y = v.y;
				if      (v.z > max_z)	max_z = v.z;
				else if (v.z < min_z)	min_z = v.z;

			}
		}}

		if (m_shared->m_restbones[b].m_bboxExists) {

			vector_3 minv = vector_3(min_x,min_y,min_z);
			vector_3 maxv = vector_3(max_x,max_y,max_z);

			m_shared->m_restbones[b].m_bboxHalfDiag = (maxv-minv) * 0.5;
			m_shared->m_restbones[b].m_bboxCenter	= (maxv+minv) * 0.5;

		}

	}}

	m_IsDeformable = m_shared->m_primarybone < 0;

	m_PrimaryBoneIsLocked = false;

	// Assign a temporary name based on the first texture
#	if GP_DEBUG
	m_dbgname.assignf("<<%s>>",m_shared->m_stringtable);
#	endif // GP_DEBUG

	// TODO:: storing the velocity here temporarily -- biddle
	m_Velocity = 0;

	m_Scale	=  vector_3::ONE;

	// Initialize the current and previous chores to 'none'

	m_CurrReqBlock = 0xfeedbeee;

	m_PrevChore = CHORE_NONE;
	m_CurrChore = CHORE_NONE;
	m_NextChore = CHORE_NONE;

	m_PrevStance = AS_PLAIN;
	m_CurrStance = AS_PLAIN;
	m_NextStance = AS_PLAIN;

	m_PrevSubAnim = 0;
	m_CurrSubAnim = 0;
	m_NextSubAnim = 0;

	m_PrevFlags = 0;
	m_CurrFlags = 0;
	m_NextFlags = 0;

	m_ChoreChangeRequested = false;

	/// When should we create a blender?
	/// we don't need one unless we are animating
	m_Blender = NULL;

	m_UpdateEnabled = true;
	m_DeformationRequested = false;

	m_BonesInRestPose = false;

	// This is cheap approximation of the COG, just the average of the verts...
	m_shared->m_CenterOfGravity = m_shared->m_CenterOfGravity/(float)m_shared->m_maxverts;

	// $$ Get rid of the callback member, replace it with a proper timeline
	m_CompletionCallback = NULL;

	m_Ambience			= 0xFFFFFFFF;
	m_Alpha				= 0x000000FF;
	m_DiffuseColor		= 0xFFFFFFFF;	// white
	m_lightVisit		= 0xFFFFFFFF;

	m_PurgeLevel = 2;

	m_shared->AddRef();

}
//************************************************************
Aspect::Aspect(AspectImpl* pimpl) {

	// This function clones the original Aspect, sharing data

	m_shared = pimpl;

	gpassertf(m_shared,("Failed to allocated an aspect implementation"));

	if (!m_shared) {
		return;
	}

	// Make sure that the offsets are identity
	m_linkOffsetRotation = Quat::IDENTITY;			// $$ could initialize to the NEMAtoSEIGE fixup rotation
	m_linkOffsetPosition = vector_3::ZERO;

	m_attachtype = UNATTACHED;
	m_linkBone = 0;
	m_parent = HAspect();

	m_instflags = NULL;

	/// When should we create a blender?
	/// we don't need one unless we are animating
	m_Blender = NULL;

	m_memblock = 0;

	m_bones = 0;
	m_corners = 0;
	m_cornercolors = 0;
	m_normals = 0;

	// **************** Make room for the textures

	m_currenttextures = new DWORD[m_shared->m_ntextures];
	::ZeroMemory( m_currenttextures, sizeof( DWORD ) * m_shared->m_ntextures );
	m_textureloadids = new siege::LoadOrderId[m_shared->m_ntextures];
	::ZeroMemory( m_textureloadids, sizeof( siege::LoadOrderId ) * m_shared->m_ntextures );

	m_IsDeformable = m_shared->m_primarybone < 0;
	m_PrimaryBoneIsLocked = false;

	// Use the partial unloaded to rebuild all of the internal data
	m_PurgeLevel = 2;
	m_BonesInRestPose = false;

	// Assign a temporary name based on the first texture
#	if GP_DEBUG
	m_dbgname.assignf("<<%s>>",m_shared->m_stringtable);
#	endif // GP_DEBUG

	m_Velocity = 0;

	m_Scale	=  vector_3::ONE;

	// Initialize the current and previous chores to 'none'

	m_CurrReqBlock = 0xfeedbeee;

	m_PrevChore = CHORE_NONE;
	m_CurrChore = CHORE_NONE;
	m_NextChore = CHORE_NONE;

	m_ChoreChangeRequested = false;

	m_PrevStance = AS_PLAIN;
	m_CurrStance = AS_PLAIN;
	m_NextStance = AS_PLAIN;

	m_PrevSubAnim = 0;
	m_CurrSubAnim = 0;
	m_NextSubAnim = 0;

	m_PrevFlags = 0;
	m_CurrFlags = 0;
	m_NextFlags = 0;

	m_UpdateEnabled = true;
	m_DeformationRequested = false;
	m_DeformNormCounter = 0;

	// $$ Get rid of the callback member, replace it with a proper timeline
	m_CompletionCallback = NULL;

	m_Ambience			= 0xFFFFFFFF;
	m_Alpha				= 0x000000FF;
	m_lightVisit		= 0xFFFFFFFF;

	m_DiffuseColor		= 0xFFFFFFFF;	// white

	m_shared->AddRef();
}


// ***************************************************
bool CheckTextureDifferent( DWORD index, gpstring& name, const Aspect& aspect )
{
	DWORD texture = aspect.GetTexture( index );

	// no texture there?
	if ( texture == 0 )
	{
		return ( false );
	}

	// can't xfer if not from disk
	if ( !gDefaultRapi.IsTextureFromDisk( texture ) )
	{
		return ( false );
	}

	// invalid texture?
	name = gDefaultRapi.GetTexturePathname( texture );
	if ( name.empty() )
	{
		return ( false );
	}

	// ok expand default name and compare against rapi's version
	const char* defName = aspect.GetDefaultTextureString( index );
	gpstring localDefName;
	if ( gNamingKey.BuildIMGLocation( defName, localDefName ) )
	{
		defName = localDefName;
	}

	// now compare
	return ( !name.same_no_case( defName ) );
}

// ***************************************************
bool ValidateLink(Aspect *parent, Aspect *child)
{

	if ( Services::IsAnimViewer() )
	{
		return true;
	}

	Go* pgo = parent ? GetGo(parent->GetGoid()) : 0 ;
	Go* kgo = child ? GetGo(child->GetGoid()) : 0;

	if (!parent)
	{
		gperrorf((
			"NEMA: ValidateLink failed: Parent handle is invalid! Child is [%s g:%08x s:%08x m:%s]\n",
			kgo ? kgo->GetTemplateName() : "<unknown>",
			kgo ? MakeInt(kgo->GetGoid()) : -1,
			kgo ? MakeInt(kgo->GetScid()) : -1,
			child ? child->GetDebugName() : "<bad handle>"
			));
		return false;
	}

	if (!child)
	{
		gperrorf((
			"NEMA: ValidateLink failed: Child handle is invalid! Parent is [%s g:%08x s:%08x m:%s]\n",
			pgo ? pgo->GetTemplateName() : "<unknown>",
			pgo ? MakeInt(pgo->GetGoid()) : -1,
			pgo ? MakeInt(pgo->GetScid()) : -1,
			parent->GetDebugName()
			));
		return false;
	}

	if (!pgo)
	{
		gperrorf((
			"NEMA: ValidateLink failed: Parent [%s] has a mangled goid [%08x]! Child is [%s g:%08x s:%08x m:%s]\n",
			parent->GetDebugName(),
			parent->GetGoid(),
			kgo ? kgo->GetTemplateName() : "<bad goid>",
			child->GetGoid(),
			kgo ? MakeInt(kgo->GetScid()) : -1,
			child->GetDebugName() 
			));
		return false;
	}

	if (!kgo)
	{
		gperrorf((
			"NEMA: ValidateLink failed: Child [%s] has a mangled goid [%08x]! Parent is [%s g:%08x s:%08x m:%s]\n",
			child->GetDebugName(),
			child->GetGoid(),
			pgo->GetTemplateName(),
			pgo->GetGoid(),
			pgo->GetScid(),
			parent->GetDebugName()
			));
		return false;
	}

	if (pgo->IsLodfi() != kgo->IsLodfi())
	{
		// Can't attach something lodfi to something that isn't lodfi
		gperrorf((
			"NEMA: ValidateLink failed: lodfi mismatch!\n"
			"\tParent [%s g:%08x s:%08x m:%s] %s\n"
			"\tChild  [%s g:%08x s:%08x m:%s] %s\n",
			pgo->GetTemplateName(),
			pgo->GetGoid(),
			pgo->GetScid(),
			parent->GetDebugName(),
			pgo->IsLodfi() ? "IS lodfi" : "is NOT lodfi",
			kgo->GetTemplateName(),
			kgo->GetGoid(),
			kgo->GetScid(),
			child->GetDebugName(), 
			kgo->IsLodfi() ? "IS lodfi" : "is NOT lodfi"
			));
		return false;
	}

	if ( pgo->IsCloneSourceGo() || kgo->IsCloneSourceGo() )
	{
		// Can't attach something lodfi to something that isn't lodfi
		gperrorf((
			"NEMA: ValidateLink failed: Attempt to link with a clone source!\n"
			"\tParent [%s g:%08x s:%08x m:%s] %s\n"
			"\tChild  [%s g:%08x s:%08x m:%s] %s\n",
			pgo->GetTemplateName(),
			pgo->GetGoid(),
			pgo->GetScid(),
			parent->GetDebugName(),
			pgo->IsCloneSourceGo() ? "IS a clone source" : "is NOT a clone source",
			kgo->GetTemplateName(),
			kgo->GetGoid(),
			kgo->GetScid(),
			child->GetDebugName(), 
			kgo->IsCloneSourceGo() ? "IS a clone source" : "is NOT a clone source"
			));
		return false;
	}

	return true;

}


// ***************************************************
bool
Aspect::Xfer( FuBi::PersistContext& persist )
{

	if ( persist.IsSaving())
	{
		ValidateParentLinks(false);
	}

	persist.Xfer           ( "m_handle",                  m_handle                  );
	persist.Xfer           ( "m_dbgname",                 m_dbgname                 );
	persist.Xfer           ( "m_owner_goid",              m_owner_goid              );
	persist.XferHex        ( "m_instflags",               m_instflags               );
	persist.Xfer           ( "m_Scale",                   m_Scale                   );

	persist.Xfer           ( "m_PurgeLevel",              m_PurgeLevel              );

	persist.Xfer           ( "m_attachtype",              m_attachtype              );

	if ((m_attachtype != UNATTACHED))
	{
		persist.Xfer       ( "m_parent",                  m_parent                  );

		if ( (m_attachtype == RIGID_LINKED) || (m_attachtype == REVERSE_RIGID_LINKED))
		{
			persist.Xfer   ( "m_linkBone",                m_linkBone                );
			persist.Xfer   ( "m_linkOffsetRotation",      m_linkOffsetRotation      );
			persist.Xfer   ( "m_linkOffsetPosition",      m_linkOffsetPosition      );
		}
	}
	else
	{
		if (persist.IsRestoring())
		{
			m_parent = HAspect();
			m_linkOffsetRotation = Quat::IDENTITY;
			m_linkOffsetPosition = vector_3::ZERO;
			m_linkBone = 0;
		}
	}

	persist.XferMap        ( "m_childlinks",              m_childlinks              );

	persist.Xfer           ( "m_UpdateEnabled",           m_UpdateEnabled           );
	persist.Xfer           ( "m_IsDeformable",            m_IsDeformable            );
	persist.Xfer           ( "m_PrimaryBoneIsLocked",     m_PrimaryBoneIsLocked     );
	persist.Xfer           ( "m_BonesInRestPose",         m_BonesInRestPose         );
	persist.Xfer           ( "m_Alpha",                   m_Alpha                   );
	persist.XferHex        ( "m_DiffuseColor",            m_DiffuseColor            );

	if (persist.IsRestoring())
	{
		m_DeformationRequested = m_IsDeformable;
	}

	FuBi::Traits <Blender*>::ms_Owner = this;
	if ( persist.IsSaving() )
	{
		if (m_Blender)
		{
			persist.EnterBlock( "blender" );
			persist.Xfer( "m_Blender" , m_Blender );
			persist.LeaveBlock();
		}
	}
	else
	{
		if (persist.EnterBlock( "blender" ))
		{
			persist.Xfer( "m_Blender" ,m_Blender );
			m_Blender->SetOwner(this);				// Setting blender owner will resolve ActiveChore owners too --biddle
		}
		else
		{
			m_Blender = NULL;
		}
		persist.LeaveBlock();
	}

	persist.Xfer       ( "m_Velocity",                m_Velocity                );
	persist.Xfer       ( "m_PrevChore",               m_PrevChore               );
	persist.Xfer       ( "m_PrevStance",              m_PrevStance              );
	persist.Xfer       ( "m_PrevSubAnim",             m_PrevSubAnim             );
	persist.XferHex    ( "m_PrevFlags",               m_PrevFlags               );
	persist.XferHex    ( "m_CurrReqBlock",            m_CurrReqBlock            );
	persist.Xfer       ( "m_CurrChore",               m_CurrChore               );
	persist.Xfer       ( "m_CurrStance",              m_CurrStance              );
	persist.Xfer       ( "m_CurrSubAnim",             m_CurrSubAnim             );
	persist.XferHex    ( "m_CurrFlags",               m_CurrFlags               );
	persist.Xfer       ( "m_NextChore",               m_NextChore               );
	persist.Xfer       ( "m_NextStance",              m_NextStance              );
	persist.Xfer       ( "m_NextSubAnim",             m_NextSubAnim             );
	persist.XferHex    ( "m_NextFlags",               m_NextFlags               );		
	persist.Xfer       ( "m_ChoreChangeRequested",    m_ChoreChangeRequested    );

	if ( persist.IsRestoring() )
	{
		// invalidate lighting
		m_Ambience = UINT_MAX;
		// Force a normal deformation
		m_DeformNormCounter = 0;
	}

	if (persist.IsSaving())
	{
	}
	else
	{
	}

	// xfer the textures - kind of involved, but we only want to persist texture
	// changes, not const data. important so the art can be rev'd without
	// screwing up saved games. also easier on the storage and cpu time.
	gpstring name;
	if ( persist.IsSaving() )
	{

		DWORD* i, * begin = m_currenttextures, * end = begin + m_shared->m_ntextures;
		for ( i = begin ; i != end ; ++i )
		{
			if ( CheckTextureDifferent( i - begin, name, *this ) )
			{
				persist.EnterColl( "m_currenttextures" );
				bool first = true;
				for ( ; i != end ; ++i )
				{
					if ( first || CheckTextureDifferent( i - begin, name, *this ) )
					{
						first = false;
						persist.AdvanceCollIter();
						int index = i - begin;
						persist.Xfer( "_index", index );
						persist.Xfer( FuBi::PersistContext::VALUE_STR, name );
					}
				}
				persist.LeaveColl();

				// always break out of here
				break;
			}
		}
	}
	else
	{

		if ( persist.EnterColl( "m_currenttextures" ) )
		{
			while ( persist.AdvanceCollIter() )
			{
				int index;
				persist.Xfer( "_index", index );
				persist.Xfer( FuBi::PersistContext::VALUE_STR, name );
				SetTextureFromFile( index, name );
				name.clear();
			}
		}
		persist.LeaveColl();

		// restore remaining default textures
		DWORD* i, * begin = m_currenttextures, * end = begin + m_shared->m_ntextures;
		for ( i = begin ; i != end ; ++i )
		{
			if ( *i == 0 )
			{
				const char* defName = GetDefaultTextureString( i - begin );
				if ( gNamingKey.BuildIMGLocation( defName, name ) )
				{
					defName = name;
				}
				SetTextureFromFile( i - begin, name );
			}
		}
	}

	return ( true );
}


// ***************************************************
bool
Aspect::XferPost( FuBi::PersistContext& persist )
{
	
	// Verify the the parent is a valid Aspect, and that all children are valid aspects

	if (!IsPurged(2))
	{
		// Have the bones been moved by something other than the blender?
		bool forceposrot = 	((m_Blender == NULL) &&
							 (m_attachtype == UNATTACHED) &&
							 !m_BonesInRestPose
							 );

		if ( persist.IsSaving() )
		{

			gpassert(m_memblock		&& !IsMemoryBadFood(*(DWORD*)m_memblock)	);
			gpassert(m_corners		&& !IsMemoryBadFood(*(DWORD*)m_corners)		);
			gpassert(m_cornercolors && !IsMemoryBadFood(*(DWORD*)m_cornercolors));
			gpassert(m_normals		&& !IsMemoryBadFood(*(DWORD*)m_normals)		);
			gpassert(m_bones		&& !IsMemoryBadFood(*(DWORD*)m_bones)		);

			sBone* i, * begin = m_bones, * end = begin + m_shared->m_nbones;

			bool incoll = false ;
			for ( i = begin ; i != end ; ++i )
			{
				if (   forceposrot
					|| i->m_frozen
					)
				{
					if (!incoll)
					{
						incoll = persist.EnterColl( "m_bones" );
					}
					persist.AdvanceCollIter();
					int index = i - begin;
					persist.Xfer( "_index", index );
					i->XferPost( persist );
				}
			}
			if (incoll)
			{
				persist.LeaveColl();
			}
		}
		else
		{
			if ( persist.EnterColl( "m_bones" ) )
			{
				while ( persist.AdvanceCollIter() )
				{
					int index;
					persist.Xfer( "_index", index );
					m_bones[ index ].XferPost( persist );
				}
			}
			persist.LeaveColl();
		}
	}

	return ( true );
}

// ***************************************************
void Aspect::ValidateParentLinks( bool fix_errors )
{
	//**
	//** NOTE:
	//**
	//** The code that 'fixes errors' hasn't actually been tested in 02.2701
	//**
	//** I ran tests by plugging this code into the 02.1902 source and then 
	//** loading in saved game files that had already been corrupted by that version
	//**
	//** I chose NOT to release any handles, hoping that I might be able to
	//** determine what became invalid by looking at the error logs when
	//** the aspect manager is destroyed (and things were left dangling)
	//**

#if GP_RETAIL
	if (!fix_errors)
	{
		// No error messages get generated, so we can early out.
		return;
	}
#endif

	ChildLinkIter it;

	// Verify the parent

	if ( m_parent != HAspect())
	{

		if (!m_parent.IsValid())
		{
			gperrorf((
				"NEMA: ValidateParentLinks [%s g:%08x s:%08x] has a mangled parent nema aspect handle!\n",
				GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
				m_owner_goid,
				GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
				));

			if (fix_errors) 
			{
				m_parent = HAspect();
				m_attachtype = UNATTACHED;
			}
		}
		else if (m_attachtype == UNATTACHED)
		{
			gperrorf((
				"NEMA: ValidateParentLinks [%s g:%08x s:%08x] has a valid parent, but thinks it is UNATTACHED !\n",
				GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
				m_owner_goid,
				GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
				));
			if (fix_errors) 
			{
				it = m_parent->m_childlinks.find(m_handle);
				if (it == m_childlinks.end())
				{
					it = m_parent->m_childlinks.erase(it);
				}
				m_parent = HAspect();
			}
		} 
		else
		{
			it = m_parent->m_childlinks.find(m_handle);
			if (it == m_childlinks.end())
			{
				gperrorf((
					"NEMA: ValidateParentLinks [%s g:%08x s:%08x] is not in it's parent's list of children!\n",
					GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					m_owner_goid,
					GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
					));

				if (fix_errors) 
				{
					m_parent = HAspect();
					m_attachtype = UNATTACHED;
				}
			}
		}
	}

	// Verify the children

	int sz = m_childlinks.size();
	int childindex = 1;

	for ( it = m_childlinks.begin(); it != m_childlinks.end(); childindex++ )
	{
		bool erase_child = false;
		if ( !(*it).first.IsValid() )
		{
			gperrorf((
				"NEMA: ValidateParentLinks [%s g:%08x s:%08x] has a mangled child nema aspect handle (child %d of %d)\n",
				GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
				m_owner_goid,
				GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
				childindex,
				sz
				));
			if (fix_errors) 
			{
				erase_child = true;
			}
		}
		else
		{
			if (!(*it).first->m_parent.IsValid())
			{
				gperrorf((
					"NEMA: ValidateParentLinks [%s g:%08x s:%08x] has child with invalid parent aspect handle [%s g:%08x s:%08x]!\n",
					GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					m_owner_goid,
					GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
					GetGo((*it).first->m_owner_goid) ? GetGo((*it).first->m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					(*it).first->m_owner_goid,
					GetGo((*it).first->m_owner_goid) ? MakeInt(GetGo((*it).first->m_owner_goid)->GetScid()) : -1
					));
				if (fix_errors) 
				{
					// I could re-attach it, but instead I will remove it
					(*it).first->m_attachtype = UNATTACHED;
					erase_child = true;
				}
			}
			else if (((*it).first)->m_attachtype == UNATTACHED)
			{
				gperrorf((
					"NEMA: ValidateParentLinks [%s g:%08x s:%08x] has child [%s g:%08x s:%08x] that has it the parent of, but that child is supposed to be 'unattached'!\n",
					GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					m_owner_goid,
					GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
					GetGo((*it).first->m_owner_goid) ? GetGo((*it).first->m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					(*it).first->m_owner_goid,
					GetGo((*it).first->m_owner_goid) ? MakeInt(GetGo((*it).first->m_owner_goid)->GetScid()) : -1
					));
				if (fix_errors) 
				{
					(*it).first->m_parent = HAspect();
					erase_child = true;
				}
			}
			else if ((*it).first->m_parent != m_handle)
			{
				gperrorf((
					"NEMA: ValidateParentLinks [%s g:%08x s:%08x] is not the parent of it's child [%s g:%08x s:%08x] parent is set to [%s g:%08x s:%08x]!\n",
					GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					m_owner_goid,
					GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
					GetGo((*it).first->m_owner_goid) ? GetGo((*it).first->m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					(*it).first->m_owner_goid,
					GetGo((*it).first->m_owner_goid) ? MakeInt(GetGo((*it).first->m_owner_goid)->GetScid()) : -1,
					GetGo((*it).first->m_parent->m_owner_goid) ? GetGo((*it).first->m_parent->m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
					(*it).first->m_parent->m_owner_goid,
					GetGo((*it).first->m_parent->m_owner_goid) ? MakeInt(GetGo((*it).first->m_parent->m_owner_goid)->GetScid()) : -1
					));
				if (fix_errors) 
				{
					// Leave it attached, just not to this parent
					erase_child = true;
				}
			}
			else if (!ValidateLink(this,(*it).first.Get()))
			{
				// Something's wrong with the type of the gos that were linked
				if (fix_errors) 
				{
					(*it).first->m_attachtype = UNATTACHED;
					(*it).first->m_parent = HAspect();
					erase_child = true;
				}
			}
		}

		if (erase_child)
		{
			it = m_childlinks.erase(it);
		}
		else
		{
			++it;
		}

	}
}

// ***************************************************
void Aspect::CommitCreation()
{
	if( m_Blender )
	{
		m_Blender->CommitCreation();
	}
}

// ***************************************************
bool Aspect::PrivateCreateTexture(DWORD index, const char* texFile )
{
	gpassert(index < m_shared->m_ntextures);
	bool ret = index < m_shared->m_ntextures;
	if (ret)
	{
		PrivateDestroyTexture(index);

		DWORD tex = gDefaultRapi.CreateTexture( texFile, true, 0, TEX_LOCKED, false );
		m_currenttextures[index] = tex;

		// See if the texture is environment (indicating we need to pass normals to the renderer)
		if (tex)
		{
			bool texture_requires_normals = gDefaultRapi.IsTextureEnvironment( tex );
			if (texture_requires_normals)
			{
				m_instflags |= INCLUDE_NORMALS;
			}
			if (!IsPurged(1))
			{
				PrivateReconstituteTexture(index, false);		// ??? Should this be immediate
			}
		}
	}
	return ret;
}
// ***************************************************
bool Aspect::PrivateDestroyTexture(DWORD index)
{
	gpassert(index < m_shared->m_ntextures);
	bool ret = index < m_shared->m_ntextures;
	if (ret)
	{
		int tex = m_currenttextures[index];
		siege::LoadOrderId tid = m_textureloadids[index];
		if (tid)
		{
			m_textureloadids[index] = 0;
			gSiegeLoadMgr.Cancel(tid);
		}
		if (tex != 0)
		{
			m_currenttextures[index] = 0;
			gDefaultRapi.DestroyTexture(tex);
		}
	}
	return ret;
}

// ***************************************************
bool Aspect::PrivatePurgeTexture(DWORD index)
{
	gpassert(index < m_shared->m_ntextures);
	bool ret = index < m_shared->m_ntextures;
	if (ret)
	{
		int tex = m_currenttextures[index];
		if (tex != 0)
		{
			if (m_textureloadids[index])
			{
				if (!gSiegeLoadMgr.Cancel(m_textureloadids[index]))
				{
					gDefaultRapi.UnloadTexture(tex);
				}
				m_textureloadids[index] = 0;
			}
			else
			{
				gDefaultRapi.UnloadTexture(tex);
			}
		}
		else
		{
			gpassert(!m_textureloadids[index]);
		}
	}
	return ret;
}

// ***************************************************
bool Aspect::PrivateReconstituteTexture(DWORD index, bool immediate)
{
	gpassert(index < m_shared->m_ntextures);
	bool ret = index < m_shared->m_ntextures;
	if (ret)
	{
		int tex = m_currenttextures[index];
		if (tex != 0) {
			if (immediate)
			{
				if (m_textureloadids[index])
				{
					gSiegeLoadMgr.Cancel(m_textureloadids[index]);
				}
				m_textureloadids[index] = 0;
				gDefaultRapi.LoadTexture( tex );
			}
			else
			{
				m_textureloadids[index] = gSiegeLoadMgr.LoadById( siege::LOADER_TEXTURE_TYPE, tex, true );
			}
			if (gDefaultRapi.IsTextureEnvironment( tex ))
			{
				m_instflags |= INCLUDE_NORMALS;
			}
		}
	}
	return ret;
}
// ***************************************************
bool
Aspect::PrivateSetTexture( DWORD index, DWORD tex )
{
	bool ret = index < m_shared->m_ntextures;

	if ( ret )
	{
		PrivateDestroyTexture(index);

		// How can we be SURE that the texture is loaded?
		m_currenttextures[index] = tex;

		// See if the texture is environment (indicating we need to pass normals to the renderer)
		bool texture_requires_normals = tex && gDefaultRapi.IsTextureEnvironment( tex );
		if (texture_requires_normals)
		{
			m_instflags |= INCLUDE_NORMALS;
		}

	}
	return ret;
}

// ***************************************************
void
Aspect::SetTextureFromFile( DWORD index, const char* texFile )
{
	gpassert (index < m_shared->m_ntextures);

	if ( index < m_shared->m_ntextures )
	{
		PrivateDestroyTexture(index);

		if ( texFile && *texFile )
		{
			// create it
			if (PrivateCreateTexture(index,texFile))
			{
				if (!IsPurged(1))
				{
					PrivateReconstituteTexture(index,true);
				}
			}
		}
	}
}

// ***************************************************
void
Aspect::SetTextureCreatedByExternalSource( DWORD index, DWORD tex )
{
	if ( index < m_shared->m_ntextures )
	{
		PrivateDestroyTexture(index);

		m_currenttextures[index] = tex;
		gpverify( gDefaultRapi.AddTextureReference( tex ) );
	}
}

// ***************************************************
void
Aspect::CopyTextures( const Aspect& src )
{
	gpassert( m_shared == src.m_shared );
	gpassert (m_shared->m_ntextures == src.m_shared->m_ntextures);

	// copy over the textures from the other aspect
	if ( m_shared->m_ntextures == src.m_shared->m_ntextures)
	{
		for ( DWORD index = 0 ; index < m_shared->m_ntextures ; index++ )
		{
			if (PrivateSetTexture( index, src.m_currenttextures[ index ] ))
			{
				if (m_currenttextures[ index ])
				{
					gpverify(gDefaultRapi.AddTextureReference( m_currenttextures[ index ] ));
					if (!IsPurged(1))
					{
						PrivateReconstituteTexture(index,true);
					}
				}
			}
		}
	}
}

// ***************************************************
void
Aspect::CopyTexture( DWORD dstindex, const Aspect& src, DWORD srcindex )
{
	// copy over the texture from the other aspect

	gpassert (srcindex < src.m_shared->m_ntextures);
	gpassert (dstindex < m_shared->m_ntextures);

	if (srcindex < src.m_shared->m_ntextures)
	{
		if (dstindex < m_shared->m_ntextures)
		{
			if (PrivateSetTexture( dstindex, src.m_currenttextures[ srcindex ] ))
			{
				if (m_currenttextures[ dstindex ])
				{
					gpverify( gDefaultRapi.AddTextureReference( m_currenttextures[ dstindex ] ) );
					if (!IsPurged(1))
					{
						PrivateReconstituteTexture(dstindex,true);
					}
				}
			}
		}
	}
}

// ***************************************************
bool Aspect::MatchPurgeStatus(Aspect* src,bool immediate)
{
	if (m_PurgeLevel != src->m_PurgeLevel)
	{
		if (m_PurgeLevel > src->m_PurgeLevel)
		{
			PrivateReconstitute(src->m_PurgeLevel,immediate);
		}
		else
		{
			Purge(src->m_PurgeLevel);
		}
	}
	return true;
}

// ***************************************************
bool Aspect::Purge(int newlevel)
{

	if ((m_PurgeLevel != (DWORD)newlevel) && siege::SiegeEngine::DoesSingletonExist() && !gSiegeEngine.IsPrimaryThread())
	{
		gperrorf(("Aspect::Purge() is changing the purge level, but it's not on the primary thread! VERBOTEN!!"));
		return false;
	}

	// Unload data that we don't need to render the aspect

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		// Unload any children, so we can delete the bones
		if ((*it).first.IsValid())
		{
			(*it).first->Purge(newlevel);
		}
	}

	if ( (m_PurgeLevel<1) && (newlevel>=1) )
	{
		for (DWORD t = 0; t < m_shared->m_ntextures; t++ )
		{
			PrivatePurgeTexture(t);
		}
		m_PurgeLevel = 1;
	}

	if ( (m_PurgeLevel<2) && (newlevel>=2) )
	{
		gpassert(m_corners		&& !IsMemoryBadFood(*(DWORD*)m_corners)		);
		gpassert(m_cornercolors && !IsMemoryBadFood(*(DWORD*)m_cornercolors));
		gpassert(m_normals		&& !IsMemoryBadFood(*(DWORD*)m_normals)		);
		gpassert(m_bones		&& !IsMemoryBadFood(*(DWORD*)m_bones)		);

		gpassert(m_memblock		&& !IsMemoryBadFood(*(DWORD*)m_memblock)	);

#if GP_DEBUG
//		gpwarningf(("Purging %8d\n", m_memsize));
#endif

		delete [] m_memblock; m_memblock = 0;

		m_corners		= 0;
		m_cornercolors	= 0;
		m_normals		= 0;
		m_bones			= 0;

		m_PurgeLevel = 2;
	}


	//$$$ todo - partially unload the blender

	return true;
}


// ***************************************************

bool Aspect::ReconstituteTop(DWORD level,bool immediate) 
{
	// find parent
	Aspect* asp = this;
	while ( asp->GetParent() )
	{
		asp = asp->GetParent().Get();
	}

	return asp->PrivateReconstitute(level,immediate);
}


// ***************************************************

bool Aspect::PrivateReconstitute( int newlevel, bool immediate_texture_load )
{
	if ((m_PurgeLevel != (DWORD)newlevel) && siege::SiegeEngine::DoesSingletonExist() && !gSiegeEngine.IsPrimaryThread() )
	{
		gperrorf(("Aspect::PrivateReconstitute() is changing the purge level, but it's not on the primary thread! VERBOTEN!!"));
		return false;
	}

	// Reconstruct all the internal data needed to render the aspect
	// from data stored in the shared pimpl

	// Only on a persist.Restoring() can newlevel be negative

	DWORD oldlevel;

	if (newlevel<0)
	{
		oldlevel = 2;
		m_PurgeLevel = ~newlevel;
	}
	else
	{
		oldlevel = m_PurgeLevel;
		m_PurgeLevel = newlevel;
	}

	if ( oldlevel < m_PurgeLevel )
	{
		m_PurgeLevel = oldlevel;
		return false;
	}

	gpassertf(!m_parent || !m_parent->IsPurged(2),("NEMA ERROR: You are trying to reconstitute a child with a purged parent!"));

	if ((oldlevel > 0) && (m_PurgeLevel < 1))
	{
		m_instflags &= ~INCLUDE_NORMALS;
		for (DWORD t = 0; t < m_shared->m_ntextures; t++ )
		{
			PrivateReconstituteTexture(t, immediate_texture_load);
		}
	}

	if ((oldlevel > 1) && (m_PurgeLevel < 2))
	{

		gpassert(m_bones == 0);
		gpassert(m_corners == 0);
		gpassert(m_cornercolors == 0);
		gpassert(m_normals == 0);

#if !GP_DEBUG
		DWORD m_memsize;
#endif

		m_memsize = m_shared->m_nbones  * sizeof( sBone );
		m_memsize += m_shared->m_nmeshes * sizeof(DWORD) * 3;

		DWORD csm;
		for (csm = 0 ; csm < m_shared->m_nmeshes; csm++)
		{
			m_memsize += (m_shared->m_ncorners[csm] * ( sizeof( sCorner ) + sizeof( vector_3 ) + sizeof( DWORD ) ) );
		}

		gpassert(m_memblock==0);

#if GP_DEBUG
//		gpwarningf(("Reconstituting %8d\n", m_memsize));
#endif

		m_memblock = new char[m_memsize+PADDING_BUFFER_SIZE];

		PADDING_BUFFER_SETUP(this,m_memsize);
		PADDING_BUFFER_CHECK(this)
		
		m_bones			= (sBone*)		&m_memblock[PADDING_BUFFER_SIZE];
		m_corners		= (sCorner**)	&m_bones[m_shared->m_nbones];
		m_normals		= (vector_3**)	&m_corners[m_shared->m_nmeshes];
		m_cornercolors	= (DWORD**)		&m_normals[m_shared->m_nmeshes];

		for (DWORD i=0; i < m_shared->m_nbones;i++)
		{
			m_bones[i].m_sharedbone =  &m_shared->m_restbones[i];
			m_bones[i].m_tweaked 	= false;
			m_bones[i].m_frozen 	= false;
			m_bones[i].m_parentBone = &(m_bones[m_shared->m_restbones[i].m_parentBoneIndex]);
		}

		m_DeformationRequested	= m_IsDeformable;

		if (m_PrimaryBoneIsLocked)
		{
			if (m_shared->m_primarybone < 0)
			{
				m_bones[0].m_position = vector_3::ZERO;
				m_bones[0].m_rotation = Quat::IDENTITY;
			}
			else
			{
				m_bones[m_shared->m_primarybone].m_position = vector_3::ZERO;
				m_bones[m_shared->m_primarybone].m_rotation = Quat::IDENTITY;
			}
		}

		if (m_parent && !m_parent->IsPurged(2))
		{
			Aspect* parent = m_parent.Get();

			ChildLinkIter it = parent->m_childlinks.find(m_handle);

			gpassert(it != parent->m_childlinks.end());

			if ((*it).second.IsFullyLinked())
			{
				for (DWORD c = 0; c < m_shared->m_nbones; c++ )
				{
					m_bones[c].m_parentBone = &parent->m_bones[c];
				}
			}
			else
			{
				DWORD c = (*it).second.ChildIndex();
				DWORD p = (*it).second.ParentIndex();
				m_bones[c].m_parentBone = &parent->m_bones[p];
			}

			m_TweakRequested = true;
			AnimateBones(0,true);
			m_BonesInRestPose = true;

			if (parent->m_Blender && (*it).second.HasWeaponTracerFlag())
			{
				parent->CalculateWeaponTracerPoints(this);
			}

		}
		else
		{
			m_TweakRequested = true;
			AnimateBones(0,true);
			m_BonesInRestPose = true;
		}

		float max_x,max_y,max_z;
		float min_x,min_y,min_z;

		max_x = max_y = max_z = RealMinimum;
		min_x = min_y = min_z = RealMaximum;

		sCorner* runner = (sCorner*)&m_cornercolors[m_shared->m_nmeshes];

		for (csm = 0 ; csm < m_shared->m_nmeshes; csm++)
		{
			DWORD nc = m_shared->m_ncorners[csm];

			m_corners[csm] 		=              runner;
			m_normals[csm]		= (vector_3*) &m_corners[csm][nc];
			m_cornercolors[csm] = (DWORD*)    &m_normals[csm][nc];
			runner				= (sCorner*)  &m_cornercolors[csm][nc];

			for (DWORD i=0; i < nc; i++)
			{
				vector_3 temp = m_shared->m_wcorners[csm][i].v;

				m_corners[csm][i].vx	= temp.x;
				m_corners[csm][i].vy	= temp.y;
				m_corners[csm][i].vz	= temp.z;
				m_corners[csm][i].color	= m_shared->m_wcorners[csm][i].color;
				m_corners[csm][i].uv	= m_shared->m_wcorners[csm][i].uv;
				m_cornercolors[csm][i]  = m_shared->m_wcorners[csm][i].color;
				m_normals[csm][i]		= m_shared->m_wcorners[csm][i].n;

				// Reconstruct the bounding box
				if      (temp.x > max_x)	max_x = temp.x;
				else if (temp.x < min_x)	min_x = temp.x;
				if      (temp.y > max_y)	max_y = temp.y;
				else if (temp.y < min_y)	min_y = temp.y;
				if      (temp.z > max_z)	max_z = temp.z;
				else if (temp.z < min_z)	min_z = temp.z;
			}
		}
		m_BBoxCenter    = vector_3((max_x+min_x)*0.5f,(max_y+min_y)*0.5f,(max_z+min_z)*0.5f);
		m_BBoxHalfDiag  = vector_3((max_x-min_x)*0.5f,(max_y-min_y)*0.5f,(max_z-min_z)*0.5f);
		m_BSphereRadius = Length(m_BBoxHalfDiag);
		m_Ambience		= 0xFFFFFFFF;
	}

	PADDING_BUFFER_CHECK(this)

	// Reconstitute the children and relink the bones.
	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			Aspect* child = (*it).first.Get();

			// Make SURE that the child is intact before we try to reconstitute
			// RAID bug #12876

			// We pay a performance penalty for the IsBadReadPtr call here

			if (child->m_shared && !IsBadReadPtr(child->m_shared,4))
			{
				child->PrivateReconstitute(newlevel,immediate_texture_load);
			}
			else
			{
				//put some info onto the stack so that I can 
				// try and figure out what is busted
				gperrorf((
					"NEMA: PrivateReconstitute() Attempted to reconstitute a child with INVALID shared [%s g:%08x s:%08x] for  [%s g:%08x s:%08x]",
					GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
					m_owner_goid,
					GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
					GetGo(child->m_owner_goid) ? GetGo(child->m_owner_goid)->GetTemplateName() : child->m_dbgname.c_str(),
					child->m_owner_goid,
					GetGo(child->m_owner_goid) ? MakeInt(GetGo(child->m_owner_goid)->GetScid()) : -1
					));
			}
		}
	}

	return true;
}

// ***************************************************
void Aspect::CalculateWeaponTracerPoints(Aspect *kid)
{
	PADDING_BUFFER_CHECK(this)

	// Calculate the relative position of the trace points on the weapon
	Quat     grot(DoNotInitialize);
	vector_3 gpos(DoNotInitialize);
	int      bi1;
	int      bi2;
	vector_3 apt1(DoNotInitialize);
	vector_3 apt2(DoNotInitialize);

	if (kid->GetBoneOrientation("grip",gpos,grot) &&
		kid->GetBoneIndex("ap_trace01",bi1) &&
		kid->GetBoneIndex("ap_trace02",bi2))
	{
		kid->GetIndexedBonePosition(bi1,apt1);
		kid->GetIndexedBonePosition(bi2,apt2);

		apt1 -= gpos;
		apt2 -= gpos;

		Quat irot = grot.Inverse();

		m_Blender->SetTracePoint0(irot.RotateVector_T(apt1));
		m_Blender->SetTracePoint1(irot.RotateVector_T(apt2));
	}

	PADDING_BUFFER_CHECK(this)
}

#if	!GP_RETAIL
// ***************************************************
float Aspect::GetCost(void)
{
	GoHandle go( GetGoid() );
	if (go && go->HasAspect() )
	{
		// get cost
		return( go->GetAspect()->GetDisplayCost() );
	}
	return 0.0f;
}
#endif  // !GP_RETAIL

// ***************************************************
void
Aspect::RegisterAnimationCompletionCallback(EventCallback const &CBInit) {

	m_CompletionCallback = CBInit;

}

// ***************************************************
void
Aspect::ResetAnimationCompletionCallback(void) {

	m_CompletionCallback = NULL;

}
// ***************************************************
EventCallback
Aspect::GetAnimationCompletionCallback(void) const
{
	return m_CompletionCallback;
}

// ***************************************************
void
Aspect::AnimationCallback(DWORD msg) {

	if ( m_CompletionCallback ) {
		m_CompletionCallback(msg);
	}
}

// ***************************************************
void
Aspect::SetHideMeshFlag(bool flag)
{
	PADDING_BUFFER_CHECK(this)
	GoHandle go( GetGoid() );
	if (go)
	{
		// Make sure that the owner Go knows that we have modified the hidden state
		go->SetBucketDirty(true);
	}

	if (flag)
	{
		SetInstanceAttrFlags(HIDE_ALLSUBMESHES);
	}
	else
	{
		ClearInstanceAttrFlags(HIDE_ALLSUBMESHES);
	}

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			Aspect* child = (*it).first.Get();
			child->SetHideMeshFlag(flag);
		}
	}
	PADDING_BUFFER_CHECK(this)

}

// ***************************************************
void
Aspect::SetFreezeMeshFlag(bool flag)
{
	PADDING_BUFFER_CHECK(this)
	if (flag)
	{
		SetInstanceAttrFlags(RENDER_FROZEN);

		if (m_IsDeformable)
		{
			bool oldUpdateEnabled = m_UpdateEnabled;
			m_UpdateEnabled = true;
			m_DeformationRequested = true;
			m_DeformNormCounter = 0;
			Deform(0,0);
			m_UpdateEnabled  = oldUpdateEnabled;
		}

	}
	else
	{
		ClearInstanceAttrFlags(RENDER_FROZEN);
	}
	PADDING_BUFFER_CHECK(this)

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			Aspect* child = (*it).first.Get();
			child->SetFreezeMeshFlag(flag);
		}
	}
	PADDING_BUFFER_CHECK(this)
}


// ***************************************************
void
Aspect::SetRenderTracersFlag(bool flag)
{
	if (flag)
	{
		SetInstanceAttrFlags(RENDER_TRACERS);
	}
	else
	{
		ClearInstanceAttrFlags(RENDER_TRACERS);
	}
}


// ***************************************************
void
Aspect::SetLockMeshFlag(bool flag)
{
	PADDING_BUFFER_CHECK(this)
	m_UpdateEnabled  = !flag;

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			Aspect* child = (*it).first.Get();
			child->SetLockMeshFlag(flag);
		}
	}

	PADDING_BUFFER_CHECK(this)
	if (m_IsDeformable && !m_parent)
	{
		// Need to be sure that children are not deformed twice,
		// the stitch verts will be corrupt
		m_UpdateEnabled = true;
		m_DeformationRequested = true;
		m_DeformNormCounter = 0;
		Deform(0,0);
		m_UpdateEnabled  = !flag;
	}

	PADDING_BUFFER_CHECK(this)
}

// ***************************************************
Blender*
Aspect::GetOrCreateBlender()
{
	if (!m_Blender)
	{
		m_Blender = new Blender(this);
	}
	return m_Blender;
}

// ***************************************************
bool
Aspect::BlenderIsIdle()
{
	return (!m_Blender || m_Blender->IsIdle());
}


// ***************************************************
void
Aspect::AddChore(const Chore* nc, bool shouldPersist)
{
	GetOrCreateBlender();
	m_Blender->AddChore(nc, shouldPersist);
}


// ***************************************************
bool
Aspect::SetNextChore(eAnimChore		ReqChore,
					 eAnimStance	ReqStance,
					 int			ReqSubAnim,
					 DWORD			ReqFlags
					 ) {

	if (m_Blender == NULL)
	{
		gperrorf(("Trying to SetNextChore to [%s:%d] without a blender for [%s]",::ToString(ReqChore),ReqStance,GetDebugName()));
		return false;
	}

	if (!m_Blender->HasValidChore(ReqChore))
	{
		gperrorf(("Trying to SetNextChore to [%s:%d] without defining it first [%s]",::ToString(ReqChore),ReqStance,GetDebugName()));
		return false;
	}

	DWORD stanceMask = m_Blender->GetStanceMask(ReqChore);

	if ((stanceMask == 0) || (ReqChore == CHORE_MISC)) {

		// $ waterwheel, doors, etc have no stance....
		// $ as do characters playing a MISC chore

		ReqStance = AS_PLAIN;

	} else {

		if (ReqStance == AS_DEFAULT)
		{
			// We need to pick anything that will work.

			DWORD SubstituteStance = AS_PLAIN;
			// Find the lowest bit set in the mask and use it as the stance
			while ( (SubstituteStance < AS_END) && (( stanceMask && (1<<SubstituteStance)) == 0 ) )
			{
				SubstituteStance++;
			}

			if (SubstituteStance == AS_END)
			{
				gperrorf(("SetNextStance() There is no legal stance defined for aspect %s",
					GetDebugName(),::ToString(m_NextChore)));
				return false;
			}

			ReqStance = (eAnimStance)SubstituteStance;
		}
		else
		{
			if (((1<<ReqStance) & stanceMask) == 0)
			{
				// Find the lowest bit set in the mask and use it as the stance
				DWORD SubstituteStance = AS_PLAIN;
				DWORD StanceMask = stanceMask;
				while ((StanceMask & (1<<SubstituteStance)) == 0)
				{
					SubstituteStance++;
				}

				gpwarningf(("SetNextChore() Substituting stance %d for illegal stance %d while switching %s to %s",
					SubstituteStance,
					ReqStance,
					GetDebugName(),
					::ToString(ReqChore)));

				ReqStance = (eAnimStance)SubstituteStance;
			}
		}
	}

	if ( m_ChoreChangeRequested )
	{
		// We were all set to changes chores and then never got a chance
		// process the skipped chore change and make sure we don't miss any
		// important events
		if ( ( m_NextChore   != ReqChore )	 ||
			 ( m_NextStance  != ReqStance )  ||
			 ( m_NextSubAnim != ReqSubAnim ) ||
			 ( m_NextFlags   != (int)ReqFlags )
			)
		{
			SkipChoreChange();
		}
	}

	m_NextChore = ReqChore;
	m_NextStance = ReqStance;
	m_NextSubAnim = ReqSubAnim;
	m_NextFlags = ReqFlags;

	// Don't change from a WALK to an identical WALK
	m_ChoreChangeRequested |= (m_CurrChore   != CHORE_WALK)  || 
							  (m_CurrChore   != m_NextChore) ||
							  (m_CurrStance  != m_NextStance) ||
							  (m_CurrSubAnim != m_NextSubAnim) ||
							  (m_CurrFlags   != m_NextFlags);

	return true;

}
// ***************************************************
bool 
Aspect::SetNextStance(	eAnimStance ReqStance )
{
	if (m_Blender == NULL)
	{
		gperrorf(("Trying to SetNextStance() to [%s:%d] without a blender for [%s]",::ToString(m_NextChore),ReqStance,GetDebugName()));
		return false;
	}

	if (!m_Blender->HasValidChore(m_NextChore))
	{
		gperrorf(("Trying to SetNextStance() to [%s:%d] without defining it first [%s]",::ToString(m_NextChore),ReqStance,GetDebugName()));
		return false;
	}

	DWORD stanceMask = m_Blender->GetStanceMask(m_NextChore);

	if ((stanceMask == 0) || (m_NextChore == CHORE_MISC)) {

		// $ waterwheel, doors, etc have no stance....
		// $ as do characters playing a MISC chore

		ReqStance = AS_PLAIN;

	} else {

		if (ReqStance == AS_DEFAULT)
		{
			// We need to pick anything that will work.

			DWORD SubstituteStance = AS_PLAIN;
			// Find the lowest bit set in the mask and use it as the stance
			while ( (SubstituteStance < AS_END) && (( stanceMask && (1<<SubstituteStance)) == 0 ) )
			{
				SubstituteStance++;
			}

			if (SubstituteStance == AS_END)
			{
				gperrorf(("SetNextStance() There is no legal stance defined for aspect %s",
					GetDebugName(),::ToString(m_NextChore)));
				return false;
			}

			ReqStance = (eAnimStance)SubstituteStance;
		}
		else
		{
			if (((1<<ReqStance) & stanceMask) == 0)
			{

				// Find the lowest bit set in the mask and use it as the stance
				DWORD SubstituteStance = AS_PLAIN;
				DWORD StanceMask = stanceMask;
				while ((StanceMask & (1<<SubstituteStance)) == 0)
				{
					SubstituteStance++;
				}

				gpwarningf(("SetNextStance() Substituting stance %d for illegal stance %d while switching %s to %s",
					SubstituteStance,ReqStance,GetDebugName(),::ToString(m_NextChore)));

				ReqStance = (eAnimStance)SubstituteStance;

			}
		}
	}

	// Always change the cod
	m_CurrChore = CHORE_NONE;
	m_ChoreChangeRequested = true;

	return true;

	
}

// ***************************************************
bool
Aspect::ChangeCurrentChore(void) {

	if (!m_Blender) {
		gperrorf(("Trying to ChangeCurrentChore to [%s] without a blender for [%s]",::ToString(m_NextChore),GetDebugName()));
		return false;
	}
	if (!m_Blender->HasValidChore(m_NextChore)) {
		gperrorf(("Trying to ChangeCurrentChore to [%s] without defining it first for [%s]",::ToString(m_NextChore),GetDebugName()));
		return false;
	}
	if (!m_Blender->HasValidSkrit(m_NextChore)) {
		gperrorf(("Trying to ChangeCurrentChore to [%s] for [%s], but the skrit is invalid",::ToString(m_NextChore),GetDebugName()));
		return false;
	}

	if ( !m_Blender->GetSubAnimsAreLoaded(m_NextChore,m_NextStance) )
	{
#if !GP_RETAIL		
		if (!gServer.IsRemote())
		{
			gperrorf(("Trying to ChangeCurrentChore to [%s] for [%s], there are no animations available yet",::ToString(m_NextChore),GetDebugName()));
		}
#endif
		return false;
	}

	// Have we been asked to change to the same chore and its not a 'special stance'?
	// We can arrive in this state while walking and changing weapons, for example....

	bool reset_required =  (m_NextChore == CHORE_MISC)
						|| (m_NextChore != m_CurrChore)
						|| (m_NextStance != m_CurrStance);


	m_PrevChore    = m_CurrChore;
	m_PrevStance   = m_CurrStance;
	m_PrevSubAnim  = m_CurrSubAnim;
	m_PrevFlags    = m_CurrFlags;

	m_CurrChore    = m_NextChore;
	m_CurrStance   = m_NextStance;
	m_CurrSubAnim  = m_NextSubAnim;
	m_CurrFlags    = m_NextFlags;

	// Force the normals to be re-calced when we change chores
	m_DeformNormCounter = 0;

	if (reset_required)
	{
		m_Blender->ResetTimeWarp();
	}

	FuBiEvent::OnStartChore(
		m_Blender->GetSkrit(m_CurrChore),
		m_CurrSubAnim,
		m_CurrFlags
		);

	if (m_CurrFlags & ANIMFLAG_STARTATEND)
	{
		UpdateBlender(m_Blender->DurationOfCurrentTimeWarp());
	}

	return true;
}

// ***************************************************
bool
Aspect::SkipChoreChange(void)
{
	// The system is running slowly and chores are being
	// changed faster than they can be played. 
	// We need to be sure that we aren't missing any critical
	// HIDE/SHOW animation events

	m_Blender->ProcessHideShowEventsForSkippedChore(m_NextChore,m_NextStance,m_NextSubAnim);

	return true;
}


// ***************************************************
void
Aspect::CopyBonesPRS(HAspect hsource)
{

	PADDING_BUFFER_CHECK(this)
	// require that these are the same skeletons for now
	Aspect* source = hsource.Get();
	gpassert( m_shared == source->m_shared );

	if (IsPurged(2))
	{
		BITCHSLAP("CopyBonesPRS - Dest");
		return;
	}
	if (source->IsPurged(2))
	{
		BITCHSLAP("CopyBonesPRS - Source");
		return;
	}

	// copy prs info
	for (DWORD b = 0; b < m_shared->m_nbones; b++) {

		m_bones[b].m_position	= source->m_bones[b].m_position;
		m_bones[b].m_rotation	= source->m_bones[b].m_rotation;
	}

	m_TweakRequested = false;
	m_BonesInRestPose = source->m_BonesInRestPose;

	// force deformation
	bool oldUpdateEnabled = m_UpdateEnabled;
	bool oldDeformable    = m_IsDeformable;
	m_UpdateEnabled = true;
	m_IsDeformable  = true;
	m_DeformationRequested = true;
	m_DeformNormCounter = 0;
	Deform(0,0);
	m_UpdateEnabled  = oldUpdateEnabled;
	m_IsDeformable   = oldDeformable;
	PADDING_BUFFER_CHECK(this)
}

// ***************************************************
void
Aspect::EnableUpdate()	{

	m_UpdateEnabled = true;

}

// ***************************************************
void
Aspect::DisableUpdate()	{

	m_UpdateEnabled = false;

}

// ***************************************************
void 
Aspect::HandleMessage( const WorldMessage& msg ) {
	if ( m_Blender && m_Blender->HasValidSkrit(m_CurrChore) )
	{
		FuBiEvent::OnHandleMessage(
			m_Blender->GetSkrit(m_CurrChore),
			msg.GetEvent(),
			msg
			);
	}
}

// ***************************************************
bool
Aspect::UpdateAnimationStateMachine(float deltat) {

	CHECK_PRIMARY_THREAD_ONLY;

	bool updated = false;

	if (m_ChoreChangeRequested)
	{
		if (!ChangeCurrentChore())
		{
			// We failed to change the chore, we try again later
			// 
			return false;
		}
		m_ChoreChangeRequested = false;
		updated = true;
	}

	if ( m_Blender && m_Blender->HasValidSkrit(m_CurrChore) )
	{
		m_instflags = (m_Blender->GetTracerKeys()) ? (m_instflags | RENDER_TRACERS) : (m_instflags & ~RENDER_TRACERS);
		GPPROFILERSAMPLE( "Nema Aspect::UpdateAnimationStateMachine - OnUpdate", SP_DRAW | SP_DRAW_CALC_GO );
		FuBiEvent::OnUpdate( m_Blender->GetSkrit(m_CurrChore), deltat );
	}

	if ( m_Blender && m_Blender->HasValidChore(m_CurrChore) )
	{
		GPPROFILERSAMPLE( "Nema Aspect::UpdateAnimationStateMachine - AnimateBones", SP_DRAW | SP_DRAW_CALC_GO );
		if ( !IsPurged(2) && AnimateBones(deltat) )
		{
//			m_instflags = (m_Blender->GetTracerKeys()) ? (m_instflags | RENDER_TRACERS) : (m_instflags & ~RENDER_TRACERS);
			updated = true;
		}
		else if ( deltat != 0 )
		{
			// We do not want to render any tracers
			m_instflags &= ~RENDER_TRACERS;
		}
	}

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			if ( (*it).first->m_Blender )
			{
				(*it).first->UpdateAnimationStateMachine(deltat);
			}
		}
	}

	return updated;

}

// ***************************************************
bool Aspect::RestartAnimationStateMachine(void)
{
	m_CurrChore = CHORE_NONE;
	m_ChoreChangeRequested = true;
	return true;
}


// ***************************************************
bool Aspect::RefreshAnimationStateMachine(float elapsed)
{

	CHECK_PRIMARY_THREAD_ONLY;

	gpassert( (elapsed>=0.0) && (elapsed<=1.0f) );

	// Blow out the curr chore 
	m_CurrChore = CHORE_NONE;
	m_ChoreChangeRequested = true;

/*
	DISABLE!!!!

	// Send an update with a small time advance to ensure that the animation state machine is up and running
	UpdateAnimationStateMachine(0.01f);

	if ( m_Blender && m_Blender->HasValidSkrit(m_CurrChore) )
	{
		FuBiEvent::OnUpdate( m_Blender->GetSkrit(m_CurrChore), m_Blender->DurationOfCurrentTimeWarp() * elapsed);
	}

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{
			if ( (*it).first->m_Blender )
			{
				(*it).first->RefreshAnimationStateMachine(elapsed);
			}
		}
	}
*/

	return true;
}

// ***************************************************
void
Aspect::SetParent(HAspect newparent)
{
	PADDING_BUFFER_CHECK(this)

	if (newparent != m_parent)
	{
		if ( m_parent.IsValid() )
		{
			PADDING_BUFFER_CHECK(m_parent.Get())
			gAspectStorage.ReleaseAspect(m_parent);
		}
		else if (m_parent != NULL)
		{
			gperrorf(("NEMA: SetParent() error:  Existing parent [%08x] is mangled!\n",m_parent));
		}

		if ( newparent.IsValid() )
		{
			PADDING_BUFFER_CHECK(newparent.Get())
			if (ValidateLink(newparent.Get(),this))
			{
				gAspectStorage.AddRefAspect(newparent);
			}
			else
			{
				gperrorf(("NEMA: SetParent failed: ValidateLink() failed! Setting parent to NULL\n"));
				newparent = HAspect();
			}
		}
		else if (newparent != NULL)
		{
			gperrorf(("NEMA: SetParent failed: New parent [%08x] is mangled! Setting parent to NULL\n",newparent));
			newparent = HAspect();
		}

		m_parent = newparent;
	}
	else if ( (m_parent != NULL) && !m_parent.IsValid() )
	{
		gperrorf(("NEMA: SetParent() error:  Existing parent [%08x] is mangled AND matches new parent!\n",m_parent));
		m_parent = HAspect();
	}

}

// ***************************************************
bool
Aspect::AttachRigidLinkedChild( HAspect hchild,
								const char* parentbonename,
								const char* childbonename,
								const Quat& offsetrot,
								const vector_3& offsetpos,
								bool hastracers
								) {

	if (!hchild.IsValid()) {
		gperrorf((
			"NEMA: AttachRigidLinkedChild() Attempted to attach an invalid child to [%s g:%08x s:%08x]",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
			));
		return false;
	}

	if (m_childlinks.find(hchild) != m_childlinks.end()) {
		gperrorf(("Child %s is already attached to parent %s",hchild->GetDebugName(),GetDebugName()));
		return false;
	}

	if (hchild == m_handle) {
		gperrorf(("Attempted to attach rigid linked child %s to itself",GetDebugName()));
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(hchild.Get())

	Aspect* child = hchild.Get();

	if (!ValidateLink(this,child))
	{
		return false;
	}

	if (child->GetParent().IsValid()) {
		gperrorf(("Child %s is already attached to %s, can't attach to %s",child->GetDebugName(),child->GetParent()->GetDebugName(),GetDebugName()));
		return false;
	}

	// Determine which child bone we are attaching to which parent bone
	// TODO get rid of the string comparison here, I should hash it or something

	int parentbone = -1;
	int childbone = -1;

	// $ Could store a sorted lookup of name to bone index rather than interate --biddle

	if (parentbonename && strlen(parentbonename)>0) {
		for (DWORD b = 0; b < m_shared->m_nbones; b++  ) {
			if (stricmp (m_shared->m_restbones[b].m_name,parentbonename) == 0 ) {
				parentbone = b;
				break;
			}
		}
	} else {
		parentbone = 0;
	}

	if (childbonename && strlen(childbonename)>0) {
		for (DWORD b = 0; b < child->m_shared->m_nbones; b++ ) {
			if (stricmp (child->m_shared->m_restbones[b].m_name,childbonename) == 0 ) {
				childbone = b;
				break;
			}
		}
	} else {
		childbone = 0;
	}

	if (parentbone < 0 || (DWORD)parentbone >= m_shared->m_nbones) {
		gpwarningf(("Invalid parent bone name\n%s has no %s",m_dbgname.c_str(),parentbonename));
		parentbone = 0;
	}

	if (childbone < 0 || (DWORD)childbone >= child->m_shared->m_nbones) {
		gpwarningf(("Invalid child bone name\n%s has no %s",child->m_dbgname.c_str(),childbonename));
		childbone = 0;
	}

	child->SetParent(m_handle);
	gAspectStorage.AddRefAspect(hchild);

	sBoneLink sbl(parentbone,childbone);
	if (hastracers)
	{
		sbl.SetHasWeaponTracerFlag();
	}
	m_childlinks[hchild] = sbl;

	child->m_attachtype = RIGID_LINKED;
	child->m_linkBone = childbone;

	// aug27/00 For a rigid attachment we need to set (and lock) the relative position of the child bone
	// once we lock them we can get rid of thse LinkOffset (if we can/need to)
	child->SetLinkOffsetRotation(offsetrot);
	child->SetLinkOffsetPosition(offsetpos);

	child->m_UpdateEnabled = true;

	child->m_BonesInRestPose = false;
	child->m_TweakRequested = true;

	if ( !IsPurged(2) && !child->IsPurged(2) )
	{
		child->m_bones[childbone].m_parentBone = &m_bones[parentbone];
		child->AnimateBones(0);
		if (m_Blender && hastracers)
		{
			// Do we have tracers to render?
			CalculateWeaponTracerPoints(child);
		}
	}

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(hchild.Get())

	child->m_DeformNormCounter = 0;

	m_DeformationRequested = true;

	return true;
}

// ***************************************************
bool
Aspect::AttachReverseLinkedChild(
								HAspect hchild,
								const char* parentbonename,
								const char* childbonename,
								const Quat& offsetrot,
								const vector_3& offsetpos
								) {

	if (!hchild.IsValid()) {
		gperrorf((
			"NEMA: AttachReverseLinkedChild() Attempted to attach an invalid child to [%s g:%08x s:%08x]",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
			));
		return false;
	}

	if (m_childlinks.find(hchild) != m_childlinks.end()) {
		gperrorf(("Child %s is already attached to parent %s",hchild->GetDebugName(),GetDebugName()));
		return false;
	}

	if (hchild == m_handle) {
		gperrorf(("Attempted to attach reverse linked child %s to itself",GetDebugName()));
		return false;
	}

	Aspect* child = hchild.Get();

	if (!ValidateLink(this,child))
	{
		return false;
	}
	
	if (child->GetParent().IsValid()) {
		gperrorf(("Child %s is already attached to %s, can't attach to %s",child->GetDebugName(),child->GetParent()->GetDebugName(),GetDebugName()));
		return false;
	}

	if (IsPurged(2)) {
		gperrorf(("%s is purged or has not been reconstituted, can't attach %s",GetDebugName(), child->GetDebugName()));
		return false;
	}

	if (child->IsPurged(2)) {
		gperrorf(("%s is purged or has not been reconstituted, can't attach to %s",child->GetDebugName(), GetDebugName()));
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(hchild.Get())

	// Determine which child bone we are attaching to which parent bone
	// TODO get rid of the string comparison here, I should hash it or something

	int parentbone = -1;
	int childbone = -1;

	if (parentbonename && strlen(parentbonename)>0) {
		for (DWORD b = 0; b < m_shared->m_nbones; b++  ) {
			if (stricmp (m_shared->m_restbones[b].m_name,parentbonename) == 0 ) {
				parentbone = b;
				break;
			}
		}
	} else {
		parentbone = 0;
	}

	if (childbonename && strlen(childbonename)>0) {
		for (DWORD b = 0; b < child->m_shared->m_nbones; b++ ) {
			if (stricmp (child->m_shared->m_restbones[b].m_name,childbonename) == 0 ) {
				childbone = b;
				break;
			}
		}
	} else {
		childbone = 0;
	}

	if (parentbone < 0 || (DWORD)parentbone >= m_shared->m_nbones) {
		gpwarningf(("Invalid parent bone name\n%s has no %s",m_dbgname.c_str(),parentbonename));
		parentbone = 0;
	}

	if (childbone < 0 || (DWORD)childbone >= child->m_shared->m_nbones) {
		gpwarningf(("Invalid child bone name\n%s has no %s",child->m_dbgname.c_str(),childbonename));
		childbone = 0;
	}

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	child->SetParent(m_handle);
	gAspectStorage.AddRefAspect(hchild);

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)
	// Attach the ROOT of the child to the parent, offset by the inverse of the child bone

	// Normally NEMA to SIEGE space conversion is done in the just once upon loading.
	// Since we are dynamically modifying the root (and assigning it a Relative controller)
	// We Must remember to apply the PreRotateNEMAtoSEIGE fixup to ensure that we have
	// everything aligned properly


	// TODO:: Keep a careful watch on whether or not these formulas are correct -- biddle
	// Look for items getting twisted around in the hands of characters

	Quat r ;

	if (m_shared->m_MajorVersion<2) {

		r = (
			PreRotateNEMAtoSEIGE
			* child->m_shared->m_restbones[childbone].m_invRestRot
			* child->m_shared->m_restbones[0].m_relRestRot
			);

	} else {

		r = offsetrot *
			child->m_shared->m_restbones[childbone].m_invRestRot *
			child->m_shared->m_restbones[0].m_relRestRot;

	}

	vector_3 v( DoNotInitialize );
	HadamardProduct(
		v,
		child->m_shared->m_restbones[childbone].m_invRestPos + child->m_shared->m_restbones[0].m_relRestPos,
		child->m_Scale);

	offsetrot.RotateVector_T(v);
	v += offsetpos;

	child->SetLinkOffsetRotation(r);
	child->SetLinkOffsetPosition(v);

	child->m_linkBone = 0;
	m_childlinks[hchild] = sBoneLink(parentbone,0);

	child->m_attachtype = REVERSE_RIGID_LINKED;
	child->m_UpdateEnabled = true;

	child->m_BonesInRestPose = false;
	child->m_TweakRequested = true;

	child->m_bones[0].m_parentBone = &m_bones[parentbone];
	child->AnimateBones(0);
	
	child->m_DeformNormCounter = 0;

	m_DeformationRequested = true;

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	return true;
}



// ***************************************************
bool
Aspect::AttachDeformableChild(HAspect hchild) {

	if (!hchild.IsValid()) {
		gperrorf((
			"NEMA: AttachDeformableChild() Attempted to attach an invalid deformable child to [%s g:%08x s:%08x]",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
			));
		return false;
	}

	if (m_childlinks.find(hchild) != m_childlinks.end()) {
		gperrorf(("Child %s is already attached to parent %s",hchild->GetDebugName(),GetDebugName()));
		return false;
	}

	if (hchild == m_handle) {
		gperrorf(("Attempted to attach deformable child %s to itself",GetDebugName()));
		return false;
	}

	Aspect* child = hchild.Get();

	if (!ValidateLink(this,child))
	{
		return false;
	}
	
	if (child->GetParent().IsValid()) {
		gperrorf(("Child %s is already attached to %s, can't attach to %s",child->GetDebugName(),child->GetParent()->GetDebugName(),GetDebugName()));
		return false;
	}

	if (child->m_shared->m_primarybone >= 0) {
		gperrorf(("Unable to attach deformable aspect %s. Aspect is not deformable (Bad art, missing Physique?)",child->GetDebugName()));
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	if (child->m_shared->m_nbones != m_shared->m_nbones)
	{
		gperrorf(("ART ERROR: Child and Parent skeletons are different\n\tAttempted to attach deformable [%s] with %d bones to [%s] with %d bones",
			child->GetDebugName(),
			child->m_shared->m_nbones,
			GetDebugName(),
			m_shared->m_nbones
			));


		return false;
	}

	child->SetParent(m_handle);

	for (DWORD b = 0; b < child->m_shared->m_nbones; b++ ) {
		if (stricmp (m_shared->m_restbones[b].m_name,child->m_shared->m_restbones[b].m_name) != 0 )
		{
			gperrorf(("ART ERROR: Child and Parent skeletons have missmatched bone #%d "
					  "Attempted to attach [%s:%s] to [%s:%s]",
			b,
			child->GetDebugName(),
			child->m_shared->m_restbones[b].m_name,
			GetDebugName(),
			m_shared->m_restbones[b].m_name
			));
		}
	}

	m_childlinks[hchild] = sBoneLink();
	gAspectStorage.AddRefAspect(hchild);

	child->m_instflags &= ~STITCH_VERTS;

	for (DWORD ps = 0; ps < m_shared->m_nstitchsets[0]; ++ps) {
		for (DWORD subm = 0; subm < child->GetNumMeshes(); subm++) {
			for (DWORD cs = 0; cs < child->m_shared->m_nstitchsets[subm]; ++cs) {
				if (m_shared->m_stitchset[0][ps].m_tag == child->m_shared->m_stitchset[subm][cs].m_tag) {
					child->m_instflags |= STITCH_VERTS;
					break;
				}
			}
		}
	}

	child->m_attachtype = DEFORMABLE_OVERLAY;
	child->m_linkBone = 0;
	child->m_IsDeformable = true;
	child->m_UpdateEnabled = true;

	child->SetLinkOffsetRotation(Quat::IDENTITY);
	child->SetLinkOffsetPosition(vector_3::ZERO);	// $$ could/should allow an offset to be included in params --biddle

	// Force an update (with no time advance) to set the linked position of the child
	m_DeformationRequested = true;
	child->m_TweakRequested = true;

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	if ( !IsPurged(2) && !child->IsPurged(2))
	{
		for (DWORD b = 0; b < child->m_shared->m_nbones; b++ )
		{
			child->m_bones[b].m_parentBone = &m_bones[b];
		}
		child->AnimateBones(0);
	}

	child->m_DeformNormCounter = 0;

	child->m_BonesInRestPose = false;

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	return true;
}

// ***************************************************
bool
Aspect::DetachAllChildren(void) {

#if GP_DEBUG
	int dbg_count = 0;
#endif

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end();) {

#if GP_DEBUG
		dbg_count++;
#endif

		if ((*it).first.IsValid())
		{

		// Reset all the children's channels that point to this aspect
		// to their original controllers/parents

			HAspect hchild = (*it).first;
			Aspect* child = hchild.Get();

			PADDING_BUFFER_CHECK(this)

			if ( !child->IsPurged(2) )
			{
				PADDING_BUFFER_CHECK(child)

				if ((*it).second.IsFullyLinked())
				{
					for (DWORD c = 0; c < child->m_shared->m_nbones; c++ )
					{
						child->m_bones[c].m_parentBone = &child->m_bones[child->m_shared->m_restbones[c].m_parentBoneIndex];
						child->m_bones[c].m_position = child->m_bones[c].m_sharedbone->m_relRestPos;
						child->m_bones[c].m_rotation = child->m_bones[c].m_sharedbone->m_relRestRot;
					}
				}
				else
				{
					DWORD c = (*it).second.ChildIndex();
					child->m_bones[c].m_parentBone = &child->m_bones[child->m_shared->m_restbones[c].m_parentBoneIndex];
					child->m_bones[c].m_position = child->m_bones[c].m_sharedbone->m_relRestPos;
					child->m_bones[c].m_rotation = child->m_bones[c].m_sharedbone->m_relRestRot;
				}
			}

			// Reset the offsets to identity
			child->SetLinkOffsetRotation(Quat::IDENTITY);
			child->SetLinkOffsetPosition(vector_3::ZERO);

			child->m_attachtype = UNATTACHED;
			child->m_linkBone = 0;

			child->m_instflags &= ~STITCH_VERTS; // No need to stitch anymore

			child->m_DeformationRequested = child->IsDeformable();

			PADDING_BUFFER_CHECK(this)

			it = m_childlinks.erase(it);

			child->SetParent(HAspect());

			PADDING_BUFFER_CHECK(this)

			// Restore the bones to the rest position
			if ( !child->IsPurged(2) )
			{
				PADDING_BUFFER_CHECK(child)
				child->m_TweakRequested = true;
				child->AnimateBones(0);
				child->m_BonesInRestPose = true;
				PADDING_BUFFER_CHECK(child)
			}
			child->m_DeformNormCounter = 0;

			PADDING_BUFFER_CHECK(this)

			gAspectStorage.ReleaseAspect(hchild);

			PADDING_BUFFER_CHECK(this)


		}
		else
		{
			gperrorf((
				"NEMA: DetachAllChildren() Encountered invalid child while deleting all children of [%s g:%08x s:%08x]",
				GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
				m_owner_goid,
				GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
				));
			it = m_childlinks.erase(it);
		}

	}

	gpassertm(it == m_childlinks.end(),"You were unable to detach all children");

	return true;
}

// ***************************************************
bool
Aspect::DetachChild(HAspect hchild) {

	if (!hchild.IsValid())
	{
		gperrorf((
			"NEMA: DetachChild() Attempted to detach an invalid child from [%s g:%08x s:%08x]",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
			));
		return false;
	}

	ChildLinkIter it = m_childlinks.find(hchild);
	Aspect* child = hchild.Get();

	PADDING_BUFFER_CHECK(this)
	PADDING_BUFFER_CHECK(child)

	if (it != m_childlinks.end()) {

		// Reset all the bones that point to bones on this aspect
		// to their original bones

		if ( !child->IsPurged(2) )
		{
			if ((*it).second.IsFullyLinked())
			{
				for (DWORD c = 0; c < child->m_shared->m_nbones; c++ )
				{
					// Is this child bone attached to any parent?
					child->m_bones[c].m_parentBone = &child->m_bones[child->m_shared->m_restbones[c].m_parentBoneIndex];
					child->m_bones[c].m_position = child->m_bones[c].m_sharedbone->m_relRestPos;
					child->m_bones[c].m_rotation = child->m_bones[c].m_sharedbone->m_relRestRot;
				}
			}
			else
			{
				DWORD c = (*it).second.ChildIndex();
				child->m_bones[c].m_parentBone = &child->m_bones[child->m_shared->m_restbones[c].m_parentBoneIndex];
				child->m_bones[c].m_position = child->m_bones[c].m_sharedbone->m_relRestPos;
				child->m_bones[c].m_rotation = child->m_bones[c].m_sharedbone->m_relRestRot;
			}
		}

		// Reset the offsets to identity
		child->SetLinkOffsetRotation(Quat::IDENTITY);
		child->SetLinkOffsetPosition(vector_3::ZERO);

		child->m_attachtype = UNATTACHED;
		child->m_linkBone = 0;

		child->m_instflags &= ~STITCH_VERTS; // No need to stitch anymore

		child->m_DeformationRequested = child->IsDeformable();

		m_childlinks.erase(it);

		child->SetParent(HAspect());

		PADDING_BUFFER_CHECK(this)
		PADDING_BUFFER_CHECK(child)

		// Restore the bones to the rest position
		if ( !child->IsPurged(2) )
		{
			child->m_TweakRequested = true;
			child->AnimateBones(0);
			child->m_BonesInRestPose = true;
		}

		child->m_DeformNormCounter = 0;

		gAspectStorage.ReleaseAspect(hchild);

		PADDING_BUFFER_CHECK(this)
		PADDING_BUFFER_CHECK(child)

	}
	else
	{
		gperrorf((
			"NEMA: DetachChild() [%s g:%08x s:%08x] does not have [%s g:%08x s:%08x] attached",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : m_dbgname.c_str(),
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
			GetGo(child->m_owner_goid) ? GetGo(child->m_owner_goid)->GetTemplateName() : child->m_dbgname.c_str(),
			child->m_owner_goid,
			GetGo(child->m_owner_goid) ? MakeInt(GetGo(child->m_owner_goid)->GetScid()) : -1
			));
	}

	return true;
}

//***********************************************************************************
bool
Aspect::LockPrimaryBone(const vector_3& bone_position, const Quat& bone_rotation) {

	if (IsPurged(2))
	{
		BITCHSLAP("LockPrimaryBone");
		
		gpwarningf(("LockPrimaryBone WARNING: Tried to lock the primary bone of [%s g:%08x s:%08x] with purged aspect",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1
			));
		return false;
	}

	int pb = m_shared->m_primarybone;
	if (pb < 0) pb = 0;

	const vector_3& pos = m_bones[pb].m_sharedbone->m_relRestPos;
	Quat rot = m_bones[pb].m_sharedbone->m_relRestRot;

	if ((m_shared->m_MajorVersion<2) && pb == 0) {
		// Special case the root bone to handle the NEMAtoSIEGE space conversion
		rot = PreRotateNEMAtoSEIGE.Inverse() * rot;
		rot.RotateVector(m_bones[pb].m_position,bone_position);
		m_bones[pb].m_position += pos;
		m_bones[pb].m_rotation = (rot * bone_rotation);
	} else {
		m_bones[pb].m_position = bone_position;
		m_bones[pb].m_rotation = bone_rotation;
	}

	m_PrimaryBoneIsLocked = true;

	PADDING_BUFFER_CHECK(this)

	// The bones are no longer in the rest pose
	m_BonesInRestPose = false;
	m_TweakRequested = true;
	AnimateBones(0);

	PADDING_BUFFER_CHECK(this)
	
	m_DeformationRequested = IsDeformable;
	m_DeformNormCounter = 0;

	return true;
}

//***********************************************************************************
bool
Aspect::UnlockPrimaryBone(void) {

	if (IsPurged(2))
	{
		BITCHSLAP("UnlockPrimaryBone");
		return false;
	}

	gpassertm(m_PrimaryBoneIsLocked , "Attempting to unlock an aspect that is not locked");

	if (!m_PrimaryBoneIsLocked) return true;

	m_PrimaryBoneIsLocked = false;

	m_TweakRequested = true;
	PADDING_BUFFER_CHECK(this)
	AnimateBones(0);
	PADDING_BUFFER_CHECK(this)

	return true;
}

//***********************************************************************************
void
Aspect::GetOrientedBoundingBox(
	Quat& orient,
	vector_3& center,
	vector_3& half_diag) const {

#if !GP_RETAIL
	if (IsBadReadPtr(this,4))
	{
		gperror("NEMA ERROR: GetOrientedBoundingBox() aspect is not initialized");
	}
	else if (IsBadReadPtr(m_shared,4))
	{
		gperror("NEMA ERROR: GetOrientedBoundingBox() SHARED impl pointer is not valid");
	}
#endif

	if (m_shared->m_primarybone < 0) {

		orient = Quat::IDENTITY;
		center = m_BBoxCenter;
		half_diag = m_BBoxHalfDiag;

	} else {

		int pb = m_shared->m_primarybone;

		if (IsPurged(2))
		{
			BITCHSLAP("GetOrientedBoundingBox");

			orient = Quat::IDENTITY;
			center = m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			orient = m_bones[pb].m_rotation;

			m_bones[pb].m_rotation.RotateVector(center,m_BBoxCenter);
			center += m_bones[pb].m_position;
		}

		half_diag = m_BBoxHalfDiag;

	}

}

//***********************************************************************************
void
Aspect::GetOrientedBoundingBoxCenter( vector_3& center ) const
{
	if (m_shared->m_primarybone < 0)
	{
		center = m_BBoxCenter;
	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("GetOrientedBoundingBoxCenter");
			center = m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			int pb = m_shared->m_primarybone;
			m_bones[pb].m_rotation.RotateVector(center,m_BBoxCenter);
			center += m_bones[pb].m_position;
		}
	}
}

//***********************************************************************************
void
Aspect::GetBoundingSphere(vector_3& center,float& radius) const
{
	radius = m_BSphereRadius;
	if (m_shared->m_primarybone < 0)
	{
	   center = m_BBoxCenter;
	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("GetBoundingSphere");
			center = m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			int pb = m_shared->m_primarybone;
			m_bones[pb].m_rotation.RotateVector(center,m_BBoxCenter);
			center += m_bones[pb].m_position;
		}
	}
}

//***********************************************************************************
vector_3
Aspect::GetBoundingSphereCenter(void) const
{
	if (m_shared->m_primarybone < 0)
	{
	   return m_BBoxCenter;
	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("GetBoundingSphereCenter");
			return m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			int pb = m_shared->m_primarybone;
			return (m_bones[pb].m_position + m_bones[pb].m_rotation.RotateVector_T(m_BBoxCenter));
		}
	}
}

//***********************************************************************************
void
Aspect::GetBoundingBox(vector_3& center,vector_3& half_diag) const
{
	half_diag = m_BBoxHalfDiag;
	if (m_shared->m_primarybone < 0)
	{
		center = m_BBoxCenter;
	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("GetBoundingBox");
			center = m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			int pb = m_shared->m_primarybone;
			m_bones[pb].m_rotation.RotateVector(center,m_BBoxCenter);
			center += m_bones[pb].m_position;
		}
	}
}

//***********************************************************************************
vector_3
Aspect:: GetBoundingBoxCenter(void) const
{
	if (m_shared->m_primarybone < 0)
	{
		return m_BBoxCenter;
	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("GetBoundingBoxCenter");
			return m_BBoxCenter;
		}
		else
		{
			PADDING_BUFFER_CHECK(this)

			int pb = m_shared->m_primarybone;
			return (m_bones[pb].m_position + m_bones[pb].m_rotation.RotateVector_T(m_BBoxCenter));
		}
	}
}

//***********************************************************************************
void
Aspect::GetBoundingSphereIncludingChildren( vector_3& center, float& radius, float scalefactor ) const
{

	bool parent_funky_scale = m_Scale != vector_3(1,1,1);

	int ppb = m_shared->m_primarybone;

	if (IsPurged(2))
	{
		BITCHSLAP("GetBoundingSphereIncludingChildren");
		radius = m_BSphereRadius * scalefactor;
		center = m_BBoxCenter * scalefactor;
		return ;
	}
	PADDING_BUFFER_CHECK(this)


	if (parent_funky_scale)
	{
		if (ppb < 0)
		{
			vector_3 temp(DoNotInitialize);
			HadamardProduct(temp,m_BBoxHalfDiag,m_Scale);
			radius = temp.Length() * scalefactor;
			HadamardProduct(center,m_BBoxCenter,m_Scale);
			center *= scalefactor;
		}
		else
		{
			vector_3 temp(DoNotInitialize);
			HadamardProduct(temp,m_BBoxHalfDiag,m_Scale);
			radius = temp.Length() * scalefactor;
			HadamardProduct(center,m_BBoxCenter,m_Scale);
			m_bones[ppb].m_rotation.RotateVectorInPlace(center);
			center += m_bones[ppb].m_position;
			center *= scalefactor;
		}
	}
	else
	{
		if (ppb < 0)
		{
			radius = m_BSphereRadius * scalefactor;
			center = m_BBoxCenter * scalefactor;
		}
		else
		{
			radius = m_BSphereRadius * scalefactor;
			m_bones[ppb].m_rotation.RotateVector(center,m_BBoxCenter);
			center += m_bones[ppb].m_position;
		}
	}

	vector_3 group_min = center - radius;
	vector_3 group_max = center + radius;

	for (ChildLinkConstIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		if ((*it).first.IsValid())
		{

			HAspect hkid = (*it).first;
			const Aspect& kid = *hkid.Get();

			// Using the radius of the child box rather than orienting all
			// eight corners of its bbox and checking for the max/min.
			// Results in a bsphere that is larger that it needs to be

			bool kid_funky_scale = kid.m_Scale != vector_3(1,1,1);

			float kid_radius;
			vector_3 kid_center(DoNotInitialize);

			if (kid_funky_scale)
			{
				vector_3 temp(DoNotInitialize);
				HadamardProduct(temp,kid.m_BBoxHalfDiag,kid.m_Scale);
				kid_radius = temp.Length();
				HadamardProduct(kid_center,kid.m_BBoxCenter,kid.m_Scale);
			}
			else
			{
				kid_radius = kid.m_BSphereRadius;
				kid_center = kid.m_BBoxCenter;
			}

			if (kid.m_attachtype == RIGID_LINKED || kid.m_attachtype == REVERSE_RIGID_LINKED)
			{
				if (parent_funky_scale)
				{
					if (ppb < 0)
					{
						ppb = m_linkBone;
					}

					// Need to scale the linking bone position in the parent's
					// primary bone frame
//					vector_3 temp1(DoNotInitialize);

					kid_center -= m_bones[ppb].m_position;
					m_bones[ppb].m_rotation.Inverse().RotateVectorInPlace(kid_center);
					kid_center *= m_Scale;
					m_bones[ppb].m_rotation.RotateVectorInPlace(kid_center);
					kid_center += m_bones[ppb].m_parentBone->m_position;

				}
			}

			kid_radius *= scalefactor;
			kid_center *= scalefactor;

			vector_3 kid_min = kid_center - kid_radius;
			vector_3 kid_max = kid_center + kid_radius;

			group_min.x = (group_min.x < kid_min.x) ? group_min.x : kid_min.x;
			group_min.y = (group_min.y < kid_min.y) ? group_min.y : kid_min.y;
			group_min.z = (group_min.z < kid_min.z) ? group_min.z : kid_min.z;

			group_max.x = (group_max.x > kid_max.x) ? group_max.x : kid_max.x;
			group_max.y = (group_max.y > kid_max.y) ? group_max.y : kid_max.y;
			group_max.z = (group_max.z > kid_max.z) ? group_max.z : kid_max.z;

		}
	}

	center = (group_max + group_min) * 0.5f;
	radius = (group_max - group_min).Length() * 0.5f;

}

//***********************************************************************************
bool
Aspect::GetOrientedBoneBoundingBox(
	DWORD	boneIndex,
	Quat& orient,
	vector_3& center,
	vector_3& half_diag) const {

	if (IsPurged(2))
	{
		BITCHSLAP("GetOrientedBoneBoundingBox");
		orient = Quat::IDENTITY;
		half_diag = vector_3::ZERO;
		center = vector_3::ZERO;
		return false;
	}

	PADDING_BUFFER_CHECK(this)

	gpassertf(boneIndex < m_shared->m_nbones,("Invalid bone index %d passed to GetOrientedBoneBoundingBox() of %s",boneIndex,GetDebugName()));

	if (boneIndex >= m_shared->m_nbones) {
		return false;
	}

	if (!m_shared->m_restbones[boneIndex].m_bboxExists) {
		return false;
	}

	orient =  m_bones[boneIndex].m_rotation;

	m_bones[boneIndex].m_rotation.RotateVector(center,m_shared->m_restbones[boneIndex].m_bboxCenter);
	center += m_bones[boneIndex].m_position;

	half_diag=  m_shared->m_restbones[boneIndex].m_bboxHalfDiag;


	return true;
}

//***********************************************************************************
bool
Aspect::GetOrientedBoneBoundingBoxCenter(
	DWORD	boneIndex,
	vector_3& center) const {

	if (IsPurged(2))
	{
		BITCHSLAP("GetOrientedBoneBoundingBoxCenter");
		center = vector_3();
		return false;
	}

	PADDING_BUFFER_CHECK(this)

	gpassertf(boneIndex < m_shared->m_nbones,("Invalid bone index %d passed to GetOrientedBoneBoundingBox() of %s",boneIndex,GetDebugName()));

	if (boneIndex >= m_shared->m_nbones) {
		return false;
	}

	if (!m_shared->m_restbones[boneIndex].m_bboxExists) {
		return false;
	}

	m_bones[boneIndex].m_rotation.RotateVector(center,m_shared->m_restbones[boneIndex].m_bboxCenter);
	center += m_bones[boneIndex].m_position;


	return true;
}

//***********************************************************************************
void
Aspect::GetTaggedBBox(
			Quat& orient,
			vector_3& center,
			vector_3& half_diag,
			DWORD tag) const
{

	stdx::linear_map< DWORD,BoundingBox >::iterator located
			= m_shared->m_boundingboxes.find(tag);

	if (located != m_shared->m_boundingboxes.end())
	{
		const BoundingBox& b = (*located).second;
		orient    = b.orientation;
		center    = b.offset;
		half_diag = b.halfdiag;
		return;
	}

	PADDING_BUFFER_CHECK(this)

	// There is no specific bbox defined, just return the calculated bbox
	if (m_shared->m_primarybone < 0) {
		orient = Quat::IDENTITY;
	} else {

		if (IsPurged(2))
		{
			BITCHSLAP("GetTaggedBBox");
			orient = Quat::IDENTITY;
		}
		else
		{
			orient = m_bones[m_shared->m_primarybone].m_rotation;
		}

	}

	HadamardProduct(center,m_BBoxCenter,m_Scale);
	HadamardProduct(half_diag,m_BBoxHalfDiag,m_Scale);

}


//***********************************************************************************
void
Aspect::GetTaggedBBoxCenter(
			vector_3& center,
			DWORD tag) const
{

	PADDING_BUFFER_CHECK(this)
	stdx::linear_map< DWORD,BoundingBox >::iterator located
			= m_shared->m_boundingboxes.find(tag);

	if (located != m_shared->m_boundingboxes.end())
	{
		const BoundingBox& b = (*located).second;
		HadamardProduct(center,b.offset,m_Scale);
		return;
	}

	HadamardProduct(center,m_BBoxCenter,m_Scale);

}

//***********************************************************************************
HAspect
Aspect::GetParent()
{
	if (m_parent.IsValid())
	{
		return m_parent;
	}
	else
	{
		if (m_parent != NULL)
		{
			gperrorf((
			"NEMA:GetParent() [%s g:%08x s:%08x] has a mangled parent, [%08x] is not valid!\n",
			GetGo(m_owner_goid) ? GetGo(m_owner_goid)->GetTemplateName() : "<UNKNOWN GO>",
			m_owner_goid,
			GetGo(m_owner_goid) ? MakeInt(GetGo(m_owner_goid)->GetScid()) : -1,
			m_parent
			));
		}
		return HAspect();
	}
}

//***********************************************************************************
HAspect
Aspect::GetChild(DWORD k)
{
	PADDING_BUFFER_CHECK(this)
	if (k+1 > m_childlinks.size())
	{
		return HAspect();
	}
	PADDING_BUFFER_CHECK(m_childlinks.at(k).first.Get())
	return m_childlinks.at(k).first;
}

//***********************************************************************************
void
Aspect::GetCombinedScale(vector_3 &s) const
{
	PADDING_BUFFER_CHECK(this)
	if (m_parent)
	{
		vector_3 parentscale(DoNotInitialize);
		m_parent->GetCombinedScale(parentscale);
		HadamardProduct(s,parentscale,m_Scale);
	}
	else
	{
		s = m_Scale;
	}
}


//***********************************************************************************
bool
Aspect::GetBoneOrientation(
	char const * const bone_name,
	vector_3& bone_position,
	Quat& bone_rotation) const {

	if (IsPurged(2))
	{
		BITCHSLAP("GetBoneOrientation");
		bone_position = vector_3::ZERO;
		bone_rotation = Quat::IDENTITY;
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	// The string comparison here is a lousy way of looking for bone....

	for (DWORD b = 0; b < m_shared->m_nbones; b++  ) {
		if (stricmp (bone_name,m_bones[b].m_sharedbone->m_name) == 0 ) {

			bone_rotation = m_bones[b].m_rotation;
			bone_position = m_bones[b].m_position;

			int pb = m_shared->m_primarybone;

			if ((( pb != (int)b ) && (m_Scale != vector_3(1,1,1)) ))
			{
				if (pb < 0)
				{
					if( !m_parent )
					{
						HadamardProduct(bone_position,bone_position,m_Scale);
					}
					else
					{
						bone_position -= m_bones[0].m_position;
						m_bones[0].m_rotation.Inverse().RotateVectorInPlace(bone_position);
						bone_position *= m_Scale;
						m_bones[0].m_rotation.RotateVectorInPlace(bone_position);
						bone_position += m_bones[0].m_position;
					}
				}
				else
				{
					bone_position -= m_bones[pb].m_position;
					m_bones[pb].m_rotation.Inverse().RotateVectorInPlace(bone_position);
					bone_position *= m_Scale;
					m_bones[pb].m_rotation.RotateVectorInPlace(bone_position);
					bone_position += m_bones[pb].m_position;
				}
			}

			return true;
		}
	}


	// We cannot locate a bone by name, but we must return 'something'

	#if GP_DEBUG
	{
	gpstring err = "Bad Bone Name: " + m_dbgname + " has no " + bone_name;
	gpassertm(0,err.c_str());
	}
	#endif

	// Give back the position of bone 0
	if ( m_shared->m_nbones > 0) {

		bone_rotation = m_bones[0].m_rotation;
		bone_position = m_bones[0].m_position;

		DWORD pb = m_shared->m_primarybone;

		if ( (( pb != (int)b ) && (m_Scale != vector_3(1,1,1)) ))
		{
			if (pb < 0)
			{
				if( !m_parent )
				{
					HadamardProduct(bone_position,bone_position,m_Scale);
				}
				else
				{
					bone_position -= m_bones[0].m_position;
					m_bones[0].m_rotation.Inverse().RotateVectorInPlace(bone_position);
					bone_position *= m_Scale;
					m_bones[0].m_rotation.RotateVectorInPlace(bone_position);
					bone_position += m_bones[0].m_position;
				}
			}
			else
			{
				bone_position -= m_bones[pb].m_position;
				m_bones[pb].m_rotation.Inverse().RotateVectorInPlace(bone_position);
				bone_position *= m_Scale;
				m_bones[pb].m_rotation.RotateVectorInPlace(bone_position);
				bone_position += m_bones[pb].m_position;
			}
		}

		// TODO:: This should be FALSE! need to return true to fake out
		// callers  I will fix this -- biddle
		return true;

	}

	bone_position = vector_3();

	bone_rotation = matrix_3x3::IDENTITY;

	return false;

}

//***********************************************************************************
bool
Aspect::GetIndexedBoneOrientation(
	int bone_index,
	vector_3& bone_position,
	Quat& bone_rotation) const {

	if (IsPurged(2))
	{
		BITCHSLAP("GetIndexedBoneOrientation");
		bone_position = vector_3::ZERO;
		bone_rotation = Quat::IDENTITY;
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	#if GP_DEBUG
	{

	gpassertm(m_shared->m_nbones > 0,"No bones in mesh");

	gpassertm(bone_index >= 0 && (DWORD)bone_index < m_shared->m_nbones ,"Bad Bone Index");

	}
	#endif

	if (m_shared->m_nbones > 0) {

		if ((DWORD)bone_index >= m_shared->m_nbones || bone_index < 0) {
			// TODO:: This should just return FALSE!
			// need to return Bone0 && true to fake out callers
			// I will be removing this -- biddle
			bone_index = 0;
		}

		bone_rotation = m_bones[bone_index].m_rotation;
		bone_position = m_bones[bone_index].m_position;

		int pb = m_shared->m_primarybone;

		if ( (( pb != bone_index ) && (m_Scale != vector_3(1,1,1)) ))
		{
			if (pb < 0)
			{
				if( !m_parent )
				{
					HadamardProduct(bone_position,bone_position,m_Scale);
				}
				else
				{
					bone_position -= m_bones[0].m_position;
					m_bones[0].m_rotation.Inverse().RotateVectorInPlace(bone_position);
					bone_position *= m_Scale;
					m_bones[0].m_rotation.RotateVectorInPlace(bone_position);
					bone_position += m_bones[0].m_position;
				}
			}
			else
			{
				bone_position -= m_bones[pb].m_position;
				m_bones[pb].m_rotation.Inverse().RotateVectorInPlace(bone_position);
				bone_position *= m_Scale;
				m_bones[pb].m_rotation.RotateVectorInPlace(bone_position);
				bone_position += m_bones[pb].m_position;
			}
		}

		return true;

	} else {

		bone_rotation = matrix_3x3::IDENTITY;
		bone_position = vector_3::ZERO;
	}

	return false;

}

//***********************************************************************************
bool
Aspect::GetPrimaryBoneOrientation(
	vector_3& bone_position,
	Quat& bone_rotation) const {

	if (IsPurged(2))
	{
		BITCHSLAP("GetPrimaryBoneOrientation");
		bone_position = vector_3::ZERO;
		bone_rotation = Quat::IDENTITY;
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	int bone_index = m_shared->m_primarybone;
	if (bone_index < 0) bone_index = 0;

	if (m_shared->m_nbones > 0) {


		if ((DWORD)bone_index >= m_shared->m_nbones || bone_index < 0) {
			// TODO:: This should just return FALSE!
			// need to return Bone0 && true to fake out callers
			// I will be removing this -- biddle
			bone_index = 0;
		}

		bone_rotation = m_bones[bone_index].m_rotation;
		bone_position = m_bones[bone_index].m_position;

		if (m_shared->m_primarybone < 0)
		{
			if( !m_parent )
			{
				HadamardProduct(bone_position,bone_position,m_Scale);
			}
			else
			{
				bone_position -= m_bones[0].m_position;
				m_bones[0].m_rotation.Inverse().RotateVectorInPlace(bone_position);
				bone_position *= m_Scale;
				m_bones[0].m_rotation.RotateVectorInPlace(bone_position);
				bone_position += m_bones[0].m_position;
			}
		}

		return true;

	} else {

		bone_rotation = matrix_3x3::IDENTITY;
		bone_position = vector_3();
	}

	return false;

}

//***********************************************************************************
bool
Aspect::GetIndexedBonePosition(int bone_index, vector_3& bone_position) const
{

	if (IsPurged(2))
	{
		BITCHSLAP("GetIndexedBonePosition");
		bone_position = vector_3::ZERO;
		return false;
	}

	PADDING_BUFFER_CHECK(this)
	#if GP_DEBUG
	{

	gpassertm(m_shared->m_nbones > 0,"No bones in mesh");

	gpassertm(bone_index >= 0 && (DWORD)bone_index < m_shared->m_nbones ,"Bad Bone Index");

	}
	#endif

	if (m_shared->m_nbones > 0) {

		if ((DWORD)bone_index >= m_shared->m_nbones || bone_index < 0) {
			// TODO:: This should just return FALSE!
			// need to return Bone0 && true to fake out callers
			// I will be removing this -- biddle
			bone_index = 0;
		}

		bone_position = m_bones[bone_index].m_position;

		int pb = m_shared->m_primarybone;

		if ( (( pb != bone_index ) && (m_Scale != vector_3(1,1,1)) ))
		{
			if (pb < 0)
			{
				if( !m_parent )
				{
					HadamardProduct(bone_position,bone_position,m_Scale);
				}
				else
				{
					bone_position -= m_bones[0].m_position;
					m_bones[0].m_rotation.Inverse().RotateVectorInPlace(bone_position);
					bone_position *= m_Scale;
					m_bones[0].m_rotation.RotateVectorInPlace(bone_position);
					bone_position += m_bones[0].m_position;
				}
			}
			else
			{
				bone_position -= m_bones[pb].m_position;
				m_bones[pb].m_rotation.Inverse().RotateVectorInPlace(bone_position);
				bone_position *= m_Scale;
				m_bones[pb].m_rotation.RotateVectorInPlace(bone_position);
				bone_position += m_bones[pb].m_position;
			}
		}

		return true;

	}

	bone_position = vector_3();

	return false;
}


//***********************************************************************************
bool
Aspect::GetBoneIndex(
	char const * const bone_name,
	int& bone_index) const {

#if !GP_RETAIL
	if( m_shared->m_nbones== 0 ) {
		gpwarningf(( "tAspect::GetBoneIndex - No bones in mesh for %s\n", m_dbgname.c_str() ));
	}
#endif

	if (m_shared->m_nbones > 0) {

		for (bone_index = 0; (DWORD)bone_index < m_shared->m_nbones; bone_index++  ) {
			if (stricmp (bone_name,m_shared->m_restbones[bone_index].m_name) == 0 ) {
				return true;
			}
		}
	}

	bone_index = -1;

	return false;

}

//***********************************************************************************
int
Aspect::GetBoneIndex(
	const gpstring& bone_name) const {

#if !GP_RETAIL
	if( m_shared->m_nbones== 0 ) {
		gpwarningf(( "tAspect::GetBoneIndex - No bones in mesh for %s\n", m_dbgname.c_str() ));
	}
#endif

	int ret = -1;
	if (m_shared->m_nbones > 0) {

		for (DWORD bone_index = 0; bone_index < m_shared->m_nbones; bone_index++  ) {
			if (same_no_case(bone_name,m_shared->m_restbones[bone_index].m_name)) {
				ret = bone_index;
				break;
			}
		}
	}

	return ret;

}
//***********************************************************************************
sBone*
Aspect::GetBone(int bone_index) {

	if (IsPurged(2))
	{
		BITCHSLAP("GetBone");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)

	gpassertm(m_shared->m_nbones > 0,"No bones in mesh");
	gpassertm(bone_index >= 0 && (DWORD)bone_index < m_shared->m_nbones ,"Bad Bone Index");

	if ((DWORD)bone_index >= m_shared->m_nbones || bone_index < 0) {
		return NULL;
	}

	return &m_bones[bone_index];
}

//***********************************************************************************
char const * const
Aspect::GetBoneName(int bone_index) {

	gpassertm(m_shared->m_nbones > 0,"No bones in mesh");
	gpassertm(bone_index >= 0 && (DWORD)bone_index < m_shared->m_nbones ,"Bad Bone Index");

	if ((DWORD)bone_index >= m_shared->m_nbones || bone_index < 0) {
		return NULL;
	}

	return m_shared->m_restbones[bone_index].m_name;
}

// ***************************************************
DWORD Aspect::GetNearestBoneIndex(const vector_3& pos) {

	if (IsPurged(2))
	{
		BITCHSLAP("GetNearestBoneIndex");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)

	DWORD minind = 0;
	float minlen = FLOAT_INFINITE;

	// Not bothering to apply the scale here!!!!!

	for (DWORD bone_index = 0; bone_index < m_shared->m_nbones; bone_index++  ) {
		float len = Length2(pos-m_bones[bone_index].m_position);
		if (minlen > len) {
			minlen = len;
			minind = bone_index;
		}
	}

	return minind;
}

// ***************************************************
bool Aspect::GetIndexedCornerPosNorm(
		DWORD sub_mesh,
		DWORD index,
		vector_3& pos,
		vector_3& norm
		) const
{
	if (IsPurged(2))
	{
		BITCHSLAP("GetIndexedCornerPosNorm");
		pos = vector_3::ZERO;
		norm = vector_3::ZERO;
		return false;
	}

	PADDING_BUFFER_CHECK(this)

	if (   (sub_mesh >= m_shared->m_nmeshes)
		|| (index >= m_shared->m_ncorners[sub_mesh]))
	{
		pos  = vector_3::ZERO;
		norm = vector_3::UP;
		return false;
	}
	pos  = *((vector_3*)&m_corners[sub_mesh][index].vx);
	norm = m_normals[sub_mesh][index];
	return true;
}

// ***************************************************
inline void SetTweaked(sBone& b, const sBoneImpl& bi, bool rel)
{
	if (!b.m_tweaked)
	{
		if (!b.m_frozen)
		{
			if (rel)
			{
				b.m_position = vector_3::ZERO;
				b.m_rotation = Quat::IDENTITY;
			}
			else
			{
				b.m_position = bi.m_relRestPos;
				b.m_rotation = bi.m_relRestRot;
			}
		}
		b.m_tweaked = true;
	}
}

// ***************************************************
void Aspect::RotateBoneX(const char* bonename, float ang, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("RotateBoneX");
		return;
	}

	PADDING_BUFFER_CHECK(this)

	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateX(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::RotateBoneY(const char* bonename, float ang, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("RotateBoneY");
		return;
	}

	PADDING_BUFFER_CHECK(this)

	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateY(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::RotateBoneZ(const char* bonename, float ang, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("RotateBoneZ");
		return;
	}

	PADDING_BUFFER_CHECK(this)

	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateZ(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::TranslateBoneX(const char* bonename, const float x, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateBoneX");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.x += x;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}

// ***************************************************
void Aspect::TranslateBoneY(const char* bonename, const float y, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateBoneY");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.y += y;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}

// ***************************************************
void Aspect::TranslateBoneZ(const char* bonename, const float z, bool rel) {

	int i;

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateBoneZ");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if (GetBoneIndex(bonename,i))
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.z += z;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}

// ***************************************************
void Aspect::RotateIndexedBoneX(int i, float ang, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("RotateIndexedBoneX");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i <  m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateX(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::RotateIndexedBoneY(int i, float ang, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("RotateIndexedBoneY");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i < m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateY(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::RotateIndexedBoneZ(int i, float ang, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("RotateIndexedBoneZ");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i <  m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_rotation.RotateZ(ang);
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}

}

// ***************************************************
void Aspect::TranslateIndexedBoneX(int i, const float x, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateIndexedBoneX");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i <  m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.x += x;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}

// ***************************************************
void Aspect::TranslateIndexedBoneY(int i, const float y, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateIndexedBoneY");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i <  m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.y += y;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}

// ***************************************************
void Aspect::TranslateIndexedBoneZ(int i, const float z, bool rel) {

	if (IsPurged(2))
	{
		BITCHSLAP("TranslateIndexedBoneZ");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	if ((DWORD)i <  m_shared->m_nbones && i >= 0)
	{
		SetTweaked(m_bones[i], m_shared->m_restbones[i],rel);
		m_bones[i].m_position.z += z;
		m_TweakRequested = true;
		m_DeformationRequested = m_shared->m_primarybone<0;
	}
}



// ***************************************************
DWORD Aspect::SetBoneFrozenState(const char* bonename, eBoneFreeze f, bool recurse)
{
	if (IsPurged(2))
	{
		BITCHSLAP("SetBoneFrozenState");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)
	DWORD prevstate = BFREEZE_NONE;
	int i;
	if ( GetBoneIndex(bonename,i) )
	{
		prevstate = m_bones[i].m_frozen;
		m_bones[i].m_frozen = f;
		if (recurse)
		{
			std::set<sBone*> parents;
			parents.insert(&m_bones[i]);
			for (DWORD c = i+1; c < m_shared->m_nbones; c++)
			{
				if (parents.find(m_bones[c].m_parentBone) != parents.end())
				{
					m_bones[c].m_frozen = f;
					parents.insert(&m_bones[c]);
				}
			}
		}
	}
	return prevstate;
}

// ***************************************************
DWORD Aspect::GetBoneFrozenState(const char* bonename) const
{
	if (IsPurged(2))
	{
		BITCHSLAP("GetBoneFrozenState");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)
	int i;
	if ( GetBoneIndex(bonename,i) )
	{
		return m_bones[i].m_frozen;
	}
	return BFREEZE_NONE;
}

// ***************************************************
DWORD Aspect::SetIndexedBoneFrozenState(DWORD bone_index, eBoneFreeze f, bool recurse)
{
	if (IsPurged(2))
	{
		BITCHSLAP("SetIndexedBoneFrozenState");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)
	DWORD prevstate = BFREEZE_NONE;
	if (bone_index <  m_shared->m_nbones && bone_index >= 0)
	{
		prevstate = m_bones[bone_index].m_frozen;
		m_bones[bone_index].m_frozen = f;
		if (recurse)
		{
			std::set<sBone*> parents;
			parents.insert(&m_bones[bone_index]);
			for (DWORD c = bone_index+1; c < m_shared->m_nbones; c++)
			{
				if (parents.find(m_bones[c].m_parentBone) != parents.end())
				{
					m_bones[c].m_frozen = f;
					parents.insert(&m_bones[c]);
				}
			}
		}
	}
	return prevstate;
}

// ***************************************************
DWORD Aspect::GetIndexedBoneFrozenState(DWORD bone_index) const
{
	if (IsPurged(2))
	{
		BITCHSLAP("GetIndexedBoneFrozenState");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)
	if (bone_index <  m_shared->m_nbones && bone_index >= 0)
	{
		return m_bones[bone_index].m_frozen;
	}
	return BFREEZE_NONE;
}


// ***************************************************
DWORD Aspect::UpdateBlender(float deltat) {

	GPSTATS_GROUP( GROUP_ANIMATE_MODELS );

	if (!m_Blender)
	{
		return 0;
	}

	DWORD ret = m_Blender->UpdateActiveAnims(deltat);

	return ret;

}

// ***************************************************
void Aspect::ResetBlender(void)
{
	// This routine was inserted to fix bug #10809
	
	if (!m_Blender)
	{
		return ;
	}
	
	PADDING_BUFFER_CHECK(this)

	// We can now update the Aspect properly, the blender is ready....
	if ((GetAttachType() == UNATTACHED) && !IsPurged(2))
	{
		bool oldEnabled = m_UpdateEnabled;
		m_UpdateEnabled = true;
		m_TweakRequested = true;
		m_DeformationRequested = true;

		m_Blender->UpdateActiveAnims(0);
		AnimateBones(0);

		Deform( 0, 0 );

		m_UpdateEnabled = oldEnabled;
	}
}

// ***************************************************
bool CheckForDirtyBones(Aspect& asp)
{
	bool ret = asp.IsTweakRequested() || (asp.HasBlender() && asp.GetBlender()->IsDirty());
	if (!ret && asp.GetParent().IsValid())
	{
		ret = CheckForDirtyBones(*asp.GetParent().Get());
	}
	return ret;
}

// ***************************************************
bool
Aspect::AnimateBones(float deltat, bool reconstituting) {

	if (IsPurged(2))
	{
		BITCHSLAP("AnimateBones");
		return 0;
	}

	PADDING_BUFFER_CHECK(this)
	
	GPSTATS_GROUP( GROUP_ANIMATE_MODELS );

	gpassertm(this,"You are trying to animate a non-existent Aspect");
	if (!this) return false;

	if (!reconstituting && !m_UpdateEnabled) return false;

	bool bones_are_dirty = CheckForDirtyBones(*this);

	if (bones_are_dirty)
	{

		m_BonesInRestPose = false;

		DWORD b;

		if (!m_PrimaryBoneIsLocked) {

			b = m_linkBone;

		} else {

			// Set the first bone so that we start the bone animation AFTER the primarybone
			// This will result in the PB not being updated, which is what we are after
			// when we lock the PB

			b = m_shared->m_primarybone+1;

		}

		bool valid_blended_bones = false;

		if (m_Blender)
		{
			valid_blended_bones = m_Blender->BlendActiveAnims(b);
		}

		if (valid_blended_bones) {

			const vector_3& blended_velocity = m_Blender->BlendedRootVelocity();

			m_Velocity = Length(blended_velocity);

		} else {

			m_Velocity = 0;

		}

		if ((m_attachtype == DEFORMABLE_OVERLAY))
		{

			// This is a deformable overlay, we just need to get the parent bone pos/rot in each case

			for (; b < m_shared->m_nbones; ++b)
			{
#if !GP_RETAIL
				gpassertf(m_bones[b].m_parentBone && !IsMemoryBadFood((DWORD)m_bones[b].m_parentBone),("NEMA ERROR: DEFORMABLE_OVERLAY - The parent_bone of bone #%d of aspect [%s] is not initialized",b,m_dbgname.c_str()));
				if (!m_bones[b].m_parentBone || IsMemoryBadFood((DWORD)m_bones[b].m_parentBone))
				{
					continue;
				}
#endif
				if (m_DeformationRequested)
				{
					m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
					m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
				}
				else
				{
					if (m_bones[b].m_rotation != m_bones[b].m_parentBone->m_rotation)
					{
						m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
						m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
						m_DeformationRequested = true;
					}
					else if (m_bones[b].m_position != m_bones[b].m_parentBone->m_position)
					{
						m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
						m_DeformationRequested = true;
					}
				}
				m_bones[b].m_tweaked = false;
				
				gpassertf(fabs(m_bones[b].m_rotation.m_x)+fabs(m_bones[b].m_rotation.m_y)+fabs(m_bones[b].m_rotation.m_z)+fabs(m_bones[b].m_rotation.m_w) < 500, ("NEMA ERROR: DEFORMABLE_OVERLAY - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));
				gpassertf(m_bones[b].m_position.Length() < 100,	("NEMA ERROR: DEFORMABLE_OVERLAY - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));

			}
		}
		else
		{
			if ((m_attachtype == RIGID_LINKED) && (b < m_shared->m_nbones))
			{
#if !GP_RETAIL
				gpassertf(m_bones[b].m_parentBone && !IsMemoryBadFood((DWORD)m_bones[b].m_parentBone),("NEMA ERROR: RIGID_LINKED - The parent_bone of bone #%d of aspect [%s] is not initialized",b,m_dbgname.c_str()));
				if (m_bones[b].m_parentBone && !IsMemoryBadFood((DWORD)m_bones[b].m_parentBone))
				{
#endif
					if (m_parent->GetCurrentScale() == vector_3(1,1,1))
					{
						if (m_DeformationRequested)
						{
							m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
							m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
						}
						else
						{
							if (m_bones[b].m_rotation != m_bones[b].m_parentBone->m_rotation)
							{
								m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
								m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
								m_DeformationRequested = true;
							}
							else if (m_bones[b].m_position != m_bones[b].m_parentBone->m_position)
							{
								m_bones[b].m_position = m_bones[b].m_parentBone->m_position;
								m_DeformationRequested = true;
							}
						}
					}
					else
					{
						vector_3 tpos(m_bones[b].m_parentBone->m_position);
						int ppb = m_parent->m_shared->m_primarybone;
						if (ppb < 0)
						{
							ppb = m_parent->m_linkBone;
						}
						if ( (ppb >= 0) && (m_bones[b].m_parentBone != &m_parent->m_bones[ppb]) )
						{
							// Need to scale the linking bone position in the parent's
							// primary bone frame
							tpos -= m_parent->m_bones[ppb].m_position;
							m_parent->m_bones[ppb].m_rotation.Inverse().RotateVectorInPlace(tpos);
							tpos *= m_Scale;
							m_parent->m_bones[ppb].m_rotation.RotateVectorInPlace(tpos);
							tpos += m_parent->m_bones[ppb].m_position;
						}

						if (m_DeformationRequested)
						{
							m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
							m_bones[b].m_position = tpos;
						}
						else
						{
							if (m_bones[b].m_rotation != m_bones[b].m_parentBone->m_rotation)
							{
								m_bones[b].m_rotation = m_bones[b].m_parentBone->m_rotation;
								m_bones[b].m_position = tpos;
								m_DeformationRequested = true;
							}
							else if (m_bones[b].m_position != tpos)
							{
								m_bones[b].m_position = tpos;
								m_DeformationRequested = true;
							}
						}
					}
#if !GP_RETAIL
				}
#endif
				gpassertf(fabs(m_bones[b].m_rotation.m_x)+fabs(m_bones[b].m_rotation.m_y)+fabs(m_bones[b].m_rotation.m_z)+fabs(m_bones[b].m_rotation.m_w) < 500, ("NEMA ERROR: RIGID_LINKED - The bone #%d of aspect [%s] is is out of control !!!",b,m_dbgname.c_str()));
				gpassertf(m_bones[b].m_position.Length() < 100,	("NEMA ERROR: RIGID_LINKED - The bone #%d of aspect [%s] is is out of control !!!",b,m_dbgname.c_str()));

				m_bones[b].m_tweaked = false;
				++b;

			}
			else if ((m_attachtype == REVERSE_RIGID_LINKED) && (b < m_shared->m_nbones))
			{

				// We are at the top of a rigid linked child heirarchy
				// Use the parent's pos/rot directly for this first child bone

#if !GP_RETAIL
				gpassertf(m_bones[b].m_parentBone && !IsMemoryBadFood((DWORD)m_bones[b].m_parentBone),("NEMA ERROR: REVERSE_RIGID_LINKED - The parent_bone of bone #%d of aspect [%s] is not initialized",b,m_dbgname.c_str()));
				if (m_bones[b].m_parentBone && !IsMemoryBadFood((DWORD)m_bones[b].m_parentBone))
				{
#endif
					Quat	 keyrot = m_bones[b].m_parentBone->m_rotation * GetLinkOffsetRotation();
					vector_3 keypos(DoNotInitialize);
					m_bones[b].m_parentBone->m_rotation.RotateVector(keypos,GetLinkOffsetPosition());
					keypos += m_bones[b].m_parentBone->m_position;

					if (m_DeformationRequested)
					{
						m_bones[b].m_rotation = keyrot;
						m_bones[b].m_position = keypos;
					}
					else
					{
						if (m_bones[b].m_rotation != keyrot)
						{
							m_bones[b].m_rotation = keyrot;
							m_bones[b].m_position = keypos;
							m_DeformationRequested = true;
						}
						else if (m_bones[b].m_position != keypos)
						{
							m_bones[b].m_position = keypos;
							m_DeformationRequested = true;
						}
					}

#if !GP_RETAIL
				}
#endif
				gpassertf(fabs(m_bones[b].m_rotation.m_x)+fabs(m_bones[b].m_rotation.m_y)+fabs(m_bones[b].m_rotation.m_z)+fabs(m_bones[b].m_rotation.m_w) < 500, ("NEMA ERROR: REVERSE_RIGID_LINKED - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));
				gpassertf(m_bones[b].m_position.Length() < 100,	("NEMA ERROR: REVERSE_RIGID_LINKED - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));

				m_bones[b].m_tweaked = false;

				++b;
			}

			for (; b < m_shared->m_nbones; ++b) {

				sBone* kbone = &m_bones[b];

				const sBone* pbone = kbone->m_parentBone;

				vector_3 keypos(DoNotInitialize);
				Quat	 keyrot(DoNotInitialize);

				if (valid_blended_bones)
				{
					if (kbone->m_tweaked)
					{
						if ((kbone->m_frozen & (BFREEZE_POSITION_RIGID)) == (BFREEZE_POSITION_RIGID))
						{
							keypos = kbone->m_position + m_shared->m_restbones[b].m_relRestPos;
						}
						else
						{
							keypos = kbone->m_position + m_Blender->BlendedBonePosition(b);
						}
						if ((kbone->m_frozen & (BFREEZE_ROTATION_RIGID)) == (BFREEZE_ROTATION_RIGID))
						{
							keyrot = kbone->m_rotation * m_shared->m_restbones[b].m_relRestRot;
						}
						else
						{
							keyrot = kbone->m_rotation * m_Blender->BlendedBoneRotation(b);
						}
					}
					else
					{
						if ((kbone->m_frozen & (BFREEZE_POSITION_RIGID)) == (BFREEZE_POSITION_RIGID))
						{
							keypos = m_shared->m_restbones[b].m_relRestPos;
						}
						else
						{
							keypos = m_Blender->BlendedBonePosition(b);
						}
						if ((kbone->m_frozen & (BFREEZE_ROTATION_RIGID)) == (BFREEZE_ROTATION_RIGID))
						{
							keyrot = m_shared->m_restbones[b].m_relRestRot;
						}
						else
						{
							keyrot = m_Blender->BlendedBoneRotation(b);
						}
					}
				}
				else
				{
					if (kbone->m_tweaked)
					{
						keypos = kbone->m_position;
						keyrot = kbone->m_rotation;
					}
					else
					{
						keypos = m_shared->m_restbones[b].m_relRestPos;
						keyrot = m_shared->m_restbones[b].m_relRestRot;
					}
				}

				if (pbone && (pbone != kbone)) {

					// We have a valid parent
					Quat	 parentrot = pbone->m_rotation;
					vector_3 parentpos = pbone->m_position;

					if (m_DeformationRequested)
					{
						kbone->m_rotation = parentrot * keyrot;
						parentrot.RotateVector(kbone->m_position,keypos);
						kbone->m_position += parentpos;
					}
					else
					{
						Quat tquat(parentrot * keyrot);
						vector_3 tpos(DoNotInitialize);
						parentrot.RotateVector(tpos,keypos);
						tpos += parentpos;

						if (kbone->m_rotation != tquat)
						{
							kbone->m_rotation = tquat;
							kbone->m_position = tpos;
							m_DeformationRequested = true;
						}
						else if (kbone->m_position != tpos)
						{
							kbone->m_position = tpos;
							m_DeformationRequested = true;
						}
					}

				} else {

					// This is the ROOT of a bone heirarchy
					if (m_DeformationRequested)
					{
						kbone->m_rotation = keyrot;
						kbone->m_position = keypos;
					}
					else
					{
						if (kbone->m_rotation != keyrot)
						{
							kbone->m_rotation = keyrot;
							kbone->m_position = keypos;
							m_DeformationRequested = true;
						}
						else if (kbone->m_position != keypos)
						{
							kbone->m_position = keypos;
							m_DeformationRequested = true;
						}
					}
				}

				kbone->m_tweaked = false;

				gpassertf(fabs(m_bones[b].m_rotation.m_x)+fabs(m_bones[b].m_rotation.m_y)+fabs(m_bones[b].m_rotation.m_z)+fabs(m_bones[b].m_rotation.m_w) < 500, ("NEMA ERROR: UPDATING_BONES - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));
				gpassertf(m_bones[b].m_position.Length() < 100,	("NEMA ERROR: UPDATING_BONES - The bone #%d of aspect [%s] is out of control !!!",b,m_dbgname.c_str()));
			}

		}
	}

	PADDING_BUFFER_CHECK(this)

	// TODO Get rid of the tail recursion

	// Don't recurse if we are reconstituting, the children aren't ready yet
	if (!reconstituting && !m_childlinks.empty()) {

		{for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {

			Aspect* aspect = (*it).first.GetValid();
			if ( aspect != NULL )
			{
				aspect->AnimateBones(deltat,false); 
			}

		}}
	}

	PADDING_BUFFER_CHECK(this)
	
	if (m_Blender)
	{
		m_Blender->SetDirtyFlag(false);
	}

	m_TweakRequested = false;

	return bones_are_dirty;

}

// ***************************************************
bool
Aspect::ShouldPersist(void) const {

	GoHandle go( GetGoid() );
	return ( go && go->ShouldPersist() );
}

//$$$$$$$$$$$
// Declare values that we can poke at from within the debugger

// Number of updates to skip between normal deformations
// a value of 0 causes refresh every frame
volatile int NORMAL_DEFORM_REFRESH_RATE = 0; // 10;
// If a bone influences more than the threshold number of vertices it's
// rotation is converted to matrix before being applied to the verts
volatile DWORD MINIMUM_BONE_VERTS_THRESHOLD = 0xffff;

// ***************************************************
bool
Aspect::Deform(DWORD CurrentFirstVert, DWORD ParentFirstVert) {

	GPSTATS_GROUP( GROUP_DEFORM_MODELS );
	CHECK_PRIMARY_THREAD_ONLY;

	PADDING_BUFFER_CHECK(this)

	gpassertm(this,"You are trying to update a non-existent Aspect");

	if (gAspectStorage.m_BlendVerts.size() < (CurrentFirstVert + m_shared->m_maxverts))
	{
		gAspectStorage.m_BlendVerts.resize((CurrentFirstVert + m_shared->m_maxverts),vector_3::ZERO);
		gAspectStorage.m_BlendQuats.resize((CurrentFirstVert + m_shared->m_maxverts),Quat::IDENTITY);
	}

	bool changed = false;
	bool updatedMinMax = false;

	if (m_UpdateEnabled && m_IsDeformable && m_DeformationRequested)
	{

		if (IsPurged(2))
		{
			BITCHSLAP("Deform");
		}
		else
		{

			bool force_deform_for_select = false;
			if (m_instflags & HIDE_ALLSUBMESHES)
			{
				// We're testing to see if the object is selectable, and if it is, let's deform it
				// even if its submeshes are hidden, that way we get valid bounding information.
				GoHandle hObject( m_owner_goid );
				force_deform_for_select = hObject.IsValid() && hObject->GetAspect()->GetIsSelectable();
			}

			gpassertm(m_shared->m_primarybone<0,"NEMA_DEFORM: Attempt to deform a rigid mesh");

			float max_x,max_y,max_z;
			float min_x,min_y,min_z;

			max_x = max_y = max_z = RealMinimum;
			min_x = min_y = min_z = RealMaximum;

			vector_3*	blendedverts = &gAspectStorage.m_BlendVerts[CurrentFirstVert];
			Quat*		blendedquats = &gAspectStorage.m_BlendQuats[CurrentFirstVert];

			{for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++) {

				if ( force_deform_for_select ||	!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm))) )
				{
					updatedMinMax = true;
					memset(blendedverts,0,sizeof(sVert)*m_shared->m_nverts[csm]);

					if (m_DeformNormCounter <= 0)
					{
						std::fill( blendedquats, blendedquats + m_shared->m_nverts[csm], Quat::IDENTITY );
					}

					for (DWORD b = 0; b < m_shared->m_nbones; b++)
					{

						const sBone& bone = m_bones[b];

						const Quat& poserot = bone.m_rotation;
						const vector_3& posepos = bone.m_position;

						const Quat& invrestrot = bone.m_sharedbone->m_invRestRot;
						const vector_3& invrestpos = bone.m_sharedbone->m_invRestPos;

						// swapping the order of operations
						const Quat rot =  poserot * invrestrot;
						vector_3 pos(DoNotInitialize);
						poserot.RotateVector(pos,invrestpos);
						pos += posepos;

						{
							const sBoneVertWeightPair* ji = bone.m_sharedbone->m_vertWeights[csm];
							const sBoneVertWeightPair* je = ji + bone.m_sharedbone->m_numberOfPairs[csm];
							const sVert* verts = m_shared->m_verts[csm];

							if (bone.m_sharedbone->m_numberOfPairs[csm] > MINIMUM_BONE_VERTS_THRESHOLD)
							{
								// There are enough verts affected, so we will convert to a matrix
								matrix_3x3 mrot(DoNotInitialize);
								rot.BuildMatrix(mrot);
								vector_3 t(DoNotInitialize);
								for ( ; ji != je ; ++ji )
								{
									const float w = ji->Weight;
									const int v   = ji->VertIndex;

									mrot.Transform(t,verts[v]);
									t += pos;
									t *= w;
									blendedverts[v] += t;

									if (m_DeformNormCounter <= 0)
									{
										SlerpInPlace( blendedquats[v], rot, w );
									}
								}

							}
							else
							{
								vector_3 t(DoNotInitialize);

								for ( ; ji != je ; ++ji )
								{
									const float w = ji->Weight;
									const int v   = ji->VertIndex;

									rot.RotateVector(t,verts[v]);
									t += pos;
									t *= w;
									blendedverts[v] += t;

									if (m_DeformNormCounter <= 0)
									{
										SlerpInPlace( blendedquats[v], rot, w );
									}
								}
							}
						}

					}

					if ((m_attachtype == RIGID_LINKED) && !IsZero(m_Scale))
					{
						// We need to handle deformable objects that are linked AND scales
						// This was the case for the minigun with a rotating muzzle bone
						// when it was carried by the farmgirl or the dwarf

						matrix_3x3 linkrot(m_bones[m_linkBone].m_rotation.BuildMatrix());
						matrix_3x3 invlrot(DoNotInitialize);
						linkrot.Transpose(invlrot);

						invlrot.GetRow0() *= m_Scale.x;
						invlrot.GetRow1() *= m_Scale.y;
						invlrot.GetRow2() *= m_Scale.z;

						linkrot = linkrot * invlrot;

						const vector_3& trans = m_bones[m_linkBone].m_position;

						for (DWORD v = 0; v < m_shared->m_nverts[csm]; v++)
						{
							blendedverts[v] -= trans;
							linkrot.Transform(blendedverts[v]);
							blendedverts[v] += trans;
						}
					}

					// Do we have any vert stitching to do?

					if (m_instflags & STITCH_VERTS) {

						const vector_3*	parentverts = &gAspectStorage.m_BlendVerts[ParentFirstVert];

						for (DWORD ps = 0; ps < m_parent->m_shared->m_nstitchsets[0]; ++ps) {
							for (DWORD cs = 0; cs < m_shared->m_nstitchsets[csm]; ++cs) {
								if (m_parent->m_shared->m_stitchset[0][ps].m_tag == m_shared->m_stitchset[csm][cs].m_tag) {
									DWORD vnum = min(m_parent->m_shared->m_stitchset[0][ps].m_nverts,m_shared->m_stitchset[csm][cs].m_nverts);
									for (DWORD i = 0; i < vnum; i++) {
										int pvi = m_parent->m_shared->m_stitchset[0][ps].m_verts[i];
										int cvi = m_shared->m_stitchset[csm][cs].m_verts[i];
										// Copy the parent's vert info into the child
										blendedverts[cvi] = parentverts[pvi];
									}
									break;
								}
							}
						}
					}


					sVert const * const source_vert = blendedverts;
					sVertMap* map_runner = m_shared->m_vertmappings[csm];
					sCorner* const dest_corn = m_corners[csm];
					vector_3* const dest_norm = m_normals[csm];

					if (m_DeformNormCounter <= 0)
					{
						for (DWORD i=0; i < m_shared->m_nverts[csm] ;i++)
						{

							const vector_3& temp	= source_vert[i];

							// Maintain the bounding box
							if      (temp.GetX() > max_x) max_x = temp.GetX();
							else if (temp.GetX() < min_x) min_x = temp.GetX();
							if      (temp.GetY() > max_y) max_y = temp.GetY();
							else if (temp.GetY() < min_y) min_y = temp.GetY();
							if      (temp.GetZ() > max_z) max_z = temp.GetZ();
							else if (temp.GetZ() < min_z) min_z = temp.GetZ();

							DWORD count = *map_runner++;

							const Quat& normq	= 	blendedquats[i];
							for (DWORD j=1; j <= count; j++, map_runner++ )
							{
								*((vector_3*)&dest_corn[*map_runner].vx) = temp;
								normq.RotateVector(*(vector_3*)&dest_norm[*map_runner],	m_shared->m_wcorners[csm][*map_runner].n);
							}

						}
						blendedverts += m_shared->m_nverts[csm];
						blendedquats += m_shared->m_nverts[csm];

						//$$$$$$$
						// How often should we deform the lighting normals?
						m_DeformNormCounter = NORMAL_DEFORM_REFRESH_RATE;
						//$$$$$$$
					}
					else
					{
						for (DWORD i=0; i < m_shared->m_nverts[csm] ;i++)
						{

							const vector_3& temp	= source_vert[i];

							// Maintain the bounding box
							if      (temp.GetX() > max_x) max_x = temp.GetX();
							else if (temp.GetX() < min_x) min_x = temp.GetX();
							if      (temp.GetY() > max_y) max_y = temp.GetY();
							else if (temp.GetY() < min_y) min_y = temp.GetY();
							if      (temp.GetZ() > max_z) max_z = temp.GetZ();
							else if (temp.GetZ() < min_z) min_z = temp.GetZ();

							DWORD count = *map_runner++;

							for (DWORD j=1; j <= count; j++, map_runner++ )
							{
								*((vector_3*)&dest_corn[*map_runner].vx) = temp;
							}
						}
						blendedverts += m_shared->m_nverts[csm];

						// Decrement the norm counter so that eventually we update
						--m_DeformNormCounter;
					}

				}


			}}

			if( updatedMinMax == false )
			{
				max_x = max_y = max_z = 1.0f;
				min_x = min_y = min_z = 0.0f;
			}

			m_BBoxCenter    = vector_3((max_x+min_x)*0.5f,(max_y+min_y)*0.5f,(max_z+min_z)*0.5f);
			m_BBoxHalfDiag  = vector_3((max_x-min_x)*0.5f,(max_y-min_y)*0.5f,(max_z-min_z)*0.5f);
			m_BSphereRadius = Length(m_BBoxHalfDiag);

			changed = true;
		}
	}

	PADDING_BUFFER_CHECK(this)

	// TODO Get rid of the tail recursion
	if (!m_childlinks.empty())
	{
		for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
		{
			if ( (*it).first.IsValid())
			{
				Aspect* child = (*it).first.Get();
				if (child->IsDeformable())
				{
					child->m_DeformationRequested = m_DeformationRequested;
					changed |= (*it).first->Deform(CurrentFirstVert+m_shared->m_maxverts,CurrentFirstVert);
				}
			}
		}
	}

	PADDING_BUFFER_CHECK(this)

	// The skin now matches the bones
	m_DeformationRequested = false;

	return changed;

}

// ***************************************************

bool
Aspect::HilightDeform(float level) {

	GPSTATS_GROUP( GROUP_DEFORM_MODELS );

	gpassertm(this,"You are trying to update a non-existent Aspect");
	if (!this) return false;

	if (!m_UpdateEnabled) return false;

	const DWORD alpha = (DWORD)((1.0f-level) * 255);
	const DWORD hilightcolor = (alpha<<24) + 0x00ffbb00;

	if (m_IsDeformable) {

		if (IsPurged(2))
		{
			BITCHSLAP("HilightDeform");
			return false;
		}

	PADDING_BUFFER_CHECK(this)

		const float scale = 1.0f+(1.3f*level);

		// TODO: There is probably an optimization in here if we had a way to easily
		// determine the first and last time we transform a vert, that way we could avoid
		// the memset of the blended verts and save ourselves the final write-back

		vector_3*	blendedverts = &gAspectStorage.m_BlendVerts[0];

		{for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++) {

			if (!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm)))) {

				memset(blendedverts,0,sizeof(sVert)*m_shared->m_nverts[csm]);

				{for (DWORD b = 0; b < m_shared->m_nbones; b++) {

					const sBone& bone = m_bones[b];

					const Quat& poserot = bone.m_rotation;
					const vector_3& posepos = bone.m_position;

					const Quat& invrestrot = bone.m_sharedbone->m_invRestRot;
					const vector_3& invrestpos = bone.m_sharedbone->m_invRestPos;


					for (DWORD j=0; j < bone.m_sharedbone->m_numberOfPairs[csm];j++) {

						const int v = bone.m_sharedbone->m_vertWeights[csm][j].VertIndex;

						const float w = bone.m_sharedbone->m_vertWeights[csm][j].Weight;

						vector_3 restspace(DoNotInitialize);

						invrestrot.RotateVector(restspace,m_shared->m_verts[csm][v]);
						restspace += invrestpos;
						restspace.y *= scale;
						restspace.z *= scale;

						poserot.RotateVectorInPlace(restspace);
						restspace += posepos;
						restspace *= w;
						blendedverts[v] += restspace;

					}
				}}

				sVert const * const source_vert = blendedverts;
				sVertMap* map_runner = m_shared->m_vertmappings[csm];
				sCorner* const dest_corn = m_corners[csm];

				{for (DWORD i=0; i < m_shared->m_nverts[csm] ;i++) {

					const vector_3& temp	= source_vert[i];

					DWORD count = *map_runner++;

					for (DWORD j=1; j <= count; j++, map_runner++ ) {

						*((vector_3*)&dest_corn[*map_runner].vx) = temp;
						dest_corn[*map_runner].color = hilightcolor;

					}

				}}
			}
		}}

	}
	else
	{
		if (IsPurged(2))
		{
			BITCHSLAP("HilightDeform (RIGID)");
			return false;
		}

	PADDING_BUFFER_CHECK(this)

		for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++)
		{
			sCorner* const dest_corn = m_corners[csm];

			for (DWORD i=0; i < m_shared->m_ncorners[csm] ;i++)
			{

				dest_corn[i].color = hilightcolor;

			}
		}
	}
	return false;

}
//***********************************************************************************
void
Aspect::Render(Rapi* hRapi,DWORD renderflags)
{

	CHECK_PRIMARY_THREAD_ONLY;

	GPSTATS_GROUP( GROUP_RENDER_MODELS );
	GPPROFILERSAMPLE( "Nema Aspect::Render() ", SP_DRAW | SP_DRAW_CALC_GO );

	gpassertm(this,"You are trying to render a non-existent Aspect");
	if (!this) return;

	gpassert(!m_DeformationRequested);

	RenderMesh(hRapi);

	// Debug Info
	float gizmoscale = 1.0f;

	if ((m_instflags|renderflags) & RENDER_NORMALS) {
		RenderNormals(hRapi,0.1f*gizmoscale);
	}

	if ((m_instflags|renderflags) & RENDER_BONES) {
		RenderBones(hRapi,0.05f*gizmoscale);
	}

	if ((m_instflags|renderflags) & RENDER_BOUNDBOXES) {
		RenderBoundBoxes(hRapi);
	}

	if ((m_instflags|renderflags) & RENDER_SHIELD) {
		RenderShield(hRapi);
	}

}

//***********************************************************************************

void
Aspect::RenderMesh(Rapi* hRapi) {

	CHECK_PRIMARY_THREAD_ONLY;

	GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() ", SP_DRAW | SP_DRAW_CALC_GO );

	gpassertm(this,"You are trying to render a non-existent Aspect");
	if (!this) return;

	if (IsPurged(2))
	{
		BITCHSLAP("RenderMesh with purged data");
		return;
	}
	else if (IsPurged(1))
	{
		BITCHWARN("RenderMesh with purged textures");
		return;
	}

	PADDING_BUFFER_CHECK(this)

	Quat fixed_rot;

	if (m_shared->m_primarybone >= 0)
	{
		hRapi->PushWorldMatrix();
		hRapi->TranslateWorldMatrix(m_bones[m_shared->m_primarybone].m_position);
		hRapi->RotateWorldMatrix(m_bones[m_shared->m_primarybone].m_rotation.BuildMatrix());
		hRapi->ScaleWorldMatrix(m_Scale);
	}

	if (m_shared->m_flags & STENCIL_ALPHA)
	{
		// Need to turn on alpha test
		hRapi->GetDevice()->SetRenderState( D3DRS_ALPHAREF, max( 0, min( ((int)m_Alpha)>>1, 100 ) ) );
	}

	{for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++)
	{

		if (!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm))))
		{

			GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() [for MESH loop overhead]", SP_DRAW | SP_DRAW_CALC_GO );

			// TODO:: Group the objects so we don't have to change the render state here -- biddle
			hRapi->SetTextureStageState( 0,
										 m_instflags & RENDER_DISABLE_TEXTURE ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE,
										 m_Alpha < 0xFF ? D3DTOP_MODULATE : ((m_shared->m_flags & VERTEX_ALPHA) ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1),
										 m_shared->m_flags & TEXTURE_UWRAP ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										 m_shared->m_flags & TEXTURE_VWRAP ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP,
										 D3DTEXF_LINEAR,
										 D3DTEXF_LINEAR,
										 D3DTEXF_LINEAR,
										 D3DBLEND_SRCALPHA,
										 D3DBLEND_INVSRCALPHA,
										 m_shared->m_flags & STENCIL_ALPHA ? true : false );


			sTri* trirunner = m_shared->m_triangles[csm];
			unsigned int startIndex;

			if (m_instflags & INCLUDE_NORMALS)
			{
				GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() [vbuffer copy (with normals)]", SP_DRAW | SP_DRAW_CALC_GO );
				lVertex* pLVerts;
				if( hRapi->LockBufferedLVertices( m_shared->m_ncorners[csm], startIndex, pLVerts ) )
				{
					for (DWORD i = 0; i < m_shared->m_ncorners[csm]; i++)
					{
						pLVerts[i].x     = m_corners[csm][i].vx;
						pLVerts[i].y     = m_corners[csm][i].vy;
						pLVerts[i].z     = m_corners[csm][i].vz;
						pLVerts[i].nx    = m_normals[csm][i].x;
						pLVerts[i].ny    = m_normals[csm][i].y;
						pLVerts[i].nz    = m_normals[csm][i].z;
						pLVerts[i].color = m_corners[csm][i].color;
						pLVerts[i].uv.u  = m_corners[csm][i].uv.u;
						pLVerts[i].uv.v  = m_corners[csm][i].uv.v;
					}
					hRapi->UnlockBufferedLVertices();
				}
				else
				{
					continue;
				}
			}
			else
			{
				GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() [vbuffer copy (without normals)]", SP_DRAW | SP_DRAW_CALC_GO );
				if( !hRapi->CopyBufferedVertices( m_shared->m_ncorners[csm], startIndex, (sVertex*)(m_corners[csm]) ) )
				{
					continue;
				}
			}

			for (DWORD mat = 0; mat < m_shared->m_submeshmatcount[csm]; mat++) {

				GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() [for MATERIAL loop overhead]", SP_DRAW | SP_DRAW_CALC_GO );

				DWORD tris = m_shared->m_submeshfacecount[csm][mat];

				if (!tris) {
					// This is a v1.0 mesh with the zero-tri bug
					continue;
				}

				DWORD lmat = m_shared->m_submeshmatlookup[csm][mat];

				{

					if (m_instflags & RENDER_DISABLE_TEXTURE)
					{
						hRapi->SetTexture( 0,0 );
					}
					else
					{
#if NEMA_DEBUGGING
						if (NEMAFLAG_ColorizedWeights)
						{
							hRapi->SetTexture( 0,0 );
						}
						else
#endif
						{
							hRapi->SetTexture( 0, m_currenttextures[lmat] );
						}
					}

					DWORD ncrns = m_shared->m_submeshcorncount[csm][mat];

#ifdef GPDEV_ONLY
					// Doing this test just to make sure that we don't draw bad art! --biddle
					if (!((m_shared->m_submeshstartindex[csm][mat]+ncrns) <= m_shared->m_ncorners[csm]))
					{
						gperrorf(("There is an data error in the [%s] ASP file, it will have to be re-exported. ",
							 m_dbgname.c_str()));
					}
					else
#endif
					{
						GPPROFILERSAMPLE( "Nema Aspect::RenderMesh() [DrawBufferedIndexedPrimitive]", SP_DRAW | SP_DRAW_CALC_GO );
						if (m_instflags & INCLUDE_NORMALS)
						{
							// Draw
							hRapi->DrawBufferedIndexedLPrimitive(
								D3DPT_TRIANGLELIST,
								startIndex+m_shared->m_submeshstartindex[csm][mat],
								ncrns,
								(WORD *)trirunner, tris*3 );
						}
						else
						{
							// Draw
							hRapi->DrawBufferedIndexedPrimitive(
								D3DPT_TRIANGLELIST,
								startIndex+m_shared->m_submeshstartindex[csm][mat],
								ncrns,
								(WORD *)trirunner, tris*3 );
						}
					}

				}

				trirunner += tris;

			}

		}
	}}

	if( m_shared->m_flags & STENCIL_ALPHA )
	{
		// Need to turn off alpha test
		hRapi->GetDevice()->SetRenderState( D3DRS_ALPHAREF, 0x00000000 );
	}

	if ( m_shared->m_primarybone >= 0 )
	{
		hRapi->PopWorldMatrix();
	}

	// TODO Get rid of the tail recursion
	PADDING_BUFFER_CHECK(this)

	{for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {

		if ((*it).first.IsValid()) {
			(*it).first->RenderMesh(hRapi);
		}
	}}

	PADDING_BUFFER_CHECK(this)
}

//***********************************************************************************
void
Aspect::RenderNormals(Rapi* hRapi,float gizmoscale) {

	CHECK_PRIMARY_THREAD_ONLY;

	if (m_shared->m_primarybone >= 0) {
		hRapi->PushWorldMatrix();
		hRapi->TranslateWorldMatrix(m_bones[m_shared->m_primarybone].m_position);
		hRapi->RotateWorldMatrix(m_bones[m_shared->m_primarybone].m_rotation.BuildMatrix());
	}

	DWORD prev;

	TRYDX (hRapi->GetDevice()->GetTextureStageState( 0, D3DTSS_COLOROP, &prev));
	hRapi->SetSingleStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );

	const DWORD Yellow = MAKEDWORDCOLOR(vector_3(1,1,0));

	for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++) {

		if (!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm)))) {

			unsigned int startIndex;
			sVertex* nVerts;

			hRapi->LockBufferedVertices( 2*m_shared->m_ncorners[csm], startIndex, nVerts );

			{for (int i = 0; i < (int)m_shared->m_ncorners[csm]; i++) {
				nVerts[2*i].x = m_corners[csm][i].vx;
				nVerts[2*i].y = m_corners[csm][i].vy;
				nVerts[2*i].z = m_corners[csm][i].vz;
				nVerts[2*i].color = Yellow;

				vector_3 norm = m_normals[csm][i] * gizmoscale;

				if ((m_shared->m_MajorVersion<2))
				{
					PreRotateNEMAtoSEIGE.RotateVectorInPlace(norm);
				}

				nVerts[2*i+1].x = m_corners[csm][i].vx + norm.x;
				nVerts[2*i+1].y = m_corners[csm][i].vy + norm.y;
				nVerts[2*i+1].z = m_corners[csm][i].vz + norm.z;
				nVerts[2*i+1].color = Yellow;
			}}

			hRapi->UnlockBufferedVertices();

			hRapi->DrawBufferedPrimitive( D3DPT_LINELIST,
										 startIndex, 2*m_shared->m_ncorners[csm]);

		}
	}

	hRapi->SetSingleStageState( 0, D3DTSS_COLOROP, prev );

	if (m_shared->m_primarybone >= 0) {
		hRapi->PopWorldMatrix();
	}

	PADDING_BUFFER_CHECK(this)
	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {
		if ((*it).first.IsValid()) {
			(*it).first->RenderNormals(hRapi,gizmoscale);
		}
	}
	PADDING_BUFFER_CHECK(this)


}


//***********************************************************************************
void
Aspect::RenderBones(Rapi* hRapi,float gizmoscale)
{
	CHECK_PRIMARY_THREAD_ONLY;

	PADDING_BUFFER_CHECK(this)
	for (int b=0;b<(int)m_shared->m_nbones;b++)
	{

		hRapi->PushWorldMatrix();
		hRapi->TranslateWorldMatrix(m_bones[b].m_position);
		hRapi->RotateWorldMatrix(m_bones[b].m_rotation.BuildMatrix());
		RP_DrawOrigin(*hRapi,gizmoscale);
		hRapi->PopWorldMatrix();

		sVertex segment[2];

		memset( segment, 0, sizeof( sVertex ) * 2 );

		vector_3 pointa = m_bones[b].m_position;
		segment[0].x = pointa.x;
		segment[0].y = pointa.y;
		segment[0].z = pointa.z;
		segment[0].color = (unsigned int)-1;

		vector_3 pointb = m_bones[b].m_position;
		segment[1].x = pointb.x;
		segment[1].y = pointb.y;
		segment[1].z = pointb.z;
		segment[1].color = (unsigned int)-1;

		hRapi->DrawPrimitive(D3DPT_LINELIST,segment,2,SVERTEX,NULL,0);

	}
	PADDING_BUFFER_CHECK(this)

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {

		if ((*it).first.IsValid()) {
			(*it).first->RenderBones(hRapi,gizmoscale);
		}
	}
	PADDING_BUFFER_CHECK(this)

}
//***********************************************************************************
void
Aspect::RenderBoundBoxes(Rapi* hRapi)
{
	CHECK_PRIMARY_THREAD_ONLY;

	PADDING_BUFFER_CHECK(this)
	static vector_3 palette[8] = {

		vector_3(	6,134,  6)*(1/255.0f),
		vector_3( 134,  6,  6)*(1/255.0f),

		vector_3(	6,134,134)*(1/255.0f),
		vector_3( 134,134,  6)*(1/255.0f),

		vector_3( 134,134,  6)*(1/255.0f),
		vector_3( 134,  6,134)*(1/255.0f),

		vector_3( 100, 50,  6)*(1/255.0f),
		vector_3(  50,  6,100)*(1/255.0f)
	};

	stdx::linear_map< DWORD,BoundingBox >::iterator beg = m_shared->m_boundingboxes.begin();
	stdx::linear_map< DWORD,BoundingBox >::iterator end = m_shared->m_boundingboxes.end();
	stdx::linear_map< DWORD,BoundingBox >::iterator it;

	int i;

	for (it=beg,i=0;it!=end;++it,++i)
	{
		const BoundingBox& b = (*it).second;

		hRapi->PushWorldMatrix();
		hRapi->TranslateWorldMatrix(b.offset);
		hRapi->RotateWorldMatrix(b.orientation.BuildMatrix());
		RP_DrawBox( *hRapi, -b.halfdiag, b.halfdiag , MAKEDWORDCOLOR(palette[i%8]));
		hRapi->PopWorldMatrix();
	}

}

//***********************************************************************************
void
Aspect::RenderShield(Rapi* hRapi)
{

	PADDING_BUFFER_CHECK(this)
	for (int b=0;b<(int)m_shared->m_nbones;b++)
	{
		hRapi->PushWorldMatrix();
		hRapi->TranslateWorldMatrix(m_bones[b].m_position);
		hRapi->RotateWorldMatrix(m_bones[b].m_rotation.BuildMatrix());
		hRapi->TranslateWorldMatrix(m_shared->m_restbones[b].m_bboxCenter);
		vector_3 half_diag = m_shared->m_restbones[b].m_bboxHalfDiag;
		RP_DrawBox( *hRapi, -half_diag, half_diag , MAKEDWORDCOLOR( vector_3(0,1,0.75f)));
		hRapi->PopWorldMatrix();
	}

}

unsigned short  box_indices[] =
{
	4,1,0,	//A
	1,3,0,	//B
	1,2,3,	//C
	5,2,1,	//D
	4,5,1,	//E
	4,6,5,	//F
	5,6,2,	//G
	6,3,2,	//H
	6,7,3,	//I
	4,7,6,	//J
	7,0,3,	//K
	7,4,0,	//L
};
unsigned short  boxoutline_indices[] =
{
	0,1,
	1,2,
	2,3,
	3,0,
	4,5,
	5,6,
	6,7,
	7,4,
	0,4,
	1,5,
	2,6,
	3,7,
};
sVertex  box_verts[] =
{
/// float x, y, z, 	DWORD color , UV	  uv;

/*0*/	 1.0f,  1.0f,  1.0f, 0xa04040ff,	{0.0f, 1.0f},
/*1*/	 1.0f,  1.0f, -1.0f, 0xa04040ff,	{1.0f, 1.0f},
/*2*/	-1.0f,  1.0f, -1.0f, 0xa04040ff,	{0.0f, 1.0f},
/*3*/	-1.0f,  1.0f,  1.0f, 0xa04040ff,	{1.0f, 1.0f},
/*4*/	 1.0f, -1.0f,  1.0f, 0xa04040ff,	{0.0f, 0.0f},
/*5*/	 1.0f, -1.0f, -1.0f, 0xa04040ff,	{1.0f, 0.0f},
/*6*/	-1.0f, -1.0f, -1.0f, 0xa04040ff,	{0.0f, 0.0f},
/*7*/	-1.0f, -1.0f,  1.0f, 0xa04040ff,	{1.0f, 0.0f},
};

//***********************************************************************************
void
Aspect::RenderFrozen(Rapi* hRapi, DWORD flags)
{

	CHECK_PRIMARY_THREAD_ONLY;

	// This is a custom render pass to draw the aspect as if it was 'frozen'
	UNREFERENCED_PARAMETER(flags);

	if (IsPurged(2))
	{
		BITCHSLAP("RenderFrozen with purged data");
		return;
	}
	else if (IsPurged(1))
	{
		BITCHWARN("RenderFrozen with purged textures");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	bool oldalpha = hRapi->EnableAlphaBlending(true);
	hRapi->SetTextureStageState( 0,
								 D3DTOP_SELECTARG2,
								 D3DTOP_SELECTARG2,
								 D3DTADDRESS_CLAMP,
								 D3DTADDRESS_CLAMP,
								 D3DTEXF_LINEAR,
								 D3DTEXF_LINEAR,
								 D3DTEXF_LINEAR,
								 D3DBLEND_SRCALPHA,
								 D3DBLEND_ONE,
								 false );

	hRapi->SetTexture( 0, NULL );

	hRapi->SetTextureStageState( 1,
								 D3DTOP_SELECTARG2,
								 D3DTOP_SELECTARG2,
								 D3DTADDRESS_CLAMP,
								 D3DTADDRESS_CLAMP,
								 D3DTEXF_LINEAR,
								 D3DTEXF_LINEAR,
								 D3DTEXF_LINEAR,
								 D3DBLEND_SRCALPHA,
								 D3DBLEND_ONE,
								 false );

	hRapi->SetTexture( 1, NULL );

	unsigned int startIndex;
	sVertex* pVerts;

	if( hRapi->LockBufferedVertices( 8, startIndex, pVerts ) )
	{
		memcpy(pVerts,&box_verts[0],8*sizeof(sVertex));
		hRapi->UnlockBufferedVertices();

		for (int b=0;b<(int)m_shared->m_nbones;b++)
		{
			hRapi->PushWorldMatrix();
			hRapi->TranslateWorldMatrix(m_bones[b].m_position);
			hRapi->RotateWorldMatrix(m_bones[b].m_rotation.BuildMatrix());
			hRapi->TranslateWorldMatrix(m_shared->m_restbones[b].m_bboxCenter);

			// Set 'non-uniform' scale
			hRapi->ScaleWorldMatrix( m_shared->m_restbones[b].m_bboxHalfDiag );

			hRapi->DrawBufferedIndexedPrimitive(
									D3DPT_TRIANGLELIST,
									startIndex, 8,
									box_indices,  36 );

			hRapi->DrawBufferedIndexedPrimitive(
									D3DPT_LINELIST,
									startIndex, 8,
									boxoutline_indices, 24 );

			hRapi->PopWorldMatrix();
		}
	}

	hRapi->EnableAlphaBlending(oldalpha);

}

unsigned short  splitmesh_indices[] =
{
  5,   0,   3,   5,   3,   8,   8,   3,   7,   3,   2,   7,
  7,   2,   4,   7,   4,   9,   9,   4,   6,   4,   1,   6,
 10,   5,  13,   5,   8,  13,  13,   8,   7,  13,   7,  12,
 12,   7,  14,   7,   9,  14,  14,   9,   6,  14,   6,  11,
 15,  10,  13,  15,  13,  18,  18,  13,  17,  13,  12,  17,
 17,  12,  14,  17,  14,  19,  19,  14,  16,  14,  11,  16,
 20,  15,  23,  15,  18,  23,  23,  18,  17,  23,  17,  22,
 22,  17,  24,  17,  19,  24,  24,  19,  16,  24,  16,  21,
 25,  20,  23,  25,  23,  28,  28,  23,  27,  23,  22,  27,
 27,  22,  24,  27,  24,  29,  29,  24,  26,  24,  21,  26,
 30,  25,  33,  25,  28,  33,  33,  28,  27,  33,  27,  32,
 32,  27,  34,  27,  29,  34,  34,  29,  26,  34,  26,  31,
 35,  30,  33,  35,  33,  38,  38,  33,  37,  33,  32,  37,
 37,  32,  34,  37,  34,  39,  39,  34,  36,  34,  31,  36,
 40,  35,  43,  35,  38,  43,  43,  38,  37,  43,  37,  42,
 42,  37,  44,  37,  39,  44,  44,  39,  36,  44,  36,  41,
 45,  40,  43,  45,  43,  48,  48,  43,  47,  43,  42,  47,
 47,  42,  44,  47,  44,  49,  49,  44,  46,  44,  41,  46,
 50,  45,  53,  45,  48,  53,  53,  48,  47,  53,  47,  52,
 52,  47,  54,  47,  49,  54,  54,  49,  46,  54,  46,  51,
 55,  50,  53,  55,  53,  58,  58,  53,  57,  53,  52,  57,
 57,  52,  54,  57,  54,  59,  59,  54,  56,  54,  51,  56,
 60,  55,  63,  55,  58,  63,  63,  58,  57,  63,  57,  62,
 62,  57,  64,  57,  59,  64,  64,  59,  56,  64,  56,  61,
 65,  60,  63,  65,  63,  68,  68,  63,  67,  63,  62,  67,
 67,  62,  64,  67,  64,  69,  69,  64,  66,  64,  61,  66,
 70,  65,  73,  65,  68,  73,  73,  68,  67,  73,  67,  72,
 72,  67,  74,  67,  69,  74,  74,  69,  66,  74,  66,  71,
 75,  70,  73,  75,  73,  78,  78,  73,  77,  73,  72,  77,
 77,  72,  74,  77,  74,  79,  79,  74,  76,  74,  71,  76,
 80,  75,  83,  75,  78,  83,  83,  78,  77,  83,  77,  82,
 82,  77,  84,  77,  79,  84,  84,  79,  76,  84,  76,  81,
 85,  80,  83,  85,  83,  88,  88,  83,  87,  83,  82,  87,
 87,  82,  84,  87,  84,  89,  89,  84,  86,  84,  81,  86,
 90,  85,  93,  85,  88,  93,  93,  88,  87,  93,  87,  92,
 92,  87,  94,  87,  89,  94,  94,  89,  86,  94,  86,  91,
 95,  90,  93,  95,  93,  98,  98,  93,  97,  93,  92,  97,
 97,  92,  94,  97,  94,  99,  99,  94,  96,  94,  91,  96,
100,  95, 103,  95,  98, 103, 103,  98,  97, 103,  97, 102,
102,  97, 104,  97,  99, 104, 104,  99,  96, 104,  96, 101,
105, 100, 103, 105, 103, 108, 108, 103, 107, 103, 102, 107,
107, 102, 104, 107, 104, 109, 109, 104, 106, 104, 101, 106,
110, 105, 113, 105, 108, 113, 113, 108, 107, 113, 107, 112,
112, 107, 114, 107, 109, 114, 114, 109, 106, 114, 106, 111,
115, 110, 113, 115, 113, 118, 118, 113, 117, 113, 112, 117,
117, 112, 114, 117, 114, 119, 119, 114, 116, 114, 111, 116,
120, 115, 123, 115, 118, 123, 123, 118, 117, 123, 117, 122,
122, 117, 124, 117, 119, 124, 124, 119, 116, 124, 116, 121,
125, 120, 123, 125, 123, 128, 128, 123, 127, 123, 122, 127,
127, 122, 124, 127, 124, 129, 129, 124, 126, 124, 121, 126,
130, 125, 133, 125, 128, 133, 133, 128, 127, 133, 127, 132,
132, 127, 134, 127, 129, 134, 134, 129, 126, 134, 126, 131,
135, 130, 133, 135, 133, 138, 138, 133, 137, 133, 132, 137,
137, 132, 134, 137, 134, 139, 139, 134, 136, 134, 131, 136,
140, 135, 143, 135, 138, 143, 143, 138, 137, 143, 137, 142,
142, 137, 144, 137, 139, 144, 144, 139, 136, 144, 136, 141,
145, 140, 143, 145, 143, 148, 148, 143, 147, 143, 142, 147,
147, 142, 144, 147, 144, 149, 149, 144, 146, 144, 141, 146,
150, 145, 153, 145, 148, 153, 153, 148, 147, 153, 147, 152,
152, 147, 154, 147, 149, 154, 154, 149, 146, 154, 146, 151,
155, 150, 153, 155, 153, 158, 158, 153, 157, 153, 152, 157,
157, 152, 154, 157, 154, 159, 159, 154, 156, 154, 151, 156,
160, 155, 163, 155, 158, 163, 163, 158, 157, 163, 157, 162,
162, 157, 164, 157, 159, 164, 164, 159, 156, 164, 156, 161,
165, 160, 163, 165, 163, 168, 168, 163, 167, 163, 162, 167,
167, 162, 164, 167, 164, 169, 169, 164, 166, 164, 161, 166,
170, 165, 173, 165, 168, 173, 173, 168, 167, 173, 167, 172,
172, 167, 174, 167, 169, 174, 174, 169, 166, 174, 166, 171,
175, 170, 173, 175, 173, 178, 178, 173, 177, 173, 172, 177,
177, 172, 174, 177, 174, 179, 179, 174, 176, 174, 171, 176,
180, 175, 183, 175, 178, 183, 183, 178, 177, 183, 177, 182,
182, 177, 184, 177, 179, 184, 184, 179, 176, 184, 176, 181,
185, 180, 183, 185, 183, 188, 188, 183, 187, 183, 182, 187,
187, 182, 184, 187, 184, 189, 189, 184, 186, 184, 181, 186,
190, 185, 193, 185, 188, 193, 193, 188, 187, 193, 187, 192,
192, 187, 194, 187, 189, 194, 194, 189, 186, 194, 186, 191,
195, 190, 193, 195, 193, 198, 198, 193, 197, 193, 192, 197,
197, 192, 194, 197, 194, 199, 199, 194, 196, 194, 191, 196
};

//***********************************************************************************
void
Aspect::RenderTracers(Rapi* hRapi, DWORD flags)
{

	CHECK_PRIMARY_THREAD_ONLY;

	UNREFERENCED_PARAMETER(flags);

	if (!m_Blender) return;

	const TracerKeys* tk = m_Blender->GetTracerKeys();

	PADDING_BUFFER_CHECK(this)
	if (tk)
	{
		stdx::fast_vector<sVertex> verts;

		DWORD prevcmode;

		TRYDX( hRapi->GetDevice()->GetRenderState( D3DRS_CULLMODE,&prevcmode ));
		TRYDX( hRapi->GetDevice()->SetRenderState( D3DRS_CULLMODE,D3DCULL_NONE ));

		bool oldalpha = hRapi->EnableAlphaBlending(true);
		hRapi->SetTextureStageState( 0,
									 D3DTOP_SELECTARG2,
									 D3DTOP_SELECTARG2,
									 D3DTADDRESS_CLAMP,
									 D3DTADDRESS_CLAMP,
									 D3DTEXF_LINEAR,
									 D3DTEXF_LINEAR,
									 D3DTEXF_LINEAR,
									 D3DBLEND_SRCALPHA,
									 D3DBLEND_ONE,
									 false );

		hRapi->SetTexture( 0, NULL );

		hRapi->SetTextureStageState( 1,
									 D3DTOP_SELECTARG2,
									 D3DTOP_SELECTARG2,
									 D3DTADDRESS_CLAMP,
									 D3DTADDRESS_CLAMP,
									 D3DTEXF_LINEAR,
									 D3DTEXF_LINEAR,
									 D3DTEXF_LINEAR,
									 D3DBLEND_SRCALPHA,
									 D3DBLEND_ONE,
									 false );

		hRapi->SetTexture( 1, NULL );


		vector_3 p0 = m_Blender->GetTracePoint0();
		vector_3 p1 = m_Blender->GetTracePoint1();

		if (p0.DotProduct(p1) > 0)
		{
			if (tk->BuildTracerTriStrip(	m_Blender->TimeOfCurrentTimeWarp(),
												p0,
												p1,
												verts) )
			{

				unsigned int startIndex;
				sVertex* pVerts;
				DWORD count = verts.size();

				if (count>5)
				{
					if( hRapi->LockBufferedVertices( count, startIndex, pVerts ) )
					{
						memcpy(pVerts,&verts[0],count*sizeof(sVertex));
						hRapi->UnlockBufferedVertices();

						hRapi->DrawBufferedPrimitive( D3DPT_TRIANGLESTRIP, startIndex, count);
					}
				}
			}
		}
		else
		{
			// This
			if ( tk->BuildTracerTriMesh(	m_Blender->TimeOfCurrentTimeWarp(),
											p0,
											p1,
											verts) )
			{

				unsigned int startIndex;
				sVertex* pVerts;
				DWORD count = verts.size();

				if (count > 200)
				{
					count = 200;
				}


				if (count>10)
				{
					if( hRapi->LockBufferedVertices( count, startIndex, pVerts ) )
					{
						memcpy(pVerts,&verts[0],count*sizeof(sVertex));
						hRapi->UnlockBufferedVertices();


						hRapi->DrawBufferedIndexedPrimitive(
									D3DPT_TRIANGLELIST,
									startIndex, count,
									splitmesh_indices, ((count/5)-1)*8*3 );
					}
				}
			}
		}

		hRapi->EnableAlphaBlending(oldalpha);

		TRYDX( hRapi->GetDevice()->SetRenderState( D3DRS_CULLMODE,prevcmode ));

	}


}


//***********************************************************************************
void Aspect::InitializeLighting( const DWORD ambience, const DWORD alpha )
{
	GPSTATS_GROUP( GROUP_LIGHT_MODELS );

	if (IsPurged(2))
	{
		BITCHSLAP("InitializeLighting");
		return;
	}

#if NEMA_DEBUGGING
	if( NEMAFLAG_ColorizedWeights )
	{
		return;
	}
#endif

	PADDING_BUFFER_CHECK(this)
	
	if( !(m_instflags & RENDER_USE_COLOR) && (m_Ambience == ambience) && (m_Alpha == alpha) )
	{
		// Go through this object's sub meshes
		for( DWORD csm = 0; csm < m_shared->m_nmeshes; csm++ )
		{
			// If this submesh is visible at current time
			if( !( m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm)) ) && m_shared->m_ncorners[csm] )
			{
				// Copy precalced corner colors into the real colors
				FillDWORDColorStrided( (void*)&m_corners[csm]->color, sizeof(sCorner), m_cornercolors[csm], m_shared->m_ncorners[csm] );
			}
		}
	}
	else
	{
		m_Ambience	= ambience;
		m_Alpha		= alpha;

		// Go through this object's sub meshes
		for( DWORD csm = 0; csm < m_shared->m_nmeshes; csm++ )
		{
			// If this submesh is visible at current time
			if( !( m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm)) ) )
			{
				unsigned int numCorners			= m_shared->m_ncorners[csm];
				sCorner* pCorners				= m_corners[csm];

				if (m_instflags & RENDER_USE_COLOR)
				{
					static const float INV = 1.0f / 255.0f;

					// $$ obviously this is slow as shit -sb

					gpcolor tempColor = m_DiffuseColor;
					gpcolor tempAmbience = ambience;
					tempColor.r = (BYTE)(tempColor.r * tempAmbience.r * INV);
					tempColor.g = (BYTE)(tempColor.g * tempAmbience.g * INV);
					tempColor.b = (BYTE)(tempColor.b * tempAmbience.b * INV);

					DWORD tempDiffuse = ScaleDWORDAlpha( SetDWORDAlpha( tempColor, m_DiffuseColor >> 24 ), alpha );

					for( DWORD i = 0; i < numCorners; i++ )
					{
						pCorners[i].color = tempDiffuse;
					}

				}
				else
				{
					// Copy the initial vert colors into the actual color array
					DWORD const * pColors			= &m_shared->m_wcorners[csm][0].color;
					unsigned long *pCornerColors	= m_cornercolors[csm];

					for( DWORD i = 0; i < numCorners; i++, pColors += (sizeof(sWeightedCorner)/sizeof(ARGB)) )
					{
						pCornerColors[i] = pCorners[i].color = ScaleDWORDAlpha( SetDWORDAlpha( ambience, *pColors >> 24 ), alpha );
					}
				}
			}
		}
	}

	// Initialize the lighting for all children
#if GP_DEBUG
	int dbg_count = 0;
#endif

	PADDING_BUFFER_CHECK(this)
	
	{for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {

		gpassertf((*it).first.IsValid(), ("Child[%d] of %s is not valid for lighting initialization",dbg_count,m_dbgname.c_str()));

#if GP_DEBUG
		dbg_count++;
#endif
		if ((*it).first.IsValid()) {
			(*it).first->InitializeLighting( ambience, alpha );
		}
	}}

	PADDING_BUFFER_CHECK(this)
}


//***********************************************************************************
void Aspect::CalculateShading(const LightSource& lightsource, const bool dbgdrawlightvect )
{
	if (IsPurged(2))
	{
		BITCHSLAP("CalculateShading");
		return;
	}

	GPSTATS_GROUP( GROUP_LIGHT_MODELS );

#if NEMA_DEBUGGING
	if( NEMAFLAG_ColorizedWeights )
	{
		return;
	}
#endif
	PADDING_BUFFER_CHECK(this)

	real Falloff;

	vector_3 relativelightpos;

	int pb = m_shared->m_primarybone;


	if( lightsource.m_Type == NLS_INFINITE_LIGHT )
	{

		if (pb >= 0 && !( m_shared->m_flags & DISABLE_LIGHTING ))
		{
			m_bones[pb].m_rotation.Inverse().RotateVector(relativelightpos,lightsource.m_Direction);
		} else
		{
			relativelightpos = lightsource.m_Direction;
		}

	}
	else
	{

		if ((pb >= 0) && !( m_shared->m_flags & DISABLE_LIGHTING ))
		{
			m_bones[pb].m_rotation.Inverse().RotateVector(relativelightpos,lightsource.m_Position - m_bones[pb].m_position);
		}
		else
		{
			relativelightpos = lightsource.m_Position;
		}

	}

	if (dbgdrawlightvect)
	{
		gDefaultRapi.PushWorldMatrix();
		if (pb >= 0)
		{
			gDefaultRapi.TranslateWorldMatrix(m_bones[pb].m_position);
			gDefaultRapi.RotateWorldMatrix(m_bones[pb].m_rotation.BuildMatrix());
		}
		RP_DrawRay(gDefaultRapi,Normalize(relativelightpos)*3,0xffffffff);
		gDefaultRapi.PopWorldMatrix();
	}

	if ((m_shared->m_MajorVersion<2) && (m_attachtype != RIGID_LINKED) && (m_attachtype != REVERSE_RIGID_LINKED)) {
		PreRotateNEMAtoSEIGE.Inverse().RotateVectorInPlace(relativelightpos);
	}

	if( lightsource.m_Type == NLS_POINT_LIGHT )
	{
		// If this is not an infinite light source, calculate falloff
		real const FalloffRange = lightsource.m_OuterRadius - lightsource.m_InnerRadius;
		if( IsZero( FalloffRange ) )
		{
			Falloff = 1.0f;
		}
		else
		{
			if( IsNegative( FalloffRange ) )
			{
				gperror( "tAspect::CalculateShading - FalloffRange must be a positive number!\n" );
				return;
			}

			float DistanceToLight = relativelightpos.Length();

			// If we are far away, this light doesn't affect us
			if( DistanceToLight > lightsource.m_OuterRadius ) {
				return;
			}

			if( IsZero( DistanceToLight, 0.001f ) )	{
				DistanceToLight = 1.0;
				relativelightpos = vector_3(0,1,0);
			}

			DistanceToLight = Minimum(DistanceToLight, lightsource.m_OuterRadius);
			DistanceToLight = Maximum(DistanceToLight, lightsource.m_InnerRadius);
			Falloff = 1.0f - (DistanceToLight - lightsource.m_InnerRadius) / FalloffRange;
		}
	}
	else
	{
		Falloff = 1.0f;
	}

	if( Falloff <= 0.0f )
	{
		return;
	}

	// Apply the intensity of the light to the falloff
	Falloff	*= lightsource.m_Intensity;
	if( IsZero( Falloff ) )
	{
		return;
	}

	vector_3 nlight			= Normalize( relativelightpos );

	// If lighting is disabled for this object, do fast version
	if( m_shared->m_flags & DISABLE_LIGHTING )
	{
		// Calculate the global color for this object
		float Intensity		= fabsf( nlight.y ) * Falloff;
		DWORD lColor		= fScaleDWORDColor( lightsource.m_Color, min_t( 1.0f, fabsf( Intensity ) ) );

		for( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++)
		{
			if( !(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm))) )
			{
				unsigned int numCorners		= m_shared->m_ncorners[csm];
				sCorner *pCorners			= m_corners[csm];

				if( IsPositive( Intensity ) )
				{
					for( DWORD i = 0; i < numCorners; i++ )
					{
						pCorners[i].color	= AddDWORDColor( pCorners[i].color, lColor );
					}
				}
				else if( IsNegative( Intensity ) )
				{
					for( DWORD i = 0; i < numCorners; i++ )
					{
						pCorners[i].color	= SubDWORDColor( pCorners[i].color, lColor );
					}
				}
			}
		}
	}
	else
	{
		for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++) {

			if (!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm)))) {

				unsigned int numCorners		= m_shared->m_ncorners[csm];
				sCorner *pCorners			= m_corners[csm];
				vector_3 *pNormals			= m_normals[csm];

				if( Falloff > 0.0f )
				{
					for	(DWORD i = 0; i < numCorners; i++ )
					{
						float Intensity	= nlight.DotProduct( pNormals[i] );
						if( Intensity > 0.0f )
						{
							fScaleAndAddDWORDColor( pCorners[i].color, lightsource.m_Color, min_t( 1.0f, Intensity * Falloff ) );
						}
					}
				}
				else if( Falloff < 0.0f )
				{
					for	(DWORD i = 0; i < numCorners; i++ )
					{
						float Intensity	= nlight.DotProduct( pNormals[i] );
						if( Intensity > 0.0f )
						{
							fScaleAndSubDWORDColor( pCorners[i].color, lightsource.m_Color, min_t( 1.0f, Intensity * -Falloff ) );
						}
					}
				}
			}
		}
	}

	// TODO Get rid of the tail recursion
#if GP_DEBUG
	int dbg_count = 0;
#endif
	PADDING_BUFFER_CHECK(this)

	{for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {

		gpassertf((*it).first.IsValid(), ("Child[%d] of %s is not valid for shading calculation",dbg_count,m_dbgname.c_str()));

#if GP_DEBUG
		dbg_count++;
#endif
		if ((*it).first.IsValid()) {
			(*it).first->CalculateShading( lightsource, dbgdrawlightvect);
		}
	}}
	PADDING_BUFFER_CHECK(this)
}

//***********************************************************************************
void Aspect::RenderShadow( Rapi& renderer, vector_3& light, float colorScale ) {

	CHECK_PRIMARY_THREAD_ONLY;

	gpassertm(this,"You are trying to render an invalid Aspect");
	if (!this) return;

	// You don't want lighting, you don't get shadows
	if( m_shared->m_flags & DISABLE_LIGHTING )
	{
		return;
	}

	if (IsPurged(2))
	{
		BITCHSLAP("RenderShadow");
		return;
	}

	PADDING_BUFFER_CHECK(this)
	bool bPrevFogState = renderer.SetFogActivatedState( false );

	renderer.PushWorldMatrix();
	RenderSingleShadow( renderer, light, colorScale );

	for(ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it)
	{
		gpassertf((*it).first.IsValid(), ("Child of %s is not valid for shadow render",m_dbgname.c_str()));

		if ((*it).first.IsValid())
		{
			renderer.PushWorldMatrix();
			(*it).first->RenderSingleShadow( renderer, light, colorScale );		// This does not recurse more than 1 level!
			renderer.PopWorldMatrix();
		}
	}

	PADDING_BUFFER_CHECK(this)
	renderer.PopWorldMatrix();

	renderer.SetFogActivatedState( bPrevFogState );

}



/* ************************************************************************
   Function: RenderShadow
   Description: Draws this object's shadow based on the given light source.
   ************************************************************************ */
void Aspect::RenderSingleShadow( Rapi& renderer, vector_3& light, float colorScale )
{

	CHECK_PRIMARY_THREAD_ONLY;

	gpassertm(this,"You are trying to RenderSingleShadow a non-existent Aspect");
	if (!this) return;

	if (IsPurged(2))
	{
		BITCHSLAP("RenderSingleShadow");
		return;
	}
	PADDING_BUFFER_CHECK(this)

	// Draw the shadow

	// If light is below or close to below me, then it needs to be raised
	// so that we don't get negative or close to zero problems that occur
	// with the math that I'm using to calculate the shadows.
	if( light.y < 0.5f )
	{
		light.y = 0.5f;
	}

	// If light is far enough away, don't draw shadow
	float length	= light.Length();
	if( length >= 8.0f || length < 0.99f )
	{
		return;
	}

	vector_3 nlight( light );
	Quat fixed_rot;

	if (m_shared->m_primarybone >= 0)
	{
		renderer.TranslateWorldMatrix(m_bones[m_shared->m_primarybone].m_position);
		renderer.RotateWorldMatrix(m_bones[m_shared->m_primarybone].m_rotation.BuildMatrix());

		nlight.y -= m_bones[m_shared->m_primarybone].m_position.y;
	}

	renderer.SetTextureStageState(	0,
						D3DTOP_SELECTARG1,
						D3DTOP_SELECTARG1,
						D3DTADDRESS_WRAP,
						D3DTADDRESS_WRAP,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DTEXF_POINT,
						D3DBLEND_SRCALPHA,
						D3DBLEND_INVSRCALPHA,
						false );

	// calculate appearance of shadow based on distance from light
	float alpha		= ((length * (1.0f/8.0f)) * 0.5f) + 0.5f;
	DWORD color		= InterpolateColor( MAKEDWORDCOLOR( alpha, alpha, alpha ), 0xFFFFFFFF, colorScale );

	renderer.GetDevice()->SetRenderState( D3DRS_TEXTUREFACTOR, color );
	renderer.SetSingleStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );

	for ( DWORD csm = 0;csm < m_shared->m_nmeshes; csm++)
	{
		if (!(m_instflags & (HIDE_ALLSUBMESHES|(HIDE_SUBMESH0<<csm))))
		{
			sTri* trirunner = m_shared->m_triangles[csm];
			unsigned int startIndex;

			// Merge the corners and the normals
			if( !renderer.CopyBufferedVertices( m_shared->m_ncorners[csm], startIndex, (sVertex*)(m_corners[csm]) ) )
			{
				continue;
			}

			for (DWORD mat = 0; mat < m_shared->m_submeshmatcount[csm]; mat++)
			{
				DWORD tris = m_shared->m_submeshfacecount[csm][mat];
				if (!tris)
				{
					continue;
				}
				// Draw
				renderer.DrawBufferedIndexedPrimitive(
					D3DPT_TRIANGLELIST,
					startIndex+m_shared->m_submeshstartindex[csm][mat],
					m_shared->m_submeshcorncount[csm][mat],
					(WORD *)trirunner, tris*3 );

				trirunner += tris;
			}
		}
	}
	PADDING_BUFFER_CHECK(this)
}


//*************************************************************************
#if !GP_RETAIL
void Aspect::DebugGetInfo(int spacing, gpstring& output ) {

	UNREFERENCED_PARAMETER(spacing);

	gpstring buildstr;
	buildstr.appendf("Aspect Name %s\n\n",m_dbgname.c_str());

	buildstr.append("Aspect File:\n");
	buildstr.appendf("%s [%d.%d]\n", gAspectStorage.GetAspectName(GetHandle()).c_str(),m_shared->m_MajorVersion,m_shared->m_MinorVersion);

	buildstr.appendf("Sub Meshes: %d\n", m_shared->m_nmeshes);
	buildstr.appendf("Num Attached Aspects: %d\n", m_childlinks.size());

	gpstring stitchsets;
	for (DWORD csm = 0; csm < m_shared->m_nmeshes; csm++)
	{
		stitchsets.clear();
		for (DWORD s = 0; s < m_shared->m_nstitchsets[csm]; s++ )
		{
			stitchsets.appendf("%s",stringtool::MakeFourCcString(REVERSE_FOURCC(m_shared->m_stitchset[csm][s].m_tag)).c_str());
			if (s != m_shared->m_nstitchsets[csm]-1)
			{
				stitchsets.append(",");
			}
			else
			{
				stitchsets.append("\n");
			}
		}

		if (!stitchsets.empty())
		{
			buildstr.appendf("Sub Mesh [%d] Stitch Sets: ",csm);
			buildstr.append(stitchsets);
		}
	}


	if (!m_childlinks.empty()) {

		DWORD kid = 0;

		for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end(); ++it) {


			if ( (*it).first.IsValid() )
			{
				for (DWORD csm = 0; csm < (*it).first->m_shared->m_nmeshes; csm++)
				{
					stitchsets.clear();
					for (DWORD s = 0; s < (*it).first->m_shared->m_nstitchsets[csm]; s++ )
					{
						stitchsets.appendf("%s",stringtool::MakeFourCcString(
							REVERSE_FOURCC((*it).first->m_shared->m_stitchset[csm][s].m_tag)).c_str());
						if (s != (*it).first->m_shared->m_nstitchsets[csm]-1)
						{
							stitchsets.append(",");
						}
						else
						{
							stitchsets.append("\n");
						}
					}
					if (!stitchsets.empty())
					{
						buildstr.appendf("Child [%d] Sub Mesh [%d] Stitch Sets: ",kid,csm);
						buildstr.append(stitchsets);
					}
				}
			}


			++kid;

		}
	}


	buildstr.append("\nTextures Used:");
	for( unsigned int i = 0; i <GetNumSubTextures(); ++i ) {
		buildstr.append("\n  ");
		buildstr.append(gDefaultRapi.GetTexturePathname(GetTexture(i)));
	}

	buildstr.append("\n");

	buildstr.appendf("Reference Count %d\n",gAspectStorage.RefCountAspect(m_handle));

	buildstr.appendf("Current Velocity %-8.4f\n",m_Velocity);

	if (m_Blender && m_Blender->HasValidChore(m_CurrChore)) {

		buildstr.appendf("Number of active CURRENT blending PRS keys %d\n",m_Blender->NumActiveAnims());

		gpstring sDebug;

		if (m_Blender->HasValidSkrit(m_CurrChore)) {
			m_Blender->GetSkrit(m_CurrChore)->DumpStateMachine( sDebug, 0 );
		}

		buildstr += sDebug;
		buildstr += "\n";

		m_Blender->StatusDump(buildstr);

	} else {
		if (m_BonesInRestPose)
		{
			buildstr.append("[BONES IN REST POSE] ");
		}
		else
		{
			buildstr.append("[BONES ARE MODIFIED] ");
		}
		buildstr.appendf("No blending chores attached to aspect\n");
	}

	buildstr += "\n\n";
	output.append( buildstr );
}
#endif // !GP_RETAIL


/* ************************************************************************
   Function: temporaryTransferInternals
   Description: added as a proof of concept for swapping the mesh out
				when a characters changes body armor

	TODO: Must isolate the mesh from the skeleton so these kinds of shenanigans
		are no longer needed

  $$$ This stuff HAS to be revised, its become FAR too ugly,

   ************************************************************************ */


bool Aspect::temporaryTransferInternals(HAspect hdest) {

	if (!hdest.IsValid())
	{
		Go* pgo = GetGo(m_owner_goid);
		gperrorf((
			"NEMA temporaryTransferInternals(): [%s g:%d s:%d] Destination is not a valid aspect handle",
			pgo ? pgo->GetTemplateName() : "<unknown>",
			pgo ? MakeInt(pgo->GetGoid()) : -1,
			pgo ? MakeInt(pgo->GetScid()) : -1
			));
		return false;
	}

	Aspect* dest = hdest.Get();

#if !GP_RETAIL
	ValidateParentLinks( false );
	dest->ValidateParentLinks( false );
#endif

	if (dest->m_shared->m_primarybone >= 0) {
		gpassertm(dest->m_shared->m_primarybone < 0, "NEMA temporaryTransferInternals(): Destination is not deformable");
		return false;
	}

	// Make sure that both aspects are either fully loaded, or NOT fully loaded
	if (IsPurged(2) != hdest->IsPurged(2))
	{
		if (IsPurged(2))
		{
			BITCHSLAP("temporaryTransferInternals SRC");
		}

		if (hdest->IsPurged(2))
		{
			BITCHSLAP("temporaryTransferInternals DEST");
		}
	}

	gpassertm(dest->m_shared->m_nbones == m_shared->m_nbones, "Destination has a different bone count");

#if !GP_RETAIL
	{for (DWORD b = 0; b < m_shared->m_nbones; b++) {

		if (stricmp(dest->m_shared->m_restbones[b].m_name,m_shared->m_restbones[b].m_name) != 0) {
			gpfatalf((
				"%s cannot wear armor %s\n\nbone[%d] '%s'\n\ndoes not match\n\nbone[%d] '%s'"
				,m_dbgname.c_str()
				,dest->m_dbgname.c_str()
				,b
				,m_shared->m_restbones[b].m_name
				,b
				,dest->m_shared->m_restbones[b].m_name
				));
			return false;
		}
	}}
#endif

	if (!IsPurged(2) && !dest->IsPurged(2))
	{
		PADDING_BUFFER_CHECK(this)
		PADDING_BUFFER_CHECK(dest)

		for (DWORD b = 0; b < m_shared->m_nbones; b++)
		{
			dest->m_bones[b].m_position		= m_bones[b].m_position;
			dest->m_bones[b].m_rotation		= m_bones[b].m_rotation;
		}
	}

	// Move the chores & Stance & Flags etc

	dest->m_CurrReqBlock = m_CurrReqBlock;

	dest->m_PrevChore = m_PrevChore;
	dest->m_CurrChore = m_CurrChore;
	dest->m_NextChore = m_NextChore;

	dest->m_PrevStance = m_PrevStance;
	dest->m_CurrStance = m_CurrStance;
	dest->m_NextStance = m_NextStance;

	dest->m_PrevSubAnim = m_PrevSubAnim;
	dest->m_CurrSubAnim = m_CurrSubAnim;
	dest->m_NextSubAnim = m_NextSubAnim;

	dest->m_PrevFlags = m_PrevFlags;
	dest->m_CurrFlags = m_CurrFlags;
	dest->m_NextFlags = m_NextFlags;

	m_CurrChore = CHORE_NONE;
	m_PrevChore = CHORE_NONE;
	m_NextChore = CHORE_NONE;

	delete dest->m_Blender;
	dest->m_Blender = m_Blender;
	m_Blender = NULL;

	if (dest->m_Blender)
	{
		dest->m_Blender->SetOwner(dest);
	}

	// Keep the MCP synchronized
	dest->m_CurrReqBlock = m_CurrReqBlock;

	dest->m_Velocity			= m_Velocity;
	dest->m_Scale				= m_Scale;
	dest->m_IsDeformable		= m_IsDeformable;
	dest->m_DeformationRequested	= m_DeformationRequested;
	dest->m_DeformNormCounter	= 0;
	dest->m_UpdateEnabled		= m_UpdateEnabled;
	dest->m_instflags			= m_instflags;
	dest->m_lightVisit			= 0xFFFFFFFF;

	dest->m_dbgname				= m_dbgname;

	dest->m_CompletionCallback = m_CompletionCallback;
	m_CompletionCallback = NULL;

	// Set the parent correctly
	dest->SetParent(m_parent);
	SetParent(HAspect());

	// Move all of the children over to the destination aspect
	dest->m_childlinks.clear();

	for (ChildLinkIter it = m_childlinks.begin(); it != m_childlinks.end();)
	{
		if ((*it).first.IsValid())
		{
			dest->m_childlinks[(*it).first] = (*it).second;

			Aspect* child = (*it).first.Get();
			child->SetParent(hdest);

			// Look for stitch sets
			child->m_instflags &= ~STITCH_VERTS;
			for (DWORD ps = 0; ps < dest->m_shared->m_nstitchsets[0]; ++ps) {
				for (DWORD subm = 0; subm < child->GetNumMeshes(); subm++) {
					for (DWORD cs = 0; cs < child->m_shared->m_nstitchsets[subm]; ++cs) {
						if (dest->m_shared->m_stitchset[0][ps].m_tag == child->m_shared->m_stitchset[subm][cs].m_tag) {
							child->m_instflags |= STITCH_VERTS;
							break;
						}
					}
				}
			}

			// Make sure that the purged states of the new dest and the children match
			if (dest->IsPurged(2))
			{
				if (!child->IsPurged(2))
				{
					child->Purge();
				}
			}
			else
			{
				if (!child->IsPurged(2))
				{

					// Set the parent bones of the existing children to the correct bone of the new parent

					if ((*it).second.IsFullyLinked())
					{
						for (DWORD c = 0; c < child->m_shared->m_nbones; c++ )
						{
							child->m_bones[c].m_parentBone = &dest->m_bones[c];
						}
					}
					else
					{
						DWORD c = (*it).second.ChildIndex();
						DWORD p = (*it).second.ParentIndex();
						child->m_bones[c].m_parentBone = &dest->m_bones[p];
					}
				}
			}
			child->m_DeformationRequested = child->m_IsDeformable;
		}

		it = m_childlinks.erase(it);
	}

	dest->m_BonesInRestPose = m_BonesInRestPose;

	// Make sure we get a deformation too!
	dest->m_DeformationRequested = dest->m_IsDeformable;

#if !GP_RETAIL
	ValidateParentLinks( false );
	dest->ValidateParentLinks( false );
#endif

	return true;


}

bool __cdecl Aspect :: FUBI_DoesHandleMgrExist( void )
{
	return ( nema::AspectStorage::DoesSingletonExist() );
}

UINT __cdecl Aspect :: FUBI_HandleAddRef( HAspect aspect )
{
	return gAspectStorage.AddRefAspect( aspect );
}

UINT __cdecl Aspect :: FUBI_HandleRelease( HAspect aspect )
{
	return gAspectStorage.ReleaseAspect( aspect );
}

bool __cdecl Aspect :: FUBI_HandleIsValid( HAspect aspect )
{
	return ( ResHandleMgr <HAspect> ::GetSingleton().IsValid( aspect ) );
}

Aspect* __cdecl Aspect :: FUBI_HandleGet( HAspect aspect )
{
	return ( ResHandleMgr <HAspect> ::GetSingleton().Get( aspect ) );
}
