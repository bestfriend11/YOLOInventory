#pragma once
#ifndef NEMA_ASPECTMGR_H
#define NEMA_ASPECTMGR_H

#include "nema_aspect.h"
#include "GoDefs.h"
#include "kerneltool.h"
#include <list>
#include <map>

/*========================================================================

	AspectMgr.h


/*========================================================================

------------------------------------------------------------------------*/

struct NemaShouldPersistHelper;

namespace nema {

	//---------------------------------

	// Ripped off Scott's Skrit Engine pretty much directly

	class AspectStorage : public Singleton <AspectStorage>
	{
	public:
		SET_NO_INHERITED( AspectStorage );

	// Ctor/dtor.

		AspectStorage( void );
	   ~AspectStorage( void );

		bool Xfer( FuBi::PersistContext& persist );
		
		bool ValidateParentLinks( bool fix_errors );

	   // 'LoadAspect' could be called 'CreateAspect' to reflect the way that Scott defines the class
	   // I think of aspects as persistent objects that get loaded... whatever --biddle
		HAspect LoadAspect(char const * const fname, char const * const dbgname);

		int AddRefAspect ( HAspect haspect );

		int ReleaseAspect( HAspect haspect, bool clearOwner = false );

		int RefCountAspect( HAspect haspect );

		gpstring GetAspectName( HAspect haspect ) const;

		void CommitRequests( void );
		
		// Declare shared blending buffer space for all aspects
		stdx::fast_vector<vector_3>	m_BlendVerts;
		stdx::fast_vector<Quat>		m_BlendQuats;

	private:
		friend NemaShouldPersistHelper;

		struct IndexEntry;
		struct AspectEntry;

		typedef std::list<HAspect> HAspectList;
		typedef std::map<gpstring, AspectEntry, istring_less> StringToAspectMap;
		typedef ResHandleExtraMgr<HAspect, IndexEntry> AspectMgr;

		struct AspectEntry
		{
			AspectImpl*		m_Impl;
			HAspectList		m_Inst;
			DWORD			m_UseCount;

			AspectEntry( void )  {  m_Impl = NULL; m_UseCount = 0; }

			bool Xfer( FuBi::PersistContext& persist );

			bool ShouldPersist( void ) const;
		};

		struct IndexEntry
		{
			StringToAspectMap::iterator	m_NameEntry;
			HAspectList::iterator		m_ListEntry;
		};

		AspectMgr						m_AspectManager;
		StringToAspectMap				m_AspectsByNameIndex;

		HAspectList						m_QueuedForRelease;

		kerneltool::Critical			m_CreateLock;

		SET_NO_COPYING( AspectStorage );
	};

	#define gAspectStorage nema::AspectStorage::GetSingleton()

}



#endif
